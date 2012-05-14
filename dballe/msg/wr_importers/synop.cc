/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <cstdlib>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

#define MISSING_BARO -10000
#define MISSING_PRESS_STD 0.0
#define MISSING_SENSOR_H -10000.0
#define MISSING_VSS 63
#define MISSING_TIME_SIG -10000

namespace {

static const Level lev_ground(1);
static const Level lev_std_wind(103, 10*1000);
static const Trange tr_std_wind(200, 0, 600);
static const Trange tr_std_wind_max10m(205, 0, 600);
static const Trange tr_std_past_wtr3(205, 0, 10800);
static const Trange tr_std_past_wtr6(205, 0, 21600);

class SynopImporter : public WMOImporter
{
protected:
    Level cloudlevel;
    double height_baro;
    double press_std;
    double height_sensor;
    bool height_sensor_seen;
    int vs;
    int time_period;
    int time_period_offset;
    bool time_period_seen;
    int time_sig;
    int hour;

    // Import builder parts
    const MsgVarShortcut* v;
    Var* temp_var;
    Level chosen_lev;
    Trange chosen_tr;
    const Var* chosen_var;

    void peek_var(const Var& var);
    void import_var_undef(const Var& var);
    void import_var(const Var& var);

    void ib_start(int shortcut, const Var& var)
    {
        v = &shortcutTable[shortcut];
        chosen_var = &var;
        if (temp_var)
        {
            delete temp_var;
            temp_var = 0;
        }
    }

    Level lev_real(const Level& standard) const
    {
        if (height_sensor == 0) return Level(1);
        if (!height_sensor_seen) return standard;
        return height_sensor == MISSING_SENSOR_H ?
            Level(103) :
            Level(103, height_sensor * 1000);
    }

    Trange tr_real(const Trange& standard) const
    {
        if (standard.pind == 254) return Trange::instant();
        if (!time_period_seen) return standard;
        return time_period == MISSING_INT ?
            Trange(standard.pind, 0) :
            Trange(standard.pind, 0, abs(time_period));
    }

    Level lev_shortcut() const { return Level(v->ltype1, v->l1, v->ltype2, v->l2); }
    Trange tr_shortcut() const { return Trange(v->pind, v->p1, v->p2); }

    void ib_use_temp_var()
    {
        if (!temp_var)
        {
            temp_var = new Var(*chosen_var);
            chosen_var = temp_var;
        }
    }
    void ib_annotate_level()
    {
        if (height_sensor == MISSING_SENSOR_H) return;
        ib_use_temp_var();
        temp_var->seta(newvar(WR_VAR(0, 7, 32), height_sensor));
    }
    void ib_annotate_trange()
    {
        if (time_period == MISSING_INT) return;
        ib_use_temp_var();
        temp_var->seta(newvar(WR_VAR(0, 4, 194), abs(time_period)));
    }

    void ib_level_use_real(const Level& standard) { chosen_lev = lev_real(standard); }
    void ib_trange_use_real(const Trange& standard) { chosen_tr = tr_real(standard); }

    void ib_level_use_shorcut_and_discard_rest() { chosen_lev = lev_shortcut(); }
    void ib_trange_use_shortcut_and_discard_rest() { chosen_tr = tr_shortcut(); }

    void ib_level_use_shorcut_and_preserve_rest(const Level& standard)
    {
        chosen_lev = lev_shortcut();
        Level real = lev_real(standard);
        if (real != chosen_lev && real != standard)
            ib_annotate_level();
    }

    void ib_trange_use_shorcut_and_preserve_rest(const Trange& standard)
    {
        chosen_tr = tr_shortcut();
        Trange real = tr_real(standard);
        if (real != chosen_tr && real != standard)
            ib_annotate_trange();
    }

    void ib_level_use_standard_and_preserve_rest(const Level& standard)
    {
        chosen_lev = standard;
        if (chosen_lev != lev_real(standard))
            ib_annotate_level();
    }

    void ib_trange_use_standard_and_preserve_rest(const Trange& standard)
    {
        chosen_tr = standard;
        if (chosen_tr != tr_real(standard))
            ib_annotate_trange();
    }

