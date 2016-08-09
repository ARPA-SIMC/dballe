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

    /**
     * Compute the most precise current level information possible, taking
     * defaults from the given standard level.
     */
    Level get_real(const Level& standard) const;
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

    /**
     * Compute the most precise current time range information possible, taking
     * defaults from the given standard time range.
     */
    Trange get_real(const Trange& standard) const;
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

/**
 * Struct used to build an interpreted value
 */
struct Interpreted
{
    /// Interpreted value being built
    std::unique_ptr<wreport::Var> var;
    /// Interpreted level
    Level level;
    /// Interpreted time range
    Trange trange;

    /**
     * Beging building using a copy of var, and level and timerange from \a
     * shortcut
     */
    Interpreted(int shortcut, const wreport::Var& var);
    ~Interpreted();

    void annotate_level(const LevelContext& level_context);
    void annotate_trange(const TimerangeContext& trange_context);

    /// Move the resulting value to msg
    void to_msg(Msg& msg);
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

    virtual void peek_var(const wreport::Var& var);
    virtual void import_var(const wreport::Var& var);

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

public:
    SynopBaseImporter(const msg::Importer::Options& opts);

    virtual void init();
    virtual void run();
};

}
}
}
#endif
