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

#include "file.h"
#include "error.h"

#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

namespace dballe {

namespace {

struct FunStore
{
	File::CreateFun* funs[ENCODING_COUNT];
	FunStore()
	{
		for (int i = 0; i < ENCODING_COUNT; ++i)
			funs[i] = 0;
	}
	static FunStore* get();
};

static FunStore* funStore = NULL;

FunStore* FunStore::get()
{
	if (funStore == NULL)
		funStore = new FunStore;
	return funStore;
}

}

File::CreateFun* File::register_type(Encoding type, CreateFun* fun)
{
	FunStore* fs = FunStore::get();
	CreateFun* res = fs->funs[type];
	fs->funs[type] = fun;
	return res;
}


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
}

File* File::create(Encoding type, const std::string& name, const char* mode)
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

	/* Call the appropriate constructor */
	FunStore* fs = FunStore::get();
	CreateFun* fun = fs->funs[type];
	if (fun == NULL)
		error_unimplemented::throwf("%s support is not available", encoding_name(type));
	return fun(name, type, fdt.release(), fdt.close_on_exit);
}

File::~File()
{
	if (fd && close_on_exit)
		fclose(fd);
}

void File::write(const Rawmsg& msg) 
{
	if (fwrite(msg.data(), msg.size(), 1, fd) != 1)
		error_system::throwf("writing message data (%d bytes) on output", msg.size());
}

bool File::seek_past_signature(const char* sig, unsigned sig_len)
{
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
		error_system::throwf("when looking for start of %s data in %s", encoding_name(type()), name().c_str());
	
	if (got != sig_len)
	{
		/* End of file: return accordingly */
		return false;
	}
	return true;
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
