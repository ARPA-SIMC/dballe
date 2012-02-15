/*
 * dballe/core/arrayfile - File I/O from in-memory vector<Rawmsg>
 *
 * Copyright (C) 2012  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_CORE_ARRAYFILE_H
#define DBALLE_CORE_ARRAYFILE_H

/** @file
 * @ingroup core
 * In-memory versions of File, to be used for testing,
 */

#include <dballe/core/file.h>
#include <dballe/core/rawmsg.h>
#include <vector>

namespace dballe {

class ArrayFile : public File
{
protected:
    Encoding file_type;

public:
    std::vector<Rawmsg> msgs;
    unsigned current;

    ArrayFile(Encoding type);
    virtual ~ArrayFile();

    virtual Encoding type() const throw ();
    virtual bool read(Rawmsg& msg);
    virtual void write(const Rawmsg& msg);
};

} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
