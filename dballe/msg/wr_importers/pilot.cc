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
#include <wreport/conv.h>
#include <cmath>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

#define MISSING_PRESS -1.0
static inline int to_h(double val)
{
	return lround(val / 9.80665);
}

class PilotImporter : public WMOImporter
{
protected:
    Level lev;

    void import_var(const Var& var);

public:
    PilotImporter(const msg::Importer::Options& opts) : WMOImporter(opts) {}
    virtual ~PilotImporter() {}

    virtual void init()
    {
        WMOImporter::init();
        lev = Level();
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

    MsgType scanType(const Bulletin& bulletin) const { return MSG_PILOT; }
};

std::auto_ptr<Importer> Importer::createPilot(const msg::Importer::Options& opts)
{
    return auto_ptr<Importer>(new PilotImporter(opts));
}

void PilotImporter::import_var(const Var& var)
{
	switch (var.code())
	{
		case WR_VAR(0, 2, 11): msg->set_sonde_type_var(var); break;
		case WR_VAR(0, 2, 12): msg->set_sonde_method_var(var); break;
		case WR_VAR(0, 7,  1): msg->set_height_station_var(var); break;
		case WR_VAR(0, 7,  4):
			lev.ltype1 = 100;
			lev.l1 = var.enqd();
			msg->set(var, WR_VAR(0, 10, 4), lev, Trange::instant());
			break;
		case WR_VAR(0, 8,  1):
			if (pos > 0 && pos < subset->size() - 1
			 && (*subset)[pos - 1].value() == NULL
			 && (*subset)[pos + 1].value() != NULL)
			{
				lev.ltype1 = 102;
				lev.l1 = to_h((*subset)[pos + 1].enqd());
			}
            {
                auto_ptr<Var> nvar(newvar(WR_VAR(0, 8, 42), convert_BUFR08001_to_BUFR08042(var.enqi())));
                nvar->copy_attrs(var);
                msg->set(nvar, lev, Trange::instant());
            }
			break;
		case WR_VAR(0, 10, 3):
			lev.ltype1 = 102;
			lev.l1 = to_h(var.enqd());
			msg->set(var, WR_VAR(0, 10, 8), lev, Trange::instant());
			break;
		case WR_VAR(0, 11, 1): msg->set(var, WR_VAR(0, 11, 1), lev, Trange::instant()); break;
		case WR_VAR(0, 11, 2): msg->set(var, WR_VAR(0, 11, 2), lev, Trange::instant()); break;
		default:
			WMOImporter::import_var(var);
			break;
	}
}

} // namespace wbimporter
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
