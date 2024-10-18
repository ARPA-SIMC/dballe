#include "dballe/msg/wr_codec.h"
#include "dballe/core/shortcuts.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <wreport/bulletin.h>

using namespace wreport;
using namespace std;

#define AIREP_NAME "airep"
#define AIREP_DESC "AIREP (autodetect)"

#define AIREP_ECMWF_NAME "airep-ecmwf"
#define AIREP_ECMWF_DESC "AIREP ECMWF (4.142)"

#define AMDAR_NAME "amdar"
#define AMDAR_DESC "AMDAR (autodetect)"

#define AMDAR_ECMWF_NAME "amdar-ecmwf"
#define AMDAR_ECMWF_DESC "AMDAR ECMWF (4.144)"

#define AMDAR_WMO_NAME "amdar-wmo"
#define AMDAR_WMO_DESC "AMDAR WMO"

#define ACARS_NAME "acars"
#define ACARS_DESC "ACARS (autodetect)"

#define ACARS_ECMWF_NAME "acars-ecmwf"
#define ACARS_ECMWF_DESC "ACARS ECMWF (4.145)"

#define ACARS_WMO_NAME "acars-wmo"
#define ACARS_WMO_DESC "ACARS WMO"

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

namespace {

// Base template for flights
struct FlightBase : public Template
{
    bool is_crex;
    const msg::Context* flight_ctx;

    FlightBase(const dballe::ExporterOptions& opts, const Messages& msgs)
        : Template(opts, msgs), flight_ctx(0) {}

    void add(wreport::Varcode code, const wreport::Var* var) const
    {
        Template::add(code, var);
    }

    void add(Varcode code, const Shortcut& shortcut) const
    {
        const Var* var = msg->get(shortcut);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    void add(Varcode code) const
    {
        add(code, code);
    }

    void add(Varcode code, Varcode srccode) const
    {
        const Var* var = flight_ctx->values.maybe_var(srccode);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    void setupBulletin(wreport::Bulletin& bulletin) override
    {
        Template::setupBulletin(bulletin);

        // Use old table for old templates
        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table_version_number = 13;
        }

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        bulletin.data_category = 4;
        bulletin.data_subcategory = 255;
    }
    void to_subset(const Message& msg, wreport::Subset& subset) override
    {
        Template::to_subset(msg, subset);

        // Find what is the level where the airplane is in
        flight_ctx = 0;

        for (const auto& ctx : msg.data)
        {
            if (ctx.trange != Trange::instant()) continue;

            bool use = false;
            switch (ctx.level.ltype1)
            {
                case 100:
                     use = ctx.values.maybe_var(sc::press.code) != nullptr
                        || ctx.values.maybe_var(sc::height_station.code) != nullptr;
                     break;
                case 102:
                     use = ctx.values.maybe_var(sc::height_station.code) != nullptr;
                     break;
            }
            if (use)
            {
                if (flight_ctx != 0)
                    error_consistency::throwf("contradicting height indication found (both %d and %d)",
                            flight_ctx->level.ltype1, ctx.level.ltype1);
                flight_ctx = &ctx;
            }
        }

        if (flight_ctx == 0)
            throw error_notfound("no airplane pressure or height found in flight message");
    }
};

struct Airep : public FlightBase
{
    Airep(const dballe::ExporterOptions& opts, const Messages& msgs)
        : FlightBase(opts, msgs) {}

    const char* name() const override { return AIREP_NAME; }
    const char* description() const override { return AIREP_DESC; }

    void setupBulletin(wreport::Bulletin& bulletin) override
    {
        FlightBase::setupBulletin(bulletin);
        bulletin.data_subcategory_local = 142;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3, 11,   1));
        if (!is_crex)
        {
            bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
            bulletin.datadesc.push_back(WR_VAR(1,  1,  18));
            bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
            bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
            bulletin.datadesc.push_back(WR_VAR(0,  1,  32));
            bulletin.datadesc.push_back(WR_VAR(1,  1,  18));
            bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }

        bulletin.load_tables();
    }

