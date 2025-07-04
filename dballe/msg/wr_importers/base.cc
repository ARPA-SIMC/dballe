#include "base.h"
#include "dballe/core/shortcuts.h"
#include "dballe/core/var.h"
#include "dballe/msg/msg.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <wreport/bulletin.h>

#define MISSING_BARO -10000.0
#define MISSING_PRESS_STD 0.0
#define MISSING_TIME_SIG -10000

using namespace wreport;
using namespace std;

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

static const Trange tr_std_past_wtr3(205, 0, 10800);
static const Trange tr_std_past_wtr6(205, 0, 21600);
static const Level lev_std_wind(103, 10 * 1000);
static const Trange tr_std_wind_max10m(205, 0, 600);

void Importer::init() {}

void Importer::import(const wreport::Subset& subset, Message& msg)
{
    this->subset = &subset;
    this->msg    = &msg;
    init();
    run();
}

void Importer::set(const wreport::Var& var, const Shortcut& shortcut)
{
    msg->set(shortcut, var);
}

void Importer::set(const wreport::Var& var, wreport::Varcode code,
                   const Level& level, const Trange& trange)
{
    msg->set(level, trange, code, var);
}

std::unique_ptr<Importer> Importer::createSat(const dballe::ImporterOptions&)
{
    throw error_unimplemented("WB sat Importers");
}

void WMOImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        // General bulletin metadata
        case WR_VAR(0, 1, 1):  set(var, sc::block); break;
        case WR_VAR(0, 1, 2):  set(var, sc::station); break;
        case WR_VAR(0, 1, 5):
        case WR_VAR(0, 1, 6):
        case WR_VAR(0, 1, 11): set(var, sc::ident); break;
        case WR_VAR(0, 1, 12): set(var, sc::st_dir); break;
        case WR_VAR(0, 1, 13): set(var, sc::st_speed); break;
        case WR_VAR(0, 1, 63): set(var, sc::st_name_icao); break;
        case WR_VAR(0, 2, 1):  set(var, sc::st_type); break;
        case WR_VAR(0, 1, 15): set(var, sc::st_name); break;
        case WR_VAR(0, 4, 1):  set(var, sc::year); break;
        case WR_VAR(0, 4, 2):  set(var, sc::month); break;
        case WR_VAR(0, 4, 3):  set(var, sc::day); break;
        case WR_VAR(0, 4, 4):  set(var, sc::hour); break;
        case WR_VAR(0, 4, 5):  set(var, sc::minute); break;
        case WR_VAR(0, 4, 6):  set(var, sc::second); break;
        case WR_VAR(0, 5, 1):
        case WR_VAR(0, 5, 2):  set(var, sc::latitude); break;
        case WR_VAR(0, 6, 1):
        case WR_VAR(0, 6, 2):  set(var, sc::longitude); break;
    }
}

void LevelContext::init()
{
    height_baro        = MISSING_BARO;
    press_std          = MISSING_PRESS_STD;
    height_sensor      = missing;
    height_sensor_seen = false;
    sea_depth          = missing;
    ground_depth       = missing;
    swell_wave_group   = false;
}

void LevelContext::peek_var(const wreport::Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0, 7, 4):
            // Remember the standard level pressure to use later as layer for
            // geopotential
            press_std = var.enq(MISSING_PRESS_STD);
            break;
        case WR_VAR(0, 7, 31):
            // Remember the height to use later as layer for pressure
            height_baro = var.enq(MISSING_BARO);
            break;
        case WR_VAR(0, 7, 32):
            // Height to use later as level for whatever needs it
            height_sensor      = missing;
            height_sensor      = var.enq(missing);
            height_sensor_seen = true;
            break;
        case WR_VAR(0, 7, 61): ground_depth = var.enq(missing); break;
        case WR_VAR(0, 7, 63): sea_depth = var.enq(missing); break;
        case WR_VAR(0, 22, 3): swell_wave_group = true; break;
    }
}

