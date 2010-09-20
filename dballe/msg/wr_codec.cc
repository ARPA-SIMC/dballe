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
dba_err bufrex_encode_bufr(dba_msgs msgs, int type, int subtype, int localsubtype, dba_rawmsg* raw)
{
	dba_err err = DBA_OK;
	bufrex_msg braw = NULL;
	*raw = NULL;

	if (msgs->len == 0) return dba_error_consistency("tried to encode an empty dba_msgs");

	/* Create and setup the bufrex_msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(BUFREX_BUFR, &braw));

	/* Always create BUFR edition 3 */
	braw->edition = 3;

	/* Compute the right type and subtype if missing */
	if (type == 0 && subtype == 0 && localsubtype == 0)
		DBA_RUN_OR_GOTO(cleanup, bufrex_infer_type_subtype(msgs->msgs[0], &(braw->type), &(braw->subtype), &(braw->localsubtype)));
	else
	{
		braw->type = type;
		braw->subtype = subtype;
		braw->localsubtype = localsubtype;
	}

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

dba_err bufrex_encode_crex(dba_msgs msgs, int type, int localsubtype, dba_rawmsg* raw)
{
	dba_err err = DBA_OK;
	bufrex_msg braw = NULL;
	*raw = NULL;

	if (msgs->len == 0) return dba_error_consistency("tried to encode an empty dba_msgs");

	/* Create and setup the bufrex_msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(BUFREX_CREX, &braw));

	/* Compute the right type and subtype if missing */
	if (type == 0 || localsubtype == 0)
		DBA_RUN_OR_GOTO(cleanup, bufrex_infer_type_subtype(msgs->msgs[0], &(braw->type), &(braw->subtype), &(braw->localsubtype)));
	else
	{
		braw->type = type;
		braw->subtype = 255;
		braw->localsubtype = localsubtype;
	}

/* fprintf(stderr, "From %d %d chosen %d %d %d\n", type, localsubtype, braw->type, braw->subtype, braw->localsubtype); */

	/* Setup encoding parameters */
	switch (msgs->msgs[0]->type)
	{
		case MSG_GENERIC:
			braw->opt.crex.master_table = 99;
			braw->edition = 2;
			braw->opt.crex.table = 3;
			break;
		default:
			braw->opt.crex.master_table = 0;
			braw->edition = 1;
			braw->opt.crex.table = 3;
			break;
	}

/* fprintf(stderr, "TYPE IS %s, chosen %d %d %d\n", dba_msg_type_name(msgs->msgs[0]->type), braw->opt.crex.master_table, braw->edition, braw->opt.crex.table); */

	/* Load appropriate tables for the target message */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_load_tables(braw));

	/* Fill in with the vales from msg */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_from_dba_msgs(braw, msgs));

	if (dba_verbose_is_allowed(DBA_VERB_BUFREX_MSG))
	{
		dba_verbose(DBA_VERB_BUFREX_MSG, "CREX data before encoding:\n");
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

dba_err bufrex_msg_from_dba_msg(bufrex_msg raw, dba_msg msg)
{
	bufrex_exporter exp;
	bufrex_subset subset;

	/* Find the appropriate exporter, and compute type and subtype if missing */
	DBA_RUN_OR_RETURN(bufrex_get_exporter(msg, raw->type, raw->subtype, raw->localsubtype, &exp));

	/* Init the bufrex_msg data descriptor chain */
	DBA_RUN_OR_RETURN(exp->datadesc(exp, msg, raw));

	/* Import the message into the first subset */
	DBA_RUN_OR_RETURN(bufrex_msg_get_subset(raw, 0, &subset));

	/* Fill up the bufrex_msg with variables from msg */
	DBA_RUN_OR_RETURN(exp->exporter(msg, raw, subset, raw->encoding_type == BUFREX_BUFR ? 0 : 1));

	/* Fill in the nominal datetime informations */
	{
		dba_var var;
		if ((var = dba_msg_get_year_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_year)));
		if ((var = dba_msg_get_month_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_month)));
		if ((var = dba_msg_get_day_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_day)));
		if ((var = dba_msg_get_hour_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_hour)));
		if ((var = dba_msg_get_minute_var(msg)) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_minute)));
	}
	
	return dba_error_ok();
}

dba_err bufrex_msg_from_dba_msgs(bufrex_msg raw, dba_msgs msgs)
{
	bufrex_exporter exp;
	int i;

	if (msgs->len == 0)
		return dba_error_consistency("tried to export an empty dba_msgs");

	/* Find the appropriate exporter, and compute type and subtype if missing */
	DBA_RUN_OR_RETURN(bufrex_get_exporter(msgs->msgs[0], raw->type, raw->subtype, raw->localsubtype, &exp));

	/* Init the bufrex_msg data descriptor chain */
	DBA_RUN_OR_RETURN(exp->datadesc(exp, msgs->msgs[0], raw));

	/* Import each message into its own subset */
	for (i = 0; i < msgs->len; ++i)
	{
		bufrex_subset subset;
		DBA_RUN_OR_RETURN(bufrex_msg_get_subset(raw, i, &subset));

		/* Fill up the bufrex_msg with variables from msg */
		DBA_RUN_OR_RETURN(exp->exporter(msgs->msgs[i], raw, subset, raw->encoding_type == BUFREX_BUFR ? 0 : 1));
	}

	/* Fill in the nominal datetime informations */
	{
		dba_var var;
		if ((var = dba_msg_get_year_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_year)));
		if ((var = dba_msg_get_month_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_month)));
		if ((var = dba_msg_get_day_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_day)));
		if ((var = dba_msg_get_hour_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_hour)));
		if ((var = dba_msg_get_minute_var(msgs->msgs[0])) != NULL)
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &(raw->rep_minute)));
	}
	
	return dba_error_ok();
}

#endif

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