    void to_subset(const Message& msg, wreport::Subset& subset) override
    {
        FlightBase::to_subset(msg, subset);

        /*  0 */ add(WR_VAR(0,  1,  6), sc::ident);
        /*  1 */ add(WR_VAR(0,  2, 61));
        do_D01011();
        do_D01012();
        /*  7 */ add(WR_VAR(0,  5,  1), sc::latitude);
        /*  8 */ add(WR_VAR(0,  6,  1), sc::longitude);
        /*  9 */ add(WR_VAR(0,  8,  4));
        /* 10 */ add(WR_VAR(0,  7,  2), WR_VAR(0,  7,  30)); /* HEIGHT OF STATION -> HEIGHT OR ALTITUDE */
        /* 11 */ add(WR_VAR(0, 12,  1), WR_VAR(0, 12, 101)); /* TEMPERATURE/DRY-BULB TEMPERATURE */
        /* 12 */ add(WR_VAR(0, 11,  1));                     /* WIND DIRECTION */
        /* 13 */ add(WR_VAR(0, 11,  2));                     /* WIND SPEED */
        /* 14 */ add(WR_VAR(0, 11, 31));                     /* DEGREE OF TURBULENCE */
        /* 15 */ add(WR_VAR(0, 11, 32));                     /* HEIGHT OF BASE OF TURBULENCE */
        /* 16 */ add(WR_VAR(0, 11, 33));                     /* HEIGHT OF TOP OF TURBULENCE */
        /* 17 */ add(WR_VAR(0, 20, 41));                     /* AIRFRAME ICING */

        if (!is_crex)
        {
            subset.append_fixed_dpb(WR_VAR(2, 22, 0), 18);
            if (opts.centre != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 31), opts.centre);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 31));
            if (opts.application != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 32), opts.application);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 32));
        }
    }
};

struct Amdar : public Airep
{
    Amdar(const dballe::ExporterOptions& opts, const Messages& msgs)
        : Airep(opts, msgs) {}

    const char* name() const override { return AMDAR_NAME; }
    const char* description() const override { return AMDAR_DESC; }

    void setupBulletin(wreport::Bulletin& bulletin) override
    {
        Airep::setupBulletin(bulletin);
        bulletin.data_subcategory_local = 144;
    }
};

struct AmdarWMO : public FlightBase
{
    AmdarWMO(const dballe::ExporterOptions& opts, const Messages& msgs)
        : FlightBase(opts, msgs) {}

    const char* name() const override { return AMDAR_WMO_NAME; }
    const char* description() const override { return AMDAR_WMO_DESC; }

    void setupBulletin(wreport::Bulletin& bulletin) override
    {
        FlightBase::setupBulletin(bulletin);
        bulletin.data_subcategory = 255;
        bulletin.data_subcategory_local = 144;

        // Data descriptor section
        bulletin.datadesc.clear();
        //bulletin.datadesc.push_back(WR_VAR(0,  1,  33)); //  0
        //bulletin.datadesc.push_back(WR_VAR(0,  1,  34)); //  1
        bulletin.datadesc.push_back(WR_VAR(3, 11,   5)); //  2
        bulletin.datadesc.push_back(WR_VAR(0,  8,   4)); // 20
        bulletin.datadesc.push_back(WR_VAR(0,  2,  64)); // 21
        bulletin.datadesc.push_back(WR_VAR(0, 13,   3)); // 22
        bulletin.datadesc.push_back(WR_VAR(0, 12, 103)); // 23
        bulletin.datadesc.push_back(WR_VAR(0, 13,   2)); // 24
        bulletin.datadesc.push_back(WR_VAR(1,  2,   0));
        bulletin.datadesc.push_back(WR_VAR(0, 31,   1)); // 25
        bulletin.datadesc.push_back(WR_VAR(0, 11,  75));
        bulletin.datadesc.push_back(WR_VAR(0, 11,  76));
        bulletin.datadesc.push_back(WR_VAR(0, 11,  37));
        bulletin.datadesc.push_back(WR_VAR(0, 11,  39));
        bulletin.datadesc.push_back(WR_VAR(0, 11,  77));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  42));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  43));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  44));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  45));
        bulletin.datadesc.push_back(WR_VAR(0, 20,  41));
        bulletin.datadesc.push_back(WR_VAR(0,  2,   5));
        bulletin.datadesc.push_back(WR_VAR(0,  2,  62));
        bulletin.datadesc.push_back(WR_VAR(0,  2,  70));
        bulletin.datadesc.push_back(WR_VAR(0,  2,  65));
        bulletin.datadesc.push_back(WR_VAR(0,  7,   4));
        bulletin.datadesc.push_back(WR_VAR(0, 33,  26));
        bulletin.load_tables();
    }

    void to_subset(const Message& msg, wreport::Subset& subset) override
    {
        FlightBase::to_subset(msg, subset);

        Level lev;

        ///*  0 */ add(WR_VAR(0,  1, 33));
        ///*  1 */ add(WR_VAR(0,  1, 34));
        /*  2 */ add(WR_VAR(0,  1,  8), sc::ident);
        /*  3 */ add(WR_VAR(0,  1, 23));
        /*  4 */ add(WR_VAR(0,  5,  1), sc::latitude);
        /*  5 */ add(WR_VAR(0,  6,  1), sc::longitude);
        do_D01011();
        do_D01013();
        /* 12 */
        if (const wreport::Var* v = flight_ctx->values.maybe_var(WR_VAR(0,  7, 30)))
            add(WR_VAR(0,  7, 10), v);
        else if (flight_ctx->level.ltype1 == 102)
            subset.store_variable_d(WR_VAR(0,  7, 10), (double)flight_ctx->level.l1 / 1000.0);
        else
            subset.store_variable_undef(WR_VAR(0,  7, 10));
        /* 13 */ add(WR_VAR(0,  8,  9));
        /* 14 */ add(WR_VAR(0, 11,  1));
        /* 15 */ add(WR_VAR(0, 11,  2));
        /* 16 */ add(WR_VAR(0, 11, 31));
        /* 17 */ add(WR_VAR(0, 11, 36));
        /* 18 */ add(WR_VAR(0, 12,101));
        /* 19 */ add(WR_VAR(0, 33, 25));
        /* 20 */ add(WR_VAR(0,  8,  4));
        /* 21 */ add(WR_VAR(0,  2, 64));
        /* 22 */ add(WR_VAR(0, 13,  3));
        /* 23 */ add(WR_VAR(0, 12,103));
        /* 24 */ add(WR_VAR(0, 13,  2));
        /* 25 */ subset.store_variable_i(WR_VAR(0, 31, 1), 0); // FIXME: no replicated section so far
        //102000 replicate 2 descriptors (delayed 031001) times
        //  011075 MEAN TURBULENCE INTENSITY (EDDY DISSIPATION RATE)[M**(2/3)/S]
        //  011076 PEAK TURBULENCE INTENSITY (EDDY DISSIPATION RATE)[M**(2/3)/S]
                 add(WR_VAR(0, 11, 37));
                 add(WR_VAR(0, 11, 39));
                 add(WR_VAR(0, 11, 77));
                 add(WR_VAR(0, 20, 42));
                 add(WR_VAR(0, 20, 43));
                 add(WR_VAR(0, 20, 44));
                 add(WR_VAR(0, 20, 45));
                 add(WR_VAR(0, 20, 41));
                 add(WR_VAR(0,  2,  5));
                 add(WR_VAR(0,  2, 62));
                 add(WR_VAR(0,  2, 70));
                 add(WR_VAR(0,  2, 65));
                 add(WR_VAR(0,  7,  4));
                 add(WR_VAR(0, 33, 26));
    }
};