    void ib_level_use_shorcut_if_standard_else_real(const Level& standard)
    {
        Level shortcut = lev_shortcut();
        Level real = lev_real(standard);
        if (real == shortcut || real == standard)
            chosen_lev = shortcut;
        else
            chosen_lev = real;
    }

    void ib_trange_use_shorcut_if_standard_else_real(const Trange& standard)
    {
        Trange shortcut = tr_shortcut();
        Trange real = tr_real(standard);
        if (real == shortcut || real == standard)
            chosen_tr = shortcut;
        else
            chosen_tr = real;
    }

    void ib_set()
    {
        msg->set(*chosen_var, v->code, chosen_lev, chosen_tr);
    }

    void set_gen_sensor(const Var& var, Varcode code, const Level& defaultLevel, const Trange& trange)
    {
        Level lev;

        if (!height_sensor_seen ||
                 (height_sensor != MISSING_SENSOR_H && (
                      defaultLevel == Level(103, height_sensor * 1000)
                  || (defaultLevel.ltype1 == 1 && height_sensor == 0))))
            msg->set(var, code, defaultLevel, trange);
        else if (height_sensor == MISSING_SENSOR_H)
        {
            if (opts.simplified)
                msg->set(var, code, defaultLevel, trange);
            else
                msg->set(var, code, Level(103, MISSING_INT), trange);
        }
        else if (opts.simplified)
        {
            Var var1(var);
            var1.seta(newvar(WR_VAR(0, 7, 32), height_sensor));
            msg->set(var1, code, defaultLevel, trange);
        } else
            msg->set(var, code, Level(103, height_sensor * 1000), trange);
    }

    void set_gen_sensor(const Var& var, int shortcut)
    {
        const MsgVarShortcut& v = shortcutTable[shortcut];
        set_gen_sensor(var, shortcut, Level(v.ltype1, v.l1, v.ltype2, v.l2), Trange(v.pind, v.p1, v.p2));
    }

    void set_gen_sensor(const Var& var, int shortcut, const Trange& tr_std, bool tr_careful=false)
    {
        ib_start(shortcut, var);
        ib_level_use_shorcut_and_discard_rest();
        if (!opts.simplified)
            ib_trange_use_real(tr_std);
        else if (tr_careful)
            ib_trange_use_shorcut_if_standard_else_real(tr_std);
        else
            ib_trange_use_shorcut_and_preserve_rest(tr_std);
        ib_set();
    }