void TimerangeContext::init()
{
    time_period        = MISSING_INT;
    time_period_offset = 0;
    time_period_seen   = false;
    time_sig           = MISSING_TIME_SIG;
    hour               = MISSING_INT;
    last_B04024_pos    = -1;
}

void TimerangeContext::peek_var(const Var& var, unsigned pos)
{
    if (var.isset())
    {
        switch (var.code())
        {
            case WR_VAR(0, 4, 4): hour = var.enqi(); break;
            case WR_VAR(0, 4, 24):
                // Time period in hours
                if ((int)pos == last_B04024_pos + 1)
                {
                    // Cope with the weird idea of using B04024 twice to
                    // indicate beginning and end of a period not ending with
                    // the SYNOP reference time
                    if (time_period != MISSING_INT && var.enqi() != 0)
                    {
                        time_period -= var.enqd() * 3600;
                        time_period_offset = var.enqd() * 3600;
                    }
                }
                else
                {
                    time_period        = var.enqd() * 3600;
                    time_period_seen   = true;
                    time_period_offset = 0;
                }
                last_B04024_pos = pos;
                break;
            case WR_VAR(0, 4, 25):
                // Time period in minutes
                time_period        = var.enqd() * 60;
                time_period_seen   = true;
                time_period_offset = 0;
                break;
            case WR_VAR(0, 8, 21):
                // Time significance
                time_sig = var.enqi();
                // If we get time significance 18 "Radiosonde launch time"
                // before getting the initial reference time, they are trying
                // to tell us that they are giving us the radiosonde launch
                // time: ignore it, since it is already what we gave for
                // granted.
                if (hour == MISSING_INT and time_sig == 18)
                    time_sig = MISSING_TIME_SIG;
                break;
        }
    }
    else
    {
        switch (var.code())
        {
            case WR_VAR(0, 4, 4): hour = MISSING_INT; break;
            case WR_VAR(0, 4, 24):
                // Time period in hours
                time_period        = MISSING_INT;
                time_period_offset = 0;
                time_period_seen   = true;
                break;
            case WR_VAR(0, 4, 25):
                // Time period in minutes
                time_period        = MISSING_INT;
                time_period_offset = 0;
                time_period_seen   = true;
                break;
            case WR_VAR(0, 8, 21):
                // Time significance
                time_sig = MISSING_TIME_SIG;
                break;
        }
    }
}

void CloudContext::init() { level = Level::cloud(MISSING_INT, MISSING_INT); }

void CloudContext::on_vss(const wreport::Subset& subset, unsigned pos)
{
    /* Vertical significance */
    if (pos == 0)
        throw error_consistency("B08002 found at beginning of message");
    Varcode prev = subset[pos - 1].code();

    if (prev == WR_VAR(0, 20, 10))
    {
        // Normal cloud data
        level.ltype2 = 258;
        level.l2     = 0;
        return;
    }

    if (pos == subset.size() - 1)
        throw error_consistency("B08002 found at end of message");
    Varcode next = subset[pos + 1].code();

    switch (next)
    {
        case WR_VAR(0, 20, 11): {
            if (pos >= subset.size() - 3)
                throw error_consistency("B08002 followed by B20011 found less "
                                        "than 3 places before end of message");
            Varcode next2 = subset[pos + 3].code();
            if (next2 == WR_VAR(0, 20, 14))
            {
                // Clouds with bases below station level
                if (level.ltype2 != 263)
                {
                    level.ltype2 = 263;
                    level.l2     = 1;
                }
                else
                {
                    ++level.l2;
                }
            }
            else
            {
                /* Individual cloud groups */
                if (level.ltype2 != 259)
                {
                    level.ltype2 = 259;
                    level.l2     = 1;
                }
                else
                {
                    ++level.l2;
                }
            }
            break;
        }
        case WR_VAR(0, 20, 54):
            // Direction of cloud drift
            if (level.ltype2 != 260)
            {
                level.ltype2 = 260;
                level.l2     = 1;
            }
            else
            {
                ++level.l2;
            }
            break;
        default: break;
#if 0
            /* Vertical significance */
            if (pos == 0) throw error_consistency("B08002 found at beginning of message");
            if (pos == subset->size() - 1) throw error_consistency("B08002 found at end of message");
            Varcode prev = (*subset)[pos - 1].code();
            Varcode next = (*subset)[pos + 1].code();

            } else if (var.value() == NULL) {
                level.ltype2 = 0;
            } else {
                /* Unless we can detect known buggy situations, raise an error */
                if (next != WR_VAR(0, 20, 62))
                    error_consistency::throwf("Vertical significance %d found in unrecognised context", var.enqi());
            }
            break;
#endif
    }
}

