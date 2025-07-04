#ifndef DBALLE_MSG_WRIMPORTER_BASE_H
#define DBALLE_MSG_WRIMPORTER_BASE_H

#include <cstdint>
#include <dballe/msg/fwd.h>
#include <dballe/msg/wr_codec.h>
#include <limits>

namespace wreport {
struct Subset;
struct Bulletin;
struct Var;
} // namespace wreport

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

class Importer
{
protected:
    const dballe::ImporterOptions& opts;
    const wreport::Subset* subset;
    impl::Message* msg;

    virtual void init();
    virtual void run() = 0;

    void set(const wreport::Var& var, const Shortcut& shortcut);
    void set(const wreport::Var& var, wreport::Varcode code, const Level& level,
             const Trange& trange);

public:
    Importer(const dballe::ImporterOptions& opts) : opts(opts) {}
    virtual ~Importer() {}

    virtual MessageType scanType(const wreport::Bulletin& bulletin) const = 0;

    void import(const wreport::Subset& subset, impl::Message& msg);

    static std::unique_ptr<Importer>
    createSynop(const dballe::ImporterOptions&);
    static std::unique_ptr<Importer> createShip(const dballe::ImporterOptions&);
    static std::unique_ptr<Importer>
    createMetar(const dballe::ImporterOptions&);
    static std::unique_ptr<Importer> createTemp(const dballe::ImporterOptions&);
    static std::unique_ptr<Importer>
    createPilot(const dballe::ImporterOptions&);
    static std::unique_ptr<Importer>
    createFlight(const dballe::ImporterOptions&);
    static std::unique_ptr<Importer> createSat(const dballe::ImporterOptions&);
    static std::unique_ptr<Importer>
    createPollution(const dballe::ImporterOptions&);
    static std::unique_ptr<Importer>
    createGeneric(const dballe::ImporterOptions&);
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
    WMOImporter(const dballe::ImporterOptions& opts) : Importer(opts) {}
    virtual ~WMOImporter() {}
};

/// Keep track of level context changes
struct LevelContext
{
    static constexpr double missing = std::numeric_limits<double>::max();
    double height_baro;
    double press_std;
    double height_sensor;
    double sea_depth;
    double ground_depth;
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
     * Distance from the standard level to the real one.
     *
     * This is used, in case multiple values get simplified to the same level,
     * to select the one closer to the standard level.
     */
    unsigned level_deviation = 0;

    /**
     * Beging building using a copy of var, and level and timerange from \a
     * shortcut
     */
    Interpreted(const Shortcut& shortcut, const wreport::Var& var);
    Interpreted(const Shortcut& shortcut, const wreport::Var& var,
                const Level& level, const Trange& trange);
    Interpreted(wreport::Varcode code, const wreport::Var& var,
                const Level& level, const Trange& trange);
    virtual ~Interpreted();

    virtual void set_sensor_height(const LevelContext& ctx)    = 0;
    virtual void set_barometer_height(const LevelContext& ctx) = 0;
    virtual void set_duration(const TimerangeContext& ctx)     = 0;
    virtual void set_wind_mean(const TimerangeContext& ctx)    = 0;
};

struct InterpretedPrecise : public Interpreted
{
    using Interpreted::Interpreted;
    void set_sensor_height(const LevelContext& ctx) override;
    void set_barometer_height(const LevelContext& ctx) override;
    void set_duration(const TimerangeContext& ctx) override;
    void set_wind_mean(const TimerangeContext& ctx) override;
};

struct InterpretedSimplified : public Interpreted
{
    using Interpreted::Interpreted;
    void set_sensor_height(const LevelContext& ctx) override;
    void set_barometer_height(const LevelContext& ctx) override;
    void set_duration(const TimerangeContext& ctx) override;
    void set_wind_mean(const TimerangeContext& ctx) override;
};

template <typename... Args>
std::unique_ptr<Interpreted> create_interpreted(bool simplified, Args&&... args)
{
    if (simplified)
        return std::unique_ptr<Interpreted>(
            new InterpretedSimplified(std::forward<Args>(args)...));
    else
        return std::unique_ptr<Interpreted>(
            new InterpretedPrecise(std::forward<Args>(args)...));
}

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
    std::vector<Interpreted*> queued;

    virtual void peek_var(const wreport::Var& var);
    virtual void import_var(const wreport::Var& var);

    void set_gen_sensor(const wreport::Var& var, wreport::Varcode code,
                        const Level& defaultLevel, const Trange& trange);
    void set_gen_sensor(const wreport::Var& var, const Shortcut& shortcut);
    void set_baro_sensor(const wreport::Var& var, const Shortcut& shortcut);
    void set_past_weather(const wreport::Var& var, const Shortcut& shortcut);
    void set_wind(const wreport::Var& var, const Shortcut& shortcut);
    void set_wind_max(const wreport::Var& var, const Shortcut& shortcut);
    void set_pressure(const wreport::Var& var);
    void set(const wreport::Var& var, const Shortcut& shortcut);
    void set(const wreport::Var& var, wreport::Varcode code, const Level& level,
             const Trange& trange);
    void set(std::unique_ptr<Interpreted> val);

public:
    SynopBaseImporter(const dballe::ImporterOptions& opts);
    ~SynopBaseImporter();

    void init() override;
    void run() override;
};

} // namespace wr
} // namespace msg
} // namespace impl
} // namespace dballe
#endif