    void set_gen_sensor(const Var& var, int shortcut, const Level& lev_std, const Trange& tr_std, bool lev_careful=false, bool tr_careful=false)
    {
        ib_start(shortcut, var);
        if (!opts.simplified)
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

#if 0
    void set_gen_sensor(const Var& var, int shortcut, const Trange& trange)
    {
        if (height_sensor == MISSING_SENSOR_H)
            msg->set_by_id(var, shortcut);
        else 
        {
            if (opts.simplified)
            {
                Var var1(var);
                var1.seta(newvar(WR_VAR(0, 7, 32), height_sensor));
                msg->set(var1, v.code, Level(103, height_sensor * 1000), trange);
            } else {
                const MsgVarShortcut& v = shortcutTable[shortcut];
                msg->set(var, v.code, Level(103, height_sensor * 1000), trange);
            }
        }
    }
#endif

    void set_baro_sensor(const Var& var, int shortcut)
    {
        if (height_baro == MISSING_BARO)
            msg->set_by_id(var, shortcut);
        else if (opts.simplified)
        {
            Var var1(var);
            var1.seta(newvar(WR_VAR(0, 7, 31), height_baro));
            msg->set_by_id(var1, shortcut);
        } else {
            const MsgVarShortcut& v = shortcutTable[shortcut];
            msg->set(var, v.code, Level(102, height_baro*1000), Trange(v.pind, v.p1, v.p2));
        }
    }

public:
    SynopImporter(const msg::Importer::Options& opts)
        : WMOImporter(opts), v(0), temp_var(0), chosen_var(0) {}
    virtual ~SynopImporter()
    {
        if (temp_var) delete temp_var;
    }

    virtual void init()
    {
        WMOImporter::init();
        cloudlevel = Level::cloud(MISSING_INT, MISSING_INT);
        height_baro = MISSING_BARO;
        press_std = MISSING_PRESS_STD;
        height_sensor = MISSING_SENSOR_H;
        height_sensor_seen = false;
        vs = MISSING_VSS;
        time_period = MISSING_INT;
        time_period_offset = 0;
        time_period_seen = false;
        time_sig = MISSING_TIME_SIG;
        hour = MISSING_INT;
    }

    virtual void run()
    {
        for (pos = 0; pos < subset->size(); ++pos)
        {
                const Var& var = (*subset)[pos];
                if (WR_VAR_F(var.code()) != 0) continue;
                if (WR_VAR_X(var.code()) < 10)
                        peek_var(var);
                if (var.value() == NULL)
                        import_var_undef(var);
                else
                        import_var(var);
        }
    }

    MsgType scanType(const Bulletin& bulletin) const
    {
        switch (bulletin.type)
        {
            case 0: return MSG_SYNOP;
            case 1:
                switch (bulletin.localsubtype)
                {
                    case 21: return MSG_BUOY;
                    case 9:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 19: return MSG_SHIP;
                    case 0: {
                        /* Guess looking at the variables */
			if (bulletin.subsets.empty())
				throw error_consistency("trying to import a SYNOP message with no data subset");
			const Subset& subset = bulletin.subsets[0];
                        if (subset.size() > 1 && subset[0].code() == WR_VAR(0, 1, 5))
                            return MSG_BUOY;
                        else
                            return MSG_SHIP;
		    }
                    default: return MSG_GENERIC;
                }
                break;
            default: return MSG_GENERIC; break;
        }
    }
};

void SynopImporter::peek_var(const Var& var)
{
    switch (var.code())
    {
/* Context items */
        case WR_VAR(0,  4,  4): hour = var.enq(MISSING_INT); break;
        case WR_VAR(0,  8,  2): {
            /* Vertical significance */
            if (pos == 0) throw error_consistency("B08002 found at beginning of message");
            if (pos == subset->size() - 1) throw error_consistency("B08002 found at end of message");
            Varcode prev = (*subset)[pos - 1].code();
            Varcode next = (*subset)[pos + 1].code();

            if (prev == WR_VAR(0, 20, 10))
            {
                /* Cloud Data */
                cloudlevel.ltype2 = 258;
                cloudlevel.l2 = 0;
            } else if (next == WR_VAR(0, 20, 11)) {
                if (pos >= subset->size() - 3)
                    throw error_consistency("B08002 followed by B20011 found less than 3 places before end of message");
                Varcode next2 = (*subset)[pos + 3].code();
                if (next2 == WR_VAR(0, 20, 14))
                {
                    /* Clouds with bases below */
                    if (cloudlevel.ltype2 != 263)
                    {
                        cloudlevel.ltype2 = 263;
                        cloudlevel.l2 = 1;
                    } else {
                        ++cloudlevel.l2;
                    }
                } else {
                    /* Individual cloud groups */
                    if (cloudlevel.ltype2 != 259)
                    {
                        cloudlevel.ltype2 = 259;
                        cloudlevel.l2 = 1;
                    } else {
                        ++cloudlevel.l2;
                    }
                }
            } else if (next == WR_VAR(0, 20, 54)) {
                /* Direction of cloud drift */
                if (cloudlevel.ltype2 != 260)
                {
                    cloudlevel.ltype2 = 260;
                    cloudlevel.l2 = 1;
                } else {
                    ++cloudlevel.l2;
                }
            } else if (var.value() == NULL) {
                cloudlevel.ltype2 = 0;
            } else {
                /* Unless we can detect known buggy situations, raise an error */
                if (next != WR_VAR(0, 20, 62))
                    error_consistency::throwf("Vertical significance %d found in unrecognised context", var.enqi());
            }
            break;
        }
        case WR_VAR(0,  5, 21):
            cloudlevel.ltype2 = 262;
            cloudlevel.l2 = 0;
            break;
    }
}

void SynopImporter::import_var_undef(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0,  4,  4):
            hour = MISSING_INT;
            break;
        case WR_VAR(0,  7, 32):
            /* Height to use later as level for what needs it */
            height_sensor = MISSING_SENSOR_H;
            height_sensor_seen = true;
            break;
        case WR_VAR(0,  8,  2):
            vs = MISSING_VSS;
            break;
        case WR_VAR(0,  4, 24):
            /* Time period in hours */
            time_period = MISSING_INT;
            time_period_offset = 0;
            time_period_seen = true;
            break;
        case WR_VAR(0,  4, 25):
            /* Time period in minutes */
            time_period = MISSING_INT;
            time_period_offset = 0;
            time_period_seen = true;
            break;
        case WR_VAR(0,  8, 21):
            /* Time significance */
            time_sig = MISSING_TIME_SIG;
            break;
        default:
	    //WMOImporter::import_var_undef(var);
            break;
    }
}

