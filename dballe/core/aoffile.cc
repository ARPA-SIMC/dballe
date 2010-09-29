/*
 * dballe/aoffile - AOF File I/O
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

#include "aoffile.h"
#include "rawmsg.h"
#include <wreport/error.h>
#include <cstdlib>
#include <cstring>
#include <byteswap.h>
#include <errno.h>

using namespace wreport;
using namespace std;

namespace dballe {

AofFile::AofFile(const std::string& name, FILE* fd, bool close_on_exit)
	: File(name, fd, close_on_exit) {}

AofFile::~AofFile() {}

bool AofFile::read(Rawmsg& msg)
{
	msg.clear();
	msg.file = m_name;
	msg.encoding = AOF;

	/* If we are at the beginning of the file, then skip the file header */
	if (ftell(fd) == 0)
		read_header();

	msg.offset = ftell(fd);

	/* Read the Observation Header */
	read_record(msg);
	if (msg.empty())
		return false;

	const uint32_t* rec = (const uint32_t*)msg.data();
	if (rec[1] != 4)
		error_parse::throwf(name().c_str(), ftell(fd),
				"value '01 length of preliminary record' should be 4, either big or little endian (it is %d (%08x) instead)", rec[1], rec[1]);

	return true;
}

void AofFile::write(const Rawmsg& msg)
{
	long pos = ftell(fd);

	if (pos == -1 && errno != ESPIPE)
		error_system::throwf("reading current position in output file %s", name().c_str());

	/* If it's a non-seekable file, we use idx to see if we're at the beginning */
	if (pos == -1 && errno == ESPIPE && idx == 0)
		pos = 0;
		
	/* If we are at the beginning of the file, write a dummy header */
	if (pos == 0)
		write_dummy_header();
	
	write_record(msg);
}

bool AofFile::read_record(std::string& res)
{
	uint32_t len_word, len_word1;
	int swapwords = 0;

	/* Read the first Fortran length of record word */
	if (fread(&len_word, 4, 1, fd) == 0)
	{
		if (feof(fd))
		{
			return false;
		}
		error_system::throwf("reading a record-length first word in %s", name().c_str());
	}

	if ((len_word & 0xFF000000) != 0)
	{
		swapwords = 1;
		len_word = bswap_32(len_word);
	}

	if (len_word % 4 != 0)
		error_parse::throwf(name().c_str(), ftell(fd), "length of record (%d) is not a multiple of 4", len_word);

	// Allocate space
	res.resize(len_word);
	
	// Read the record
	uint32_t* buf = (uint32_t*)res.data();
	if (fread(buf, len_word, 1, fd) == 0)
	{
		error_system::throwf("reading a %d-bytes record from %s", len_word, name().c_str());
	}

	// Read the last Fortran length of record word
	if (fread(&len_word1, 4, 1, fd) == 0)
		error_system::throwf("reading a record-length last word in %s", name().c_str());

	// Swap words if needed
	if (swapwords)
	{
		for (unsigned i = 0; i < len_word / 4; i++)
			buf[i] = bswap_32(buf[i]);
		len_word1 = bswap_32(len_word1);
	}

	if (len_word != len_word1)
		throw error_parse(name().c_str(), ftell(fd), "initial length of record is different than the final length of record");

	return true;
}

static enum { INVALID = 0, END_ARCH = 1, END_LE = 2, END_BE = 3 } writer_endianness = INVALID;

static void init_writer_endiannes_if_needed()
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

void AofFile::write_word(uint32_t word)
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
		case INVALID: throw error_consistency("trying to write a word without knowing its endianness");
	}
	if (fwrite(&oword, sizeof(uint32_t), 1, fd) != 1)
		error_system::throwf("writing 4 bytes on %s", name().c_str());
}

void AofFile::write_record(const std::string& res)
{
	const uint32_t* rec = (const uint32_t*)res.data();
	write_record(rec, res.size() / sizeof(uint32_t));
}

void AofFile::write_record(const uint32_t* words, size_t wordcount)
{
	init_writer_endiannes_if_needed();

	/* Write the leading length of record word */
	write_word(wordcount * sizeof(uint32_t));
	for (unsigned i = 0; i < wordcount; ++i)
		write_word(words[i]);
	/* Write the trailing length of record word */
	write_word(wordcount * sizeof(uint32_t));
}


// Read the file header, performing some consistency checks then discarding
// the data
void AofFile::read_header()
{
	string buf;

	/* Read the First Data Record */
	if (!read_record(buf))
		throw error_parse(name().c_str(), ftell(fd),
				"AOF file is empty or does not contain AOF data");

	if (buf.size() / sizeof(uint32_t) != 14)
		error_parse::throwf(name().c_str(), ftell(fd),
				"FDR contains %zd octets instead of 14", buf.size());

	uint32_t* fdr = (uint32_t*)buf.data();

	/* Consistency checks */
	if (fdr[0] != 14)
		error_parse::throwf(name().c_str(), ftell(fd),
				"first word of FDR is %d instead of 14", fdr[0]);

	/* Read Data Descriptor Record */
	if (!read_record(buf))
		error_parse::throwf(name().c_str(), ftell(fd),
				"AOF file is truncated after First Data Record");

	if (buf.size() / sizeof(uint32_t) != 17)
		error_parse::throwf(name().c_str(), ftell(fd),
				"DDR contains %zd octets instead of 17", buf.size());

	// uint32_t* ddr = (uint32_t*)buf.data();

#if 0
	reader->start.tm_hour = ddr[10] % 100;
	reader->start.tm_mday = ((ddr[10] / 100) % 100);
	reader->start.tm_mon = ((ddr[10] / 10000) % 100) - 1;
	reader->start.tm_year = (ddr[10] / 1000000) - 1900;

	reader->end.tm_hour = ddr[12] % 100;
	reader->end.tm_mday = ((ddr[12] / 100) % 100);
	reader->end.tm_mon = ((ddr[12] / 10000) % 100) - 1;
	reader->end.tm_year = (ddr[12] / 1000000) - 1900;
#endif
}

