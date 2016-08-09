#include "base.h"
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <cstdlib>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

namespace {

class SynopImporter : public SynopBaseImporter
{
protected:
    virtual void import_var(const Var& var);

public:
    SynopImporter(const msg::Importer::Options& opts)
        : SynopBaseImporter(opts) {}
    virtual ~SynopImporter() {}

    MsgType scanType(const Bulletin& bulletin) const
    {
        switch (bulletin.data_category)
        {
            case 0: return MSG_SYNOP;
            default: return MSG_GENERIC; break;
        }
    }
};

void SynopImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        // Direction and elevation of cloud (complete)
        case WR_VAR(0, 5, 21): msg->set(var, WR_VAR(0, 5, 21), Level::cloud(262, 0), Trange::instant()); break;
        case WR_VAR(0, 7, 21): msg->set(var, WR_VAR(0, 7, 21), Level::cloud(262, 0), Trange::instant()); break;
        // Cloud type is handled by the generic cloud type handler

        // State of ground, snow depth, ground minimum temperature (complete)
        case WR_VAR(0, 20,  62): msg->set_state_ground_var(var); break;
        case WR_VAR(0, 13,  13): msg->set_tot_snow_var(var); break;
        case WR_VAR(0, 12, 113): msg->set(var, WR_VAR(0, 12, 121), Level(1), Trange(3, 0, 43200)); break;

        // Basic synoptic "period" data

        // Sunshine data (complete)
        case WR_VAR(0, 14, 31): msg->set(var, WR_VAR(0, 14, 31), Level(1), Trange(1, 0, abs(trange.time_period))); break;

        // Evaporation data
        case WR_VAR(0, 2, 4): msg->set(var, WR_VAR(0, 2, 4), Level(1), Trange::instant()); break;
        case WR_VAR(0, 13, 33):
            if (trange.time_period == MISSING_INT)
                msg->set(var, WR_VAR(0, 13, 33), Level(1), Trange(1));
            else
                msg->set(var, WR_VAR(0, 13, 33), Level(1), Trange(1, 0, abs(trange.time_period)));
            break;

        // Radiation data
        case WR_VAR(0, 14,  2):
        case WR_VAR(0, 14,  4):
        case WR_VAR(0, 14, 16):
        case WR_VAR(0, 14, 28):
        case WR_VAR(0, 14, 29):
        case WR_VAR(0, 14, 30):
            if (trange.time_period == MISSING_INT)
                msg->set(var, var.code(), Level(1), Trange(1));
            else
                msg->set(var, var.code(), Level(1), Trange(1, 0, abs(trange.time_period)));
            break;

        // Temperature change
        case WR_VAR(0, 12, 49):
            msg->set(var, WR_VAR(0, 12, 49), Level(1), Trange(4, -abs(trange.time_period_offset), abs(trange.time_period)));
            break;
        case WR_VAR(0, 22, 42): msg->set_water_temp_var(var); break;
        case WR_VAR(0, 12,  5): msg->set_wet_temp_2m_var(var); break;
        case WR_VAR(0, 10,197): msg->set_height_anem_var(var); break;

        default: SynopBaseImporter::import_var(var); break;
    }
}

} // anonynmous namespace

std::unique_ptr<Importer> Importer::createSynop(const msg::Importer::Options& opts)
{
    return unique_ptr<Importer>(new SynopImporter(opts));
}


}
}
}
