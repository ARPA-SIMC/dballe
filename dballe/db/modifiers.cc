/*
 * dballe/db/modifiers - Parse query=* modifiers
 *
 * Copyright (C) 2007--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "modifiers.h"
#include <dballe/core/record.h>
#include <wreport/error.h>
#include <cstring>
#include <cstdlib>

using namespace wreport;

namespace dballe {
namespace db {

unsigned parse_modifiers(const Record& rec)
{
    /* Decode query modifiers */
    const char* val = rec.key_peek_value(DBA_KEY_QUERY);
    if (!val) return 0;

    unsigned modifiers = 0;

    const char* s = val;
    while (*s)
    {
        size_t len = strcspn(s, ",");
        int got = 1;
        switch (len)
        {
            case 0:
                /* If it's an empty token, skip it */
                break;
            case 4:
                /* "best": if more values exist in a point, get only the
                   best one */
                if (strncmp(s, "best", 4) == 0)
                    modifiers |= DBA_DB_MODIFIER_BEST;
                else
                    got = 0;
                break;
            case 6:
                /* "bigana": optimize with date first */
                if (strncmp(s, "bigana", 6) == 0)
                    modifiers |= DBA_DB_MODIFIER_BIGANA;
                else if (strncmp(s, "nosort", 6) == 0)
                    modifiers |= DBA_DB_MODIFIER_UNSORTED;
                else if (strncmp(s, "stream", 6) == 0)
                    modifiers |= DBA_DB_MODIFIER_STREAM;
                else
                    got = 0;
                break;
            case 7:
                if (strncmp(s, "details", 7) == 0)
                    modifiers |= DBA_DB_MODIFIER_SUMMARY_DETAILS;
                else
                    got = 0;
                break;
            default:
                got = 0;
                break;
        }

        /* Check that we parsed it correctly */
        if (!got)
            error_consistency::throwf("Query modifier \"%.*s\" is not recognized", (int)len, s);

        /* Move to the next token */
        s += len;
        if (*s == ',')
            ++s;
    }

    return modifiers;
}

}
}