struct Acars : public FlightBase
{
    Acars(const dballe::ExporterOptions& opts, const Messages& msgs)
        : FlightBase(opts, msgs) {}

    const char* name() const override { return ACARS_NAME; }
    const char* description() const override { return ACARS_DESC; }

    void setupBulletin(wreport::Bulletin& bulletin) override
    {
        FlightBase::setupBulletin(bulletin);

        bulletin.data_subcategory_local = 145;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(0,  1,  6));
        bulletin.datadesc.push_back(WR_VAR(0,  1,  8));
        bulletin.datadesc.push_back(WR_VAR(0,  2, 61));
        bulletin.datadesc.push_back(WR_VAR(0,  2, 62));
        bulletin.datadesc.push_back(WR_VAR(0,  2,  2));
        bulletin.datadesc.push_back(WR_VAR(0,  2,  5));
        bulletin.datadesc.push_back(WR_VAR(0,  2, 70));
        bulletin.datadesc.push_back(WR_VAR(0,  2, 63));
        bulletin.datadesc.push_back(WR_VAR(0,  2,  1));
        bulletin.datadesc.push_back(WR_VAR(3,  1, 11));
        bulletin.datadesc.push_back(WR_VAR(3,  1, 12));
        bulletin.datadesc.push_back(WR_VAR(3,  1, 23));
        bulletin.datadesc.push_back(WR_VAR(0,  8,  4));
        bulletin.datadesc.push_back(WR_VAR(0,  7,  4));
        bulletin.datadesc.push_back(WR_VAR(0,  8, 21));
        bulletin.datadesc.push_back(WR_VAR(0, 11,  1));
        bulletin.datadesc.push_back(WR_VAR(0, 11,  2));
        bulletin.datadesc.push_back(WR_VAR(0, 11, 31));
        bulletin.datadesc.push_back(WR_VAR(0, 11, 34));
        bulletin.datadesc.push_back(WR_VAR(0, 11, 35));
        bulletin.datadesc.push_back(WR_VAR(0, 12,  1));
        bulletin.datadesc.push_back(WR_VAR(0, 12,  3));
        bulletin.datadesc.push_back(WR_VAR(0, 13,  3));
        bulletin.datadesc.push_back(WR_VAR(0, 20, 41));
        if (!is_crex)
        {
            bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
            bulletin.datadesc.push_back(WR_VAR(1,  1,  28));
            bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
            bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
            bulletin.datadesc.push_back(WR_VAR(0,  1, 201));
            bulletin.datadesc.push_back(WR_VAR(1,  1,  28));
            bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }

