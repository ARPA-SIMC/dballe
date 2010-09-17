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

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

static TemplateRegistry* registry = NULL;
const TemplateRegistry& TemplateRegistry::get()
{
    if (!registry)
    {
        registry = new TemplateRegistry;
        // TODO: populate it
        // registry->insert("synop", ...)
        // registry->insert("synop-high", ...)
        // registry->insert("wmo-synop", ...)
        // registry->insert("wmo-synop-high", ...)
        // registry->insert("ecmwf-synop", ...)
        // registry->insert("ecmwf-synop-high", ...)
    }
    return *registry;
}

TemplateFactory TemplateRegistry::get(const std::string& name)
{
    const TemplateRegistry& tr = get();
    TemplateRegistry::const_iterator i = tr.find(name);
    if (i == tr.end())
        error_notfound::throwf("requested export template %s which does not exist", name.c_str());
    return *(i->second);
}


void Template::to_bulletin(wreport::Bulletin& bulletin)
{
    setupBulletin(msgs, bulletin);

    for (unsigned i = 0; i < msgs.size(); ++i)
    {
        Subset& s = bulletin.obtain_subset(i);
        to_subset(*msgs[i], s);
    }
}

void Template::setupBulletin(const Msgs& msgs, wreport::Bulletin& bulletin)
{
#if 0
    if (BUFR)
    {
        // Take from opts
        braw->opt.bufr.centre = 200;
        braw->opt.bufr.subcentre = 0;
    }
#endif
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
