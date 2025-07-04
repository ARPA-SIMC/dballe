#include "base.h"
#include "dballe/msg/context.h"
#include "dballe/msg/msg.h"
#include <cmath>
#include <ostream>
#include <wreport/bulletin.h>
#include <wreport/codetables.h>
#include <wreport/conv.h>
#include <wreport/notes.h>
#include <wreport/subset.h>

// Define to debug the sounding group matching algorithm
// #define DEBUG_GROUPS

#ifdef DEBUG_GROUPS
#define debug_groups(...) fprintf(stderr, "grpmatch:" __VA_ARGS__)
#else
#define debug_groups(...)                                                      \
    do                                                                         \
    {                                                                          \
    } while (0)
#endif

using namespace wreport;
using namespace std;

#define MISSING_PRESS -1.0

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

class TempImporter : public WMOImporter
{
protected:
    double press;
    const Var* press_var;
    double surface_press;
    const Var* surface_press_var;

    void import_var(const Var& var);

    /**
     * If we identify sounding groups, this function can perform more accurate
     * sounding group import
     */
    void import_group(unsigned start, unsigned length);

public:
    TempImporter(const dballe::ImporterOptions& opts) : WMOImporter(opts) {}
    virtual ~TempImporter() {}

    void init() override
    {
        WMOImporter::init();

        press             = MISSING_PRESS;
        press_var         = NULL;
        surface_press     = MISSING_PRESS;
        surface_press_var = NULL;
    }

    /// Return true if \a code can be found at the start of a sounding group
    bool is_possible_group_var(Varcode code)
    {
        return WR_VAR_F(code) == 0 && WR_VAR_X(code) != 31 &&
               WR_VAR_X(code) != 1;
    }

    /// If the next variables in the current subset look like \a group_count
    /// sounding groups, scan them and return true; otherwise return false.
    bool try_soundings(unsigned group_count)
    {
        // Check if the first var in the first candidate group is a likely start
        // of a sounding group
        if (pos + 1 == subset->size())
            return false;
        const Var& start_var = (*subset)[pos + 1];
        if (not is_possible_group_var(start_var.code()))
            return false;

        // start_var marks the start of a sounding group
        debug_groups(
            "Candidate start var: %d (%01d%02d%03d) with group count %u\n",
            pos + 1, WR_VAR_F(start_var.code()), WR_VAR_X(start_var.code()),
            WR_VAR_Y(start_var.code()), group_count);

        // Seek forward until the same variable is found again, to compute the
        // group length
        unsigned group_length = 0;
        unsigned start        = pos + 1;
        unsigned cur          = start + 1;
        for (; cur < subset->size(); ++cur)
        {
            const Var& next = (*subset)[cur];
            if (next.code() == start_var.code() ||
                !is_possible_group_var(next.code()))
            {
                group_length = cur - start;
                if (start + group_count * group_length > subset->size())
                    return false;
                break;
            }
        }
        if (cur == subset->size())
            group_length = cur - start;

        // Validate group_length checking that all groups start with start_var
        for (unsigned i = 0; i < group_count; ++i)
        {
            const Var& next = (*subset)[pos + 1 + i * group_length];
            if (next.code() != start_var.code() &&
                !is_possible_group_var(next.code()))
            {
                debug_groups("  Group count %u/%u fails check at leading var "
                             "%01d%02d%03d\n",
                             i, group_count, WR_VAR_F(next.code()),
                             WR_VAR_X(next.code()), WR_VAR_Y(next.code()));
                return false;
            }
        }

        debug_groups("Validated first group: %d+%d\n", start, group_length);

        // Now we know how many groups there are and how long they are: iterate
        // them importing one at a time
        for (unsigned i = 0; i < group_count; ++i)
            import_group(pos + 1 + i * group_length, group_length);

        pos += 1 + group_count * group_length;
        return true;
    }