const Level& CloudContext::clcmch()
{
    if (level.ltype2 == 258)
        ++level.l2;
    return level;
}

void UnsupportedContext::init() { B08023 = nullptr; }

bool UnsupportedContext::is_unsupported() const { return B08023 != nullptr; }

void UnsupportedContext::peek_var(const wreport::Var& var, unsigned pos)
{
    switch (var.code())
    {
        case WR_VAR(0, 8, 23):
            if (var.isset())
                B08023 = &var;
            else
                B08023 = nullptr;
            break;
    }
}

Interpreted::Interpreted(const Shortcut& shortcut, const wreport::Var& var)
{
    level     = shortcut.level;
    trange    = shortcut.trange;
    this->var = var_copy_without_unset_attrs(var, shortcut.code);
}

Interpreted::Interpreted(const Shortcut& shortcut, const wreport::Var& var,
                         const Level& level, const Trange& trange)
    : level(level), trange(trange)
{
    this->var = var_copy_without_unset_attrs(var, shortcut.code);
}

Interpreted::Interpreted(wreport::Varcode code, const wreport::Var& var,
                         const Level& level, const Trange& trange)
    : level(level), trange(trange)
{
    this->var = var_copy_without_unset_attrs(var, code);
}

Interpreted::~Interpreted() {}

void InterpretedPrecise::set_sensor_height(const LevelContext& ctx)
{
    if (ctx.height_sensor == 0)
        level = Level(1);
    else if (ctx.height_sensor_seen)
        level = ctx.height_sensor == LevelContext::missing
                    ? Level(103)
                    : Level(103, round(ctx.height_sensor * 1000.0));
}

void InterpretedSimplified::set_sensor_height(const LevelContext& ctx)
{
    if (ctx.height_sensor == 0)
        return;
    if (!ctx.height_sensor_seen)
        return;
    if (ctx.height_sensor == LevelContext::missing)
        return;
    if (level == Level(103, ctx.height_sensor * 1000))
        return;
    var->seta(newvar(WR_VAR(0, 7, 32), ctx.height_sensor));
    if (level.ltype1 == 103 && level.l1 != MISSING_INT)
        level_deviation = abs(level.l1 - (int)(ctx.height_sensor * 1000));
}

void InterpretedPrecise::set_barometer_height(const LevelContext& ctx)
{
    if (ctx.height_baro == MISSING_BARO)
        return;
    level = Level(102, ctx.height_baro * 1000);
}

void InterpretedSimplified::set_barometer_height(const LevelContext& ctx)
{
    if (ctx.height_baro == MISSING_BARO)
        return;
    var->seta(newvar(WR_VAR(0, 7, 31), ctx.height_baro));
    if (level.ltype1 == 102 && level.l1 != MISSING_INT)
        level_deviation = abs(level.l1 - (int)(ctx.height_baro * 1000));
}

void InterpretedPrecise::set_duration(const TimerangeContext& ctx)
{
    if (trange.pind == 254)
        trange = Trange::instant();
    else if (ctx.time_period_seen)
        trange = ctx.time_period == MISSING_INT
                     ? Trange(trange.pind, 0)
                     : Trange(trange.pind, 0, abs(ctx.time_period));
}

