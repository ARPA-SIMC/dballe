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

#ifndef DBA_FORMATTER_H
#define DBA_FORMATTER_H

/** @file
 * @ingroup dballe
 * Common functions for commandline tools
 */

#include <string>

namespace dballe {

/**
 * Describe a level layer.
 *
 * @param ltype  The level type
 * @param l1 The l1 value for the level
 * @returns
 *   The formatted layer description.
 */
std::string describe_level(int ltype, int l1);

/**
 * Describe a level or a layer.
 *
 * @param ltype1  The type of the first level
 * @param l1 The value of the first level
 * @param ltype2  The type of the second level
 * @param l2 The value of the second level
 * @returns
 *   The formatted layer description.
 */
std::string describe_level_or_layer(int ltype1, int l1, int ltype2, int l2);

/**
 * Describe a time range.
 *
 * @param ptype  The time range type
 * @param p1 The p1 value for the time range
 * @param p2 The p2 value for the time range
 * @returns
 *   The formatted time range description.
 */
std::string describe_trange(int ptype, int p1, int p2);

}

/* vim:set ts=4 sw=4: */
#endif
