/*
 * dballe/wr_importers/base - Base infrastructure for wreport importers
 *
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "base.h"
#include "msg/msgs.h"
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

void Importer::import(const wreport::Subset& subset, Msg& msg)
{
    this->subset = &subset;
    this->msg = &msg;
    datetime = Datetime();
    init();
    run();
    if (datetime.date.year == 0xffff)
        msg.set_datetime(Datetime());
    else
    {
        if (datetime.date.month == 0xff)
            throw error_consistency("no month information found in message to import");
        if (datetime.date.day == 0xff)
            throw error_consistency("no day information found in message to import");
        if (datetime.time.hour == 0xff)
            throw error_consistency("no hour information found in message to import");
        msg.set_datetime(datetime.lower_bound());
    }
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
        case WR_VAR(0,  4,  1):
            datetime.date.year = var.enqi();
            if (var.next_attr())
                msg->set(var, WR_VAR(0, 4, 1), Level::ana(), Trange::ana());
            break;
        case WR_VAR(0,  4,  2):
            datetime.date.month = var.enqi();
            if (var.next_attr())
                msg->set(var, WR_VAR(0, 4, 2), Level::ana(), Trange::ana());
            break;
        case WR_VAR(0,  4,  3):
            datetime.date.day = var.enqi();
            if (var.next_attr())
                msg->set(var, WR_VAR(0, 4, 3), Level::ana(), Trange::ana());
            break;
        case WR_VAR(0,  4,  4):
            datetime.time.hour = var.enqi();
            if (var.next_attr())
                msg->set(var, WR_VAR(0, 4, 4), Level::ana(), Trange::ana());
            break;
        case WR_VAR(0,  4,  5):
            datetime.time.minute = var.enqi();
            if (var.next_attr())
                msg->set(var, WR_VAR(0, 4, 5), Level::ana(), Trange::ana());
            break;
        case WR_VAR(0,  4,  6):
            datetime.time.second = var.enqi();
            if (var.next_attr())
                msg->set(var, WR_VAR(0, 4, 6), Level::ana(), Trange::ana());
            break;
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
                if ((int)pos == last_B04024_pos + 1 && var.enqi() != 0)
                {
                    // Cope with the weird idea of using B04024 twice to indicate
                    // beginning and end of a period not ending with the SYNOP
                    // reference time
                    if (time_period != MISSING_INT)
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

ContextChooser::ContextChooser(const LevelContext& level, const TimerangeContext& trange)
    : level(level), trange(trange), simplified(false), var(0)
{
}

ContextChooser::~ContextChooser()
{
    if (var) delete var;
}

void ContextChooser::init(Msg& _msg, bool simplified)
{
    this->simplified = simplified;
    msg = &_msg;
}

void ContextChooser::ib_start(int shortcut, const Var& var)
{
    v = &shortcutTable[shortcut];
    if (this->var)
        delete this->var;
    this->var = var_copy_without_unset_attrs(var, v->code).release();
}

Level ContextChooser::lev_real(const Level& standard) const
{
    if (level.height_sensor == 0) return Level(1);
    if (!level.height_sensor_seen) return standard;
    return level.height_sensor == MISSING_SENSOR_H ?
        Level(103) :
        Level(103, level.height_sensor * 1000);
}

Trange ContextChooser::tr_real(const Trange& standard) const
{
    if (standard.pind == 254) return Trange::instant();
    if (!trange.time_period_seen) return standard;
    return trange.time_period == MISSING_INT ?
        Trange(standard.pind, 0) :
        Trange(standard.pind, 0, abs(trange.time_period));
}

void ContextChooser::ib_annotate_level()
{
    if (level.height_sensor == MISSING_SENSOR_H) return;
    var->seta(ap_newvar(WR_VAR(0, 7, 32), level.height_sensor));
}
void ContextChooser::ib_annotate_trange()
{
    if (trange.time_period == MISSING_INT) return;
    var->seta(ap_newvar(WR_VAR(0, 4, 194), abs(trange.time_period)));
}

void ContextChooser::ib_level_use_shorcut_and_preserve_rest(const Level& standard)
{
    chosen_lev = lev_shortcut();
    Level real = lev_real(standard);
    if (real != chosen_lev && real != standard)
        ib_annotate_level();
}

void ContextChooser::ib_trange_use_shorcut_and_preserve_rest(const Trange& standard)
{
    chosen_tr = tr_shortcut();
    Trange real = tr_real(standard);
    if (real != chosen_tr && real != standard)
        ib_annotate_trange();
}

void ContextChooser::ib_level_use_standard_and_preserve_rest(const Level& standard)
{
    chosen_lev = standard;
    if (chosen_lev != lev_real(standard))
        ib_annotate_level();
}

void ContextChooser::ib_trange_use_standard_and_preserve_rest(const Trange& standard)
{
    chosen_tr = standard;
    if (chosen_tr != tr_real(standard))
        ib_annotate_trange();
}

void ContextChooser::ib_level_use_shorcut_if_standard_else_real(const Level& standard)
{
    Level shortcut = lev_shortcut();
    Level real = lev_real(standard);
    if (real == shortcut || real == standard)
        chosen_lev = shortcut;
    else
        chosen_lev = real;
}

void ContextChooser::ib_trange_use_shorcut_if_standard_else_real(const Trange& standard)
{
    Trange shortcut = tr_shortcut();
    Trange real = tr_real(standard);
    if (real == shortcut || real == standard)
        chosen_tr = shortcut;
    else
        chosen_tr = real;
}

void ContextChooser::ib_set()
{
    unique_ptr<Var> handover(var);
    var = 0;
    msg->set(move(handover), chosen_lev, chosen_tr);
}

void ContextChooser::set_gen_sensor(const Var& var, Varcode code, const Level& defaultLevel, const Trange& trange)
{
    Level lev;

    if (!level.height_sensor_seen ||
                (level.height_sensor != MISSING_SENSOR_H && (
                    defaultLevel == Level(103, level.height_sensor * 1000)
                || (defaultLevel.ltype1 == 1 && level.height_sensor == 0))))
        msg->set(var, code, defaultLevel, trange);
    else if (level.height_sensor == MISSING_SENSOR_H)
    {
        if (simplified)
            msg->set(var, code, defaultLevel, trange);
        else
            msg->set(var, code, Level(103, MISSING_INT), trange);
    }
    else if (simplified)
    {
        Var var1(var);
        var1.seta(auto_ptr<Var>(newvar(WR_VAR(0, 7, 32), level.height_sensor).release()));
        msg->set(var1, code, defaultLevel, trange);
    } else
        msg->set(var, code, Level(103, level.height_sensor * 1000), trange);
}

void ContextChooser::set_gen_sensor(const Var& var, int shortcut)
{
    const MsgVarShortcut& v = shortcutTable[shortcut];
    set_gen_sensor(var, shortcut, Level(v.ltype1, v.l1, v.ltype2, v.l2), Trange(v.pind, v.p1, v.p2));
}

void ContextChooser::set_gen_sensor(const Var& var, int shortcut, const Trange& tr_std, bool tr_careful)
{
    ib_start(shortcut, var);
    ib_level_use_shorcut_and_discard_rest();
    if (!simplified)
        ib_trange_use_real(tr_std);
    else if (tr_careful)
        ib_trange_use_shorcut_if_standard_else_real(tr_std);
    else
        ib_trange_use_shorcut_and_preserve_rest(tr_std);
    ib_set();
}

void ContextChooser::set_gen_sensor(const Var& var, int shortcut, const Level& lev_std, const Trange& tr_std, bool lev_careful, bool tr_careful)
{
    ib_start(shortcut, var);
    if (!simplified)
    {
        ib_level_use_real(lev_std);
        ib_trange_use_real(tr_std);
    }
    else
    {
        if (lev_careful)
            ib_level_use_shorcut_if_standard_else_real(lev_std);
        else
            ib_level_use_shorcut_and_preserve_rest(lev_std);
        if (tr_careful)
            ib_trange_use_shorcut_if_standard_else_real(tr_std);
        else
            ib_trange_use_shorcut_and_preserve_rest(tr_std);
    }
    ib_set();
}

void ContextChooser::set_baro_sensor(const Var& var, int shortcut)
{
    if (level.height_baro == MISSING_BARO)
        msg->set_by_id(var, shortcut);
    else if (simplified)
    {
        Var var1(var);
        var1.seta(auto_ptr<Var>(newvar(WR_VAR(0, 7, 31), level.height_baro).release()));
        msg->set_by_id(var1, shortcut);
    } else {
        const MsgVarShortcut& v = shortcutTable[shortcut];
        msg->set(var, v.code, Level(102, level.height_baro*1000), Trange(v.pind, v.p1, v.p2));
    }
}

void ContextChooser::set_past_weather(const wreport::Var& var, int shortcut)
{
    ib_start(shortcut, var);
    ib_level_use_shorcut_and_discard_rest();
    if (simplified)
        ib_trange_use_standard_and_preserve_rest((trange.hour % 6 == 0) ? tr_std_past_wtr6 : tr_std_past_wtr3);
    else
        ib_trange_use_real((trange.hour % 6 == 0) ? tr_std_past_wtr6 : tr_std_past_wtr3);
    ib_set();
}

void ContextChooser::set_wind(const wreport::Var& var, int shortcut)
{
    if (trange.time_sig != MISSING_TIME_SIG && trange.time_sig != 2)
        error_consistency::throwf("Found unsupported time significance %d for wind direction", trange.time_sig);
    set_gen_sensor(var, shortcut, lev_std_wind, tr_std_wind);
}

void ContextChooser::set_wind_max(const wreport::Var& var, int shortcut)
{
    set_gen_sensor(var, shortcut, lev_std_wind, tr_std_wind_max10m, false, true);
}

void ContextChooser::set_pressure(const wreport::Var& var)
{
    if (level.press_std == MISSING_PRESS_STD)
        msg->set(var, WR_VAR(0, 10,  8), Level(100), Trange::instant());
    else
        msg->set(var, WR_VAR(0, 10,  8), Level(100, level.press_std), Trange::instant());
}

void ContextChooser::set_water_temperature(const wreport::Var& var)
{
    if (level.depth == MISSING_SENSOR_H)
        msg->set(var, WR_VAR(0, 22, 43), Level(1), Trange::instant());
    else
        msg->set(var, WR_VAR(0, 22, 43), Level(160, level.depth * 1000), Trange::instant());
}

void ContextChooser::set_swell_waves(const wreport::Var& var)
{
    msg->set(var, var.code(), Level::waves(261, level.swell_wave_group), Trange::instant());
}

SynopBaseImporter::SynopBaseImporter(const msg::Importer::Options& opts)
    : WMOImporter(opts), ctx(level, trange)
{
}

void SynopBaseImporter::init()
{
    WMOImporter::init();
    clouds.init();
    level.init();
    trange.init();
    ctx.init(*msg, opts.simplified);
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
            msg->set(var, WR_VAR(0, 8, 2), clouds.level, Trange::instant());
            break;

        // Ship identification, movement, date/time, horizontal and vertical
        // coordinates
        case WR_VAR(0,  7,  1):
        case WR_VAR(0,  7, 30): msg->set_height_station_var(var); break;
        case WR_VAR(0,  7, 31):
            /* Store also in the ana level, so that if the
             * pressure later is missing we still have
             * access to the value */
            msg->set_height_baro_var(var);
            break;

        // Pressure data (complete)
        case WR_VAR(0, 10,  4): ctx.set_baro_sensor(var, DBA_MSG_PRESS); break;
        case WR_VAR(0, 10, 51): ctx.set_baro_sensor(var, DBA_MSG_PRESS_MSL); break;
        case WR_VAR(0, 10, 61): ctx.set_baro_sensor(var, DBA_MSG_PRESS_3H); break;
        case WR_VAR(0, 10, 62): ctx.set_baro_sensor(var, DBA_MSG_PRESS_24H); break;
        case WR_VAR(0, 10, 63): ctx.set_baro_sensor(var, DBA_MSG_PRESS_TEND); break;
        case WR_VAR(0, 10,  3):
        case WR_VAR(0, 10,  8):
        case WR_VAR(0, 10,  9): ctx.set_pressure(var); break;

        // Ship “instantaneous” data

        // Temperature and humidity data (complete)
        case WR_VAR(0, 12,   4):
        case WR_VAR(0, 12, 101): ctx.set_gen_sensor(var, DBA_MSG_TEMP_2M); break;
        case WR_VAR(0, 12,   6):
        case WR_VAR(0, 12, 103): ctx.set_gen_sensor(var, DBA_MSG_DEWPOINT_2M); break;
        case WR_VAR(0, 13,   3): ctx.set_gen_sensor(var, DBA_MSG_HUMIDITY); break;
        case WR_VAR(0, 12,   2):
        case WR_VAR(0, 12, 102): ctx.set_gen_sensor(var, DBA_MSG_WET_TEMP_2M); break;

        // Visibility data (complete)
        case WR_VAR(0, 20,  1): ctx.set_gen_sensor(var, DBA_MSG_VISIBILITY); break;

        // Precipitation past 24h (complete)
        case WR_VAR(0, 13, 19): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC1); break;
        case WR_VAR(0, 13, 20): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC3); break;
        case WR_VAR(0, 13, 21): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC6); break;
        case WR_VAR(0, 13, 22): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC12); break;
        case WR_VAR(0, 13, 23): ctx.set_gen_sensor(var, DBA_MSG_TOT_PREC24); break;

        // Cloud data
        case WR_VAR(0, 20, 10): msg->set_cloud_n_var(var); break;

        // Individual cloud layers or masses (complete)
        // Clouds with bases below station level (complete)
        // Direction of cloud drift (complete)
        case WR_VAR(0, 20, 11):
        case WR_VAR(0, 20, 13):
        case WR_VAR(0, 20, 17):
        case WR_VAR(0, 20, 54): msg->set(var, var.code(), clouds.level, Trange::instant()); break;
        case WR_VAR(0, 20, 12): // CH CL CM
            msg->set(var, WR_VAR(0, 20, 12), clouds.clcmch(), Trange::instant());
            break;

        // Present and past weather (complete)
        case WR_VAR(0, 20,  3): msg->set_pres_wtr_var(var); break;
        case WR_VAR(0, 20,  4): ctx.set_past_weather(var, DBA_MSG_PAST_WTR1_6H); break;
        case WR_VAR(0, 20,  5): ctx.set_past_weather(var, DBA_MSG_PAST_WTR2_6H); break;

        // Precipitation measurement (complete)
        case WR_VAR(0, 13, 11): ctx.set_gen_sensor(var, WR_VAR(0, 13, 11), Level(1), Trange(1, 0, abs(trange.time_period))); break;

        // Extreme temperature data
        case WR_VAR(0, 12, 111):
            ctx.set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1), Trange(2, -abs(trange.time_period_offset), abs(trange.time_period)));
            break;
        case WR_VAR(0, 12, 112):
            ctx.set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1), Trange(3, -abs(trange.time_period_offset), abs(trange.time_period)));
            break;

        // Wind data (complete)
        case WR_VAR(0, 2, 2): msg->set_wind_inst_var(var); break;

        /* Note B/C 1.10.5.3.2 Calm shall be reported by
         * setting wind direction to 0 and wind speed to 0.
         * Variable shall be reported by setting wind direction
         * to 0 and wind speed to a positive value, not a
         * missing value indicator.
         */
        case WR_VAR(0, 11,  1):
        case WR_VAR(0, 11, 11): ctx.set_wind(var, DBA_MSG_WIND_DIR); break;
        case WR_VAR(0, 11,  2):
        case WR_VAR(0, 11, 12): ctx.set_wind(var, DBA_MSG_WIND_SPEED); break;
        case WR_VAR(0, 11, 43): ctx.set_wind_max(var, DBA_MSG_WIND_GUST_MAX_DIR); break;
        case WR_VAR(0, 11, 41): ctx.set_wind_max(var, DBA_MSG_WIND_GUST_MAX_SPEED); break;

        case WR_VAR(0, 12,  5): msg->set_wet_temp_2m_var(var); break;
        case WR_VAR(0, 10,197): msg->set_height_anem_var(var); break;

        default: WMOImporter::import_var(var); break;
    }
}

} // namespace wr
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
