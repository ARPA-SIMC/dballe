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

#include "wr_codec.h"
#include <wreport/bulletin.h>
#include <wreport/conv.h>
#include "msgs.h"
#include "context.h"

using namespace wreport;
using namespace std;

#define TEMP_NAME "temp"
#define TEMP_DESC "Temp (autodetect)"

#define TEMP_SHIP_NAME "temp-ship"
#define TEMP_SHIP_DESC "Temp ship (autodetect)"

#define TEMP_WMO_NAME "temp-wmo"
#define TEMP_WMO_DESC "Temp WMO (2.101)"

#define TEMP_ECMWF_NAME "temp-ecmwf"
#define TEMP_ECMWF_DESC "Temp ECMWF (2.101)"

#define TEMP_ECMWF_SHIP_NAME "temp-ecmwf-ship"
#define TEMP_ECMWF_SHIP_DESC "Temp ECMWF ship (2.102)"

namespace dballe {
namespace msg {
namespace wr {

namespace {

// Base template for ships
struct TempBase : public Template
{
    bool is_crex;

    TempBase(const Exporter::Options& opts, const Msgs& msgs)
        : Template(opts, msgs) {}

    /// Count the number of sounding levels
    int count_levels() const
    {
        int lev_no = 0;
        for (size_t i = 0; i < msg->data.size(); ++i)
        {
            if (msg->data[i]->find_vsig() != NULL)
                ++lev_no;
        }
        return lev_no;
    }

    int add_sounding_levels()
    {
        int count = count_levels();
        subset->store_variable_i(WR_VAR(0, 31,  1), count);

        // Iterate backwards as we need to add levels in decreasing pressure order
        for (int i = msg->data.size() - 1; i >= 0; --i)
        {
            msg::Context& c = *msg->data[i];

            // We only want levels with a vertical sounding significance
            const Var* vss = c.find_vsig();
            if (vss == NULL) continue;

            /* We only want pressure levels */
            if (c.level.ltype1 != 100) continue;
            double press = c.level.l1;

            /* Add pressure */
            if (const Var* var = c.find(WR_VAR(0, 10, 4)))
                subset->store_variable(WR_VAR(0,  7,  4), *var);
            else
                subset->store_variable_d(WR_VAR(0,  7,  4), press);

            /* Add vertical sounding significance */
            {
                Var nvar(subset->btable->query(WR_VAR(0, 8, 1)), convert_BUFR08042_to_BUFR08001(vss->enqi()));
                nvar.copy_attrs(*vss);
                subset->store_variable(WR_VAR(0, 8, 1), nvar);
            }

            /* Add the rest */
            add(WR_VAR(0, 10, 3), &c, WR_VAR(0, 10,   8));
            add(WR_VAR(0, 12, 1), &c, WR_VAR(0, 12, 101));
            add(WR_VAR(0, 12, 3), &c, WR_VAR(0, 12, 103));
            add(WR_VAR(0, 11, 1), &c, WR_VAR(0, 11,   1));
            add(WR_VAR(0, 11, 2), &c, WR_VAR(0, 11,   2));
        }

        return count;
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

        bulletin.type = 2;
        bulletin.subtype = 255;
    }

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);
    }
};

struct TempWMO : public TempBase
{
    TempWMO(const Exporter::Options& opts, const Msgs& msgs)
        : TempBase(opts, msgs) {}

    virtual const char* name() const { return TEMP_WMO_NAME; }
    virtual const char* description() const { return TEMP_WMO_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        TempBase::setupBulletin(bulletin);
        bulletin.localsubtype = 101;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3, 9, 52));
        bulletin.load_tables();
    }

    void do_D03054(const msg::Context& c)
    {
        add(WR_VAR(0,  4,  86), &c);
        add(WR_VAR(0,  8,  42), &c);
        subset->store_variable_d(WR_VAR(0, 7, 4), c.level.l1);
        add(WR_VAR(0, 10,   9), &c, WR_VAR(0, 10, 8));
        add(WR_VAR(0,  5,  15), &c);
        add(WR_VAR(0,  6,  15), &c);
        add(WR_VAR(0, 12, 101), &c);
        add(WR_VAR(0, 12, 103), &c);
        add(WR_VAR(0, 11,   1), &c);
        add(WR_VAR(0, 11,   2), &c);
    }

