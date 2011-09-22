/*
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <cmath>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

class FlightImporter : public WMOImporter
{
protected:
    Level lev;
    std::vector<Var*> deferred;

    void import_var(const Var& var);

public:
    FlightImporter(const msg::Importer::Options& opts) : WMOImporter(opts) {}
    virtual ~FlightImporter()
    {
        // If there are leftover variables in deferred, deallocate them
        for (std::vector<Var*>::iterator i = deferred.begin();
                i != deferred.end(); ++i)
            if (*i)
                delete *i;
    }

    virtual void init()
    {
        WMOImporter::init();
        lev = Level();
        deferred.clear();
    }

    void acquire(const Var& var)
    {
        if (lev.ltype1 == MISSING_INT)
        {
            // If we don't have a level yet, defer adding the variable until we
            // have one
            auto_ptr<Var> copy(var_copy_without_unset_attrs(var));
            deferred.push_back(copy.release());
        }
        else
            msg->set(var, var.code(), lev, Trange::instant());
    }

    void acquire(const Var& var, Varcode code)
    {
        if (lev.ltype1 == MISSING_INT)
        {
            // If we don't have a level yet, defer adding the variable until we
            // have one
            auto_ptr<Var> copy(var_copy_without_unset_attrs(var, code));
            deferred.push_back(copy.release());
        }
        else
            msg->set(var, code, lev, Trange::instant());
    }

    void set_level(const Level& newlev)
    {
        if (lev.ltype1 != MISSING_INT)
            error_consistency::throwf("found two flight levels: %s and %s",
                    lev.describe().c_str(), newlev.describe().c_str());
        lev = newlev;

        // Flush deferred variables
        for (vector<Var*>::iterator i = deferred.begin();
                i != deferred.end(); ++i)
        {
            auto_ptr<Var> var(*i);
            *i = 0;
            msg->set(var, lev, Trange::instant());
        }
        deferred.clear();
    }

    virtual void run()
    {
        for (pos = 0; pos < subset->size(); ++pos)
        {
            const Var& var = (*subset)[pos];
            if (WR_VAR_F(var.code()) != 0) continue;
            if (var.value() != NULL)
                import_var(var);
        }
    }

    MsgType scanType(const Bulletin& bulletin) const
    {
        switch (bulletin.localsubtype)
        {
            case 142: return MSG_AIREP;
            case 144: return MSG_AMDAR;
            case 145: return MSG_ACARS;
            default: return MSG_GENERIC;
        }
    }
};

std::auto_ptr<Importer> Importer::createFlight(const msg::Importer::Options& opts)
{
    return auto_ptr<Importer>(new FlightImporter(opts));
}

void FlightImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        case WR_VAR(0,  1,  8): acquire(var); break;
        case WR_VAR(0,  2, 61): acquire(var); break;
        case WR_VAR(0,  2, 62): acquire(var); break;
        case WR_VAR(0,  2,  2): acquire(var); break;
        case WR_VAR(0,  2,  5): acquire(var); break;
        case WR_VAR(0,  2, 70): acquire(var); break;
        case WR_VAR(0,  2, 63): acquire(var); break;
        case WR_VAR(0,  2,  1): acquire(var); break;
        case WR_VAR(0,  8,  4): acquire(var); break;
        case WR_VAR(0,  8, 21): acquire(var); break;
        case WR_VAR(0,  7,  2):
            // Specific Altitude Above Mean Sea Level in mm
            set_level(Level(102, var.enqd() * 1000));
            acquire(var, WR_VAR(0,  7, 30));
            break;
        case WR_VAR(0,  7,  4):
            // Isobaric Surface in Pa
            set_level(Level(100, var.enqd()));
            acquire(var, WR_VAR(0, 10,  4));
            break;
        case WR_VAR(0, 11,  1): acquire(var); break;
        case WR_VAR(0, 11,  2): acquire(var); break;
        case WR_VAR(0, 11, 31): acquire(var); break;
        case WR_VAR(0, 11, 32): acquire(var); break;
        case WR_VAR(0, 11, 33): acquire(var); break;
        case WR_VAR(0, 11, 34): acquire(var); break;
        case WR_VAR(0, 11, 35): acquire(var); break;
        case WR_VAR(0, 12,  1): acquire(var, WR_VAR(0, 12, 101)); break;
        case WR_VAR(0, 12,  3): acquire(var, WR_VAR(0, 12, 103)); break;
        case WR_VAR(0, 13,  3): acquire(var); break;
        case WR_VAR(0, 20, 41): acquire(var); break;
        default:
            WMOImporter::import_var(var);
            break;
    }
}

} // namespace wbimporter
} // namespace msg
} // namespace dballe


/* vim:set ts=4 sw=4: */
