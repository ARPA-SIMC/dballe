/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_INIT_H
#define DBA_INIT_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @init
 * @ingroup core
 * Initialise and shutdown the library
 */

#include <dballe/core/error.h>

/**
 * Initialise DB-All.e
 * @returns
 *   The error indicator for the function.  See @ref error.h
 */
void dba_init();

/**
 * Shut down DB-All.e
 */
void dba_shutdown();

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
