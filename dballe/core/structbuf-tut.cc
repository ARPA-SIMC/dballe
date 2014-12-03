/*
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "test-utils-core.h"
#include "structbuf.h"

using namespace dballe;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct core_structbuf_shar
{
};
TESTGRP(core_structbuf);

// Test an in-memory structbuf
template<> template<>
void to::test<1>()
{
    Structbuf<int, 3> buf;
    wassert(actual(buf.size()) == 0);
    wassert(actual(buf.is_file_backed()).isfalse());

    buf.append(1);
    wassert(actual(buf.size()) == 1);
    wassert(actual(buf.is_file_backed()).isfalse());

    buf.append(2);
    wassert(actual(buf.size()) == 2);
    wassert(actual(buf.is_file_backed()).isfalse());

    buf.append(3);
    wassert(actual(buf.size()) == 3);
    wassert(actual(buf.is_file_backed()).isfalse());

    buf.ready_to_read();
    for (unsigned i = 0; i < 3; ++i)
        wassert(actual(buf[i]) == i + 1);
}

// Test an file-backed structbuf
template<> template<>
void to::test<2>()
{
    Structbuf<int, 3> buf;
    buf.append(1);
    buf.append(2);
    buf.append(3);
    wassert(actual(buf.size()) == 3);
    wassert(actual(buf.is_file_backed()).isfalse());

    buf.append(4);
    wassert(actual(buf.size()) == 4);
    wassert(actual(buf.is_file_backed()).istrue());

    buf.append(5);
    wassert(actual(buf.size()) == 5);
    buf.append(6);
    wassert(actual(buf.size()) == 6);
    buf.append(7);
    wassert(actual(buf.size()) == 7);
    buf.append(8);
    wassert(actual(buf.size()) == 8);

    buf.ready_to_read();
    for (unsigned i = 0; i < 8; ++i)
        wassert(actual(buf[i]) == i + 1);
}

}


