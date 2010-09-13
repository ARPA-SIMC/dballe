/*
 * bufrex/bufrex_codec - dballe::File support for BUFR and CREX
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

#include <dballe/core/file.h>
#include <dballe/core/error.h>
#include <netinet/in.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

using namespace dballe;

namespace bufrex {

class BufrFile : public dballe::File
{
public:
	BufrFile(const std::string& name, FILE* fd, bool close_on_exit=true)
		: File(name, fd, close_on_exit) {}

	virtual Encoding type() const throw () { return BUFR; }
	bool read(Rawmsg& msg);
};

static bool seek_to_signature(FILE* fd, const char* sig, unsigned sig_len, const std::string& fname)
{
	const char* target = "BUFR";
	static const int target_size = 4;
	int got = 0;
	int c;

	errno = 0;
	while (got < sig_len && (c = getc(fd)) != EOF)
	{
		if (c == sig[got])
			got++;
		else
			got = 0;
	}

	if (errno != 0)
		error_system::throwf("when looking for start of %.4s data in %s", sig, fname.c_str());
	
	if (got != sig_len)
	{
		/* End of file: return accordingly */
		return false;
	}
	return true;
}

/*
 * Implementation of BUFR and CREX support for dba_file
 */
bool BufrFile::read(Rawmsg& msg)
{
	/* A BUFR message is easy to just read: it starts with "BUFR", then the
	 * message length encoded in 3 bytes */

	// Reset bufr_message data in case this message has been used before
	msg.clear();
	msg.file = this;

	/* Seek to start of BUFR data */
	if (!seek_past_signature("BUFR", 4))
		return false;
	msg.offset = ftell(fd) - 4;
	msg += "BUFR";

	// Read the remaining 4 bytes of section 0
	msg.resize(8);
	if (fread((char*)msg.data() + 4, 4, 1, fd) != 1)
		throw error_system("reading BUFR section 0 from " + name());

	{
		/* Read the message length */
		int bufrlen = ntohl(*(uint32_t*)(msg.data()+4)) >> 8;
		if (bufrlen < 12)
			error_consistency::throwf("the size declared by the BUFR message (%d) is less than the minimum of 12", bufrlen);

		/* Allocate enough space to fit the message */
		msg.resize(bufrlen);

		/* Read the rest of the BUFR message */
		if (fread((char*)msg.data() + 8, bufrlen - 8, 1, fd) != 1)
			throw error_system("reading BUFR message from " + name());
	}
	
	msg.encoding = BUFR;

	return true;
}

class CrexFile : public dballe::File
{
public:
	CrexFile(const std::string& name, FILE* fd, bool close_on_exit=true)
		: File(name, fd, close_on_exit) {}

	virtual Encoding type() const throw () { return CREX; }
	bool read(Rawmsg& msg);
	void write(const Rawmsg& msg);
};

bool CrexFile::read(Rawmsg& msg)
{
/*
 * The CREX message starts with "CREX" and ends with "++\r\r\n7777".  Ideally
 * any combination of \r and \n should be supported.
 */
	/* Reset crex_message data in case this message has been used before */
	msg.clear();
	msg.file = this;

	/* Seek to start of CREX data */
	if (!seek_past_signature("CREX++", 6))
		return false;
	msg.offset = ftell(fd) - 6;
	msg += "CREX++";

	/* Read until "\+\+(\r|\n)+7777" */
	{
		const char* target = "++\r\n7777";
		static const int target_size = 8;
		int got = 0;
		int c;

		errno = 0;
		while (got < 8 && (c = getc(fd)) != EOF)
		{
			if (target[got] == '\r' && (c == '\n' || c == '\r'))
				got++;
			else if (target[got] == '\n' && (c == '\n' || c == '\r'))
				;
			else if (target[got] == '\n' && c == '7')
				got += 2;
			else if (c == target[got])
				got++;
			else
				got = 0;

			msg += (char)c;
		}
		if (errno != 0)
			throw error_system("when looking for end of CREX data in " + name());

		if (got != target_size)
			throw error_parse(name().c_str(), ftell(fd), "CREX message is incomplete");
	}

	msg.encoding = CREX;

	return true;
}

void CrexFile::write(const Rawmsg& msg)
{
	File::write(msg);
	if (fputs("\r\r\n", fd) == EOF)
		throw error_system("writing CREX data on output");
}

namespace {
static File* bufr_file_create(const std::string& fname, Encoding, FILE* fd, bool close_on_exit)
{
	return new BufrFile(fname, fd, close_on_exit);
}

static File* crex_file_create(const std::string& fname, Encoding, FILE* fd, bool close_on_exit)
{
	return new CrexFile(fname, fd, close_on_exit);
}

/* Register / deregister the codec with dba_file */

static File::CreateFun* old_bufr_create_fun;
static File::CreateFun* old_crex_create_fun;

}

void codec_init(void)
{
	old_bufr_create_fun = File::register_type(BUFR, bufr_file_create);
	old_crex_create_fun = File::register_type(CREX, crex_file_create);
}

void codec_shutdown(void)
{
	File::register_type(BUFR, old_bufr_create_fun);
	File::register_type(CREX, old_crex_create_fun);
}

}

/* vim:set ts=4 sw=4: */