    void run() override
    {
        for (pos = 0; pos < subset->size();)
        {
            const Var& var = (*subset)[pos];
            if (WR_VAR_F(var.code()) != 0 || !var.isset())
            {
                // Ignore non-B variables and variables that are unset
                ++pos;
            }
            else if ((var.code() == WR_VAR(0, 31, 1) ||
                      var.code() == WR_VAR(0, 31, 2)))
            { // delayed descriptor replication count
                // Try to see if this is a sounding block, and import it a group
                // at a time
                if (!try_soundings(var.enqi()))
                    // If it does not look like a sounding, ignore delayed
                    // repetition count and proceed normally
                    ++pos;
            }
            else
            {
                // Import all non-sounding vars
                import_var(var);
                ++pos;
            }
        }

        /* Extract surface data from the surface level */
        if (surface_press != -1)
        {
            // Pressure is taken from a saved variable referencing to the
            // original pressure data in the message, to preserve data
            // attributes
            if (surface_press_var && surface_press_var->isset() &&
                msg->get_press_var())
                msg->set_press_var(*surface_press_var);

            const Context* sfc =
                msg->find_context(Level(100, surface_press), Trange::instant());
            if (sfc != NULL)
            {
                const Var* var = sfc->values.maybe_var(WR_VAR(0, 12, 1));
                if (var && msg->get_temp_2m_var())
                    msg->set_temp_2m_var(*var);

                var = sfc->values.maybe_var(WR_VAR(0, 12, 3));
                if (var && msg->get_dewpoint_2m_var())
                    msg->set_dewpoint_2m_var(*var);

                var = sfc->values.maybe_var(WR_VAR(0, 11, 1));
                if (var && !msg->get_wind_dir_var())
                    msg->set_wind_dir_var(*var);

                var = sfc->values.maybe_var(WR_VAR(0, 11, 2));
                if (var && !msg->get_wind_speed_var())
                    msg->set_wind_speed_var(*var);
            }
        }
    }

    MessageType scanType(const Bulletin& bulletin) const override
    {
        switch (bulletin.data_category)
        {
            case 2:
                switch (bulletin.data_subcategory)
                {
                    case 1: // 001 for PILOT data,
                    case 2: // 002 for PILOT SHIP data,
                    case 3: // 003 for PILOT MOBIL data.
                        return MessageType::PILOT;
                    case 4: return MessageType::TEMP;
                    case 5: return MessageType::TEMP_SHIP;
                    case 255:
                        switch (bulletin.data_subcategory_local)
                        {
                            case 0: {
                                /* Guess looking at the variables */
                                if (bulletin.subsets.empty())
                                    throw error_consistency(
                                        "trying to import a TEMP message with "
                                        "no data subset");
                                const Subset& subset = bulletin.subsets[0];
                                if (subset.size() > 1 &&
                                    subset[0].code() == WR_VAR(0, 1, 11))
                                    return MessageType::TEMP_SHIP;
                                else
                                    return MessageType::TEMP;
                            }
                            case 101: return MessageType::TEMP;
                            case 102: return MessageType::TEMP_SHIP;
                            case 91:
                            case 92:  return MessageType::PILOT;
                        }
                }
                break;
            case 6: return MessageType::TEMP;
        }
        return MessageType::TEMP;
    }
};

std::unique_ptr<Importer>
Importer::createTemp(const dballe::ImporterOptions& opts)
{
    return unique_ptr<Importer>(new TempImporter(opts));
}

