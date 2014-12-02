/*
 * msg/codec - General codec options
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

#include "codec.h"
#include "aof_codec.h"
#include "wr_codec.h"
#include <dballe/core/rawmsg.h>
#include <wreport/error.h>
#include <wreport/bulletin.h>

#include "config.h"

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {

void Importer::Options::print(FILE* out)
{
    string str = to_string();
    fputs(str.c_str(), out);
}

std::string Importer::Options::to_string() const
{
    string res;
    res += simplified ? "simplified" : "accurate";
    return res;
}

Importer::Options Importer::Options::from_string(const std::string& s)
{
    Importer::Options res;
    if (s.empty()) return res;
    if (s == "simplified") return res;
    if (s == "accurate")
        res.simplified = false;
    return res;
}

Importer::Importer(const Options& opts)
    : opts(opts)
{
}

Importer::~Importer()
{
}

std::unique_ptr<Importer> Importer::create(Encoding type, const Options& opts)
{
    switch (type)
    {
        case BUFR:
            return unique_ptr<Importer>(new BufrImporter(opts));
        case CREX:
            return unique_ptr<Importer>(new CrexImporter(opts));
        case AOF:
            return unique_ptr<Importer>(new AOFImporter(opts));
        default:
            error_unimplemented::throwf("%s importer is not implemented yet", encoding_name(type));
    }
}


void Exporter::Options::print(FILE* out)
{
    string str = to_string();
    fputs(str.c_str(), out);
}

std::string Exporter::Options::to_string() const
{
    string res;
    char buf[100];

    if (!template_name.empty())
        res += "tpl " + template_name;

    if (centre != MISSING_INT)
    {
        if (!res.empty()) res += ", ";
        snprintf(buf, 100, "centre %d", centre);
        res += buf;
    }

    if (subcentre != MISSING_INT)
    {
        if (!res.empty()) res += ", ";
        snprintf(buf, 100, "subcentre %d", subcentre);
        res += buf;
    }

    if (application != MISSING_INT)
    {
        if (!res.empty()) res += ", ";
        snprintf(buf, 100, "application %d", application);
        res += buf;
    }

    return res;
}

Exporter::Exporter(const Options& opts)
    : opts(opts)
{
}

Exporter::~Exporter()
{
}

std::unique_ptr<wreport::Bulletin> Exporter::make_bulletin() const
{
    return std::unique_ptr<wreport::Bulletin>(nullptr);
}

std::unique_ptr<Exporter> Exporter::create(Encoding type, const Options& opts)
{
    switch (type)
    {
        case BUFR:
            return unique_ptr<Exporter>(new BufrExporter(opts));
        case CREX:
            return unique_ptr<Exporter>(new CrexExporter(opts));
        case AOF:
            //return unique_ptr<Exporter>(new AOFExporter(opts));
        default:
            error_unimplemented::throwf("%s exporter is not implemented yet", encoding_name(type));
    }
}

#if 0

Decoder::Decoder()
	: opt_simplified(false)
{
}

dba_err Decoder::decode(dba_rawmsg rmsg, dba_msgs *msgs)
{
	switch (rmsg->encoding)
	{
#ifdef HAVE_DBALLE_BUFREX
		case BUFR: DBA_RUN_OR_RETURN(bufrex_decode_bufr(rmsg, opts, msgs)); break;
		case CREX: DBA_RUN_OR_RETURN(bufrex_decode_crex(rmsg, msgs)); break;
#endif
		case AOF: DBA_RUN_OR_RETURN(aof_codec_decode(rmsg, msgs)); break;
	}
	return dba_error_ok();
}

dba_err Decoder::read(dba_file file, dba_msgs* msgs, int* found)
{
	dba_err err = DBA_OK;
	dba_rawmsg rm = NULL;
	
	DBA_RUN_OR_RETURN(dba_rawmsg_create(&rm));
	DBA_RUN_OR_GOTO(cleanup, dba_file_read(file, rm, found));
	if (*found)
		/* Parse the message */
		DBA_RUN_OR_GOTO(cleanup, decode(rm, msgs));
	else
		*msgs = NULL;

cleanup:
	if (rm != NULL)
		dba_rawmsg_delete(rm);
	return err == DBA_OK ? dba_error_ok() : err;
}

void Decoder::print(FILE* out)
{
	putc('[', out);
	fputs(opt_simplified ? "simplified" : "accurate", out);
	putc(']', out);
}

Encoder::Encoder()
{
}

dba_err Encoder::encode(dba_msgs msgs, dba_encoding type, dba_rawmsg *rmsg)
{
	switch (type)
	{
#ifdef HAVE_DBALLE_BUFREX
		case BUFR: DBA_RUN_OR_RETURN(bufrex_encode_bufr(msgs, 0, 0, 0, rmsg)); break;
		case CREX: DBA_RUN_OR_RETURN(bufrex_encode_crex(msgs, 0, 0, rmsg)); break;
#endif
		case AOF: return dba_error_unimplemented("exporting to AOF"); break;
	}
	return dba_error_ok();
}

dba_err Encoder::write(dba_file file, dba_msgs msgs, int cat, int subcat, int localsubcat)
{
	dba_err err = DBA_OK;
	dba_rawmsg raw = NULL;

	switch (dba_file_type(file))
	{
		case BUFR:
			DBA_RUN_OR_GOTO(cleanup, bufrex_encode_bufr(msgs, cat, subcat, localsubcat, &raw));
			DBA_RUN_OR_GOTO(cleanup, dba_file_write(file, raw));
			break;
		case CREX:
			DBA_RUN_OR_GOTO(cleanup, bufrex_encode_crex(msgs, cat, localsubcat, &raw));
			DBA_RUN_OR_GOTO(cleanup, dba_file_write(file, raw));
			break;
		case AOF: 
			err = dba_error_unimplemented("export to AOF format");
			goto cleanup;
	}

cleanup:
	if (raw != NULL)
		dba_rawmsg_delete(raw);
	return err == DBA_OK ? dba_error_ok() : err;
}

#endif

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
