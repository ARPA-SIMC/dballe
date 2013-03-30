/*
 * dballe/wr_exporter/common - Common infrastructure for wreport exporters
 *
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

#ifndef DBALLE_MSG_WREXPORTER_BASE_H
#define DBALLE_MSG_WREXPORTER_BASE_H

#include <dballe/msg/msg.h>

namespace wreport {
struct Subset;
struct Bulletin;
struct Var;
}

namespace dballe {
namespace msg {
namespace wr {

class ExporterModule
{
protected:
    // Subset being written
    wreport::Subset* subset;
    const msg::Context* c_ana;
    const msg::Context* c_surface_instant;

    void add(wreport::Varcode code, const msg::Context* ctx, int shortcut) const;
    void add(wreport::Varcode code, const msg::Context* ctx, wreport::Varcode srccode) const;
    void add(wreport::Varcode code, const msg::Context* ctx) const;

public:
    void init(wreport::Subset& subset);
    void scan_context(const msg::Context& c);

    int get_hour();

    void add_year_to_minute();
    void add_latlon_coarse();
    void add_latlon_high();
    void add_station_name(wreport::Varcode code);
    void add_station_height();
    void add_ecmwf_synop_head();
    void add_ship_head();
    // SYNOP Fixed surface station identification, time, horizontal and
    // vertical coordinates
    void add_D01090();
    void add_D01093();
};

class CommonSynopExporter : public ExporterModule
{
protected:

public:
    void init(wreport::Subset& subset);
    void scan_context(const msg::Context& c);
};

}
}
}

#endif
