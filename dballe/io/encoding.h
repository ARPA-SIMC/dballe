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

#ifndef DBA_IO_ENCODING_H
#define DBA_IO_ENCODING_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup io
 * The dba_encoding enum.
 */

/**
 * Supported encodings for report data
 */
typedef enum {
	BUFR = 0,
	CREX = 1,
	AOF = 2
} dba_encoding;

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
