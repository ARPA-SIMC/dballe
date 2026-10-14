/*
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

#ifndef FDBA_HANDLES_H
#define FDBA_HANDLES_H

#include <wreport/error.h>

namespace dballe {
struct DB;

namespace fortran {
struct API;

struct HBase
{
    bool used;

    HBase() : used(false) {}

    void start() { used = true; }
    void stop() { used = false; }
};

template <typename T, int MAX> struct Handler
{
    T* records;
    int next;
    const char* name;
    const char* def;
    size_t in_use;

    void init(const char* name, const char* def)
    {
        this->name = name;
        this->def  = def;
        records    = new T[MAX];
        next       = 0;
        in_use     = 0;
    }

    T& get(int id)
    {
        if (!records[id].used)
            wreport::error_handles::throwf(
                "Handle %d used after it has been closed, or when it has never "
                "been opened",
                id);
        return records[id];
    }

    int request()
    {
        for (int i = 0; i < MAX && (records[next].used); ++i)
            next = (next + 1) % MAX;
        if (records[next].used)
            wreport::error_handles::throwf(
                "No more handles for %s. The maximum limit is %d: to increase "
                "it, recompile DB-All.e setting %s to a higher value",
                name, MAX, def);
        /* Setup the new handle */
        records[next].start();
        ++in_use;
        return next;
    }

    void release(int h)
    {
        if (!in_use)
            wreport::error_handles::throwf(
                "Attempted to close handle %d when no handles are in use", h);
        if (!records[h].used)
            wreport::error_handles::throwf(
                "Attempted to close handle %d which was not open", h);
        records[h].stop();
        --in_use;
    }
};

/// Initialise error handlers
void error_init();

} // namespace fortran
} // namespace dballe

#endif
