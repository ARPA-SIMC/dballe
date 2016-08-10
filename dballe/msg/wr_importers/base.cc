#include "base.h"
#include "core/var.h"
#include <wreport/bulletin.h>
#include <cstdlib>
#include <iostream>

#define MISSING_BARO -10000.0
#define MISSING_PRESS_STD 0.0
#define MISSING_SENSOR_H -10000.0
#define MISSING_TIME_SIG -10000

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

static const Trange tr_std_past_wtr3(205, 0, 10800);
static const Trange tr_std_past_wtr6(205, 0, 21600);
static const Level lev_std_wind(103, 10*1000);
static const Trange tr_std_wind(200, 0, 600);
static const Trange tr_std_wind_max10m(205, 0, 600);

void Importer::init()
{
    ye = mo = da = ho = mi = se = MISSING_INT;
}

void Importer::import(const wreport::Subset& subset, Msg& msg)
{
    this->subset = &subset;
    this->msg = &msg;
    init();
    run();

    // Postprocess extracting rep_memo information
    const Var* rep_memo = msg.get_rep_memo_var();
    if (rep_memo)
        msg.set_rep_memo(rep_memo->enqc());
    else
        msg.set_rep_memo(std::string());

    // Postprocess extracting coordinate information
    const Var* lat = msg.get_latitude_var();
    const Var* lon = msg.get_longitude_var();
    if (lat && lon)
        msg.set_coords(Coords(lat->enqd(), lon->enqd()));
    else
        msg.set_coords(Coords());

    // Postprocess extracting ident information
    const Var* ident = msg.get_ident_var();
    if (ident)
        msg.set_ident(Ident(ident->enqc()));
    else
        msg.set_ident(Ident());

    // Postprocess extracting datetime information
    if (ye == MISSING_INT)
        msg.set_datetime(Datetime());
    else
    {
        if (mo == MISSING_INT)
            throw error_consistency("no month information found in message to import");
        if (da == MISSING_INT)
            throw error_consistency("no day information found in message to import");
        if (ho == MISSING_INT)
            throw error_consistency("no hour information found in message to import");
        if (mi == MISSING_INT)
            throw error_consistency("no minute information found in message to import");
        if (se == MISSING_INT)
            se = 0;
        // Accept an hour of 24:00:00 and move it to 00:00:00 of the following
        // day
        Datetime::normalise_h24(ye, mo, da, ho, mi, se);
        msg.set_datetime(Datetime(ye, mo, da, ho, mi, se));
    }
}

void Importer::set(const wreport::Var& var, int shortcut)
{
    msg->set_by_id(var, shortcut);
}

void Importer::set(const wreport::Var& var, wreport::Varcode code, const Level& level, const Trange& trange)
{
    msg->set(var, code, level, trange);
}

std::unique_ptr<Importer> Importer::createSat(const msg::Importer::Options&) { throw error_unimplemented("WB sat Importers"); }

void WMOImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        // General bulletin metadata
        case WR_VAR(0,  1,  1): set(var, DBA_MSG_BLOCK); break;
        case WR_VAR(0,  1,  2): set(var, DBA_MSG_STATION); break;
        case WR_VAR(0,  1,  5):
        case WR_VAR(0,  1,  6):
        case WR_VAR(0,  1, 11): set(var, DBA_MSG_IDENT); break;
        case WR_VAR(0,  1, 12): set(var, DBA_MSG_ST_DIR); break;
        case WR_VAR(0,  1, 13): set(var, DBA_MSG_ST_SPEED); break;
        case WR_VAR(0,  1, 63): set(var, DBA_MSG_ST_NAME_ICAO); break;
        case WR_VAR(0,  2,  1): set(var, DBA_MSG_ST_TYPE); break;
        case WR_VAR(0,  1, 15): set(var, DBA_MSG_ST_NAME); break;
        case WR_VAR(0,  4,  1): ye = var.enqi(); if (var.next_attr()) set(var, DBA_MSG_YEAR); break;
        case WR_VAR(0,  4,  2): mo = var.enqi(); if (var.next_attr()) set(var, DBA_MSG_MONTH); break;
        case WR_VAR(0,  4,  3): da = var.enqi(); if (var.next_attr()) set(var, DBA_MSG_DAY); break;
        case WR_VAR(0,  4,  4): ho = var.enqi(); if (var.next_attr()) set(var, DBA_MSG_HOUR); break;
        case WR_VAR(0,  4,  5): mi = var.enqi(); if (var.next_attr()) set(var, DBA_MSG_MINUTE); break;
        case WR_VAR(0,  4,  6): se = var.enqi(); if (var.next_attr()) set(var, DBA_MSG_SECOND); break;
        case WR_VAR(0,  5,  1):
        case WR_VAR(0,  5,  2): set(var, DBA_MSG_LATITUDE); break;
        case WR_VAR(0,  6,  1):
        case WR_VAR(0,  6,  2): set(var, DBA_MSG_LONGITUDE); break;
    }
}

