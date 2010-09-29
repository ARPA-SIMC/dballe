/*
 * dballe/file - File I/O
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

#include "file.h"
#include "aoffile.h"
#include "rawmsg.h"
#include <wreport/bulletin.h>

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

using namespace wreport;
using namespace std;

namespace dballe {

File::File(const std::string& name, FILE* fd, bool close_on_exit)
	: m_name(name), fd(fd), close_on_exit(close_on_exit), idx(0)
{
}

namespace {

struct fd_tracker
{
	FILE* fd;
	bool close_on_exit;

	fd_tracker() : fd(NULL), close_on_exit(true) {}
	~fd_tracker() { if (fd && close_on_exit) fclose(fd); }
	void track(FILE* fd, bool close_on_exit=true)
	{
		this->fd = fd;
		this->close_on_exit = close_on_exit; 
	}
	FILE* release()
	{
		FILE* res = fd;
		fd = NULL;
		return res;
	}
};

class BufrFile : public dballe::File
{
public:
	BufrFile(const std::string& name, FILE* fd, bool close_on_exit=true)
		: File(name, fd, close_on_exit) {}

	virtual Encoding type() const throw () { return BUFR; }

	bool read(Rawmsg& msg)
	{
		msg.file = m_name;
		msg.encoding = BUFR;
		return BufrBulletin::read(fd, msg, m_name.c_str(), &msg.offset);
	}

	void write(const Rawmsg& msg)
	{
		BufrBulletin::write(msg, fd, m_name.c_str());
	}

};

class CrexFile : public dballe::File
{
public:
	CrexFile(const std::string& name, FILE* fd, bool close_on_exit=true)
		: File(name, fd, close_on_exit) {}

	virtual Encoding type() const throw () { return CREX; }

	bool read(Rawmsg& msg)
	{
		msg.file = m_name;
		msg.encoding = CREX;
		return CrexBulletin::read(fd, msg, m_name.c_str(), &msg.offset);
	}

	void write(const Rawmsg& msg)
	{
		CrexBulletin::write(msg, fd, m_name.c_str());
	}
};

} // anonymous namespace

auto_ptr<File> File::create(Encoding type, const std::string& name, const char* mode)
{
	fd_tracker fdt;

	/* Open the file */
	if (name == "(stdin)")
		fdt.track(stdin, false);
	else if (name == "(stdout)")
		fdt.track(stdout, false);
	else if (name == "(stderr)")
		fdt.track(stderr, false);
	else
	{
		fdt.track(fopen(name.c_str(), mode), true);
		if (fdt.fd == NULL)
			error_system::throwf("opening %s with mode '%s'", name.c_str(), mode);
	}

	/* Attempt auto-detect if needed */
	if (type == -1)
	{
		int c = getc(fdt.fd);
		if (c == EOF)
		{
			// In case of EOF, pick any type that will handle EOF gracefully.
			c = 'B';
		} else if (ungetc(c, fdt.fd) == EOF)
			error_system::throwf("putting the first byte of %s back into the input stream", name.c_str());
		
		switch (c)
		{
			case 'B': type = BUFR; break;
			case 'C': type = CREX; break;
			case 0: type = AOF; break;
			case 0x38: type = AOF; break;
			default:
				throw error_notfound("could not detect the encoding of " + name);
		}
	}

	switch (type)
	{
		case BUFR: return auto_ptr<File>(new BufrFile(name, fdt.release(), fdt.close_on_exit));
		case CREX: return auto_ptr<File>(new CrexFile(name, fdt.release(), fdt.close_on_exit));
		case AOF: return auto_ptr<File>(new AofFile(name, fdt.release(), fdt.close_on_exit));
	}
	error_consistency::throwf("requested unknown %d file type", (int)type);
}

File::~File()
{
	if (fd && close_on_exit)
		fclose(fd);
}

void File::write(const Rawmsg& msg) 
{
	if (fwrite(msg.data(), msg.size(), 1, fd) != 1)
		error_system::throwf("writing message data (%zd bytes) on output", msg.size());
}

#if 0
dba_err dba_file_read(dba_file file, dba_rawmsg msg, int* found)
{
	if (file->fun_read == NULL)
		return dba_error_unimplemented("reading %s files is not implemented", dba_encoding_name(file->type));
	else
	{
		DBA_RUN_OR_RETURN(file->fun_read(file, msg, found));
		++file->idx;
		return dba_error_ok();
	}
}

dba_err dba_file_write(dba_file file, dba_rawmsg msg)
{
	if (file->fun_write == NULL)
		return dba_error_unimplemented("writing %s files is not implemented", dba_encoding_name(file->type));
	else
	{
		DBA_RUN_OR_RETURN(file->fun_write(file, msg));
		++file->idx;
		return dba_error_ok();
	}
}

#endif

}

/* vim:set ts=4 sw=4: */
