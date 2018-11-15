#include "base.h"
#include "dballe/msg/msg.h"
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <cstdlib>

using namespace wreport;
using namespace std;

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

namespace {

class ShipImporter : public SynopBaseImporter
{
protected:
    virtual void import_var(const Var& var);

public:
    ShipImporter(const ImporterOptions& opts)
        : SynopBaseImporter(opts) {}
    virtual ~ShipImporter() {}

    MessageType scanType(const Bulletin& bulletin) const
    {
        switch (bulletin.data_category)
        {
            case 1:
                switch (bulletin.data_subcategory_local)
                {
                    case 21: return MessageType::BUOY;
                    case 9:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 19: return MessageType::SHIP;
                    case 0: {
                        // Guess looking at the variables
                        if (bulletin.subsets.empty())
                            throw error_consistency("trying to import a SYNOP message with no data subset");
                        const Subset& subset = bulletin.subsets[0];
                        if (subset.size() > 1 && subset[0].code() == WR_VAR(0, 1, 5))
                            return MessageType::BUOY;
                        else
                            return MessageType::SHIP;
                    }
                    default: return MessageType::SHIP;
                }
                break;
            default: return MessageType::GENERIC; break;
        }
    }
};

void ShipImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        // Icing and ice
        case WR_VAR(0, 20, 31):
        case WR_VAR(0, 20, 32):
        case WR_VAR(0, 20, 33):
        case WR_VAR(0, 20, 34):
        case WR_VAR(0, 20, 35):
        case WR_VAR(0, 20, 36):
        case WR_VAR(0, 20, 37):
        case WR_VAR(0, 20, 38): msg->set(Level(1), Trange::instant(), var.code(), var); break;

        // Ship marine data
        case WR_VAR(0,  2, 38): msg->station_data.set(var.code(), var); break;
        case WR_VAR(0,  2, 39): msg->station_data.set(var.code(), var); break;
        case WR_VAR(0, 22, 42):
        case WR_VAR(0, 22, 43):
            if (level.sea_depth == LevelContext::missing)
                set(var, WR_VAR(0, 22, 43), Level(1), Trange::instant());
            else
                set(var, WR_VAR(0, 22, 43), Level(160, level.sea_depth * 1000), Trange::instant());
            break;

        // Waves
        case WR_VAR(0, 22,  1):
        case WR_VAR(0, 22, 11):
        case WR_VAR(0, 22, 21):
        case WR_VAR(0, 22,  2):
        case WR_VAR(0, 22, 12):
        case WR_VAR(0, 22, 22): msg->set(Level(1), Trange::instant(), var.code(), var); break;
            break;

        // D03023 swell waves (2 grups)
        case WR_VAR(0, 22,  3): // Direction of swell waves
        case WR_VAR(0, 22, 13): // Period of swell waves
        case WR_VAR(0, 22, 23): // Height of swell waves
            set(var, var.code(), Level(264, MISSING_INT, 261, level.swell_wave_group), Trange::instant());
            break;

        default: SynopBaseImporter::import_var(var); break;
    }
}

} // anonynmous namespace

std::unique_ptr<Importer> Importer::createShip(const ImporterOptions& opts)
{
    return unique_ptr<Importer>(new ShipImporter(opts));
}


}
}
}
}
