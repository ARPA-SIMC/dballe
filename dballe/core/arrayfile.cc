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

#include "arrayfile.h"

using namespace std;

namespace dballe {

ArrayFile::ArrayFile(Encoding type)
    : File("array", NULL, false), file_type(type), current(0)
{
}

ArrayFile::~ArrayFile()
{
}

Encoding ArrayFile::type() const throw () { return file_type; }

void ArrayFile::write(const Rawmsg& msg)
{
    msgs.push_back(msg);
    current = msgs.size();
}

bool ArrayFile::read(Rawmsg& msg)
{
    if (current >= msgs.size())
        return false;
    msg = msgs[current];
    ++current;
    return true;
}

}

/* vim:set ts=4 sw=4: */
