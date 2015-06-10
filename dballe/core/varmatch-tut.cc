/*
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "var.h"
#include "varmatch.h"

using namespace dballe;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct core_varmatch_shar
{
};

TESTGRP(core_varmatch);

template<> template<> void to::test<1>()
{
    Var var(varinfo(WR_VAR(0, 1, 1)), 42);

    wassert(actual((*Varmatch::parse("B01001<43"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001<42"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01001<41"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001<=43"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001<=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001<=41"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001>41"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001>42"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01001>43"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001>=41"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001>=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001>=43"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001==42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001==43"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01001=43"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01001<>43"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01001<>42"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("41<=B01001<=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("42<=B01001<=42"))(var)).istrue());
    wassert(actual((*Varmatch::parse("42<=B01001<=43"))(var)).istrue());
    wassert(actual((*Varmatch::parse("40<=B01001<=41"))(var)).isfalse());
}

template<> template<> void to::test<2>()
{
    Var var(varinfo(WR_VAR(0, 12, 101)), 273.15);

    wassert(actual((*Varmatch::parse("B12101<274"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101<273.15"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B12101<273"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101<=274"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101<=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101<=273"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101>273"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101>273.15"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B12101>274"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101>=273"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101>=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101>=274"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101==273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101==274"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B12101=274"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B12101<>274"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B12101<>273.15"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("273<=B12101<=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("273.15<=B12101<=273.15"))(var)).istrue());
    wassert(actual((*Varmatch::parse("273.15<=B12101<=274"))(var)).istrue());
    wassert(actual((*Varmatch::parse("272<=B12101<=273"))(var)).isfalse());
}

template<> template<> void to::test<3>()
{
    Var var(varinfo(WR_VAR(0, 1, 11)), "enrico");

    wassert(actual((*Varmatch::parse("B01011<paolo"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011<enrico"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01011<emanuele"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011<=paolo"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011<=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011<=emanuele"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011>emanuele"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011>enrico"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01011>paolo"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011>=emanuele"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011>=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011>=paolo"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011==enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011==paolo"))(var)).isfalse());
    wassert(actual((*Varmatch::parse("B01011=paolo"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("B01011<>paolo"))(var)).istrue());
    wassert(actual((*Varmatch::parse("B01011<>enrico"))(var)).isfalse());

    wassert(actual((*Varmatch::parse("emanuele<=B01011<=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("enrico<=B01011<=enrico"))(var)).istrue());
    wassert(actual((*Varmatch::parse("enrico<=B01011<=paolo"))(var)).istrue());
    wassert(actual((*Varmatch::parse("daniele<=B01011<=emanuele"))(var)).isfalse());
}

}
