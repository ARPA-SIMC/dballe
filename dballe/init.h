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

#ifndef DBA_INIT
#define DBA_INIT

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup dballe
 * Library initialisation and shutdown
 */

#include <dballe/err/dba_error.h>
	
/**
 * Initialize the library.
 *
 * This function needs to be called just once at the beginning of the work.
 */
dba_err dba_init();

/**
 * Shutdown the library.
 *
 * This function needs to be called just once at the end of the work.
 */
void dba_shutdown();


#ifdef  __cplusplus
}
#endif

#endif