void AofFile::write_dummy_header()
{
	uint32_t fdr[14];
	uint32_t ddr[17];
	/* Use 'now' for start and end times */
	time_t tnow = time(NULL);
	struct tm* now = gmtime(&tnow);
	struct tm* start = now;
	struct tm* end = now;

	/* Write FDR */
	fdr[ 0] =   14;
	fdr[ 1] =   13;
	fdr[ 2] =    0;
	fdr[ 3] = 2048;
	fdr[ 4] =    2;
	fdr[ 5] = (now->tm_year + 1900) * 10000 + (now->tm_mon + 1) * 100 + now->tm_mday;
	fdr[ 6] = now->tm_hour * 100 + now->tm_min;
	fdr[ 7] = ((uint32_t)1 << 31) - 1;
	fdr[ 8] =    1;
	fdr[ 9] =   60;
	fdr[10] = ((uint32_t)1 << 31) - 1;
	fdr[11] = ((uint32_t)1 << 31) - 1;
	fdr[12] =    1;
	fdr[13] = ((uint32_t)1 << 31) - 1;

	write_record(fdr, 14);

	/* Write DDR */
	ddr[ 0] =  17;
	ddr[ 1] =  16;
	ddr[ 2] =   0;
	ddr[ 3] = 820;
	ddr[ 4] =   2;
	ddr[ 5] = (now->tm_year + 1900) * 10000 + (now->tm_mon + 1) * 100 + now->tm_mday;
	ddr[ 6] = now->tm_hour * 100 + now->tm_min;
	ddr[ 7] = ((uint32_t)1 << 31) - 1;
	ddr[ 8] = ((uint32_t)1 << 31) - 1;
	ddr[ 9] =   60;
	ddr[10] = (start->tm_year + 1900) * 1000000 + (start->tm_mon + 1) * 10000 +
				(start->tm_mday * 100) + start->tm_hour;
	ddr[11] =   1;
	ddr[12] = (end->tm_year + 1900) * 1000000 + (end->tm_mon + 1) * 10000 +
				(end->tm_mday * 100) + end->tm_hour;
	ddr[13] = ((uint32_t)1 << 31) - 1;
	ddr[14] = ((uint32_t)1 << 31) - 1;
	ddr[15] =    2;
	ddr[16] = ((uint32_t)1 << 31) - 1;

	write_record(ddr, 17);
}

void AofFile::fix_header()
{
	string buf;
	uint32_t start = 0xffffffff;
	uint32_t end = 0;
	size_t pos = 0;

	/* Read the FDR */
	read_record(buf);
	pos += buf.size();

	/* Read the DDR */
	read_record(buf);
	pos += buf.size();

	/* Iterate through all the records in the file */
	while (read_record(buf))
	{
		if (buf.size() < 11)
			error_parse::throwf(name().c_str(), pos, "observation record is too short (%zd bytes)", buf.size());

		const uint32_t* rec = (const uint32_t*)buf.data();

		/* Compute the extremes of start and end */
		uint32_t cur = rec[10-1] * 100 + rec[11-1]/100;
		if (cur < start)
			start = cur;
		if (cur > end)
			end = cur;

		pos += buf.size();
	}

	/* Update the header with the new extremes */

	/* Check if we need to swap bytes to match the header encoding */
	if (fseek(fd, 0, SEEK_SET) == -1)
		error_system::throwf("trying to seek to start of file %s", name().c_str());

	uint32_t endianness_test;
	if (fread(&endianness_test, 4, 1, fd) == 0)
		error_system::throwf("reading the first word of file %s", name().c_str());

	if ((endianness_test & 0xFF000000) != 0)
	{
		start = bswap_32(start);
		end = bswap_32(end);
	}

	/* Write start of observation period */
	if (fseek(fd, 14 + 10, SEEK_SET) == -1)
		error_system::throwf("trying to seek in file %s", name().c_str());
	if (fwrite(&start, sizeof(uint32_t), 1, fd) != 1)
		error_system::throwf("rewriting 4 bytes on %s", name().c_str());

	/* Write end of observation period */
	if (fseek(fd, 14 + 12, SEEK_SET) == -1)
		error_system::throwf("trying to seek in file %s", name().c_str());
	if (fwrite(&end, sizeof(uint32_t), 1, fd) != 1)
		error_system::throwf("rewriting 4 bytes on %s", name().c_str());
}

} // namespace dballe

/* vim:set ts=4 sw=4: */
