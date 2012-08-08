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

#include "msg/wr_codec.h"
#include <wreport/bulletin.h>
#include "msg/msgs.h"
#include "msg/context.h"

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
namespace msg {
namespace wr {

namespace {

// Base template for flights
struct FlightBase : public Template
{
    bool is_crex;
    const msg::Context* flight_ctx;

    FlightBase(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs), flight_ctx(0) {}

    void add(wreport::Varcode code, const wreport::Var* var) const
    {
        Template::add(code, var);
    }

    void add(Varcode code, int shortcut) const
    {
        const Var* var = msg->find_by_id(shortcut);
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
        const Var* var = flight_ctx->find(srccode);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        // Use old table for old templates
        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table = 13;
        }

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        bulletin.type = 4;
        bulletin.subtype = 255;
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);

        // Find what is the level where the airplane is in
        flight_ctx = 0;

        for (unsigned i = 0; i < msg.data.size(); ++i)
        {
            const msg::Context& ctx = *msg.data[i];
            if (ctx.trange != Trange::instant()) continue;

            bool use = false;
            switch (ctx.level.ltype1)
            {
                case 100:
                     use = ctx.find_by_id(DBA_MSG_PRESS) != NULL
                        || ctx.find_by_id(DBA_MSG_HEIGHT_STATION) != NULL;
                     break;
                case 102:
                     use = ctx.find_by_id(DBA_MSG_HEIGHT_STATION) != NULL;
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
    Airep(const Exporter::Options& opts, const Msgs& msgs)
        : FlightBase(opts, msgs) {}

    virtual const char* name() const { return AIREP_NAME; }
    virtual const char* description() const { return AIREP_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        FlightBase::setupBulletin(bulletin);
        bulletin.localsubtype = 142;

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

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        FlightBase::to_subset(msg, subset);

        /*  0 */ add(WR_VAR(0,  1,  6), DBA_MSG_IDENT);
        /*  1 */ add(WR_VAR(0,  2, 61));
        /*  2 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /*  3 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /*  4 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /*  5 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /*  6 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /*  7 */ add(WR_VAR(0,  5,  1), DBA_MSG_LATITUDE);
        /*  8 */ add(WR_VAR(0,  6,  1), DBA_MSG_LONGITUDE);
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
    Amdar(const Exporter::Options& opts, const Msgs& msgs)
        : Airep(opts, msgs) {}

    virtual const char* name() const { return AMDAR_NAME; }
    virtual const char* description() const { return AMDAR_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Airep::setupBulletin(bulletin);
        bulletin.localsubtype = 144;
    }
};

struct AmdarWMO : public FlightBase
{
    AmdarWMO(const Exporter::Options& opts, const Msgs& msgs)
        : FlightBase(opts, msgs) {}

    virtual const char* name() const { return AMDAR_WMO_NAME; }
    virtual const char* description() const { return AMDAR_WMO_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        FlightBase::setupBulletin(bulletin);
        bulletin.subtype = 255;
        bulletin.localsubtype = 255;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(0,  1,  33)); //  0
        bulletin.datadesc.push_back(WR_VAR(0,  1,  34)); //  1
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

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        FlightBase::to_subset(msg, subset);

        Level lev;

        /*  0 */ add(WR_VAR(0,  1, 33));
        /*  1 */ add(WR_VAR(0,  1, 34));
        /*  2 */ add(WR_VAR(0,  1,  8), DBA_MSG_IDENT);
        /*  3 */ add(WR_VAR(0,  1, 23));
        /*  4 */ add(WR_VAR(0,  5,  1), DBA_MSG_LATITUDE);
        /*  5 */ add(WR_VAR(0,  6,  1), DBA_MSG_LONGITUDE);
        /*  6 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /*  7 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /*  8 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /*  9 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /* 10 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /* 11 */ add(WR_VAR(0,  4,  6), DBA_MSG_SECOND);
        /* 12 */
        if (const wreport::Var* v = flight_ctx->find(WR_VAR(0,  7, 30)))
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
    Acars(const Exporter::Options& opts, const Msgs& msgs)
        : FlightBase(opts, msgs) {}

    virtual const char* name() const { return ACARS_NAME; }
    virtual const char* description() const { return ACARS_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        FlightBase::setupBulletin(bulletin);

        bulletin.localsubtype = 145;

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

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        FlightBase::to_subset(msg, subset);
        /*  0 */ add(WR_VAR(0,  1,  6));
        /*  1 */ add(WR_VAR(0,  1,  8), DBA_MSG_IDENT);
        /*  2 */ add(WR_VAR(0,  2, 61));
        /*  3 */ add(WR_VAR(0,  2, 62));
        /*  4 */ add(WR_VAR(0,  2,  2));
        /*  5 */ add(WR_VAR(0,  2,  5));
        /*  6 */ add(WR_VAR(0,  2, 70));
        /*  7 */ add(WR_VAR(0,  2, 63));
        /*  8 */ add(WR_VAR(0,  2,  1));
        /*  9 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /* 10 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /* 11 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /* 12 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /* 13 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /* 14 */ add(WR_VAR(0,  5,  2), DBA_MSG_LATITUDE);
        /* 15 */ add(WR_VAR(0,  6,  2), DBA_MSG_LONGITUDE);
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


struct AirepFactory : public TemplateFactory
{
    AirepFactory() { name = AIREP_NAME; description = AIREP_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Airep(opts, msgs));
    }
};
struct AirepEcmwfFactory : public TemplateFactory
{
    AirepEcmwfFactory() { name = AIREP_ECMWF_NAME; description = AIREP_ECMWF_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Airep(opts, msgs));
    }
};
struct AmdarFactory : public TemplateFactory
{
    AmdarFactory() { name = AMDAR_NAME; description = AMDAR_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Amdar(opts, msgs));
    }
};
struct AmdarEcmwfFactory : public TemplateFactory
{
    AmdarEcmwfFactory() { name = AMDAR_ECMWF_NAME; description = AMDAR_ECMWF_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Amdar(opts, msgs));
    }
};
struct AmdarWMOFactory : public TemplateFactory
{
    AmdarWMOFactory() { name = AMDAR_WMO_NAME; description = AMDAR_WMO_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new AmdarWMO(opts, msgs));
    }
};
struct AcarsFactory : public TemplateFactory
{
    AcarsFactory() { name = ACARS_NAME; description = ACARS_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Acars(opts, msgs));
    }
};
struct AcarsEcmwfFactory : public TemplateFactory
{
    AcarsEcmwfFactory() { name = ACARS_ECMWF_NAME; description = ACARS_ECMWF_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Acars(opts, msgs));
    }
};
struct AcarsWMOFactory : public TemplateFactory
{
    AcarsWMOFactory() { name = ACARS_WMO_NAME; description = ACARS_WMO_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new Acars(opts, msgs));
    }
};

} // anonymous namespace

