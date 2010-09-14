/*
 * core/fusb - Read and write Fortran Unformatted Sequential Binary records
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

#include "fusb.h"
#include <config.h>

#include <wreport/error.h>
#include <dballe/core/file.h>

#include <stdint.h>
#include <cstdlib>
#include <cstring>
#include <byteswap.h>

using namespace wreport;
using namespace std;

namespace dballe {

/*
 * AOF I/O utilities
 */

bool fusb_read(File& file, std::string& res)
{
	uint32_t len_word, len_word1;
	FILE* in = file.stream();
	int swapwords = 0;

	/* Read the first Fortran length of record word */
	if (fread(&len_word, 4, 1, in) == 0)
	{
		if (feof(in))
		{
			return false;
		}
		error_system::throwf("reading a record-length first word in %s", file.name().c_str());
	}

	if ((len_word & 0xFF000000) != 0)
	{
		swapwords = 1;
		len_word = bswap_32(len_word);
	}

	if (len_word % 4 != 0)
		error_parse::throwf(file.name().c_str(), ftell(in), "length of record (%d) is not a multiple of 4", len_word);

	// Allocate space
	res.reserve(len_word);
	
	// Read the record
	uint32_t* buf = (uint32_t*)res.data();
	if (fread(buf, len_word, 1, in) == 0)
	{
		error_system::throwf("reading a %d-bytes record from %s", len_word, file.name().c_str());
	}

	// Read the last Fortran length of record word
	if (fread(&len_word1, 4, 1, in) == 0)
		error_system::throwf("reading a record-length last word in %s", file.name().c_str());

	// Swap words if needed
	if (swapwords)
	{
		int i;
		for (i = 0; i < len_word / 4; i++)
			buf[i] = bswap_32(buf[i]);
		len_word1 = bswap_32(len_word1);
	}

	if (len_word != len_word1)
		throw error_parse(file.name().c_str(), ftell(in), "initial length of record is different than the final length of record");
}

static enum { INVALID = 0, END_ARCH = 1, END_LE = 2, END_BE = 3 } writer_endianness = INVALID;

void init_writer_endiannes_if_needed()
{
	if (writer_endianness == INVALID)
	{
		char* env_swap = getenv("DBA_AOF_ENDIANNESS");
		if (env_swap == NULL)
			writer_endianness = END_ARCH;
		else if (strcmp(env_swap, "ARCH") == 0)
			writer_endianness = END_ARCH;
		else if (strcmp(env_swap, "LE") == 0)
			writer_endianness = END_LE;
		else if (strcmp(env_swap, "BE") == 0)
			writer_endianness = END_BE;
		else
			writer_endianness = END_ARCH;
	}
}

static void output_word(File& file, uint32_t word)
{
	uint32_t oword;
	switch (writer_endianness)
	{
		case END_ARCH: oword = word; break;
#if __BYTE_ORDER == __BIG_ENDIAN
		case END_LE: oword = bswap_32(word); break;
		case END_BE: oword = word; break;
#else
#if __BYTE_ORDER == __LITTLE_ENDIAN
		case END_LE: oword = word; break;
		case END_BE: oword = bswap_32(word); break;
#else
		case END_LE: oword = bswap_32(htonl(word)); break;
		case END_BE: oword = htonl(word); break;
#endif
#endif
	}
	if (fwrite(&oword, sizeof(uint32_t), 1, file.stream()) != 1)
		error_system::throwf("writing 4 bytes on %s", file.name().c_str());
}

void fusb_write(File& file, const std::string& res)
{
	const uint32_t* rec = (const uint32_t*)res.data();

	init_writer_endiannes_if_needed();

	/* Write the leading length of record word */
	output_word(file, res.size());
	for (int i = 0; i < res.size() / sizeof(uint32_t); ++i)
		output_word(file, rec[i]);
	/* Write the trailing length of record word */
	output_word(file, res.size());
}

} // namespace dballe

/* vim:set ts=4 sw=4: */