void TempImporter::import_var(const Var& var)
{
    switch (var.code())
    {
            /* Identification of launch site and instrumentation */
        case WR_VAR(0, 2, 3):  msg->set_meas_equip_type_var(var); break;
        case WR_VAR(0, 2, 11): msg->set_sonde_type_var(var); break;
        case WR_VAR(0, 2, 12): msg->set_sonde_method_var(var); break;
        case WR_VAR(0, 2, 13): msg->set_sonde_correction_var(var); break;
        case WR_VAR(0, 2, 14):
            msg->set_sonde_tracking_var(var);
            break;
            /* Date/time of launch */
        case WR_VAR(0, 8, 21):
            if (var.enqi() != 18)
                notes::log() << "TEMP time significance is " << var.enqi()
                             << " instead of 18" << endl;
            break;
            /* Horizontal and vertical coordinates of launch site */
        case WR_VAR(0, 7, 1):
        case WR_VAR(0, 7, 30): msg->set_height_station_var(var); break;
        case WR_VAR(0, 7, 31): msg->set_height_baro_var(var); break;
        case WR_VAR(0, 7, 7):  msg->set_height_release_var(var); break;
        case WR_VAR(0, 33, 24):
            msg->set_station_height_quality_var(var);
            break;
            /* Cloud information reported with vertical soundings */
        case WR_VAR(0, 8, 2):
            msg->set(Level::cloud(258, 0), Trange::instant(), WR_VAR(0, 8, 2),
                     var);
            break;
        case WR_VAR(0, 20, 10): msg->set_cloud_n_var(var); break;
        case WR_VAR(0, 20, 11): msg->set_cloud_nh_var(var); break;
        case WR_VAR(0, 20, 13): msg->set_cloud_hh_var(var); break;
        case WR_VAR(0, 20, 12): { // CH CL CM
            int l2 = 1;
            if (pos > 0 && (*subset)[pos - 1].code() == WR_VAR(0, 20, 12))
            {
                ++l2;
                if (pos > 1 && (*subset)[pos - 2].code() == WR_VAR(0, 20, 12))
                    ++l2;
            }
            msg->set(Level::cloud(258, l2), Trange::instant(),
                     WR_VAR(0, 20, 12), var);
            break;
        }
        case WR_VAR(0, 22, 43):
            msg->set_water_temp_var(var);
            break;
            /* Temperature, dew-point and wind data at pressure levels */
        // Long time period or displacement (since launch time)
        case WR_VAR(0, 4, 16):
        case WR_VAR(0, 4, 86):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 4, 86),
                     var);
            break;
        // Extended vertical sounding significance
        case WR_VAR(0, 8, 42): {
            if (pos == subset->size() - 1)
                throw error_consistency("B08042 found at end of message");
            if ((*subset)[pos + 1].code() == WR_VAR(0, 7, 4))
            {
                // Pressure is reported later, we need to look ahead to compute
                // the right level
                press_var = &((*subset)[pos + 1]);
                if (press_var->isset())
                    press = press_var->enqd();
                else
                    press = MISSING_INT;
            }
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 8, 42),
                     var);
            break;
        }
        // Pressure
        case WR_VAR(0, 7, 4):
            press     = var.enqd();
            press_var = &var;
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 10, 4),
                     var);
            break;
        // Vertical sounding significance
        case WR_VAR(0, 8, 1): {
            // This account for weird data that has '1' for VSS
            unsigned val = convert_BUFR08001_to_BUFR08042(var.enqi());
            if (val != BUFR08042::ALL_MISSING)
            {
                unique_ptr<Var> nvar(newvar(WR_VAR(0, 8, 42), (int)val));
                nvar->setattrs(var);
                msg->set(Level(100, press), Trange::instant(), move(nvar));
            }
        }
            if (var.enqi() & BUFR08001::SURFACE)
            {
                surface_press     = press;
                surface_press_var = press_var;
            }
            break;
        // Geopotential
        case WR_VAR(0, 10, 3):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 10, 8),
                     var);
            break;
        case WR_VAR(0, 10, 8):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 10, 8),
                     var);
            break;
        case WR_VAR(0, 10, 9):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 10, 8),
                     var);
            break;
        // Latitude displacement
        case WR_VAR(0, 5, 15):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 5, 15),
                     var);
            break;
        // Longitude displacement
        case WR_VAR(0, 6, 15):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 6, 15),
                     var);
            break;
        // Dry bulb temperature
        case WR_VAR(0, 12, 1):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 12, 101),
                     var);
            break;
        case WR_VAR(0, 12, 101):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 12, 101),
                     var);
            break;
        // Wet bulb temperature
        case WR_VAR(0, 12, 2):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 12, 2),
                     var);
            break;
        // Dew point temperature
        case WR_VAR(0, 12, 3):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 12, 103),
                     var);
            break;
        case WR_VAR(0, 12, 103):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 12, 103),
                     var);
            break;
        // Wind direction
        case WR_VAR(0, 11, 1):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 11, 1),
                     var);
            break;
        // Wind speed
        case WR_VAR(0, 11, 2):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 11, 2),
                     var);
            break;
            /* Wind shear data at a pressure level */
        case WR_VAR(0, 11, 61):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 11, 61),
                     var);
            break;
        case WR_VAR(0, 11, 62):
            msg->set(Level(100, press), Trange::instant(), WR_VAR(0, 11, 62),
                     var);
            break;
        default: WMOImporter::import_var(var); break;
    }
}