void register_flight(TemplateRegistry& r)
{
static const TemplateFactory* airep = NULL;
static const TemplateFactory* airepecmwf = NULL;
static const TemplateFactory* amdar = NULL;
static const TemplateFactory* amdarecmwf = NULL;
static const TemplateFactory* amdarwmo = NULL;
static const TemplateFactory* acars = NULL;
static const TemplateFactory* acarsecmwf = NULL;
static const TemplateFactory* acarswmo = NULL;

    if (!airep) airep = new AirepFactory;
    if (!airepecmwf) airepecmwf = new AirepEcmwfFactory;
    if (!amdar) amdar = new AmdarFactory;
    if (!amdarecmwf) amdarecmwf = new AmdarEcmwfFactory;
    if (!amdarwmo) amdarwmo = new AmdarWMOFactory;
    if (!acars) acars = new AcarsFactory;
    if (!acarsecmwf) acarsecmwf = new AcarsEcmwfFactory;
    if (!acarswmo) acarswmo = new AcarsWMOFactory;

    r.register_factory(airep);
    r.register_factory(airepecmwf);
    r.register_factory(amdar);
    r.register_factory(amdarecmwf);
    r.register_factory(amdarwmo);
    r.register_factory(acars);
    r.register_factory(acarsecmwf);
    r.register_factory(acarswmo);
}

}
}
}

/* vim:set ts=4 sw=4: */
