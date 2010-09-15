/*
 * DB-ALLe - Archive for point-based meteorological data
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

#include <config.h>

#include "aof_codec.h"
#include "aof_importers/common.h"
#include "msg.h"
#include <dballe/core/file.h>
#include <dballe/msg/msgs.h>
//#include <dballe/core/file_internals.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <byteswap.h>
#include <time.h>
#include <errno.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {

AOFImporter::AOFImporter(const import::Options& opts)
    : Importer(opts) {}
AOFImporter::~AOFImporter() {}

void AOFImporter::import(const Rawmsg& msg, Msgs& msgs) const
{
	/* char id[10]; */
	TRACE("aof_message_decode\n");

	/* Access the raw data in a more comfortable form */
	const uint32_t* obs = (const uint32_t*)msg.data();
	int obs_len = msg.size() / sizeof(uint32_t);

	TRACE("05 grid box number: %d\n", OBS(5));
	TRACE("obs type: %d, %d\n", OBS(6), OBS(7));

    auto_ptr<Msg> out(new Msg);

#if 0
	/* 13 Station ID (1:4) */
	/* 14 Station ID (5:8) */
	/* B01011 [CHARACTER] SHIP OR MOBILE LAND STATION IDENTIFIER */
	parse_station_id(msg, id);
	TRACE("ID: %s\n", id);
	for (i = 0; i < 8 && isspace(id[i]); i++)
		/* Skip leading spaces */ ;
	DBA_RUN_OR_RETURN(aof_message_store_variable_c(msg, DBA_VAR(0,  1,  11), id + i));
#endif

	/* 06 Observation type */
	/* 07 Code type */
	switch (OBS(6))
	{
		case 1: read_synop(obs, obs_len, *out); break;
		case 2: read_flight(obs, obs_len, *out); break;
		case 3: read_satob(obs, obs_len, *out); break;
		case 4: read_dribu(obs, obs_len, *out); break;
		case 5: read_temp(obs, obs_len, *out); break;
		case 6: read_pilot(obs, obs_len, *out); break;
		case 7: read_satem(obs, obs_len, *out); break;
		default:
                error_parse::throwf(msg.filename().c_str(), msg.offset,
					"cannot handle AOF observation type %d subtype %d",
					OBS(5), OBS(6));
	}

    msgs.acquire(out);
}

void AOFImporter::import(const bufrex::Msg& msg, Msgs& msgs) const
{
    throw error_unimplemented("AOF importer cannot import from bufrex::Msg");
}

void AOFImporter::get_category(const Rawmsg& msg, int* category, int* subcategory)
{
	/* Access the raw data in a more comfortable form */
	const uint32_t* obs = (const uint32_t*)msg.data();
	int obs_len = msg.size() / sizeof(uint32_t);

	if (obs_len < 7)
		throw error_parse(msg.filename().c_str(), msg.offset,
				"the buffer is too short to contain an AOF message");

	*category = obs[5];
	*subcategory = obs[6];
}

void AOFImporter::dump(const Rawmsg& msg, FILE* out)
{
	/* Access the raw data in a more comfortable form */
	const uint32_t* obs = (const uint32_t*)msg.data();
	int obs_len = msg.size() / sizeof(uint32_t);

	for (int i = 0; i < obs_len; i++)
		if (obs[i] == 0x7fffffff)
			fprintf(out, "%2d %10s\n", i+1, "missing");
		else
		{
			int j;
			uint32_t x = obs[i];
			fprintf(out, "%2d %10u %8x ", i+1, obs[i], obs[i]);
			for (j = 0; j < 32; j++)
			{
				fputc((x & 0x80000000) != 0 ? '1' : '0', out);
				x <<= 1;
				if ((j+1) % 8 == 0)
					fputc(' ', out);
			}
			fputc('\n', out);
		}
}


void AOFImporter::read_satob(const uint32_t* obs, int obs_len, Msg& msg)
{
	throw error_unimplemented("parsing AOF SATOB observations");
}

void AOFImporter::read_satem(const uint32_t* obs, int obs_len, Msg& msg)
{
	throw error_unimplemented("parsing AOF SATEM observations");
}

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