void TempImporter::import_group(unsigned start, unsigned length)
{
    // Compute vertical level information
    Level lev;
    for (unsigned i = 0; i < length && lev.ltype1 == MISSING_INT; ++i)
    {
        const Var& var = (*subset)[start + i];
        switch (var.code())
        {
            // Height level
            case WR_VAR(0, 7, 2):
            case WR_VAR(0, 7, 9): // Geopotential height, for pilots (FIXME:
                                  // above ground or above msl?)
                if (var.isset())
                    lev = Level(102, var.enqd());
                break;
            // Height level converted in mm
            case WR_VAR(0, 7, 7):
                if (var.isset())
                    lev = Level(102, var.enqd() * 1000);
                break;
            // Pressure level
            case WR_VAR(0, 7, 4):
                if (var.isset())
                    lev = Level(100, var.enqd());
                break;
            case WR_VAR(0, 10, 3):
                // Convert geopotential to height
                if (var.isset())
                    lev = Level(102, lround(var.enqd() / 9.80665));
                break;
        }
    }
    if (lev.ltype1 == MISSING_INT)
        return;
    // throw error_consistency("neither B07002 nor B07004 nor B10003 found in
    // sounding group");

    // Import all values
    for (unsigned i = 0; i < length; ++i)
    {
        const Var& var = (*subset)[start + i];
        if (!var.isset())
        {
            switch (var.code())
            {
                case WR_VAR(0, 8, 1):
                case WR_VAR(0, 8, 42):
                    // Preserve missing VSS with only the one missing bit set,
                    // to act as a sounding context marker
                    msg->set(lev, Trange::instant(),
                             newvar(WR_VAR(0, 8, 42), (int)BUFR08042::MISSING));
                    break;
            }
            continue;
        }
        switch (var.code())
        {
            case WR_VAR(0, 4, 16):
            case WR_VAR(0, 4, 86):
                msg->set(lev, Trange::instant(), WR_VAR(0, 4, 86), var);
                break;
            case WR_VAR(0, 5, 1):
            case WR_VAR(0, 5, 2):
                msg->set(lev, Trange::instant(), WR_VAR(0, 5, 1), var);
                break;
            case WR_VAR(0, 5, 15):
                msg->set(lev, Trange::instant(), WR_VAR(0, 5, 15), var);
                break;
            case WR_VAR(0, 6, 1):
            case WR_VAR(0, 6, 2):
                msg->set(lev, Trange::instant(), WR_VAR(0, 6, 1), var);
                break;
            case WR_VAR(0, 6, 15):
                msg->set(lev, Trange::instant(), WR_VAR(0, 6, 15), var);
                break;
            case WR_VAR(0, 8, 1): {
                // This accounts for weird data that has '1' for VSS
                unsigned val = convert_BUFR08001_to_BUFR08042(var.enqi());
                if (val == BUFR08042::ALL_MISSING)
                    msg->set(lev, Trange::instant(),
                             newvar(WR_VAR(0, 8, 42), (int)BUFR08042::MISSING));
                else
                {
                    unique_ptr<Var> nvar(newvar(WR_VAR(0, 8, 42), (int)val));
                    nvar->setattrs(var);
                    msg->set(lev, Trange::instant(), std::move(nvar));
                }
            }
            break;
            case WR_VAR(0, 8, 42):
                msg->set(lev, Trange::instant(), WR_VAR(0, 8, 42), var);
                break;
            case WR_VAR(0, 10, 3):
                msg->set(lev, Trange::instant(), WR_VAR(0, 10, 8), var);
                break;
            case WR_VAR(0, 10, 9):
                msg->set(lev, Trange::instant(), WR_VAR(0, 10, 8), var);
                break;
            case WR_VAR(0, 12, 1):
            case WR_VAR(0, 12, 101):
                msg->set(lev, Trange::instant(), WR_VAR(0, 12, 101), var);
                break;
            case WR_VAR(0, 12, 3):
            case WR_VAR(0, 12, 103):
                msg->set(lev, Trange::instant(), WR_VAR(0, 12, 103), var);
                break;
            case WR_VAR(0, 7, 4):
            case WR_VAR(0, 10, 4):
                msg->set(lev, Trange::instant(), WR_VAR(0, 10, 4), var);
                break;
            case WR_VAR(0, 11, 1):
            case WR_VAR(0, 11, 2):
                msg->set(lev, Trange::instant(), var.code(), var);
                break;
            // Variables from Radar doppler wind profiles
            case WR_VAR(0, 11, 6):
                msg->set(lev, Trange::instant(), var.code(), var);
                break;
            case WR_VAR(0, 11, 50):
                msg->set(lev, Trange::instant(), var.code(), var);
                break;
            case WR_VAR(0, 33, 2):
                // Doppler wind profiles transmit quality information inline,
                // following the variable they refer to.
                if (i > 0)
                {
                    // So we look at the varcode of the previous value
                    Varcode prev_code = (*subset)[start + i - 1].code();
                    switch (prev_code)
                    {
                        // And if it is one of those for which quality info is
                        // set in this way...
                        case WR_VAR(0, 11, 2):
                        case WR_VAR(0, 11, 6):
                            // ...we lookup the variable we previously set in
                            // msg and add the attribute to it
                            if (wreport::Var* prev_var = msg->edit(
                                    prev_code, lev, Trange::instant()))
                                prev_var->seta(var);
                            break;
                    }
                }
                break;
                // default:
                //     msg->set(lev, Trange::instant(), var.code(), var);
                //     break;
        }
    }
}

} // namespace wr
} // namespace msg
} // namespace impl
} // namespace dballe
