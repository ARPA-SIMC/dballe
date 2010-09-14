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

#ifndef DBA_CORE_FILE_H
#define DBA_CORE_FILE_H

/** @file
 * @ingroup core
 * File I/O for files containing meterorological messages.
 *
 * This module provides a unified interface to read and write messages to files
 * in various formats.
 *
 * Format-specific implementation is not provided by this module, but other
 * libraries can implement format specific read and write functions and
 * register them with ::dba_file.
 */

#include <dballe/core/rawmsg.h>
#include <stdio.h>

namespace dballe {

class File
{
protected:
	/** Name of the file */
	std::string m_name;
	/** FILE structure used to read or write to the file */
	FILE* fd;
	/** Set to true if fd should be closed when dba_file_delete() is called */
	bool close_on_exit;
	/** Index of the last message read from the file or written to the file */
	int idx;

public:
	File(const std::string& name, FILE* fd, bool close_on_exit=true);
	virtual ~File();

	/**
	 * Get the type of the dba_file
	 *
	 * @return 
	 *   The file encoding.
	 */
	const std::string& name() const throw () { return m_name; }

	/**
	 * Get the name of the dba_file
	 *
	 * @return 
	 *   The file name.
	 */
	virtual Encoding type() const throw () = 0;

	/**
	 * Read a message from the file.
	 *
	 * @param msg
	 *   The dba_rawmsg that will hold the data.
	 * @return
	 *   true if a message has been found in the file, else false.
	 */
	virtual bool read(Rawmsg& msg) = 0;

	/**
	 * Write the encoded message data to the file
	 *
	 * @param msg
	 *   The ::dba_rawmsg with the encoded data to write
	 */
	virtual void write(const Rawmsg& msg);

	/**
	 * Create a dba_file structure.
	 *
	 * @param type
	 *   The type of data contained in the file.  If -1 is passed, then
	 *   create will attempt to autodetect the file type from its first
	 *   byte.
	 * @param name
	 *   The name of the file to access.  "(stdin)", "(stdout)" and "(stderr)" are
	 *   special file names, that will use the corresponding stream instead of open
	 *   a file.
	 * @param mode
	 *   The opening mode of the file, as used by fopen.
	 * @returns
	 *   The newly allocated File, that needs to be deleted by the caller.
	 */
	static File* create(Encoding type, const std::string& name, const char* mode);
};

}

/* vim:set ts=4 sw=4: */
#endif