    bool do_D03051(const msg::Context& c)
    {
        add(WR_VAR(0,  4, 86), &c);
        add(WR_VAR(0,  8, 42), &c);
        subset->store_variable_d(WR_VAR(0, 7, 4), c.level.l1);
        add(WR_VAR(0,  5, 15), &c);
        add(WR_VAR(0,  6, 15), &c);
        add(WR_VAR(0, 11, 61), &c);
        add(WR_VAR(0, 11, 62), &c);
        return true;
    }

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        TempBase::to_subset(msg, subset);
        do_D01001(); // station id
        add(WR_VAR(0,  1, 11), c_station, DBA_MSG_IDENT);
        add(WR_VAR(0,  2, 11), c_gnd_instant, DBA_MSG_SONDE_TYPE);
        add(WR_VAR(0,  2, 13), c_gnd_instant, DBA_MSG_SONDE_CORRECTION);
        add(WR_VAR(0,  2, 14), c_gnd_instant, DBA_MSG_SONDE_TRACKING);
        add(WR_VAR(0,  2,  3), c_gnd_instant, DBA_MSG_MEAS_EQUIP_TYPE);
        subset.store_variable_i(WR_VAR(0, 8, 21), 18);
        do_D01011(); // date
        do_D01013(); // time
        do_D01021(); // coordinates
        add(WR_VAR(0,  7, 30), c_station, DBA_MSG_HEIGHT);
        add(WR_VAR(0,  7, 31), c_station, DBA_MSG_HEIGHT_BARO);
        add(WR_VAR(0,  7,  7), c_station, DBA_MSG_HEIGHT_RELEASE);
        add(WR_VAR(0, 33, 24), c_station, DBA_MSG_STATION_HEIGHT_QUALITY);

        // Cloud information reported with vertical soundings
        add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
        add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
        add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
        add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
        add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);
        subset.store_variable_undef(WR_VAR(0, 8, 2));
        add(WR_VAR(0, 22, 43), c_gnd_instant, DBA_MSG_WATER_TEMP);

        // Undef for now, we fill it later
        size_t rep_count_pos = subset.size();
        subset.store_variable_undef(WR_VAR(0, 31, 2));

        // Temperature, dew-point and wind data at pressure levels
        int group_count = 0;
        for (std::vector<msg::Context*>::const_reverse_iterator i = msg.data.rbegin();
                i != msg.data.rend(); ++i)
        {
            // Iterate backwards to get pressure levels sorted from the higher
            // to the lower pressure
            const msg::Context* c = *i;
            // Skip non-presure levels
            if (c->level.ltype1 != 100) continue;
            do_D03054(*c);
            ++group_count;
        }
        subset[rep_count_pos].seti(group_count);

        // Wind shear data
        rep_count_pos = subset.size();
        subset.store_variable_undef(WR_VAR(0, 31, 1));
        group_count = 0;
        for (std::vector<msg::Context*>::const_reverse_iterator i = msg.data.rbegin();
                i != msg.data.rend(); ++i)
        {
            // Iterate backwards to get pressure levels sorted from the higher
            // to the lower pressure
            const msg::Context* c = *i;
            // Skip non-presure levels
            if (c->level.ltype1 != 100) continue;
            if (do_D03051(*c))
                ++group_count;
        }
        subset[rep_count_pos].seti(group_count);
    }
};

struct TempEcmwf : public TempBase
{
    TempEcmwf(const Exporter::Options& opts, const Msgs& msgs)
        : TempBase(opts, msgs) {}

