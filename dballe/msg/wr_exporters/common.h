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
    void add(wreport::Varcode code, const wreport::Var* var) const;

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
    const msg::Context* c_geopotential;
    const msg::Context* c_thermo;
    const msg::Context* c_tmax;
    const msg::Context* c_tmin;
    const msg::Context* c_prec1;
    const msg::Context* c_prec2;
    const msg::Context* c_prec24;
    const msg::Context* c_cloud_cover;
    const msg::Context* c_cloud_data[4];
    // FIXME: what is the maximum number of cloud groups we should support?
    const msg::Context* c_cloud_group[8];
    const msg::Context* c_wind;
    const msg::Context* c_gust1;
    const msg::Context* c_gust2;
    const msg::Context* c_visib;
    const msg::Context* c_past_wtr;
    const msg::Context* c_depth;

public:
    const wreport::Var* v_press;
    const wreport::Var* v_pressmsl;
    const wreport::Var* v_pchange3;
    const wreport::Var* v_pchange24;
    const wreport::Var* v_ptend;
    const wreport::Var* v_geopotential;

    void init(wreport::Subset& subset);
    void scan_context(const msg::Context& c);

    // Pressure data
    void add_D02001();
    // synop: pressure data
    void add_D02031();
    // synop: temperature and humidity
    void add_D02032();
    // ship: temperature and humidity
    void add_D02052();
    // synop: extreme temperature data
    void add_D02041();
    // ship: extreme temperature data
    void add_D02058();
    void add_pressure();
    void add_geopotential(wreport::Varcode code);
    // Precipitation past 24 hours
    void add_D02034();
    // Precipitation measurement
    void add_D02040();
    // synop: wind data
    void add_D02042();
    // ship: wind data
    void add_D02059();
    // Present and past weather
    void add_D02038();
    void add_ecmwf_synop_weather();
    // Basic synoptic "instantaneous" data
    void add_D02035();
    // Icing and ice
    void add_D02055();
    // ship: visibility data
    void add_D02053();
    // Ship marine data
    void add_D02056();

    /**
     * Add B07032 sensor height, taking the value from the var attributes or
     * the context, as appropriate.
     */
    void add_sensor_height(const msg::Context& c, const wreport::Var* sample_var=NULL);

    /**
     * Add B07032 and B07033 sensor heights, taking the value from the var
     * attributes or the context, as appropriate.
     */
    void add_marine_sensor_height(const msg::Context& c, const wreport::Var* sample_var=NULL);

    /**
     * Add an extreme temperature group, with the measured value added with the
     * given code, from temperature data found on the given context
     */
    void add_xtemp_group(wreport::Varcode code, const msg::Context* c);

    /**
     * Add time period and total precipitation from the given context
     */
    void add_prec_group(const msg::Context* c);

    /**
     * Add cloud data, as D02004 and a delayed replication of D02005
     */
    void add_cloud_data();

    /// Add a wind gust block with info from the given context
    void add_wind_gust(const msg::Context* c);

    /**
     * Add a B04025 or B04025 time period variable, with data taken from its
     * parameters as needed
     */
    void add_time_period(wreport::Varcode code, const msg::Context& c, const wreport::Var* sample_var, const Trange& tr_std);
};

}
}
}

#endif
