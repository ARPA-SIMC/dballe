/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006,2007  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_MSG_FILTER_H
#define DBA_MSG_FILTER_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup msg
 *
 * Filtering functions for dba_msg objects.
 */

#include <dballe/msg/msg.h>
#include <dballe/core/record.h>

/**
 * Create a new dba_msg
 *
 * @retval msg
 *   The newly created dba_msg.
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err dba_msg_filter_copy(dba_msg src, dba_msg* dst, dba_record filter);

#ifdef  __cplusplus
}
#endif

// vim:set ts=4 sw=4:
#endif