    virtual const char* name() const { return TEMP_ECMWF_NAME; }
    virtual const char* description() const { return TEMP_ECMWF_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        TempBase::setupBulletin(bulletin);
        bulletin.localsubtype = 101;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  9,  7));
        if (!is_crex)
        {
                bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
                bulletin.datadesc.push_back(WR_VAR(1,  1,   0));
                bulletin.datadesc.push_back(WR_VAR(0, 31,   2));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  32));
                bulletin.datadesc.push_back(WR_VAR(1,  1,   0));
                bulletin.datadesc.push_back(WR_VAR(0, 31,   2));
                bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }

        bulletin.load_tables();
    }

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        TempBase::to_subset(msg, subset);
        /*  0 */ add(WR_VAR(0,  1,  1), DBA_MSG_BLOCK);
        /*  1 */ add(WR_VAR(0,  1,  2), DBA_MSG_STATION);
        /*  2 */ add(WR_VAR(0,  2, 11), DBA_MSG_SONDE_TYPE);
        /*  3 */ add(WR_VAR(0,  2, 12), DBA_MSG_SONDE_METHOD);
        /*  4 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /*  5 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /*  6 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /*  7 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /*  8 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /*  9 */ add(WR_VAR(0,  5,  1), DBA_MSG_LATITUDE);
        /* 10 */ add(WR_VAR(0,  6,  1), DBA_MSG_LONGITUDE);
        /* 11 */ add(WR_VAR(0,  7,  1), DBA_MSG_HEIGHT);
        /* 12 */ add(WR_VAR(0, 20, 10), DBA_MSG_CLOUD_N);
        /* 13 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        /* 14 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
        /* 15 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
        /* 16 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
        /* 17 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
        /* 18 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);
        int lev_no = add_sounding_levels();
        if (!is_crex)
        {
            int count = subset.append_dpb(WR_VAR(2, 22, 0), subset.size(), WR_VAR(0, 33, 7));
            if (opts.centre != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 31), opts.centre);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 31));
            if (opts.application != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 32), opts.application);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 32));
            subset.store_variable_i(WR_VAR(0, 31, 2), count);
        }
    }
};

struct TempEcmwfShip : public TempBase
{
    TempEcmwfShip(const Exporter::Options& opts, const Msgs& msgs)
        : TempBase(opts, msgs) {}

    virtual const char* name() const { return TEMP_ECMWF_SHIP_NAME; }
    virtual const char* description() const { return TEMP_ECMWF_SHIP_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        TempBase::setupBulletin(bulletin);
        bulletin.localsubtype = 102;

        // Data descriptor section
        bulletin.datadesc.clear();

        /* FIXME: some expansions are not in CREX D tables yet, so we need to use
         * the expanded version here.  The code needs to be changed when newer CREX
         * D tables are available */
        /* WR_VAR(3,  9, 196), Replaced with expansion: */
                bulletin.datadesc.push_back(WR_VAR(3, 1,  3));
                bulletin.datadesc.push_back(WR_VAR(0, 2, 11));
                bulletin.datadesc.push_back(WR_VAR(0, 2, 12));
                bulletin.datadesc.push_back(WR_VAR(3, 1, 11));
                bulletin.datadesc.push_back(WR_VAR(3, 1, 12));
                bulletin.datadesc.push_back(WR_VAR(3, 1, 23));
                bulletin.datadesc.push_back(WR_VAR(0, 7,  1));
        bulletin.datadesc.push_back(WR_VAR(3,  2,  4));
        bulletin.datadesc.push_back(WR_VAR(1,  1,  0));
        bulletin.datadesc.push_back(WR_VAR(0, 31,  1));
        bulletin.datadesc.push_back(WR_VAR(3,  3, 14));
        if (!is_crex)
        {
                bulletin.datadesc.push_back(WR_VAR(2, 22,  0));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  0));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  2));
                bulletin.datadesc.push_back(WR_VAR(0, 31, 31));
                bulletin.datadesc.push_back(WR_VAR(0,  1, 31));
                bulletin.datadesc.push_back(WR_VAR(0,  1, 32));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  0));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  2));
                bulletin.datadesc.push_back(WR_VAR(0, 33,  7));
        }

        bulletin.load_tables();
    }

    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        TempBase::to_subset(msg, subset);
        /*  0 */ add(WR_VAR(0,  1, 11), DBA_MSG_IDENT);
        /*  1 */ add(WR_VAR(0,  1, 12), DBA_MSG_ST_DIR);
        /*  2 */ add(WR_VAR(0,  1, 13), DBA_MSG_ST_SPEED);
        /*  3 */ add(WR_VAR(0,  2, 11), DBA_MSG_SONDE_TYPE);
        /*  4 */ add(WR_VAR(0,  2, 12), DBA_MSG_SONDE_METHOD);
        /*  5 */ add(WR_VAR(0,  4,  1), DBA_MSG_YEAR);
        /*  6 */ add(WR_VAR(0,  4,  2), DBA_MSG_MONTH);
        /*  7 */ add(WR_VAR(0,  4,  3), DBA_MSG_DAY);
        /*  8 */ add(WR_VAR(0,  4,  4), DBA_MSG_HOUR);
        /*  9 */ add(WR_VAR(0,  4,  5), DBA_MSG_MINUTE);
        /* 10 */ add(WR_VAR(0,  5,  2), DBA_MSG_LATITUDE);
        /* 11 */ add(WR_VAR(0,  6,  2), DBA_MSG_LONGITUDE);
        /* 12 */ add(WR_VAR(0,  7,  1), DBA_MSG_HEIGHT);
        /* 13 */ add(WR_VAR(0, 20, 10), DBA_MSG_CLOUD_N);
        /* 14 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        /* 15 */ add(WR_VAR(0, 20, 11), DBA_MSG_CLOUD_NH);
        /* 16 */ add(WR_VAR(0, 20, 13), DBA_MSG_CLOUD_HH);
        /* 17 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CL);
        /* 18 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CM);
        /* 19 */ add(WR_VAR(0, 20, 12), DBA_MSG_CLOUD_CH);
        int lev_no = add_sounding_levels();
        if (!is_crex)
        {
            int count = subset.append_dpb(WR_VAR(2, 22, 0), subset.size(), WR_VAR(0, 33, 7));
            if (opts.centre != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 31), opts.centre);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 31));
            if (opts.application != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 32), opts.application);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 32));
            subset.store_variable_i(WR_VAR(0, 31, 2), count);
        }
    }
};

