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

#include "msg/test-utils-msg.h"
#include "msg/codec.h"

using namespace dballe;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct msg_codec_shar
{
};
TESTGRP(msg_codec);

template<> template<>
void to::test<1>()
{
    using namespace dballe::msg;

    Importer::Options simplified;
    Importer::Options accurate;
    accurate.simplified = false;

    wassert(actual(Importer::Options::from_string("") == simplified).istrue());
    wassert(actual(Importer::Options::from_string("simplified") == simplified).istrue());
    wassert(actual(Importer::Options::from_string("accurate") == accurate).istrue());
}

}
