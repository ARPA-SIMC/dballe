/*
 * dump-core-info - Dump core information from the library in order to generate documentation
 *
 * Copyright (C) 2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "record.h"
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

using namespace wreport;
using namespace dballe;
using namespace std;

int do_keywords()
{
    for (int i = 0; i < DBA_KEY_COUNT; ++i)
    {
        wreport::Varinfo info = Record::keyword_info((dba_keyword)i);
        printf("%s,%s,%d,%d,%s\n",
            Record::keyword_name((dba_keyword)i),
            info->unit,
            info->len,
            info->scale,
            info->desc);
    }
    return 0;
}

int main(int argc, const char* argv[])
{
    string cmd = argv[1];
    if (cmd == "keywords")
        return do_keywords();
    else
    {
        fprintf(stderr, "Unknown command: %s\n", cmd.c_str());
        return 1;
    }
}

/* vim:set ts=4 sw=4: */