struct TempFactory : public TemplateFactory
{
    TempFactory() { name = TEMP_NAME; description = TEMP_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new TempEcmwf(opts, msgs));
    }
};
struct TempShipFactory : public TemplateFactory
{
    TempShipFactory() { name = TEMP_SHIP_NAME; description = TEMP_SHIP_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new TempEcmwfShip(opts, msgs));
    }
};
struct TempWMOFactory : public TemplateFactory
{
    TempWMOFactory() { name = TEMP_WMO_NAME; description = TEMP_WMO_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new TempWMO(opts, msgs));
    }
};
struct TempEcmwfFactory : public TemplateFactory
{
    TempEcmwfFactory() { name = TEMP_ECMWF_NAME; description = TEMP_ECMWF_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new TempEcmwf(opts, msgs));
    }
};
struct TempEcmwfShipFactory : public TemplateFactory
{
    TempEcmwfShipFactory() { name = TEMP_ECMWF_SHIP_NAME; description = TEMP_ECMWF_SHIP_DESC; }

    std::auto_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        return auto_ptr<Template>(new TempEcmwfShip(opts, msgs));
    }
};

} // anonymous namespace

void register_temp(TemplateRegistry& r)
{
static const TemplateFactory* temp = NULL;
static const TemplateFactory* tempship = NULL;
static const TemplateFactory* tempwmo = NULL;
static const TemplateFactory* tempecmwf = NULL;
static const TemplateFactory* tempecmwfship = NULL;

    if (!temp) temp = new TempFactory;
    if (!tempship) tempship = new TempShipFactory;
    if (!tempwmo) tempwmo = new TempWMOFactory;
    if (!tempecmwf) tempecmwf = new TempEcmwfFactory;
    if (!tempecmwfship) tempecmwfship = new TempEcmwfShipFactory;

    r.register_factory(temp);
    r.register_factory(tempship);
    r.register_factory(tempwmo);
    r.register_factory(tempecmwf);
    r.register_factory(tempecmwfship);
}

}
}
}

/* vim:set ts=4 sw=4: */