        bulletin.load_tables();
    }

    void to_subset(const Message& msg, wreport::Subset& subset) override
    {
        FlightBase::to_subset(msg, subset);
        /*  0 */ add(WR_VAR(0,  1,  6));
        /*  1 */ add(WR_VAR(0,  1,  8), sc::ident);
        /*  2 */ add(WR_VAR(0,  2, 61));
        /*  3 */ add(WR_VAR(0,  2, 62));
        /*  4 */ add(WR_VAR(0,  2,  2));
        /*  5 */ add(WR_VAR(0,  2,  5));
        /*  6 */ add(WR_VAR(0,  2, 70));
        /*  7 */ add(WR_VAR(0,  2, 63));
        /*  8 */ add(WR_VAR(0,  2,  1));
        do_D01011();
        do_D01012();
        /* 14 */ add(WR_VAR(0,  5,  2), sc::latitude);
        /* 15 */ add(WR_VAR(0,  6,  2), sc::longitude);
        /* 16 */ add(WR_VAR(0,  8,  4));
        /* 17 */ add(WR_VAR(0,  7,  4), WR_VAR(0, 10,   4));
        /* 18 */ add(WR_VAR(0,  8, 21));
        /* 19 */ add(WR_VAR(0, 11,  1));                     /* WIND DIRECTION */
        /* 20 */ add(WR_VAR(0, 11,  2));                     /* WIND SPEED */
        /* 21 */ add(WR_VAR(0, 11, 31));                     /* DEGREE OF TURBULENCE */
        /* 22 */ add(WR_VAR(0, 11, 34));                     /* VERTICAL GUST VELOCITY */
        /* 23 */ add(WR_VAR(0, 11, 35));                     /* VERTICAL GUST ACCELERATION */
        /* 24 */ add(WR_VAR(0, 12,  1), WR_VAR(0, 12, 101)); /* TEMPERATURE/DRY-BULB TEMPERATURE */
        /* 25 */ add(WR_VAR(0, 12,  3), WR_VAR(0, 12, 103)); /* DEW-POINT TEMPERATURE */
        /* 26 */ add(WR_VAR(0, 13,  3));                     /* RELATIVE HUMIDITY */
        /* 27 */ add(WR_VAR(0, 20, 41));                     /* AIRFRAME ICING */

        if (!is_crex)
        {
            subset.append_fixed_dpb(WR_VAR(2, 22, 0), 28);
            if (opts.centre != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 31), opts.centre);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 31));
            if (opts.application != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 201), opts.application);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 201));
        }
    }
};

struct AcarsWMO : public AmdarWMO
{
    AcarsWMO(const dballe::ExporterOptions& opts, const Messages& msgs)
        : AmdarWMO(opts, msgs) {}

    const char* name() const override { return ACARS_WMO_NAME; }
    const char* description() const override { return ACARS_WMO_DESC; }

    void setupBulletin(wreport::Bulletin& bulletin) override
    {
        AmdarWMO::setupBulletin(bulletin);

        bulletin.data_subcategory = 255;
        bulletin.data_subcategory_local = 145;
    }
};


} // anonymous namespace

void register_flight(TemplateRegistry& r);

void register_flight(TemplateRegistry& r)
{
    r.register_factory(4, AIREP_NAME, AIREP_DESC,
            [](const dballe::ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new Airep(opts, msgs));
            });
    r.register_factory(4, AIREP_ECMWF_NAME, AIREP_ECMWF_DESC,
            [](const dballe::ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new Airep(opts, msgs));
            });
    r.register_factory(4, AMDAR_NAME, AMDAR_DESC,
            [](const dballe::ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new Amdar(opts, msgs));
            });
    r.register_factory(4, AMDAR_ECMWF_NAME, AMDAR_ECMWF_DESC,
            [](const dballe::ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new Amdar(opts, msgs));
            });
    r.register_factory(4, AMDAR_WMO_NAME, AMDAR_WMO_DESC,
            [](const dballe::ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new AmdarWMO(opts, msgs));
            });
    r.register_factory(4, ACARS_NAME, ACARS_DESC,
            [](const dballe::ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new Acars(opts, msgs));
            });
    r.register_factory(4, ACARS_ECMWF_NAME, ACARS_ECMWF_DESC,
            [](const dballe::ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new Acars(opts, msgs));
            });
    r.register_factory(4, ACARS_WMO_NAME, ACARS_WMO_DESC,
            [](const dballe::ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new AcarsWMO(opts, msgs));
            });
}

}
}
}
}
