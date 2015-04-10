/*
 * dballe/wr_codec - BUFR/CREX import and export
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

#include "wr_codec.h"
#include "msgs.h"
#include "context.h"
#include <wreport/bulletin.h>
#include "wr_importers/base.h"
//#include <dballe/core/verbose.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {

WRImporter::WRImporter(const Options& opts)
    : Importer(opts) {}

BufrImporter::BufrImporter(const Options& opts)
    : WRImporter(opts) {}
BufrImporter::~BufrImporter() {}

bool BufrImporter::foreach_decoded(const Rawmsg& msg, std::function<bool(std::unique_ptr<Msg>)> dest) const
{
    unique_ptr<BufrBulletin> bulletin(BufrBulletin::create());
    bulletin->decode(msg);
    foreach_decoded_bulletin(*bulletin, dest);
}

CrexImporter::CrexImporter(const Options& opts)
    : WRImporter(opts) {}
CrexImporter::~CrexImporter() {}

bool CrexImporter::foreach_decoded(const Rawmsg& msg, std::function<bool(std::unique_ptr<Msg>)> dest) const
{
    unique_ptr<CrexBulletin> bulletin(CrexBulletin::create());
    bulletin->decode(msg);
    foreach_decoded_bulletin(*bulletin, dest);
}

void WRImporter::from_bulletin(const wreport::Bulletin& msg, Msgs& msgs) const
{
    foreach_decoded_bulletin(msg, [&](unique_ptr<Msg> m) { msgs.acquire(move(m)); return true; });
}

bool WRImporter::foreach_decoded_bulletin(const wreport::Bulletin& msg, std::function<bool(std::unique_ptr<Msg>)> dest) const
{
    // Infer the right importer. See Common Code Table C-13
    std::unique_ptr<wr::Importer> importer;
    switch (msg.type)
    {
        // Surface data - land
        case 0:
            switch (msg.subtype)
            {
                // Routine aeronautical observations (METAR)
                case 10:
                    importer = wr::Importer::createMetar(opts);
                    break;
                default:
                    // Old ECMWF METAR type
                    if (msg.localsubtype == 140)
                        importer = wr::Importer::createMetar(opts);
                    else
                        importer = wr::Importer::createSynop(opts);
                    break;
            }
            break;
        // Surface data - sea
        case 1: importer = wr::Importer::createShip(opts); break;
        // Vertical soundings (other than satellite)
        case 2: importer = wr::Importer::createTemp(opts); break;
        // Vertical soundings (satellite)
        case 3: importer = wr::Importer::createSat(opts); break;
        // Single level upper-air data (other than satellite)
        case 4: importer = wr::Importer::createFlight(opts); break;
        // Radar data
        case 6:
            if (msg.subtype == 1)
                // Doppler wind profiles
                importer = wr::Importer::createTemp(opts);
            else
                importer = wr::Importer::createGeneric(opts);
            break;
        // Physical/chemical constituents
        case 8: importer = wr::Importer::createPollution(opts); break;
        default: importer = wr::Importer::createGeneric(opts); break;
    }

    MsgType type = importer->scanType(msg);
    for (unsigned i = 0; i < msg.subsets.size(); ++i)
    {
        std::unique_ptr<Msg> newmsg(new Msg);
        newmsg->type = type;
        importer->import(msg.subsets[i], *newmsg);
        if (!dest(move(newmsg)))
            return false;
    }
    return true;
}


WRExporter::WRExporter(const Options& opts)
    : Exporter(opts) {}

BufrExporter::BufrExporter(const Options& opts)
    : WRExporter(opts) {}

BufrExporter::~BufrExporter() {}

std::unique_ptr<wreport::Bulletin> BufrExporter::make_bulletin() const
{
    return std::unique_ptr<wreport::Bulletin>(BufrBulletin::create().release());
}

void BufrExporter::to_rawmsg(const Msgs& msgs, Rawmsg& msg) const
{
    unique_ptr<BufrBulletin> bulletin(BufrBulletin::create());
    to_bulletin(msgs, *bulletin);
    bulletin->encode(msg);
}

CrexExporter::CrexExporter(const Options& opts)
    : WRExporter(opts) {}
CrexExporter::~CrexExporter() {}

std::unique_ptr<wreport::Bulletin> CrexExporter::make_bulletin() const
{
    return std::unique_ptr<wreport::Bulletin>(CrexBulletin::create().release());
}

void CrexExporter::to_rawmsg(const Msgs& msgs, Rawmsg& msg) const
{
    unique_ptr<CrexBulletin> bulletin(CrexBulletin::create());
    to_bulletin(msgs, *bulletin);
    bulletin->encode(msg);
}

namespace {

const char* infer_from_message(const Msg& msg)
{
    switch (msg.type)
    {
        case MSG_TEMP_SHIP: return "temp-ship";
        default: break;
    }
    return msg_type_name(msg.type);
}

}

void WRExporter::to_bulletin(const Msgs& msgs, wreport::Bulletin& bulletin) const
{
    if (msgs.empty())
        throw error_consistency("trying to export an empty message set");

    // Select initial template name
    string tpl = opts.template_name;
    if (tpl.empty())
        tpl = infer_from_message(*msgs[0]);

    // Get template factory
    const wr::TemplateFactory& fac = wr::TemplateRegistry::get(tpl);
    std::unique_ptr<wr::Template> encoder = fac.make(opts, msgs);
    // fprintf(stderr, "Encoding with template %s\n", encoder->name());
    encoder->to_bulletin(bulletin);
}

namespace wr {

extern void register_synop(TemplateRegistry&);
extern void register_ship(TemplateRegistry&);
extern void register_buoy(TemplateRegistry&);
extern void register_metar(TemplateRegistry&);
extern void register_temp(TemplateRegistry&);
extern void register_flight(TemplateRegistry&);
extern void register_generic(TemplateRegistry&);
extern void register_pollution(TemplateRegistry&);

namespace {
struct WMOFactory : public TemplateFactory
{
    WMOFactory() { name = "wmo"; description = "WMO style templates (autodetect)"; }

    std::unique_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const
    {
        const Msg& msg = *msgs[0];
        string tpl;
        switch (msg.type)
        {
            case MSG_TEMP_SHIP: tpl = "temp-wmo"; break;
            default:
                tpl = msg_type_name(msg.type);
                tpl += "-wmo";
                break;
        }
        const wr::TemplateFactory& fac = wr::TemplateRegistry::get(tpl);
        return fac.make(opts, msgs);
    }
};
}

static TemplateRegistry* registry = NULL;
const TemplateRegistry& TemplateRegistry::get()
{
static const TemplateFactory* wmo = NULL;

    if (!registry)
    {
        registry = new TemplateRegistry;

        if (!wmo) wmo = new WMOFactory;
        registry->register_factory(wmo);

        // Populate it
        register_synop(*registry);
        register_ship(*registry);
        register_buoy(*registry);
        register_metar(*registry);
        register_temp(*registry);
        register_flight(*registry);
        register_generic(*registry);
        register_pollution(*registry);

        // registry->insert("synop", ...)
        // registry->insert("synop-high", ...)
        // registry->insert("wmo-synop", ...)
        // registry->insert("wmo-synop-high", ...)
        // registry->insert("ecmwf-synop", ...)
        // registry->insert("ecmwf-synop-high", ...)
    }
    return *registry;
}

const TemplateFactory& TemplateRegistry::get(const std::string& name)
{
    const TemplateRegistry& tr = get();
    TemplateRegistry::const_iterator i = tr.find(name);
    if (i == tr.end())
        error_notfound::throwf("requested export template %s which does not exist", name.c_str());
    return *(i->second);
}

void TemplateRegistry::register_factory(const TemplateFactory* fac)
{
    insert(make_pair(fac->name, fac));
}

void Template::to_bulletin(wreport::Bulletin& bulletin)
{
    setupBulletin(bulletin);

    for (unsigned i = 0; i < msgs.size(); ++i)
    {
        Subset& s = bulletin.obtain_subset(i);
        to_subset(*msgs[i], s);
    }
}

void Template::setupBulletin(wreport::Bulletin& bulletin)
{
    // Get reference time from first msg in the set
    // If not found, use current time.
    const Msg& msg = *msgs[0];
    bool has_date = true;
    if (const Var* v = msg.get_year_var())
        bulletin.rep_year = v->enqi();
    else
        has_date = false;
    if (const Var* v = msg.get_month_var())
        bulletin.rep_month = v->enqi();
    else
        has_date = false;
    if (const Var* v = msg.get_day_var())
        bulletin.rep_day = v->enqi();
    else
        has_date = false;
    if (const Var* v = msg.get_hour_var())
        bulletin.rep_hour = v->enqi();
    else
        bulletin.rep_hour = 0;
    if (const Var* v = msg.get_minute_var())
        bulletin.rep_minute = v->enqi();
    else
        bulletin.rep_minute = 0;
    if (const Var* v = msg.get_second_var())
        bulletin.rep_second = v->enqi();
    else
        bulletin.rep_second = 0;
    bulletin.master_table_number = 0;
    if (!has_date)
    {
        // use today
        time_t tnow = time(NULL);
        struct tm now;
        gmtime_r(&tnow, &now);
        bulletin.rep_year = now.tm_year + 1900;
        bulletin.rep_month = now.tm_mon + 1;
        bulletin.rep_day = now.tm_mday;
        bulletin.rep_hour = now.tm_hour;
        bulletin.rep_minute = now.tm_min;
        bulletin.rep_second = now.tm_sec;
    }

    if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
    {
        // Take from opts
        b->centre = opts.centre != MISSING_INT ? opts.centre : 255;
        b->subcentre = opts.subcentre != MISSING_INT ? opts.subcentre : 255;
        b->master_table = 14;
        b->local_table = 0;
        b->compression = 0;
        b->update_sequence_number = 0;
        b->edition = 4;
    }
    if (CrexBulletin* b = dynamic_cast<CrexBulletin*>(&bulletin))
    {
        b->table = 3;
        b->has_check_digit = 0;
        b->edition = 2;
    }
}

void Template::to_subset(const Msg& msg, wreport::Subset& subset)
{
    this->msg = &msg;
    this->subset = &subset;
    this->c_station = msg.find_context(Level::ana(), Trange::ana());
    this->c_gnd_instant = msg.find_context(Level(1), Trange::instant());
}

void Template::add(Varcode code, const msg::Context* ctx, int shortcut) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->find_by_id(shortcut))
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void Template::add(Varcode code, const msg::Context* ctx, Varcode srccode) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->find(srccode))
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void Template::add(Varcode code, const msg::Context* ctx) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->find(code))
        subset->store_variable(*var);
    else
        subset->store_variable_undef(code);
}

void Template::add(Varcode code, int shortcut) const
{
    add(code, msg->find_by_id(shortcut));
}

void Template::add(Varcode code, Varcode srccode, const Level& level, const Trange& trange) const
{
    add(code, msg->find(srccode, level, trange));
}

void Template::add(wreport::Varcode code, const wreport::Var* var) const
{
    if (var)
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void Template::add_st_name(wreport::Varcode dstcode, const msg::Context* ctx) const
{
    if (!ctx)
        subset->store_variable_undef(dstcode);
    else if (const wreport::Var* var = ctx->find_by_id(DBA_MSG_ST_NAME))
    {
        Varinfo info = subset->btable->query(dstcode);
        Var name(info);
        if (var->value())
            name.setc_truncate(var->value());
        subset->store_variable(name);
    }
    else
        subset->store_variable_undef(dstcode);
}

void Template::do_ecmwf_past_wtr() const
{
    int hour = 0;
    if (const Var* var = msg->get_hour_var())
        hour = var->enqi();
    if (hour % 6 == 0)
    {
        add(WR_VAR(0, 20,  4), DBA_MSG_PAST_WTR1_6H);
        add(WR_VAR(0, 20,  5), DBA_MSG_PAST_WTR2_6H);
    } else {
        add(WR_VAR(0, 20,  4), DBA_MSG_PAST_WTR1_3H);
        add(WR_VAR(0, 20,  5), DBA_MSG_PAST_WTR2_3H);
    }
}

void Template::do_D01001() const
{
    add(WR_VAR(0,  1,  1), c_station, DBA_MSG_BLOCK);
    add(WR_VAR(0,  1,  2), c_station, DBA_MSG_STATION);
}

void Template::do_D01004() const
{
    do_D01001();
    add_st_name(WR_VAR(0, 1, 15), c_station);
    add(WR_VAR(0,  2,  1), c_station, DBA_MSG_ST_TYPE);
}

void Template::do_D01011() const
{
    add(WR_VAR(0,  4,  1), c_station, DBA_MSG_YEAR);
    add(WR_VAR(0,  4,  2), c_station, DBA_MSG_MONTH);
    add(WR_VAR(0,  4,  3), c_station, DBA_MSG_DAY);
}

int Template::do_D01012() const
{
    const Var* v_hour = c_station ? c_station->find_by_id(DBA_MSG_HOUR) : NULL;
    add(WR_VAR(0,  4,  4), v_hour);
    add(WR_VAR(0,  4,  5), c_station, DBA_MSG_MINUTE);
    return v_hour ? v_hour->enqi() : MISSING_INT;
}

void Template::do_D01013() const
{
    do_D01012();
    add(WR_VAR(0,  4,  6), c_station, DBA_MSG_SECOND);
}

void Template::do_D01021() const
{
    add(WR_VAR(0,  5,  1), c_station, DBA_MSG_LATITUDE);
    add(WR_VAR(0,  6,  1), c_station, DBA_MSG_LONGITUDE);
}

void Template::do_D01022() const
{
    do_D01021();
    add(WR_VAR(0,  7,  1), c_station, DBA_MSG_HEIGHT_STATION);
}

} // namespace wr


} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