void SynopImporter::import_var(const Var& var)
{
    switch (var.code())
    {
/* Context items */
        case WR_VAR(0,  7, 32):
            /* Height to use later as level for what needs it */
            height_sensor = var.enqd();
            height_sensor_seen = true;
            break;
        case WR_VAR(0,  8,  2):
            /* Vertical significance */
            vs = var.enqd();
            /* Store original VS value as a measured value */
            msg->set(var, WR_VAR(0, 8, 2), cloudlevel, Trange::instant());
            break;
        case WR_VAR(0,  4, 24):
            /* Time period in hours */
            if (pos > 0 && (*subset)[pos-1].code() == WR_VAR(0, 4, 24) && var.enqi() != 0)
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
            break;
        case WR_VAR(0,  4, 25):
            /* Time period in minutes */
            time_period = var.enqd() * 60;
            time_period_seen = true;
            time_period_offset = 0;
            break;
        case WR_VAR(0,  8, 21):
            /* Time significance */
            time_sig = var.enqi();
            break;

/* Fixed surface station identification, time, horizontal and vertical
 * coordinates (complete) */
        case WR_VAR(0,  7,  1):
        case WR_VAR(0,  7, 30): msg->set_height_station_var(var); break;
        /* case WR_VAR(0,  7,  4): DBA_RUN_OR_RETURN(dba_msg_set_isobaric_surface_var(msg, var)); break; */
        case WR_VAR(0,  7, 31):
            /* Remember the height to use later as layer for pressure */
            height_baro = var.enqd();
            /* Store also in the ana level, so that if the
             * pressure later is missing we still have
             * access to the value */
            msg->set_height_baro_var(var);
            break;

/* Pressure data (complete) */
        case WR_VAR(0, 10,  4): set_baro_sensor(var, DBA_MSG_PRESS); break;
        case WR_VAR(0, 10, 51): set_baro_sensor(var, DBA_MSG_PRESS_MSL); break;
        case WR_VAR(0, 10, 61): set_baro_sensor(var, DBA_MSG_PRESS_3H); break;
        case WR_VAR(0, 10, 62): set_baro_sensor(var, DBA_MSG_PRESS_24H); break;
        case WR_VAR(0, 10, 63): set_baro_sensor(var, DBA_MSG_PRESS_TEND); break;
        case WR_VAR(0,  7,  4):
            /* Remember the standard level pressure to use later as layer for geopotential */
            press_std = var.enqd();
            /* DBA_RUN_OR_RETURN(dba_msg_set_height_baro_var(msg, var)); */
            break;
        case WR_VAR(0, 10,  3):
        case WR_VAR(0, 10,  8):
        case WR_VAR(0, 10,  9):
            if (press_std == MISSING_PRESS_STD)
                throw error_consistency("B10009 given without pressure of standard level");
            msg->set(var, WR_VAR(0, 10,  8), Level(100, press_std), Trange::instant());
            break;

        /* Legacy bits */
/* Basic synoptic "instantaneous" data */

/* Temperature and humidity data (complete) */
        case WR_VAR(0, 12,   4):
        case WR_VAR(0, 12, 101): set_gen_sensor(var, DBA_MSG_TEMP_2M); break;
        case WR_VAR(0, 12,   6):
        case WR_VAR(0, 12, 103): set_gen_sensor(var, DBA_MSG_DEWPOINT_2M); break;
        case WR_VAR(0, 13,   3): set_gen_sensor(var, DBA_MSG_HUMIDITY); break;

/* Visibility data (complete) */
        case WR_VAR(0, 20,  1): set_gen_sensor(var, DBA_MSG_VISIBILITY); break;

/* Precipitation past 24h (complete) */
        case WR_VAR(0, 13, 19): set_gen_sensor(var, DBA_MSG_TOT_PREC1); break;
        case WR_VAR(0, 13, 20): set_gen_sensor(var, DBA_MSG_TOT_PREC3); break;
        case WR_VAR(0, 13, 21): set_gen_sensor(var, DBA_MSG_TOT_PREC6); break;
        case WR_VAR(0, 13, 22): set_gen_sensor(var, DBA_MSG_TOT_PREC12); break;
        case WR_VAR(0, 13, 23): set_gen_sensor(var, DBA_MSG_TOT_PREC24); break;

/* Cloud data */
        case WR_VAR(0, 20, 10): msg->set_cloud_n_var(var); break;

/* Individual cloud layers or masses (complete) */
/* Clouds with bases below station level (complete) */
/* Direction of cloud drift (complete) */
        case WR_VAR(0, 20, 11):
        case WR_VAR(0, 20, 13):
        case WR_VAR(0, 20, 17):
        case WR_VAR(0, 20, 54): msg->set(var, var.code(), cloudlevel, Trange::instant()); break;
        case WR_VAR(0, 20, 12): { // CH CL CM
            int lt2 = cloudlevel.ltype2, l2=cloudlevel.l2;
            if (lt2 == 258)
            {
                l2 = 1;
                if (pos > 0 && (*subset)[pos - 1].code() == WR_VAR(0, 20, 12))
                {
                    ++l2;
                    if (pos > 1 && (*subset)[pos - 2].code() == WR_VAR(0, 20, 12))
                        ++l2;
                }
            }
            msg->set(var, WR_VAR(0, 20, 12), Level::cloud(lt2, l2), Trange::instant());
            break;
        }

/* Direction and elevation of cloud (complete) */
        case WR_VAR(0, 5, 21):
            cloudlevel.ltype2 = 262;
            cloudlevel.l2 = 0;
            msg->set(var, WR_VAR(0, 5, 21), cloudlevel, Trange::instant());
            break;
        case WR_VAR(0, 7, 21): msg->set(var, WR_VAR(0, 7, 21), cloudlevel, Trange::instant()); break;
        /* Cloud type is handled by the generic cloud type handler */

/* State of ground, snow depth, ground minimum temperature (complete) */
        case WR_VAR(0, 20,  62): msg->set_state_ground_var(var); break;
        case WR_VAR(0, 13,  13): msg->set_tot_snow_var(var); break;
        case WR_VAR(0, 12, 113): msg->set(var, WR_VAR(0, 12, 121), Level(1), Trange(3, 0, 43200)); break;

/* Basic synoptic "period" data */

/* Present and past weather (complete) */
        case WR_VAR(0, 20,  3): msg->set_pres_wtr_var(var); break;
        case WR_VAR(0, 20,  4):
            ib_start(DBA_MSG_PAST_WTR1_6H, var);
            ib_level_use_shorcut_and_discard_rest();
            if (opts.simplified)
                ib_trange_use_standard_and_preserve_rest((hour % 6 == 0) ? tr_std_past_wtr6 : tr_std_past_wtr3);
            else
                ib_trange_use_real((hour % 6 == 0) ? tr_std_past_wtr6 : tr_std_past_wtr3);
            ib_set();
            break;
        case WR_VAR(0, 20,  5):
            ib_start(DBA_MSG_PAST_WTR2_6H, var);
            ib_level_use_shorcut_and_discard_rest();
            if (opts.simplified)
                ib_trange_use_standard_and_preserve_rest((hour % 6 == 0) ? tr_std_past_wtr6 : tr_std_past_wtr3);
            else
                ib_trange_use_real((hour % 6 == 0) ? tr_std_past_wtr6 : tr_std_past_wtr3);
            ib_set();
            break;

/* Sunshine data (complete) */
        case WR_VAR(0, 14, 31): msg->set(var, WR_VAR(0, 14, 31), Level(1), Trange(1, 0, abs(time_period))); break;

/* Precipitation measurement (complete) */
        case WR_VAR(0, 13, 11): set_gen_sensor(var, WR_VAR(0, 13, 11), Level(1), Trange(1, 0, abs(time_period))); break;

/* Extreme temperature data */
        case WR_VAR(0, 12, 111):
            set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1), Trange(2, -abs(time_period_offset), abs(time_period)));
            break;
        case WR_VAR(0, 12, 112):
            set_gen_sensor(var, WR_VAR(0, 12, 101), Level(1), Trange(3, -abs(time_period_offset), abs(time_period)));
            break;

