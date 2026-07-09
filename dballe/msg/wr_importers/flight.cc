#include "base.h"
#include "dballe/core/var.h"
#include "dballe/msg/msg.h"
#include <cmath>
#include <wreport/bulletin.h>
#include <wreport/conv.h>
#include <wreport/subset.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

class FlightImporter : public WMOImporter
{
protected:
    Level lev;
    std::vector<Var*> deferred;
    const Var* b01006;
    const Var* b01008;

    void import_var(const Var& var);

public:
    FlightImporter(const dballe::ImporterOptions& opts) : WMOImporter(opts) {}
    virtual ~FlightImporter()
    {
        // If there are leftover variables in deferred, deallocate them
        for (std::vector<Var*>::iterator i = deferred.begin();
             i != deferred.end(); ++i)
            if (*i)
                delete *i;
    }

    void init() override
    {
        WMOImporter::init();
        lev = Level();
        deferred.clear();
        b01006 = b01008 = NULL;
    }

    void acquire(const Var& var)
    {
        if (lev.ltype1 == MISSING_INT)
        {
            // If we don't have a level yet, defer adding the variable until we
            // have one
            unique_ptr<Var> copy(var_copy_without_unset_attrs(var));
            deferred.push_back(copy.release());
        }
        else
            msg->set(lev, Trange::instant(), var.code(), var);
    }

    void acquire(const Var& var, Varcode code)
    {
        if (lev.ltype1 == MISSING_INT)
        {
            // If we don't have a level yet, defer adding the variable until we
            // have one
            unique_ptr<Var> copy(var_copy_without_unset_attrs(var, code));
            deferred.push_back(copy.release());
        }
        else
            msg->set(lev, Trange::instant(), code, var);
    }

    void set_level(const Level& newlev)
    {
        if (lev.ltype1 != MISSING_INT)
            error_consistency::throwf("found two flight levels: %s and %s",
                                      lev.describe().c_str(),
                                      newlev.describe().c_str());
        lev = newlev;

        // Flush deferred variables
        for (vector<Var*>::iterator i = deferred.begin(); i != deferred.end();
             ++i)
        {
            unique_ptr<Var> var(*i);
            *i = 0;
            msg->set(lev, Trange::instant(), move(var));
        }
        deferred.clear();
    }

    void run() override
    {
        WMOImporter::run();
        for (pos = 0; pos < subset->size(); ++pos)
        {
            const Var& var = (*subset)[pos];
            if (WR_VAR_F(var.code()) != 0)
                continue;
            if (var.isset())
                import_var(var);
        }
        if (b01008)
        {
            msg->set_ident_var(*b01008);
            if (b01006)
                acquire(*b01006);
        }
        else if (b01006)
            msg->set_ident_var(*b01006);
    }

    MessageType scanTypeFromVars(const Subset& subset) const
    {
        for (unsigned i = 0; i < subset.size(); ++i)
        {
            switch (subset[i].code())
            {
                case WR_VAR(0, 2, 65): // ACARS GROUND RECEIVING STATION
                    if (subset[0].isset())
                        return MessageType::ACARS;
                    break;
            }
        }
        return MessageType::AMDAR;
    }

    MessageType scanType(const Bulletin& bulletin) const override
    {
        switch (bulletin.data_subcategory_local)
        {
            case 142: return MessageType::AIREP;
            case 144: return MessageType::AMDAR;
            case 145: return MessageType::ACARS;
            default:
                // Scan for the presence of significant B codes
                if (bulletin.subsets.empty())
                    return MessageType::GENERIC;
                return scanTypeFromVars(bulletin.subsets[0]);
        }
    }
};

std::unique_ptr<Importer>
Importer::createFlight(const dballe::ImporterOptions& opts)
{
    return unique_ptr<Importer>(new FlightImporter(opts));
}

void FlightImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0, 1, 6):  b01006 = &var; break;
        case WR_VAR(0, 1, 8):  b01008 = &var; break;
        case WR_VAR(0, 1, 23): acquire(var); break;
        case WR_VAR(0, 2, 1):  acquire(var); break;
        case WR_VAR(0, 2, 2):  acquire(var); break;
        case WR_VAR(0, 2, 5):  acquire(var); break;
        case WR_VAR(0, 2, 61): acquire(var); break;
        case WR_VAR(0, 2, 62): acquire(var); break;
        case WR_VAR(0, 2, 63): acquire(var); break;
        case WR_VAR(0, 2, 64): acquire(var); break;
        case WR_VAR(0, 2, 70): acquire(var); break;
        case WR_VAR(0, 7, 2):
            // Specific Altitude Above Mean Sea Level in mm
            set_level(Level(102, var.enqd() * 1000));
            acquire(var, WR_VAR(0, 7, 30));
            break;
        case WR_VAR(0, 7, 4):
            // Isobaric Surface in Pa
            if (lev.ltype1 == MISSING_INT)
                set_level(Level(100, var.enqd()));
            acquire(var, WR_VAR(0, 10, 4));
            break;
        case WR_VAR(0, 7, 10):
            // Flight level
            if (opts.simplified)
            {
                // Convert to pressure using formula from
                // http://www.wmo.int/pages/prog/www/IMOP/publications/CIMO-Guide/CIMO%20Guide%207th%20Edition,%202008/Part%20II/Chapter%203.pdf
                double p_hPa =
                    1013.25 *
                    pow(1.0 - 0.000001 * 6.8756 * var.enqd() * 3.28084, 5.2559);
                set_level(Level(100, round(p_hPa * 100)));
            }
            else
                // Specific Altitude Above Mean Sea Level in mm
                set_level(Level(102, var.enqd() * 1000));
            acquire(var, WR_VAR(0, 7, 30));
            break;
        case WR_VAR(0, 8, 4):    acquire(var); break;
        case WR_VAR(0, 8, 9):    acquire(var); break;
        case WR_VAR(0, 8, 21):   acquire(var); break;
        case WR_VAR(0, 11, 1):   acquire(var); break;
        case WR_VAR(0, 11, 2):   acquire(var); break;
        case WR_VAR(0, 11, 31):  acquire(var); break;
        case WR_VAR(0, 11, 32):  acquire(var); break;
        case WR_VAR(0, 11, 33):  acquire(var); break;
        case WR_VAR(0, 11, 34):  acquire(var); break;
        case WR_VAR(0, 11, 35):  acquire(var); break;
        case WR_VAR(0, 11, 36):  acquire(var); break;
        case WR_VAR(0, 11, 37):  acquire(var); break;
        case WR_VAR(0, 11, 39):  acquire(var); break;
        case WR_VAR(0, 11, 77):  acquire(var); break;
        case WR_VAR(0, 12, 1):   acquire(var, WR_VAR(0, 12, 101)); break;
        case WR_VAR(0, 12, 101): acquire(var); break;
        case WR_VAR(0, 12, 3):   acquire(var, WR_VAR(0, 12, 103)); break;
        case WR_VAR(0, 12, 103): acquire(var); break;
        case WR_VAR(0, 13, 2):   acquire(var); break;
        case WR_VAR(0, 13, 3):   acquire(var); break;
        case WR_VAR(0, 20, 41):  acquire(var); break;
        case WR_VAR(0, 20, 42):  acquire(var); break;
        case WR_VAR(0, 20, 43):  acquire(var); break;
        case WR_VAR(0, 20, 44):  acquire(var); break;
        case WR_VAR(0, 20, 45):  acquire(var); break;
        case WR_VAR(0, 33, 25):
            acquire(var);
            break;
            // TODO: repeated 011075 MEAN TURBULENCE INTENSITY (EDDY DISSIPATION
            // RATE)[M**(2/3)/S]
            // TODO: repeated 011076 PEAK TURBULENCE INTENSITY (EDDY DISSIPATION
            // RATE)[M**(2/3)/S]
        default: WMOImporter::import_var(var); break;
    }
}

} // namespace wr
} // namespace msg
} // namespace impl
} // namespace dballe