void InterpretedSimplified::set_duration(const TimerangeContext& ctx)
{
    if (trange.pind == 254)
        return;
    if (!ctx.time_period_seen)
        return;
    if (ctx.time_period == MISSING_INT)
        return;
    Trange real = Trange(trange.pind, 0, abs(ctx.time_period));
    if (trange == real)
        return;
    var->seta(newvar(WR_VAR(0, 4, 194), abs(ctx.time_period)));
}

void InterpretedPrecise::set_wind_mean(const TimerangeContext& ctx)
{
    if (!ctx.time_period_seen)
        trange = Trange(200, 0, 600);
    else
        trange = ctx.time_period == MISSING_INT
                     ? Trange(200, 0)
                     : Trange(200, 0, abs(ctx.time_period));
}

void InterpretedSimplified::set_wind_mean(const TimerangeContext& ctx)
{
    if (!ctx.time_period_seen)
        return;
    if (ctx.time_period == MISSING_INT)
        return;
    Trange real = Trange(200, 0, abs(ctx.time_period));
    if (real.p2 == 600)
        return;
    if (real == trange)
        return;
    var->seta(newvar(WR_VAR(0, 4, 194), abs(ctx.time_period)));
}

void SynopBaseImporter::set_gen_sensor(const Var& var, Varcode code,
                                       const Level& lev_std,
                                       const Trange& tr_std)
{
    if (unsupported.is_unsupported())
        return;
    auto res = create_interpreted(opts.simplified, code, var, lev_std, tr_std);
    res->set_sensor_height(level);
    res->set_duration(trange);
    set(move(res));
}

void SynopBaseImporter::set_gen_sensor(const Var& var, const Shortcut& shortcut)
{
    if (unsupported.is_unsupported())
        return;
    auto res = create_interpreted(opts.simplified, shortcut, var);
    res->set_sensor_height(level);
    res->set_duration(trange);
    set(move(res));
}

void SynopBaseImporter::set_baro_sensor(const Var& var,
                                        const Shortcut& shortcut)
{
    if (unsupported.is_unsupported())
        return;
    auto res = create_interpreted(opts.simplified, shortcut, var);
    res->set_barometer_height(level);
    res->set_duration(trange);
    set(move(res));
}

void SynopBaseImporter::set_past_weather(const wreport::Var& var,
                                         const Shortcut& shortcut)
{
    if (unsupported.is_unsupported())
        return;
    auto res = create_interpreted(opts.simplified, shortcut, var);
    res->trange =
        Trange((trange.hour % 6 == 0) ? tr_std_past_wtr6 : tr_std_past_wtr3);
    res->set_duration(trange);
    set(move(res));
}

void SynopBaseImporter::set_wind(const wreport::Var& var,
                                 const Shortcut& shortcut)
{
    if (trange.time_sig != MISSING_TIME_SIG && trange.time_sig != 2)
        error_consistency::throwf(
            "Found unsupported time significance %d for wind direction",
            trange.time_sig);

    if (unsupported.is_unsupported())
        return;
    auto res   = create_interpreted(opts.simplified, shortcut, var);
    res->level = lev_std_wind;
    res->set_sensor_height(level);
    res->set_wind_mean(trange);
    set(move(res));
}

void SynopBaseImporter::set_wind_max(const wreport::Var& var,
                                     const Shortcut& shortcut)
{
    if (unsupported.is_unsupported())
        return;
    auto res = create_interpreted(opts.simplified, shortcut, var, lev_std_wind,
                                  tr_std_wind_max10m);
    res->set_sensor_height(level);
    // Always use real trange
    if (trange.time_period_seen)
        res->trange = trange.time_period == MISSING_INT
                          ? Trange(205, 0)
                          : Trange(205, 0, abs(trange.time_period));
    set(move(res));
}

void SynopBaseImporter::set_pressure(const wreport::Var& var)
{
    if (level.press_std == MISSING_PRESS_STD)
        set(var, WR_VAR(0, 10, 8), Level(100), Trange::instant());
    else
        set(var, WR_VAR(0, 10, 8), Level(100, level.press_std),
            Trange::instant());
}

void SynopBaseImporter::set(const wreport::Var& var, const Shortcut& shortcut)
{
    if (unsupported.is_unsupported())
        return;
    WMOImporter::set(var, shortcut);
}

