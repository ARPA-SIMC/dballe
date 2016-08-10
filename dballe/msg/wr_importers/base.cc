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
        case WR_VAR(0,  1,  1): msg->set_block_var(var); break;
        case WR_VAR(0,  1,  2): msg->set_station_var(var); break;
        case WR_VAR(0,  1,  5):
        case WR_VAR(0,  1,  6):
        case WR_VAR(0,  1, 11): msg->set_ident_var(var); break;
        case WR_VAR(0,  1, 12): msg->set_st_dir_var(var); break;
        case WR_VAR(0,  1, 13): msg->set_st_speed_var(var); break;
        case WR_VAR(0,  1, 63): msg->set_st_name_icao_var(var); break;
        case WR_VAR(0,  2,  1): msg->set_st_type_var(var); break;
        case WR_VAR(0,  1, 15): msg->set_st_name_var(var); break;
        case WR_VAR(0,  4,  1): ye = var.enqi(); if (var.next_attr()) msg->set_year_var(var); break;
        case WR_VAR(0,  4,  2): mo = var.enqi(); if (var.next_attr()) msg->set_month_var(var); break;
        case WR_VAR(0,  4,  3): da = var.enqi(); if (var.next_attr()) msg->set_day_var(var); break;
        case WR_VAR(0,  4,  4): ho = var.enqi(); if (var.next_attr()) msg->set_hour_var(var); break;
        case WR_VAR(0,  4,  5): mi = var.enqi(); if (var.next_attr()) msg->set_minute_var(var); break;
        case WR_VAR(0,  4,  6): se = var.enqi(); if (var.next_attr()) msg->set_second_var(var); break;
        case WR_VAR(0,  5,  1):
        case WR_VAR(0,  5,  2): msg->set_latitude_var(var); break;
        case WR_VAR(0,  6,  1):
        case WR_VAR(0,  6,  2): msg->set_longitude_var(var); break;
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

Level LevelContext::get_real(const Level& standard) const
{
    if (height_sensor == 0) return Level(1);
    if (!height_sensor_seen) return standard;
    return height_sensor == MISSING_SENSOR_H ?
        Level(103) :
        Level(103, height_sensor * 1000);
}

