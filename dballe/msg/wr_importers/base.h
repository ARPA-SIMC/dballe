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

#ifndef DBALLE_MSG_WRIMPORTER_BASE_H
#define DBALLE_MSG_WRIMPORTER_BASE_H

#include <dballe/msg/wr_codec.h>
#include <dballe/msg/msg.h>
#include <stdint.h>

namespace wreport {
struct Subset;
struct Bulletin;
struct Var;
}

namespace dballe {
namespace msg {
namespace wr {

class Importer
{
protected:
    const msg::Importer::Options& opts;
    const wreport::Subset* subset;
    Msg* msg;
    int ye, mo, da, ho, mi, se;

    virtual void init();
    virtual void run() = 0;

public:
    Importer(const msg::Importer::Options& opts) : opts(opts) {}
    virtual ~Importer() {}

    virtual MsgType scanType(const wreport::Bulletin& bulletin) const = 0;

    void import(const wreport::Subset& subset, Msg& msg);

    static std::unique_ptr<Importer> createSynop(const msg::Importer::Options&);
    static std::unique_ptr<Importer> createShip(const msg::Importer::Options&);
    static std::unique_ptr<Importer> createMetar(const msg::Importer::Options&);
    static std::unique_ptr<Importer> createTemp(const msg::Importer::Options&);
    static std::unique_ptr<Importer> createPilot(const msg::Importer::Options&);
    static std::unique_ptr<Importer> createFlight(const msg::Importer::Options&);
    static std::unique_ptr<Importer> createSat(const msg::Importer::Options&);
    static std::unique_ptr<Importer> createPollution(const msg::Importer::Options&);
    static std::unique_ptr<Importer> createGeneric(const msg::Importer::Options&);
};

class WMOImporter : public Importer
{
protected:
    unsigned pos;

    void import_var(const wreport::Var& var);

    virtual void init()
    {
	    pos = 0;
	    Importer::init();
    }

public:
    WMOImporter(const msg::Importer::Options& opts) : Importer(opts) {}
    virtual ~WMOImporter() {}
};

/// Keep track of level context changes
struct LevelContext
{
    double height_baro;
    double press_std;
    double height_sensor;
    double depth;
    bool height_sensor_seen;
    bool swell_wave_group;

    void init();
    void peek_var(const wreport::Var& var);
};

/// Keep track of time range context changes
struct TimerangeContext
{
    int time_period;
    int time_period_offset;
    bool time_period_seen;
    int time_sig;
    int hour;
    int last_B04024_pos;

    void init();
    void peek_var(const wreport::Var& var, unsigned pos);
};

/**
 * Keep track of the current cloud metadata
 */
struct CloudContext
{
    Level level;

    const Level& clcmch();

    void init();
    void on_vss(const wreport::Subset& subset, unsigned pos);
};

struct ContextChooser
{
    const LevelContext& level;
    const TimerangeContext& trange;

    // Configuration
    bool simplified;

    // Import builder parts
    const MsgVarShortcut* v;
    wreport::Var* var;
    Level chosen_lev;
    Trange chosen_tr;

    // Output message
    Msg* msg;

    ContextChooser(const LevelContext& level, const TimerangeContext& trange);
    ~ContextChooser();

    void init(Msg& msg, bool simplified);

    void set_gen_sensor(const wreport::Var& var, wreport::Varcode code, const Level& defaultLevel, const Trange& trange);
    void set_gen_sensor(const wreport::Var& var, int shortcut);
    void set_gen_sensor(const wreport::Var& var, int shortcut, const Trange& tr_std, bool tr_careful=false);
    void set_gen_sensor(const wreport::Var& var, int shortcut, const Level& lev_std, const Trange& tr_std, bool lev_careful=false, bool tr_careful=false);
    void set_baro_sensor(const wreport::Var& var, int shortcut);
    void set_past_weather(const wreport::Var& var, int shortcut);
    void set_wind(const wreport::Var& var, int shortcut);
    void set_wind_max(const wreport::Var& var, int shortcut);
    void set_pressure(const wreport::Var& var);
    void set_water_temperature(const wreport::Var& var);
    void set_swell_waves(const wreport::Var& var);

protected:
    void ib_start(int shortcut, const wreport::Var& var);
    Level lev_real(const Level& standard) const;
    Trange tr_real(const Trange& standard) const;
    Level lev_shortcut() const { return Level(v->ltype1, v->l1, v->ltype2, v->l2); }
    Trange tr_shortcut() const { return Trange(v->pind, v->p1, v->p2); }
    void ib_annotate_level();
    void ib_annotate_trange();
    void ib_level_use_real(const Level& standard) { chosen_lev = lev_real(standard); }
    void ib_trange_use_real(const Trange& standard) { chosen_tr = tr_real(standard); }
    void ib_level_use_shorcut_and_discard_rest() { chosen_lev = lev_shortcut(); }
    void ib_trange_use_shortcut_and_discard_rest() { chosen_tr = tr_shortcut(); }
    void ib_level_use_shorcut_and_preserve_rest(const Level& standard);
    void ib_trange_use_shorcut_and_preserve_rest(const Trange& standard);
    void ib_level_use_standard_and_preserve_rest(const Level& standard);
    void ib_trange_use_standard_and_preserve_rest(const Trange& standard);
    void ib_level_use_shorcut_if_standard_else_real(const Level& standard);
    void ib_trange_use_shorcut_if_standard_else_real(const Trange& standard);
    void ib_set();
};

/**
 * Base class for synop, ship and other importer with synop-like data
 */
class SynopBaseImporter : public WMOImporter
{
protected:
    CloudContext clouds;
    LevelContext level;
    TimerangeContext trange;
    ContextChooser ctx;

    virtual void peek_var(const wreport::Var& var);
    virtual void import_var(const wreport::Var& var);

public:
    SynopBaseImporter(const msg::Importer::Options& opts);

    virtual void init();
    virtual void run();
};

} // namespace wr
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