void SynopBaseImporter::set(const wreport::Var& var, wreport::Varcode code,
                            const Level& level, const Trange& trange)
{
    if (unsupported.is_unsupported())
        return;
    WMOImporter::set(var, code, level, trange);
}

void SynopBaseImporter::set(std::unique_ptr<Interpreted> val)
{
    if (opts.simplified)
        queued.push_back(val.release());
    else
        msg->set(val->level, val->trange, move(val->var));
}

SynopBaseImporter::SynopBaseImporter(const dballe::ImporterOptions& opts)
    : WMOImporter(opts)
{
}

SynopBaseImporter::~SynopBaseImporter()
{
    for (auto& i : queued)
        delete i;
}

void SynopBaseImporter::init()
{
    WMOImporter::init();
    clouds.init();
    level.init();
    trange.init();
    unsupported.init();
    for (auto& i : queued)
        delete i;
    queued.clear();
}

void SynopBaseImporter::run()
{
    for (pos = 0; pos < subset->size(); ++pos)
    {
        const Var& var = (*subset)[pos];
        if (WR_VAR_F(var.code()) != 0)
            continue;
        if (WR_VAR_X(var.code()) < 10 || var.code() == WR_VAR(0, 22, 3))
            peek_var(var);
        if (var.isset())
            import_var(var);
    }

    std::sort(queued.begin(), queued.end(),
              [](const Interpreted* a, const Interpreted* b) {
                  if (a->level < b->level)
                      return true;
                  if (a->level > b->level)
                      return false;
                  if (a->trange < b->trange)
                      return true;
                  if (a->trange > b->trange)
                      return false;
                  if (a->level_deviation > b->level_deviation)
                      return true;
                  return false;
              });
    for (auto& i : queued)
    {
        msg->set(i->level, i->trange, move(i->var));
        delete i;
        i = nullptr;
    }
    queued.clear();
}

void SynopBaseImporter::peek_var(const Var& var)
{
    unsupported.peek_var(var, pos);
    level.peek_var(var);

    switch (var.code())
    {
        case WR_VAR(0, 4, 4):
        case WR_VAR(0, 4, 24):
        case WR_VAR(0, 4, 25):
        case WR_VAR(0, 8, 21): trange.peek_var(var, pos); break;
        case WR_VAR(0, 8, 2):  clouds.on_vss(*subset, pos); break;
    }
}

void SynopBaseImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0, 8, 2):
            // Store original VS value as a measured value
            set(var, WR_VAR(0, 8, 2), clouds.level, Trange::instant());
            break;

        // Ship identification, movement, date/time, horizontal and vertical
        // coordinates
        case WR_VAR(0, 7, 1):
        case WR_VAR(0, 7, 30): set(var, sc::height_station); break;
        case WR_VAR(0, 7, 31):
            /* Store also in the ana level, so that if the
             * pressure later is missing we still have
             * access to the value */
            set(var, sc::height_baro);
            break;

        // Pressure data (complete)
        case WR_VAR(0, 10, 4):  set_baro_sensor(var, sc::press); break;
        case WR_VAR(0, 10, 51): set_baro_sensor(var, sc::press_msl); break;
        case WR_VAR(0, 10, 61): set_baro_sensor(var, sc::press_3h); break;
        case WR_VAR(0, 10, 62): set_baro_sensor(var, sc::press_24h); break;
        case WR_VAR(0, 10, 63): set_baro_sensor(var, sc::press_tend); break;
        case WR_VAR(0, 10, 3):
        case WR_VAR(0, 10, 8):
        case WR_VAR(0, 10, 9):  set_pressure(var); break;

        // Ship “instantaneous” data

        // Temperature and humidity data (complete)
        case WR_VAR(0, 12, 4):
        case WR_VAR(0, 12, 101): set_gen_sensor(var, sc::temp_2m); break;
        case WR_VAR(0, 12, 6):
        case WR_VAR(0, 12, 103): set_gen_sensor(var, sc::dewpoint_2m); break;
        case WR_VAR(0, 13, 3):   set_gen_sensor(var, sc::humidity); break;
        case WR_VAR(0, 12, 2):
        case WR_VAR(0, 12, 102): set_gen_sensor(var, sc::wet_temp_2m); break;

        // Visibility data (complete)
        case WR_VAR(0, 20, 1): set_gen_sensor(var, sc::visibility); break;

        // Precipitation past 24h (complete)
        case WR_VAR(0, 13, 19): set_gen_sensor(var, sc::tot_prec1); break;
        case WR_VAR(0, 13, 20): set_gen_sensor(var, sc::tot_prec3); break;
        case WR_VAR(0, 13, 21): set_gen_sensor(var, sc::tot_prec6); break;
        case WR_VAR(0, 13, 22): set_gen_sensor(var, sc::tot_prec12); break;
        case WR_VAR(0, 13, 23): set_gen_sensor(var, sc::tot_prec24); break;

        // Cloud data
        case WR_VAR(0, 20, 10): set(var, sc::cloud_n); break;

        // Individual cloud layers or masses (complete)
        // Clouds with bases below station level (complete)
        // Direction of cloud drift (complete)
        case WR_VAR(0, 20, 11):
        case WR_VAR(0, 20, 13):
        case WR_VAR(0, 20, 17):
        case WR_VAR(0, 20, 54):
            set(var, var.code(), clouds.level, Trange::instant());
            break;
        case WR_VAR(0, 20, 12): // CH CL CM
            set(var, WR_VAR(0, 20, 12), clouds.clcmch(), Trange::instant());
            break;

        // Present and past weather (complete)
        case WR_VAR(0, 20, 3): set(var, sc::pres_wtr); break;
        case WR_VAR(0, 20, 4): set_past_weather(var, sc::past_wtr1_6h); break;
        case WR_VAR(0, 20, 5): set_past_weather(var, sc::past_wtr2_6h); break;

        // Precipitation measurement (complete)
        case WR_VAR(0, 13, 11):
            set_gen_sensor(var, WR_VAR(0, 13, 11), Level(1),
                           Trange(1, 0, abs(trange.time_period)));
            break;

        // Extreme temperature data
        case WR_VAR(0, 12, 111):
            set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1),
                           Trange(2, -abs(trange.time_period_offset),
                                  abs(trange.time_period)));
            break;
        case WR_VAR(0, 12, 112):
            set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1),
                           Trange(3, -abs(trange.time_period_offset),
                                  abs(trange.time_period)));
            break;

        // Wind data (complete)
        case WR_VAR(0, 2, 2): set(var, sc::wind_inst); break;

        /* Note B/C 1.10.5.3.2 Calm shall be reported by
         * setting wind direction to 0 and wind speed to 0.
         * Variable shall be reported by setting wind direction
         * to 0 and wind speed to a positive value, not a
         * missing value indicator.
         */
        case WR_VAR(0, 11, 1):
        case WR_VAR(0, 11, 11): set_wind(var, sc::wind_dir); break;
        case WR_VAR(0, 11, 2):
        case WR_VAR(0, 11, 12): set_wind(var, sc::wind_speed); break;
        case WR_VAR(0, 11, 43): set_wind_max(var, sc::wind_gust_max_dir); break;
        case WR_VAR(0, 11, 41):
            set_wind_max(var, sc::wind_gust_max_speed);
            break;

        case WR_VAR(0, 12, 5):   set(var, sc::wet_temp_2m); break;
        case WR_VAR(0, 10, 197): set(var, sc::height_anem); break;

        case WR_VAR(0, 12, 30):
            set(var, WR_VAR(0, 12, 30),
                Level(106, level.ground_depth == LevelContext::missing
                               ? MISSING_INT
                               : level.ground_depth * 1000),
                Trange::instant());
            break;

        default: WMOImporter::import_var(var); break;
    }
}

} // namespace wr
} // namespace msg
} // namespace impl
} // namespace dballe
