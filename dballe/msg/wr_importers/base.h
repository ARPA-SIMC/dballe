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

    void set(const wreport::Var& var, int shortcut);
    void set(const wreport::Var& var, wreport::Varcode code, const Level& level, const Trange& trange);

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

    void init() override
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

    Level get_real_baro() const;
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
 * Check if the current context state of BUFR information is something that we
 * currently cannot handle.
 *
 * For example, a BUFR can provide a B12101 as a measured temperature, or a
 * B12101 as a standard deviation of temperature in the last 10 minutes. The
 * former we can handle, the latter we cannot.
 *
 * This class keeps track of when we are in such unusual states.
 *
 * See https://github.com/ARPA-SIMC/dballe/issues/47
 */
struct UnsupportedContext
{
    const wreport::Var* B08023 = nullptr; // First order statistics (code table)

    bool is_unsupported() const;

    void init();
    void peek_var(const wreport::Var& var, unsigned pos);
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
    Interpreted(int shortcut, const wreport::Var& var, const Level& level, const Trange& trange);
    Interpreted(wreport::Varcode code, const wreport::Var& var, const Level& level, const Trange& trange);
    ~Interpreted();

    void set_sensor_height(const LevelContext& ctx, bool simplified=false);
    void set_barometer_height(const LevelContext& ctx, bool simplified=false);
    void set_duration(const TimerangeContext& ctx, bool simplified=false);

    void annotate_if_needed(const Trange& real);

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
    UnsupportedContext unsupported;

    virtual void peek_var(const wreport::Var& var);
    virtual void import_var(const wreport::Var& var);

    void set_gen_sensor(const wreport::Var& var, wreport::Varcode code, const Level& defaultLevel, const Trange& trange);
    void set_gen_sensor(const wreport::Var& var, int shortcut);
    void set_baro_sensor(const wreport::Var& var, int shortcut);
    void set_past_weather(const wreport::Var& var, int shortcut);
    void set_wind(const wreport::Var& var, int shortcut);
    void set_wind_max(const wreport::Var& var, int shortcut);
    void set_pressure(const wreport::Var& var);
    void set_water_temperature(const wreport::Var& var);
    void set_swell_waves(const wreport::Var& var);
    void set(const wreport::Var& var, int shortcut);
    void set(const wreport::Var& var, wreport::Varcode code, const Level& level, const Trange& trange);

public:
    SynopBaseImporter(const msg::Importer::Options& opts);

    void init() override;
    void run() override;
};

}
}
}
#endif
