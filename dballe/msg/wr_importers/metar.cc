#include "base.h"
#include "dballe/core/shortcuts.h"
#include "dballe/msg/msg.h"
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <cmath>

using namespace wreport;
using namespace std;

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

#define MISSING_SENSOR_H -10000

class MetarImporter : public WMOImporter
{
protected:
    double height_sensor;

    void peek_var(const Var& var);
    void import_var(const Var& var);

    void set_gen_sensor(const Var& var, Varcode code, const Level& defaultLevel, const Trange& trange)
    {
        if (height_sensor == MISSING_SENSOR_H || defaultLevel == Level(103, height_sensor * 1000))
            msg->set(defaultLevel, trange, code, var);
        else if (opts.simplified)
        {
            Var var1(var);
            var1.seta(newvar(WR_VAR(0, 7, 32), height_sensor));
            msg->set(defaultLevel, trange, code, var1);
        } else
            msg->set(Level(103, height_sensor * 1000), trange, code, var);
    }

    void set_gen_sensor(const Var& var, const Shortcut& shortcut)
    {
        set_gen_sensor(var, shortcut.code, shortcut.level, shortcut.trange);
    }

public:
    MetarImporter(const dballe::ImporterOptions& opts) : WMOImporter(opts) {}
    virtual ~MetarImporter() {}

    virtual void init()
    {
        WMOImporter::init();
        height_sensor = MISSING_SENSOR_H;
    }

    virtual void run()
    {
        for (pos = 0; pos < subset->size(); ++pos)
        {
            const Var& var = (*subset)[pos];
            if (WR_VAR_F(var.code()) != 0) continue;
            if (WR_VAR_X(var.code()) < 10)
                peek_var(var);
            if (var.isset())
                import_var(var);
        }
    }

    MessageType scanType(const Bulletin& bulletin) const { return MessageType::METAR; }
};

std::unique_ptr<Importer> Importer::createMetar(const dballe::ImporterOptions& opts)
{
    return unique_ptr<Importer>(new MetarImporter(opts));
}

void MetarImporter::peek_var(const Var& var)
{
    switch (var.code())
    {
        // Context items
        case WR_VAR(0,  7,  6):
            if (var.isset())
                height_sensor = var.enqi();
            else
                height_sensor = MISSING_SENSOR_H;
            break;
    }
}

void MetarImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0,  7,  1): msg->set_height_station_var(var); break;
        case WR_VAR(0, 11,  1): set_gen_sensor(var, sc::wind_dir); break;
        case WR_VAR(0, 11, 16): set_gen_sensor(var, sc::ex_ccw_wind); break;
        case WR_VAR(0, 11, 17): set_gen_sensor(var, sc::ex_cw_wind); break;
        case WR_VAR(0, 11,  2): set_gen_sensor(var, sc::wind_speed); break;
        case WR_VAR(0, 11, 41): set_gen_sensor(var, sc::wind_speed); break;
        case WR_VAR(0, 12,  1): set_gen_sensor(var, sc::temp_2m); break;
        case WR_VAR(0, 12,  3): set_gen_sensor(var, sc::dewpoint_2m); break;
        case WR_VAR(0, 10, 52): set_gen_sensor(var, sc::qnh); break;
        case WR_VAR(0, 20,  9): set_gen_sensor(var, sc::metar_wtr); break;
        default:
            WMOImporter::import_var(var);
            break;
    }
}

}
}
}
}
