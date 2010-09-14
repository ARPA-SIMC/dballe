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

#ifndef DBA_AOF_IMPORTERS_COMMON_H
#define DBA_AOF_IMPORTERS_COMMON_H

/*
 * Common functions for all AOF decoders.
 */

#include <dballe/core/conv.h>
#include <dballe/msg/msg.h>
#include <dballe/msg/aof_codec.h>

#include <stdio.h>
#include <stdint.h>		/* uint32_t */
#include <math.h>

// #define TRACE_DECODER

#ifdef TRACE_DECODER
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif

#define AOF_UNDEF 0x7fffffff

#define OBS(n) (obs[n-1])

namespace dballe {
namespace msg {

/* Parse a 2 bit confidence interval into a percent confidence interval */
static inline int get_conf2(uint32_t conf)
{
	switch (conf & 3)
	{
		case 0: return 76; break;
		case 1: return 51; break;
		case 2: return 26; break;
		case 3: return 0; break;
	}
	return 0;
}

/* Parse a 6 bit confidence interval into a percent confidence interval */
static inline int get_conf6(uint32_t conf)
{
	return get_conf2(conf >> 3);
}

/* Convert Kelvin from AOF to Kelvins */
static inline double totemp(double k) { return k / 10.0; }

#if 0
/* Dump a word */
void dba_aof_dump_word(const char* prefix, uint32_t x);

/* Parse latitude, longitude, date and time in the Observation Header */
dba_err dba_aof_parse_lat_lon_datetime(dba_msg msg, const uint32_t* obs);

/* Parse WMO block and station numbers in the Observation Header */
dba_err dba_aof_parse_st_block_station(dba_msg msg, const uint32_t* obs);

/* Parse string ident in the Observation Header */
dba_err dba_aof_parse_st_ident(dba_msg msg, const uint32_t* obs);

/* Parse station altitude the Observation Header */
dba_err dba_aof_parse_altitude(dba_msg msg, const uint32_t* obs);

/* Parse 27 Weather group in Synop observations */
dba_err dba_aof_parse_weather_group(dba_msg msg, const uint32_t* obs);

/* Parse 28 General cloud group in Synop observations */
dba_err dba_aof_parse_general_cloud_group(dba_msg msg, const uint32_t* obs);

/* Parse a bit-packed cloud group in Synop observations */
dba_err dba_aof_parse_cloud_group(uint32_t val, int* ns, int* c, int* h);

uint32_t dba_aof_get_extra_conf(const uint32_t* obs, int idx);

/* Count the number of bits present in a word */
static inline int count_bits(uint32_t v)
{
	/* Algoritmo basato su Magic Binary Numbers
	 * http://graphics.stanford.edu/~seander/bithacks.html
	const int S[] = {1, 2, 4, 8, 16}; // Magic Binary Numbers
	const int B[] = {0x55555555, 0x33333333, 0x0F0F0F0F, 0x00FF00FF, 0x0000FFFF};

	v = ((v >> S[0]) & B[0]) + (v & B[0]);
	v = ((v >> S[1]) & B[1]) + (v & B[1]);
	v = ((v >> S[2]) & B[2]) + (v & B[2]);
	v = ((v >> S[3]) & B[3]) + (v & B[3]);
	v = ((v >> S[4]) & B[4]) + (v & B[4]);

	return c;
	*/

	/* Algoritmo di Kernigan (1 iterazione per bit settato a 1): ci va bene
	 * perché solitamente sono pochi */

	unsigned int c; // c accumulates the total bits set in v
	for (c = 0; v; c++)
	{
		v &= v - 1; // clear the least significant bit set
	}

	return c;
}
#endif

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