/* Wind data (complete) */
        case WR_VAR(0, 2, 2): msg->set_wind_inst_var(var); break;

        /* Note B/C 1.10.5.3.2 Calm shall be reported by
         * setting wind direction to 0 and wind speed to 0.
         * Variable shall be reported by setting wind direction
         * to 0 and wind speed to a positive value, not a
         * missing value indicator.
         */
        case WR_VAR(0, 11,  1):
        case WR_VAR(0, 11, 11):
            if (time_sig != MISSING_TIME_SIG && time_sig != 2)
                    error_consistency::throwf("Found unsupported time significance %d for wind direction", time_sig);
            set_gen_sensor(var, DBA_MSG_WIND_DIR, lev_std_wind, tr_std_wind);
            break;
        case WR_VAR(0, 11,  2):
        case WR_VAR(0, 11, 12):
            if (time_sig != MISSING_TIME_SIG && time_sig != 2)
                error_consistency::throwf("Found unsupported time significance %d for wind speed", time_sig);
            set_gen_sensor(var, DBA_MSG_WIND_SPEED, lev_std_wind, tr_std_wind);
            break;
        case WR_VAR(0, 11, 43): set_gen_sensor(var, DBA_MSG_WIND_GUST_MAX_DIR, lev_std_wind, tr_std_wind_max10m, false, true); break;
        case WR_VAR(0, 11, 41): set_gen_sensor(var, DBA_MSG_WIND_GUST_MAX_SPEED, lev_std_wind, tr_std_wind_max10m, false, true); break;

