/*
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <dballe/core/var.h>
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

    void import_var(const Var& var);
    void import_var_undef(const Var& var);

public:
    GenericImporter(const msg::Importer::Options& opts) : Importer(opts) {}
    virtual ~GenericImporter() {}

    virtual void init()
    {
        Importer::init();
        lev = Level();
        tr = Trange();
    }

    virtual void run()
    {
        for (size_t pos = 0; pos < subset->size(); ++pos)
        {
                const Var& var = (*subset)[pos];
                if (WR_VAR_F(var.code()) != 0) continue;
                if (var.value() == NULL)
                {
                    /* Also skip attributes if there are some following */
                    for ( ; pos + 1 < subset->size() &&
                            WR_VAR_X((*subset)[pos + 1].code()) == 33; ++pos)
                        ;
                    import_var_undef(var);
                    continue;
                }
                if (pos + 1 < subset->size() &&
                        WR_VAR_X((*subset)[pos + 1].code()) == 33)
                {
                    Var copy(var);
                    for ( ; pos + 1 < subset->size() &&
                            WR_VAR_X((*subset)[pos + 1].code()) == 33; ++pos)
                        copy.seta((*subset)[pos + 1]);
                    import_var(copy);
                } else
                    import_var(var);
        }
    }

    MsgType scanType(const Bulletin&) const
    {
        return MSG_GENERIC;
    }
};

std::unique_ptr<Importer> Importer::createGeneric(const msg::Importer::Options& opts)
{
    return unique_ptr<Importer>(new GenericImporter(opts));
}

void GenericImporter::import_var_undef(const Var& var)
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

void GenericImporter::import_var(const Var& var)
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
            // Set the rep memo if we found it
            msg->type = Msg::type_from_repmemo(var.value());
            msg->set_rep_memo(var.value(), -1);
            break;
        // Legacy variable conversions
        case WR_VAR(0, 8, 1): {
            unique_ptr<Var> nvar(newvar(WR_VAR(0, 8, 42), convert_BUFR08001_to_BUFR08042(var.enqi())));
            nvar->copy_attrs(var);
            msg->set(move(nvar), lev, tr);
            break;
        }
        default:
            // Adjust station info level for pre-dballe-5.0 generics
            if (lev.ltype1 == 257)
                msg->set(var, map_code_to_dballe(var.code()), Level::ana(), Trange());
            else
                msg->set(var, map_code_to_dballe(var.code()), lev, tr);
            break;
    }
}

} // namespace wbimporter
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
