/*
 * dballe/wr_codec - BUFR/CREX import and export
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

#include "wr_codec.h"
#include "msgs.h"
#include <wreport/bulletin.h>
#include "wr_importers/base.h"
#include "wr_exporters/base.h"
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

void BufrImporter::from_rawmsg(const Rawmsg& msg, Msgs& msgs) const
{
	BufrBulletin bulletin;
	bulletin.decode(msg);
	from_bulletin(bulletin, msgs);
}

CrexImporter::CrexImporter(const Options& opts)
	: WRImporter(opts) {}
CrexImporter::~CrexImporter() {}

void CrexImporter::from_rawmsg(const Rawmsg& msg, Msgs& msgs) const
{
	CrexBulletin bulletin;
	bulletin.decode(msg);
	from_bulletin(bulletin, msgs);
}

void WRImporter::from_bulletin(const wreport::Bulletin& msg, Msgs& msgs) const
{
	// Infer the right importer
	std::auto_ptr<wr::Importer> importer;
	switch (msg.type)
	{
		case 0:
		case 1:
			if (msg.localsubtype == 140)
				importer = wr::Importer::createMetar(opts);
			else
				importer = wr::Importer::createSynop(opts);
			break;
		case 2:
			if (msg.localsubtype == 91 || msg.localsubtype == 92)
				importer = wr::Importer::createPilot(opts);
			else
				importer = wr::Importer::createTemp(opts);
			break;
		case 3: importer = wr::Importer::createSat(opts); break;
		case 4: importer = wr::Importer::createFlight(opts); break;
		case 8: importer = wr::Importer::createPollution(opts); break;
		default: importer = wr::Importer::createGeneric(opts); break;
	}

	MsgType type = importer->scanType(msg);
	for (unsigned i = 0; i < msg.subsets.size(); ++i)
	{
		std::auto_ptr<Msg> newmsg(new Msg);
		newmsg->type = type;
		importer->import(msg.subsets[i], *newmsg);
		msgs.acquire(newmsg);
	}
}


WRExporter::WRExporter(const Options& opts)
	: Exporter(opts) {}

BufrExporter::BufrExporter(const Options& opts)
	: WRExporter(opts) {}
BufrExporter::~BufrExporter() {}

void BufrExporter::to_rawmsg(const Msgs& msgs, Rawmsg& msg) const
{
	BufrBulletin bulletin;
	to_bulletin(msgs, bulletin);
	bulletin.encode(msg);
}

CrexExporter::CrexExporter(const Options& opts)
	: WRExporter(opts) {}
CrexExporter::~CrexExporter() {}

void CrexExporter::to_rawmsg(const Msgs& msgs, Rawmsg& msg) const
{
	CrexBulletin bulletin;
	to_bulletin(msgs, bulletin);
	bulletin.encode(msg);
}

void WRExporter::to_bulletin(const Msgs& msgs, wreport::Bulletin& bulletin) const
{
	if (msgs.empty())
		throw error_consistency("trying to export an empty message set");

	// Select initial template name
	string tpl = opts.template_name;
	if (tpl.empty())
    {
        switch (msgs[0]->type)
        {
            case MSG_TEMP_SHIP: tpl = "temp-ship"; break;
            default: tpl = msg_type_name(msgs[0]->type); break;
        }
    }

	// Get template factory
	const wr::TemplateFactory& fac = wr::TemplateRegistry::get(tpl);
	std::auto_ptr<wr::Template> encoder = fac.make(opts, msgs);
	encoder->to_bulletin(bulletin);
}

#if 0
	/* Setup encoding parameters */
	if (braw->type == 255 && braw->subtype == 255 && braw->localsubtype == 0)
	{
		braw->opt.bufr.centre = 200;
		braw->opt.bufr.subcentre = 0;
		braw->opt.bufr.master_table = 14;
		braw->opt.bufr.local_table = 0;
	} else if (braw->type == 8 && braw->subtype == 255 && braw->localsubtype == 171) {
		braw->opt.bufr.centre = 98;
		braw->opt.bufr.subcentre = 0;
		braw->opt.bufr.master_table = 13;
		braw->opt.bufr.local_table = 102;
	} else {
		braw->opt.bufr.centre = 98;
		braw->opt.bufr.subcentre = 0;
		braw->opt.bufr.master_table = 6;
		braw->opt.bufr.local_table = 1;
	}

	/* Load appropriate tables for the target message */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_load_tables(braw));

	/* Fill in with the vales from msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_from_dba_msgs(braw, msgs));

	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "BUFR data before encoding:\n");
		bufrex_msg_print(braw, DBA_VERBOSE_STREAM);
	}

	/* Encode */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_encode(braw, raw));

cleanup:
	if (braw != NULL)
		bufrex_msg_delete(braw);
	if (err != DBA_OK && *raw != NULL)
	{
		dba_rawmsg_delete(*raw);
		*raw = 0;
	}
	return err == DBA_OK ? dba_error_ok() : err;
}
#endif

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