/* Evaporation data */
        case WR_VAR(0, 2, 4): msg->set(var, WR_VAR(0, 2, 4), Level(1), Trange::instant()); break;
        case WR_VAR(0, 13, 33):
            if (time_period == MISSING_INT)
                msg->set(var, WR_VAR(0, 13, 33), Level(1), Trange(1));
            else
                msg->set(var, WR_VAR(0, 13, 33), Level(1), Trange(1, 0, abs(time_period)));
            break;

/* Radiation data */
        case WR_VAR(0, 14, 2):
            throw error_unimplemented("wow, a synop with radiation info, please give it to Enrico");

/* Temperature change */
        case WR_VAR(0, 12, 49):
            throw error_unimplemented("wow, a synop with temperature change info, please give it to Enrico");

        case WR_VAR(0, 22, 42): msg->set_water_temp_var(var); break;
        case WR_VAR(0, 12,  5): msg->set_wet_temp_2m_var(var); break;
        case WR_VAR(0, 10,197): msg->set_height_anem_var(var); break;

        default:
                WMOImporter::import_var(var);
                break;
    }
}

} // anonynmous namespace

std::auto_ptr<Importer> Importer::createSynop(const msg::Importer::Options& opts)
{
    return auto_ptr<Importer>(new SynopImporter(opts));
}


} // namespace wbimporter
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
