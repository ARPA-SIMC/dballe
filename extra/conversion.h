/*
 * DB-ALLe - Archive for punctual meteorological data
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

#ifndef CONVERSION_H
#define CONVERSION_H

#include <dballe/core/file.h>
#include <dballe/core/rawmsg.h>
#include <dballe/msg/msgs.h>
#include <dballe/bufrex/msg.h>


struct conversion_info
{
	dba_file file;
	int dest_type;
	int dest_subtype;
	int dest_localsubtype;
	const char* dest_rep_memo;
#if 0
	dba_encoding outType;
	void* outAction;
	void* outActionData;
#endif
};

dba_err convert_message(dba_rawmsg msg, bufrex_msg braw, dba_msgs decoded, void* data);

#endif
