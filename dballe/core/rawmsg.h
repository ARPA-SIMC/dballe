/*
 * wreport/rawmsg - annotated raw buffer
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

#ifndef WREPORT_RAWMSG_H
#define WREPORT_RAWMSG_H

#include <string>

namespace wreport {

/** @file
 * @ingroup io
 * In-memory storage of an encoded weather report.
 */

/**
 * Annotated string buffer for encoded messages.
 */
struct Rawmsg : public std::string
{
	/**
	 * Pathname of the file from where the Rawmsg has been read.  It can be
	 * empty when not applicable, such as when the message is created from
	 * scratch and not yet written
	 */
	std::string file;
	/** Start offset of this message inside the file where it is found */
	int offset;
	/** Index of the message within the source */
	int index;

	Rawmsg() : offset(0), index(0) {}

	// Clear all the contents of this dballe::Rawmsg
	void clear() throw ()
	{
		std::string::clear();
		file.clear();
		offset = 0;
		index = 0;
	}
};

}

/* vim:set ts=4 sw=4: */
#endif