void LevelContext::init()
{
    height_baro = MISSING_BARO;
    press_std = MISSING_PRESS_STD;
    height_sensor = MISSING_SENSOR_H;
    height_sensor_seen = false;
    depth = MISSING_SENSOR_H;
    swell_wave_group = 0;
}

void LevelContext::peek_var(const wreport::Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0,  7,  4):
            // Remember the standard level pressure to use later as layer for geopotential
            press_std = var.enq(MISSING_PRESS_STD);
            break;
        case WR_VAR(0,  7, 31):
            // Remember the height to use later as layer for pressure
            height_baro = var.enq(MISSING_BARO);
            break;
        case WR_VAR(0,  7, 32):
            // Height to use later as level for whatever needs it
            height_sensor = MISSING_SENSOR_H;
            height_sensor = var.enq(MISSING_SENSOR_H);
            height_sensor_seen = true;
            break;
        case WR_VAR(0,  7, 63):
            depth = var.enq(MISSING_SENSOR_H);
            break;
    }
}


void TimerangeContext::init()
{
    time_period = MISSING_INT;
    time_period_offset = 0;
    time_period_seen = false;
    time_sig = MISSING_TIME_SIG;
    hour = MISSING_INT;
    last_B04024_pos = -1;
}

void TimerangeContext::peek_var(const Var& var, unsigned pos)
{
    if (var.isset())
    {
        switch (var.code())
        {
            case WR_VAR(0,  4,  4): hour = var.enqi(); break;
            case WR_VAR(0,  4, 24):
                // Time period in hours
                if ((int)pos == last_B04024_pos + 1)
                {
                    // Cope with the weird idea of using B04024 twice to indicate
                    // beginning and end of a period not ending with the SYNOP
                    // reference time
                    if (time_period != MISSING_INT && var.enqi() != 0)
                    {
                        time_period -= var.enqd() * 3600;
                        time_period_offset = var.enqd() * 3600;
                    }
                } else {
                    time_period = var.enqd() * 3600;
                    time_period_seen = true;
                    time_period_offset = 0;
                }
                last_B04024_pos = pos;
                break;
            case WR_VAR(0,  4, 25):
                // Time period in minutes
                time_period = var.enqd() * 60;
                time_period_seen = true;
                time_period_offset = 0;
                break;
            case WR_VAR(0,  8, 21):
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
    } else {
        switch (var.code())
        {
            case WR_VAR(0,  4,  4): hour = MISSING_INT; break;
            case WR_VAR(0,  4, 24):
                // Time period in hours
                time_period = MISSING_INT;
                time_period_offset = 0;
                time_period_seen = true;
                break;
            case WR_VAR(0,  4, 25):
                // Time period in minutes
                time_period = MISSING_INT;
                time_period_offset = 0;
                time_period_seen = true;
                break;
            case WR_VAR(0,  8, 21):
                // Time significance
                time_sig = MISSING_TIME_SIG;
                break;
        }
    }
}

void CloudContext::init()
{
    level = Level::cloud(MISSING_INT, MISSING_INT);
}

void CloudContext::on_vss(const wreport::Subset& subset, unsigned pos)
{
    /* Vertical significance */
    if (pos == 0) throw error_consistency("B08002 found at beginning of message");
    Varcode prev = subset[pos - 1].code();

    if (prev == WR_VAR(0, 20, 10))
    {
        // Normal cloud data
        level.ltype2 = 258;
        level.l2 = 0;
        return;
    }

    if (pos == subset.size() - 1) throw error_consistency("B08002 found at end of message");
    Varcode next = subset[pos + 1].code();

    switch (next)
    {
        case WR_VAR(0, 20, 11): {
            if (pos >= subset.size() - 3)
                throw error_consistency("B08002 followed by B20011 found less than 3 places before end of message");
            Varcode next2 = subset[pos + 3].code();
            if (next2 == WR_VAR(0, 20, 14))
            {
                // Clouds with bases below station level
                if (level.ltype2 != 263)
                {
                    level.ltype2 = 263;
                    level.l2 = 1;
                } else {
                    ++level.l2;
                }
            } else {
                /* Individual cloud groups */
                if (level.ltype2 != 259)
                {
                    level.ltype2 = 259;
                    level.l2 = 1;
                } else {
                    ++level.l2;
                }
            }
            break;
        } case WR_VAR(0, 20, 54):
            // Direction of cloud drift
            if (level.ltype2 != 260)
            {
                level.ltype2 = 260;
                level.l2 = 1;
            } else {
                ++level.l2;
            }
            break;
        default:
            break;
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


void UnsupportedContext::init()
{
    B08023 = nullptr;
}

bool UnsupportedContext::is_unsupported() const
{
    return B08023 != nullptr;
}

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


Interpreted::Interpreted(int shortcut, const wreport::Var& var)
{
    const auto& v = shortcutTable[shortcut];
    level = Level(v.ltype1, v.l1, v.ltype2, v.l2);
    trange = Trange(v.pind, v.p1, v.p2);
    this->var = var_copy_without_unset_attrs(var, v.code);
}

Interpreted::Interpreted(int shortcut, const wreport::Var& var, const Level& level, const Trange& trange)
    : level(level), trange(trange)
{
    const auto& v = shortcutTable[shortcut];
    this->var = var_copy_without_unset_attrs(var, v.code);
}

Interpreted::Interpreted(wreport::Varcode code, const wreport::Var& var, const Level& level, const Trange& trange)
    : level(level), trange(trange)
{
    this->var = var_copy_without_unset_attrs(var, code);
}

Interpreted::~Interpreted()
{
}

void Interpreted::set_sensor_height(const LevelContext& ctx, bool simplified)
{
    if (simplified)
    {
        if (ctx.height_sensor == 0) return;
        if (!ctx.height_sensor_seen) return;
        if (ctx.height_sensor == MISSING_SENSOR_H) return;
        if (level == Level(103, ctx.height_sensor * 1000)) return;
        var->seta(newvar(WR_VAR(0, 7, 32), ctx.height_sensor));
    }
    else
    {
        if (ctx.height_sensor == 0)
            level = Level(1);
        else if (ctx.height_sensor_seen)
            level = ctx.height_sensor == MISSING_SENSOR_H ? Level(103) : Level(103, ctx.height_sensor * 1000);
    }
}

void Interpreted::set_barometer_height(const LevelContext& ctx, bool simplified)
{
    if (ctx.height_baro == MISSING_BARO) return;

    if (simplified)
        var->seta(newvar(WR_VAR(0, 7, 31), ctx.height_baro));
    else
        level = Level(102, ctx.height_baro * 1000);
}

void Interpreted::set_duration(const TimerangeContext& ctx, bool simplified)
{
    if (simplified)
    {
        if (trange.pind == 254) return;
        if (!ctx.time_period_seen) return;
        if (ctx.time_period == MISSING_INT) return;
        Trange real = Trange(trange.pind, 0, abs(ctx.time_period));
        if (trange == real) return;
        var->seta(newvar(WR_VAR(0, 4, 194), abs(ctx.time_period)));
    }
    else
    {
        if (trange.pind == 254)
            trange = Trange::instant();
        else if (ctx.time_period_seen)
            trange = ctx.time_period == MISSING_INT ? Trange(trange.pind, 0) : Trange(trange.pind, 0, abs(ctx.time_period));
    }
}

void Interpreted::set_wind_mean(const TimerangeContext& ctx, bool simplified)
{
    Trange real;
    if (tr_std_wind.pind == 254) real = Trange::instant();
    else if (!ctx.time_period_seen) real = tr_std_wind;
    else real = ctx.time_period == MISSING_INT ? Trange(tr_std_wind.pind, 0) : Trange(tr_std_wind.pind, 0, abs(ctx.time_period));

    if (!simplified)
        trange = real; // Use real timerange
    else
    {
        if (real != trange && real != tr_std_wind)
        {
            // Use shortcut and preserve the rest
            if (ctx.time_period != MISSING_INT)
                var->seta(newvar(WR_VAR(0, 4, 194), abs(ctx.time_period)));
        }
    }
}

void Interpreted::to_msg(Msg& msg)
{
    msg.set(move(var), level, trange);
}


void SynopBaseImporter::set_gen_sensor(const Var& var, Varcode code, const Level& lev_std, const Trange& tr_std)
{
    if (unsupported.is_unsupported()) return;
    Interpreted res(code, var, lev_std, tr_std);
    res.set_sensor_height(level, opts.simplified);
    res.set_duration(trange, opts.simplified);
    res.to_msg(*msg);
}

void SynopBaseImporter::set_gen_sensor(const Var& var, int shortcut)
{
    if (unsupported.is_unsupported()) return;
    Interpreted res(shortcut, var);
    res.set_sensor_height(level, opts.simplified);
    res.set_duration(trange, opts.simplified);
    res.to_msg(*msg);
}

void SynopBaseImporter::set_baro_sensor(const Var& var, int shortcut)
{
    if (unsupported.is_unsupported()) return;
    Interpreted res(shortcut, var);
    res.set_barometer_height(level, opts.simplified);
    res.set_duration(trange, opts.simplified);
    res.to_msg(*msg);
}

void SynopBaseImporter::set_past_weather(const wreport::Var& var, int shortcut)
{
    if (unsupported.is_unsupported()) return;
    Interpreted res(shortcut, var);
    res.trange = Trange((trange.hour % 6 == 0) ? tr_std_past_wtr6 : tr_std_past_wtr3);
    res.set_duration(trange, opts.simplified);
    res.to_msg(*msg);
}

void SynopBaseImporter::set_wind(const wreport::Var& var, int shortcut)
{
    if (trange.time_sig != MISSING_TIME_SIG && trange.time_sig != 2)
        error_consistency::throwf("Found unsupported time significance %d for wind direction", trange.time_sig);

    if (unsupported.is_unsupported()) return;
    Interpreted res(shortcut, var);
    res.level = lev_std_wind;
    res.set_sensor_height(level, opts.simplified);
    res.set_wind_mean(trange, opts.simplified);
    res.to_msg(*msg);
}

void SynopBaseImporter::set_wind_max(const wreport::Var& var, int shortcut)
{
    if (unsupported.is_unsupported()) return;
    Interpreted res(shortcut, var, lev_std_wind, tr_std_wind_max10m);
    res.set_sensor_height(level, opts.simplified);
    res.set_duration(trange); // Always use real trange
    res.to_msg(*msg);
}

void SynopBaseImporter::set_pressure(const wreport::Var& var)
{
    if (level.press_std == MISSING_PRESS_STD)
        set(var, WR_VAR(0, 10,  8), Level(100), Trange::instant());
    else
        set(var, WR_VAR(0, 10,  8), Level(100, level.press_std), Trange::instant());
}

void SynopBaseImporter::set_water_temperature(const wreport::Var& var)
{
    if (level.depth == MISSING_SENSOR_H)
        set(var, WR_VAR(0, 22, 43), Level(1), Trange::instant());
    else
        set(var, WR_VAR(0, 22, 43), Level(160, level.depth * 1000), Trange::instant());
}

void SynopBaseImporter::set_swell_waves(const wreport::Var& var)
{
    set(var, var.code(), Level(264, MISSING_INT, 261, level.swell_wave_group), Trange::instant());
}

void SynopBaseImporter::set(const wreport::Var& var, int shortcut)
{
    if (unsupported.is_unsupported()) return;
    WMOImporter::set(var, shortcut);
}

void SynopBaseImporter::set(const wreport::Var& var, wreport::Varcode code, const Level& level, const Trange& trange)
{
    if (unsupported.is_unsupported()) return;
    WMOImporter::set(var, code, level, trange);
}

SynopBaseImporter::SynopBaseImporter(const msg::Importer::Options& opts)
    : WMOImporter(opts)
{
}

void SynopBaseImporter::init()
{
    WMOImporter::init();
    clouds.init();
    level.init();
    trange.init();
    unsupported.init();
}

void SynopBaseImporter::run()
{
    for (pos = 0; pos < subset->size(); ++pos)
    {
        const Var& var = (*subset)[pos];
        if (WR_VAR_F(var.code()) != 0) continue;
        if (WR_VAR_X(var.code()) < 10 || var.code() == WR_VAR(0, 22, 3)) peek_var(var);
        if (var.isset()) import_var(var);
    }
}

void SynopBaseImporter::peek_var(const Var& var)
{
    unsupported.peek_var(var, pos);

    switch (var.code())
    {
        case WR_VAR(0,  4,  4):
        case WR_VAR(0,  4, 24):
        case WR_VAR(0,  4, 25):
        case WR_VAR(0,  8, 21): trange.peek_var(var, pos); break;
        case WR_VAR(0,  8,  2): clouds.on_vss(*subset, pos); break;
        case WR_VAR(0,  7,  4):
        case WR_VAR(0,  7, 31):
        case WR_VAR(0,  7, 32):
        case WR_VAR(0,  7, 63): level.peek_var(var); break;
        case WR_VAR(0, 22,  3): ++level.swell_wave_group; break;
    }
}

void SynopBaseImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0,  8,  2):
            // Store original VS value as a measured value
            set(var, WR_VAR(0, 8, 2), clouds.level, Trange::instant());
            break;

        // Ship identification, movement, date/time, horizontal and vertical
        // coordinates
        case WR_VAR(0,  7,  1):
        case WR_VAR(0,  7, 30): set(var, DBA_MSG_HEIGHT_STATION); break;
        case WR_VAR(0,  7, 31):
            /* Store also in the ana level, so that if the
             * pressure later is missing we still have
             * access to the value */
            set(var, DBA_MSG_HEIGHT_BARO);
            break;

        // Pressure data (complete)
        case WR_VAR(0, 10,  4): set_baro_sensor(var, DBA_MSG_PRESS); break;
        case WR_VAR(0, 10, 51): set_baro_sensor(var, DBA_MSG_PRESS_MSL); break;
        case WR_VAR(0, 10, 61): set_baro_sensor(var, DBA_MSG_PRESS_3H); break;
        case WR_VAR(0, 10, 62): set_baro_sensor(var, DBA_MSG_PRESS_24H); break;
        case WR_VAR(0, 10, 63): set_baro_sensor(var, DBA_MSG_PRESS_TEND); break;
        case WR_VAR(0, 10,  3):
        case WR_VAR(0, 10,  8):
        case WR_VAR(0, 10,  9): set_pressure(var); break;

        // Ship “instantaneous” data

        // Temperature and humidity data (complete)
        case WR_VAR(0, 12,   4):
        case WR_VAR(0, 12, 101): set_gen_sensor(var, DBA_MSG_TEMP_2M); break;
        case WR_VAR(0, 12,   6):
        case WR_VAR(0, 12, 103): set_gen_sensor(var, DBA_MSG_DEWPOINT_2M); break;
        case WR_VAR(0, 13,   3): set_gen_sensor(var, DBA_MSG_HUMIDITY); break;
        case WR_VAR(0, 12,   2):
        case WR_VAR(0, 12, 102): set_gen_sensor(var, DBA_MSG_WET_TEMP_2M); break;

        // Visibility data (complete)
        case WR_VAR(0, 20,  1): set_gen_sensor(var, DBA_MSG_VISIBILITY); break;

        // Precipitation past 24h (complete)
        case WR_VAR(0, 13, 19): set_gen_sensor(var, DBA_MSG_TOT_PREC1); break;
        case WR_VAR(0, 13, 20): set_gen_sensor(var, DBA_MSG_TOT_PREC3); break;
        case WR_VAR(0, 13, 21): set_gen_sensor(var, DBA_MSG_TOT_PREC6); break;
        case WR_VAR(0, 13, 22): set_gen_sensor(var, DBA_MSG_TOT_PREC12); break;
        case WR_VAR(0, 13, 23): set_gen_sensor(var, DBA_MSG_TOT_PREC24); break;

        // Cloud data
        case WR_VAR(0, 20, 10): set(var, DBA_MSG_CLOUD_N); break;

        // Individual cloud layers or masses (complete)
        // Clouds with bases below station level (complete)
        // Direction of cloud drift (complete)
        case WR_VAR(0, 20, 11):
        case WR_VAR(0, 20, 13):
        case WR_VAR(0, 20, 17):
        case WR_VAR(0, 20, 54): set(var, var.code(), clouds.level, Trange::instant()); break;
        case WR_VAR(0, 20, 12): // CH CL CM
            set(var, WR_VAR(0, 20, 12), clouds.clcmch(), Trange::instant());
            break;

        // Present and past weather (complete)
        case WR_VAR(0, 20,  3): set(var, DBA_MSG_PRES_WTR); break;
        case WR_VAR(0, 20,  4): set_past_weather(var, DBA_MSG_PAST_WTR1_6H); break;
        case WR_VAR(0, 20,  5): set_past_weather(var, DBA_MSG_PAST_WTR2_6H); break;

        // Precipitation measurement (complete)
        case WR_VAR(0, 13, 11): set_gen_sensor(var, WR_VAR(0, 13, 11), Level(1), Trange(1, 0, abs(trange.time_period))); break;

        // Extreme temperature data
        case WR_VAR(0, 12, 111):
            set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1), Trange(2, -abs(trange.time_period_offset), abs(trange.time_period)));
            break;
        case WR_VAR(0, 12, 112):
            set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1), Trange(3, -abs(trange.time_period_offset), abs(trange.time_period)));
            break;

        // Wind data (complete)
        case WR_VAR(0, 2, 2): set(var, DBA_MSG_WIND_INST); break;

        /* Note B/C 1.10.5.3.2 Calm shall be reported by
         * setting wind direction to 0 and wind speed to 0.
         * Variable shall be reported by setting wind direction
         * to 0 and wind speed to a positive value, not a
         * missing value indicator.
         */
        case WR_VAR(0, 11,  1):
        case WR_VAR(0, 11, 11): set_wind(var, DBA_MSG_WIND_DIR); break;
        case WR_VAR(0, 11,  2):
        case WR_VAR(0, 11, 12): set_wind(var, DBA_MSG_WIND_SPEED); break;
        case WR_VAR(0, 11, 43): set_wind_max(var, DBA_MSG_WIND_GUST_MAX_DIR); break;
        case WR_VAR(0, 11, 41): set_wind_max(var, DBA_MSG_WIND_GUST_MAX_SPEED); break;

        case WR_VAR(0, 12,  5): set(var, DBA_MSG_WET_TEMP_2M); break;
        case WR_VAR(0, 10,197): set(var, DBA_MSG_HEIGHT_ANEM); break;

        default: WMOImporter::import_var(var); break;
    }
}

}
}
}