Trange TimerangeContext::get_real(const Trange& standard) const
{
    if (standard.pind == 254) return Trange::instant();
    if (!time_period_seen) return standard;
    return time_period == MISSING_INT ?
        Trange(standard.pind, 0) :
        Trange(standard.pind, 0, abs(time_period));
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


Interpreted::Interpreted(int shortcut, const wreport::Var& var)
{
    const auto& v = shortcutTable[shortcut];
    level = Level(v.ltype1, v.l1, v.ltype2, v.l2);
    trange = Trange(v.pind, v.p1, v.p2);
    this->var = var_copy_without_unset_attrs(var, v.code);
}

Interpreted::~Interpreted()
{
}

void Interpreted::to_msg(Msg& msg)
{
    msg.set(move(var), level, trange);
}

void Interpreted::annotate_level(const LevelContext& level_context)
{
    if (level_context.height_sensor == MISSING_SENSOR_H) return;
    var->seta(newvar(WR_VAR(0, 7, 32), level_context.height_sensor));
}

void Interpreted::annotate_trange(const TimerangeContext& trange_context)
{
    if (trange_context.time_period == MISSING_INT) return;
    var->seta(newvar(WR_VAR(0, 4, 194), abs(trange_context.time_period)));
}


void SynopBaseImporter::set_gen_sensor(const Var& var, Varcode code, const Level& defaultLevel, const Trange& trange)
{
    if (!level.height_sensor_seen ||
                (level.height_sensor != MISSING_SENSOR_H && (
                    defaultLevel == Level(103, level.height_sensor * 1000)
                || (defaultLevel.ltype1 == 1 && level.height_sensor == 0))))
        msg->set(var, code, defaultLevel, trange);
    else if (level.height_sensor == MISSING_SENSOR_H)
    {
        if (opts.simplified)
            msg->set(var, code, defaultLevel, trange);
        else
            msg->set(var, code, Level(103, MISSING_INT), trange);
    }
    else if (opts.simplified)
    {
        Var var1(var);
        var1.seta(newvar(WR_VAR(0, 7, 32), level.height_sensor));
        msg->set(var1, code, defaultLevel, trange);
    } else
        msg->set(var, code, Level(103, level.height_sensor * 1000), trange);
}

void SynopBaseImporter::set_gen_sensor(const Var& var, int shortcut)
{
    const MsgVarShortcut& v = shortcutTable[shortcut];
    set_gen_sensor(var, shortcut, Level(v.ltype1, v.l1, v.ltype2, v.l2), Trange(v.pind, v.p1, v.p2));
}

void SynopBaseImporter::set_gen_sensor(const Var& var, int shortcut, const Trange& tr_std, bool tr_careful)
{
    Interpreted res(shortcut, var);

    if (!opts.simplified)
        res.trange = trange.get_real(tr_std); // Use real timerange
    else {
        Trange real = trange.get_real(tr_std);
        if (real != res.trange && real != tr_std)
        {
            if (tr_careful)
                // Use shortcut if standard, else real
                res.trange = real;
            else
                // Use shortcut and preserve the rest
                res.annotate_trange(trange);
        }
    }

    res.to_msg(*msg);
}

void SynopBaseImporter::set_gen_sensor(const Var& var, int shortcut, const Level& lev_std, const Trange& tr_std, bool lev_careful, bool tr_careful)
{
    Interpreted res(shortcut, var);

    if (!opts.simplified)
    {
        res.level = level.get_real(lev_std); // Use real level
        res.trange = trange.get_real(tr_std); // Use real timerange
    }
    else
    {
        Level lreal = level.get_real(lev_std);
        if (lreal != res.level && lreal != lev_std)
        {
            if (lev_careful)
                // use shorcut level if standard, else real
                res.level = lreal;
            else
                // use shourtcut level and preserve the real value
                res.annotate_level(level);
        }

        Trange treal = trange.get_real(tr_std);
        if (treal != res.trange && treal != tr_std)
        {
            if (tr_careful)
                // Use shortcut if standard, else real
                res.trange = treal;
            else
                // Use shortcut and preserve the rest
                res.annotate_trange(trange);
        }
    }

    res.to_msg(*msg);
}

void SynopBaseImporter::set_baro_sensor(const Var& var, int shortcut)
{
    if (level.height_baro == MISSING_BARO)
        msg->set_by_id(var, shortcut);
    else if (opts.simplified)
    {
        Var var1(var);
        var1.seta(newvar(WR_VAR(0, 7, 31), level.height_baro));
        msg->set_by_id(var1, shortcut);
    } else {
        const MsgVarShortcut& v = shortcutTable[shortcut];
        msg->set(var, v.code, Level(102, level.height_baro*1000), Trange(v.pind, v.p1, v.p2));
    }
}

void SynopBaseImporter::set_past_weather(const wreport::Var& var, int shortcut)
{
    Interpreted res(shortcut, var);
    res.trange = Trange((trange.hour % 6 == 0) ? tr_std_past_wtr6 : tr_std_past_wtr3);
    if (opts.simplified)
    {
        // Use standard and preserve the rest
        if (res.trange != trange.get_real(res.trange))
            res.annotate_trange(trange);
    }
    else
        res.trange = trange.get_real(res.trange); // Use real timerange
    res.to_msg(*msg);
}

void SynopBaseImporter::set_wind(const wreport::Var& var, int shortcut)
{
    if (trange.time_sig != MISSING_TIME_SIG && trange.time_sig != 2)
        error_consistency::throwf("Found unsupported time significance %d for wind direction", trange.time_sig);
    set_gen_sensor(var, shortcut, lev_std_wind, tr_std_wind);
}

void SynopBaseImporter::set_wind_max(const wreport::Var& var, int shortcut)
{
    set_gen_sensor(var, shortcut, lev_std_wind, tr_std_wind_max10m, false, true);
}

void SynopBaseImporter::set_pressure(const wreport::Var& var)
{
    if (level.press_std == MISSING_PRESS_STD)
        msg->set(var, WR_VAR(0, 10,  8), Level(100), Trange::instant());
    else
        msg->set(var, WR_VAR(0, 10,  8), Level(100, level.press_std), Trange::instant());
}

void SynopBaseImporter::set_water_temperature(const wreport::Var& var)
{
    if (level.depth == MISSING_SENSOR_H)
        msg->set(var, WR_VAR(0, 22, 43), Level(1), Trange::instant());
    else
        msg->set(var, WR_VAR(0, 22, 43), Level(160, level.depth * 1000), Trange::instant());
}

void SynopBaseImporter::set_swell_waves(const wreport::Var& var)
{
    msg->set(var, var.code(), Level(264, MISSING_INT, 261, level.swell_wave_group), Trange::instant());
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
