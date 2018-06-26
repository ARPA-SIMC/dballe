/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "base.h"
#include "dballe/core/var.h"
#include "dballe/msg/msg.h"
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <wreport/conv.h>
#include <cmath>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

class GenericImporter : public Importer
{
protected:
    Level lev;
    Trange tr;

    /// Import an undefined value
    void import_defined(const Var& var);
    /// Import a defined value, which can be a variable or context
    void import_undef(const Var& var);
    /// Import a defined value, with the context being properly set
    void import_var(const Var& var);

public:
    GenericImporter(const msg::ImporterOptions& opts) : Importer(opts) {}
    virtual ~GenericImporter() {}

    void init() override
    {
        Importer::init();
        lev = Level();
        tr = Trange();
    }

    void run() override
    {
        for (size_t pos = 0; pos < subset->size(); ++pos)
        {
                const Var& var = (*subset)[pos];
                // Skip non-variable entries
                if (WR_VAR_F(var.code()) != 0) continue;
                // Special processing for undefined variables
                if (!var.isset())
                {
                    // Also skip attributes of undefined variables if there are
                    // some following
                    for ( ; pos + 1 < subset->size() &&
                            WR_VAR_X((*subset)[pos + 1].code()) == 33; ++pos)
                        ;
                    import_undef(var);
                    continue;
                }
                // A variable with a value: add attributes to it if any are
                // found
                if (pos + 1 < subset->size() &&
                        WR_VAR_X((*subset)[pos + 1].code()) == 33)
                {
                    Var copy(var);
                    for ( ; pos + 1 < subset->size() &&
                            WR_VAR_X((*subset)[pos + 1].code()) == 33; ++pos)
                        copy.seta((*subset)[pos + 1]);
                    import_defined(copy);
                } else
                    import_defined(var);
        }
    }

    MsgType scanType(const Bulletin&) const override
    {
        return MSG_GENERIC;
    }
};

std::unique_ptr<Importer> Importer::createGeneric(const msg::ImporterOptions& opts)
{
    return unique_ptr<Importer>(new GenericImporter(opts));
}

void GenericImporter::import_undef(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0, 4, 192): tr.pind = MISSING_INT; break;
        case WR_VAR(0, 4, 193): tr.p1 = MISSING_INT; break;
        case WR_VAR(0, 4, 194): tr.p2 = MISSING_INT; break;
        case WR_VAR(0, 7, 192): lev.ltype1 = MISSING_INT; break;
        case WR_VAR(0, 7, 193): lev.l1 = MISSING_INT; break;
        case WR_VAR(0, 7, 194): lev.l2 = MISSING_INT; break;
        case WR_VAR(0, 7, 195): lev.ltype2 = MISSING_INT; break;
    }
}

void GenericImporter::import_defined(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0, 4, 192): tr.pind = var.enqi(); break;
        case WR_VAR(0, 4, 193): tr.p1 = var.enqi(); break;
        case WR_VAR(0, 4, 194): tr.p2 = var.enqi(); break;
        case WR_VAR(0, 7, 192): lev.ltype1 = var.enqi(); break;
        case WR_VAR(0, 7, 193): lev.l1 = var.enqi(); break;
        case WR_VAR(0, 7, 194): lev.l2 = var.enqi(); break;
        case WR_VAR(0, 7, 195): lev.ltype2 = var.enqi(); break;
        case WR_VAR(0, 1, 194):
            if (var.isset())
            {
                // Set the rep memo if we found it
                const char* repmemo = var.enqc();
                msg->type = Msg::type_from_repmemo(repmemo);
                msg->set_rep_memo(repmemo, -1);
            }
            break;
        default:
            import_var(var);
            break;
    }
}

void GenericImporter::import_var(const Var& var)
{
    // Adjust station info level for pre-dballe-5.0 generics
    if (lev.ltype1 == 257)
    {
        lev = Level();
        tr = Trange();
    }

    switch (var.code())
    {
        // Legacy variable conversions
        case WR_VAR(0, 8, 1): {
            unique_ptr<Var> nvar(newvar(WR_VAR(0, 8, 42), (int)convert_BUFR08001_to_BUFR08042(var.enqi())));
            nvar->setattrs(var);
            msg->set(move(nvar), lev, tr);
            break;
        }
        // Datetime entries that may have attributes to store
        case WR_VAR(0,  4,  1): ye = var.enqi(); if (var.next_attr()) msg->set_year_var(var); break;
        case WR_VAR(0,  4,  2): mo = var.enqi(); if (var.next_attr()) msg->set_month_var(var); break;
        case WR_VAR(0,  4,  3): da = var.enqi(); if (var.next_attr()) msg->set_day_var(var); break;
        case WR_VAR(0,  4,  4): ho = var.enqi(); if (var.next_attr()) msg->set_hour_var(var); break;
        case WR_VAR(0,  4,  5): mi = var.enqi(); if (var.next_attr()) msg->set_minute_var(var); break;
        case WR_VAR(0,  4,  6): se = var.enqi(); if (var.next_attr()) msg->set_second_var(var); break;
        // Anything else
        default:
            msg->set(var, map_code_to_dballe(var.code()), lev, tr);
            break;
    }
}

}
}
}
