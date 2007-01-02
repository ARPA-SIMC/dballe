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

#ifndef DBA_MSG_FILE_H
#define DBA_MSG_FILE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup dballe
 * High-level access to files with encoded weather data.
 */

#include <dballe/core/file.h>
#include <dballe/msg/msgs.h>

/**
 * Read and parse a message from the file.
 *
 * @param file
 *   ::dba_file to read from
 * @retval msg
 *   The resulting ::dba_msg
 * @retval found
 *   Will be set to true if a message has been found in the file, else to false.
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_file_read_msgs(dba_file file, dba_msgs* msgs, int* found);

/**
 * Write a message to the file.
 *
 * @param file
 *   ::dba_file to write to
 * @param msg
 *   The dba_msg with to encode and write.
 * @param cat
 *   The BUFR of CREX message category to use for encoding (0 for auto detect)
 * @param subcat
 *   The BUFR of CREX message subcategory to use for encoding (0 for auto detect)
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_file_write_msgs(dba_file file, dba_msgs msgs, int cat, int subcat);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
