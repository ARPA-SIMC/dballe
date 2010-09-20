/*
 * dballe/wr_importers/base - Base infrastructure for wreport importers
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "msgs.h"
#include <wreport/bulletin.h>
#include <ctime>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

extern void register_synop(TemplateRegistry&);
extern void register_ship(TemplateRegistry&);
extern void register_buoy(TemplateRegistry&);
extern void register_metar(TemplateRegistry&);
extern void register_temp(TemplateRegistry&);
extern void register_pilot(TemplateRegistry&);
extern void register_flight(TemplateRegistry&);
extern void register_generic(TemplateRegistry&);

static TemplateRegistry* registry = NULL;
const TemplateRegistry& TemplateRegistry::get()
{
    if (!registry)
    {
        registry = new TemplateRegistry;
        
        // Populate it
        register_synop(*registry);
        register_ship(*registry);
        register_buoy(*registry);
        register_metar(*registry);
        register_temp(*registry);
        register_pilot(*registry);
        register_flight(*registry);
        register_generic(*registry);

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
    }
    if (CrexBulletin* b = dynamic_cast<CrexBulletin*>(&bulletin))
    {
        b->master_table = 0;
        b->table = 3;
        b->has_check_digit = 0;
    }
}

void Template::to_subset(const Msg& msg, wreport::Subset& subset)
{
    this->msg = &msg;
    this->subset = &subset;
}

/*
std::auto_ptr<Exporter> Exporter::createMetar(const msg::Exporter::Options&) { throw error_unimplemented("WB Exporters"); }
std::auto_ptr<Exporter> Exporter::createTemp(const msg::Exporter::Options&) { throw error_unimplemented("WB Exporters"); }
std::auto_ptr<Exporter> Exporter::createPilot(const msg::Exporter::Options&) { throw error_unimplemented("WB Exporters"); }
std::auto_ptr<Exporter> Exporter::createFlight(const msg::Exporter::Options&) { throw error_unimplemented("WB Exporters"); }
std::auto_ptr<Exporter> Exporter::createSat(const msg::Exporter::Options&) { throw error_unimplemented("WB Exporters"); }
std::auto_ptr<Exporter> Exporter::createPollution(const msg::Exporter::Options&) { throw error_unimplemented("WB Exporters"); }
std::auto_ptr<Exporter> Exporter::createGeneric(const msg::Exporter::Options&) { throw error_unimplemented("WB Exporters"); }
*/

} // namespace wr
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
