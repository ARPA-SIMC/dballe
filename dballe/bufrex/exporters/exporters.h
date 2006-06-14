/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef BUFREX_EXPORTERS_H
#define BUFREX_EXPORTERS_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/core/dba_var.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/bufrex/bufrex_raw.h>
	
/**
 * Type can be:
 *  0: encode a BUFR
 *  1: encode a CREX
 */
typedef dba_err (*bufrex_exporter_func)(dba_msg src, bufrex_raw dst, int type);

struct _bufrex_exporter
{
	int type;
	int subtype;

	/* dba_msg type it can convert from */
	dba_msg_type accept_type;

	/* Data descriptor section */
	dba_varcode* ddesc;

	/* Exporter function */
	bufrex_exporter_func exporter;
};

typedef struct _bufrex_exporter bufrex_exporter;

/* ID of originating center to put in encoded messages */
#define ORIG_CENTRE_ID 255
/* ID of originating application to put in encoded messages */
#define ORIG_APP_ID 0

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
