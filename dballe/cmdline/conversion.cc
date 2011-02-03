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

#include "conversion.h"
#include "processor.h"

#include <dballe/core/file.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/codec.h>

#include <wreport/bulletin.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace cmdline {

Converter::~Converter()
{
	if (file) delete file;
	if (importer) delete importer;
	if (exporter) delete exporter;
}

void Converter::process_bufrex_msg(const Bulletin& msg)
{
	Rawmsg raw;
	msg.encode(raw);
	file->write(raw);
}

void Converter::process_dba_msg(const Msgs& msgs)
{
	Rawmsg raw;
	exporter->to_rawmsg(msgs, raw);
	file->write(raw);
}

void Converter::operator()(const cmdline::Item& item)
{
    if (item.msgs == NULL || item.msgs->size() == 0)
    {
        fprintf(stderr, "No interpreted information available: is a recoding enough?\n");
        // See if we can just recode the raw data

        // We want bufrex raw data
        if (item.bulletin == NULL)
        {
            fprintf(stderr, "No BUFREX raw data to attempt low-level bufrex recoding\n");
            return;
        }

		// No report override
		if (dest_rep_memo != NULL)
		{
			fprintf(stderr, "report override not allowed for low-level bufrex recoding\n");
			return;
		}

		// No template change
		if (dest_template != NULL)
		{
			fprintf(stderr, "template change not supported for low-level bufrex recoding\n");
			return;
		}

        // Same encoding
        if ((file->type() == BUFR && string(item.bulletin->encoding_name()) == "CREX")
                || (file->type() == CREX && string(item.bulletin->encoding_name()) == "BUFR"))
        {
            fprintf(stderr, "encoding change not yet supported for low-level bufrex recoding\n");
            return;
        }

        // We can just recode the raw braw
        fprintf(stderr, "we can do a low-level bufrex recoding\n");
        process_bufrex_msg(*item.bulletin);
        return;
    }

    if (dest_rep_memo != NULL)
    {
        // Force message type (will also influence choice of template later)
        MsgType type = Msg::type_from_repmemo(dest_rep_memo);
        for (size_t i = 0; i < item.msgs->size(); ++i)
            (*item.msgs)[i]->type = type;
    }

    process_dba_msg(*item.msgs);
}

}
}

/* vim:set ts=4 sw=4: */
