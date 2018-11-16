#include "common.h"
#include "dballe/core/shortcuts.h"
#include <wreport/subset.h>
#include <dballe/msg/context.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

static const Trange tr_std_past_wtr3(205, 0, 10800);
static const Trange tr_std_past_wtr6(205, 0, 21600);

// If var is not NULL and has a B04194 attribute, return its value
// otherwise, return orig
static int override_trange(const Var* var, int orig)
{
    if (var)
        if (const Var* a = var->enqa(WR_VAR(0, 4, 194)))
            return a->enq(orig);
    return orig;
}

void ExporterModule::init(const Message& msg, wreport::Subset& subset)
{
    this->msg = &msg;
    this->subset = &subset;
    c_surface_instant = 0;
    c_ana = 0;
}

void ExporterModule::scan_context(const msg::Context& c)
{
    switch (c.level.ltype1)
    {
        case 1:
            if (c.trange.pind == 254)
                c_surface_instant = &c;
            break;
        case MISSING_INT: c_ana = &c; break;
    }
}

void ExporterModule::add(Varcode code, const msg::Context* ctx, const Shortcut& shortcut) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->values.maybe_var(shortcut.code))
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void ExporterModule::add(Varcode code, const msg::Context* ctx, Varcode srccode) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->values.maybe_var(srccode))
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void ExporterModule::add(Varcode code, const msg::Context* ctx) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->values.maybe_var(code))
        subset->store_variable(*var);
    else
        subset->store_variable_undef(code);
}

void ExporterModule::add(wreport::Varcode code, const wreport::Var* var) const
{
    if (var)
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void ExporterModule::add_ecmwf_synop_head()
{
    add(WR_VAR(0,  1,  1), c_ana);
    add(WR_VAR(0,  1,  2), c_ana);
    add(WR_VAR(0,  2,  1), c_ana);
}

void CommonSynopExporter::init(const Message& msg, wreport::Subset& subset)
{
    ExporterModule::init(msg, subset);
    c_geopotential = 0;
    c_thermo = 0;
    c_tmax = 0;
    c_tmin = 0;
    c_prec1 = 0;
    c_prec2 = 0;
    c_prec24 = 0;
    c_cloud_cover = 0;
    for (int i = 0; i < 4; ++i)
        c_cloud_data[i] = 0;
    for (unsigned i = 0; i < sizeof(c_cloud_group) / sizeof(c_cloud_group[0]); ++i)
        c_cloud_group[i] = 0;
    c_wind = 0;
    c_gust1 = 0;
    c_gust2 = 0;
    c_visib = 0;
    c_past_wtr = 0;
    c_depth = 0;
    for (unsigned i = 0; i < sizeof(c_swell_waves) / sizeof(c_swell_waves[0]); ++i)
        c_swell_waves[i] = 0;
    v_press = 0;
    v_pressmsl = 0;
    v_pchange3 = 0;
    v_pchange24 = 0;
    v_ptend = 0;
    v_geopotential = 0;
}

void CommonSynopExporter::scan_context(const msg::Context& c)
{
    ExporterModule::scan_context(c);
    switch (c.level.ltype1)
    {
        case 1:
            switch (c.trange.pind)
            {
                case 1:
                    if (c.values.maybe_var(WR_VAR(0, 13, 11)))
                    {
                        if (c.trange.p2 == 86400)
                            c_prec24 = &c;
                        else if (!c_prec1)
                            c_prec1 = &c;
                        else if (!c_prec2)
                            c_prec2 = &c;
                    }
                    break;
                case 2:
                    if (c.values.maybe_var(WR_VAR(0, 12, 101))) c_tmax = &c;
                    break;
                case 3:
                    if (c.values.maybe_var(WR_VAR(0, 12, 101))) c_tmin = &c;
                    break;
                case 4:
                    if (const Var* v = c.values.maybe_var(sc::press_3h.code))
                        switch (c.trange.p2)
                        {
                            case  3*3600: v_pchange3 = v; break;
                            case 24*3600: v_pchange24 = v; break;
                        }
                    break;
                case 205:
                    if (const Var* v = c.values.maybe_var(sc::press_tend.code))
                        v_ptend = v;
                    if (c.values.maybe_var(WR_VAR(0, 20, 4)) || c.values.maybe_var(WR_VAR(0, 20, 5)))
                        c_past_wtr = &c;
                    break;
                case 254:
                    if (const Var* v = c.values.maybe_var(sc::press.code))
                        v_press = v;
                    if (const Var* v = c.values.maybe_var(sc::press_msl.code))
                        v_pressmsl = v;
                    if (c.values.maybe_var(sc::visibility.code))
                        c_visib = &c;
                    if (c.values.maybe_var(WR_VAR(0, 22, 43)))
                        c_depth = &c;
                    break;
            }
            break;
        case 100:
            // Look for geopotential
            if (const Var* v = c.values.maybe_var(WR_VAR(0, 10, 8)))
            {
                c_geopotential = &c;
                v_geopotential = v;
            }
            break;
        case 101:
        case 102:
            switch (c.trange.pind)
            {
                case 4:
                    if (const Var* v = c.values.maybe_var(sc::press_3h.code))
                        switch (c.trange.p2)
                        {
                            case  3*3600: v_pchange3 = v; break;
                            case 24*3600: v_pchange24 = v; break;
                        }
                    break;
                case 205:
                    if (const Var* v = c.values.maybe_var(sc::press_tend.code))
                        v_ptend = v;
                    break;
                case 254:
                    if (const Var* v = c.values.maybe_var(sc::press.code))
                        v_press = v;
                    if (const Var* v = c.values.maybe_var(sc::press_msl.code))
                        v_pressmsl = v;
                    break;
            }
            break;
        case 103:
            if (c.values.maybe_var(WR_VAR(0, 11, 1)) || c.values.maybe_var(WR_VAR(0, 11, 2)))
                if (!c_wind)
                    c_wind = &c;
            if (c.values.maybe_var(WR_VAR(0, 11, 41)) || c.values.maybe_var(WR_VAR(0, 11, 43)))
            {
                if (!c_gust1)
                    c_gust1 = &c;
                else if (!c_gust2)
                    c_gust2 = &c;
            }
            if (c.values.maybe_var(sc::visibility.code))
                c_visib = &c;
            switch (c.trange.pind)
            {
                case 1:
                    if (c.values.maybe_var(WR_VAR(0, 13, 11)))
                    {
                        if (c.trange.p2 == 86400)
                            c_prec24 = &c;
                        else if (!c_prec1)
                            c_prec1 = &c;
                        else if (!c_prec2)
                            c_prec2 = &c;
                    }
                    break;
                case 2:
                    if (c.values.maybe_var(WR_VAR(0, 12, 101))) c_tmax = &c;
                    break;
                case 3:
                    if (c.values.maybe_var(WR_VAR(0, 12, 101))) c_tmin = &c;
                    break;
                case 254:
                    if (c.values.maybe_var(sc::temp_2m.code) || c.values.maybe_var(sc::dewpoint_2m.code) || c.values.maybe_var(sc::humidity.code))
                        c_thermo = &c;
                    break;
            }
            break;
        case 160:
            if (c.find_by_id(WR_VAR(0, 22, 43)))
                c_depth = &c;
            break;
        case 256:
            // Clouds
            switch (c.level.ltype2)
            {
                case 258:
                    if (c.level.l2 >= 0 && c.level.l2 < 4)
                        c_cloud_data[c.level.l2] = &c;
                    break;
                case 259:
                    if (c.level.l2 >= 0 && c.level.l2 < (int)(sizeof(c_cloud_group) / sizeof(c_cloud_group[0])))
                        c_cloud_group[c.level.l2] = &c;
                    break;
                case MISSING_INT:
                    c_cloud_cover = &c;
                    break;
            }
            break;
        case 264:
            // Swell wave groups
            switch (c.level.ltype2)
            {
                case 261:
                    if (c.level.l2 >= 1 && c.level.l2 <= (int)(sizeof(c_swell_waves) / sizeof(c_swell_waves[0])))
                        c_swell_waves[c.level.l2 - 1] = &c;
                    break;
            }
            break;
    }
};

void CommonSynopExporter::add_D02001()
{
    add(WR_VAR(0, 10,  4), v_press);
    add(WR_VAR(0, 10, 51), v_pressmsl);
    add(WR_VAR(0, 10, 61), v_pchange3);
    add(WR_VAR(0, 10, 63), v_ptend);
}

void CommonSynopExporter::add_D02031()
{
    add_D02001();
    add(WR_VAR(0, 10, 62), v_pchange24);
    add_pressure();
    add_geopotential(WR_VAR(0, 10,  9));
}

void CommonSynopExporter::add_pressure()
{
    if (c_geopotential)
    {
        if (c_geopotential->level.l1 == MISSING_INT)
            subset->store_variable_undef(WR_VAR(0, 7, 4));
        else
            subset->store_variable_d(WR_VAR(0, 7, 4), c_geopotential->level.l1);
    }
    else
        subset->store_variable_undef(WR_VAR(0,  7,  4));
}

void CommonSynopExporter::add_geopotential(wreport::Varcode code)
{
    add(code, v_geopotential);
}

void CommonSynopExporter::add_D02032()
{
    if (!c_thermo)
    {
        subset->store_variable_undef(WR_VAR(0,  7,  32));
        subset->store_variable_undef(WR_VAR(0, 12, 101));
        subset->store_variable_undef(WR_VAR(0, 12, 103));
        subset->store_variable_undef(WR_VAR(0, 13,   3));
    } else {
        const Var* var_t = c_thermo->values.maybe_var(sc::temp_2m.code);
        const Var* var_d = c_thermo->values.maybe_var(sc::dewpoint_2m.code);
        const Var* var_h = c_thermo->values.maybe_var(sc::humidity.code);
        add_sensor_height(*c_thermo, var_t ? var_t : var_d ? var_d : var_h);
        add(WR_VAR(0, 12, 101), var_t);
        add(WR_VAR(0, 12, 103), var_d);
        add(WR_VAR(0, 13,   3), var_h);
    }
}
void CommonSynopExporter::add_D02052()
{
    if (!c_thermo)
    {
        subset->store_variable_undef(WR_VAR(0,  7,  32));
        subset->store_variable_undef(WR_VAR(0,  7,  33));
        subset->store_variable_undef(WR_VAR(0, 12, 101));
        subset->store_variable_undef(WR_VAR(0,  2,  39));
        subset->store_variable_undef(WR_VAR(0, 12, 102));
        subset->store_variable_undef(WR_VAR(0, 12, 103));
        subset->store_variable_undef(WR_VAR(0, 13,   3));
    } else {
        const Var* var_t = c_thermo->values.maybe_var(sc::temp_2m.code);
        const Var* var_w = c_thermo->values.maybe_var(sc::wet_temp_2m.code);
        const Var* var_d = c_thermo->values.maybe_var(sc::dewpoint_2m.code);
        const Var* var_h = c_thermo->values.maybe_var(sc::humidity.code);
        add_marine_sensor_height(*c_thermo, var_t ? var_t : var_w ? var_w : var_d ? var_d : var_h);
        add(WR_VAR(0, 12, 101), var_t);
        add(WR_VAR(0,  2,  39), c_ana);
        add(WR_VAR(0, 12, 102), var_w);
        add(WR_VAR(0, 12, 103), var_d);
        add(WR_VAR(0, 13,   3), var_h);
    }
}

void CommonSynopExporter::add_D02041()
{
    if (c_tmax || c_tmin)
    {
        const msg::Context* c_first = c_tmax ? c_tmax : c_tmin;
        add_sensor_height(*c_first, c_first->values.maybe_var(WR_VAR(0, 12, 101)));
    }
    else
        subset->store_variable_undef(WR_VAR(0,  7, 32));
    add_xtemp_group(WR_VAR(0, 12, 111), c_tmax);
    add_xtemp_group(WR_VAR(0, 12, 112), c_tmin);
}

void CommonSynopExporter::add_D02058()
{
    if (c_tmax || c_tmin)
    {
        const msg::Context* c_first = c_tmax ? c_tmax : c_tmin;
        add_marine_sensor_height(*c_first, c_first->values.maybe_var(WR_VAR(0, 12, 101)));
    }
    else
    {
        subset->store_variable_undef(WR_VAR(0,  7, 32));
        subset->store_variable_undef(WR_VAR(0,  7, 33));
    }
    add_xtemp_group(WR_VAR(0, 12, 111), c_tmax);
    add_xtemp_group(WR_VAR(0, 12, 112), c_tmin);
}

void CommonSynopExporter::add_D02034()
{
    if (c_prec24)
    {
        const Var* var = c_prec24->values.maybe_var(WR_VAR(0, 13, 11));
        add_sensor_height(*c_prec24, var);
        add(WR_VAR(0, 13, 23), c_prec24, sc::tot_prec24);
    } else {
        subset->store_variable_undef(WR_VAR(0,  7, 32));
        subset->store_variable_undef(WR_VAR(0, 13, 23));
    }
    subset->store_variable_undef(WR_VAR(0, 7, 32));
}

void CommonSynopExporter::add_D02040()
{
    if (c_prec1)
    {
        const Var* prec_var = c_prec1->values.maybe_var(WR_VAR(0, 13, 11));
        add_sensor_height(*c_prec1, prec_var);
    } else {
        subset->store_variable_undef(WR_VAR(0,  7, 32));
    }
    add_prec_group(c_prec1);
    add_prec_group(c_prec2);
}

void CommonSynopExporter::add_D02042()
{
    if (c_wind || c_gust1 || c_gust2)
    {
        // Look for the sensor height in the context of any of the found levels
        const msg::Context* c_first = c_wind ? c_wind : c_gust1 ? c_gust1 : c_gust2;
        const Var* sample_var = c_first->values.maybe_var(WR_VAR(0, 11, 1));
        if (!sample_var) sample_var = c_first->values.maybe_var(WR_VAR(0, 11, 2));
        if (!sample_var) sample_var = c_first->values.maybe_var(WR_VAR(0, 11, 41));
        if (!sample_var) sample_var = c_first->values.maybe_var(WR_VAR(0, 11, 43));
        add_sensor_height(*c_first, sample_var);

        add(WR_VAR(0, 2, 2), c_ana);
        subset->store_variable_i(WR_VAR(0, 8, 21), 2);

        if (c_wind)
        {
            // Compute time range from level and attrs
            const Var* var_dir = c_wind->values.maybe_var(WR_VAR(0, 11, 1));
            const Var* var_speed = c_wind->values.maybe_var(WR_VAR(0, 11, 2));
            int tr = MISSING_INT;
            if (c_wind->trange.pind == 200)
                tr = c_wind->trange.p2;
            tr = override_trange(var_dir, override_trange(var_speed, tr));
            if (tr == MISSING_INT)
                subset->store_variable_undef(WR_VAR(0,  4, 25));
            else
                subset->store_variable_d(WR_VAR(0,  4, 25), -tr / 60.0);

            add(WR_VAR(0, 11,  1), var_dir);
            add(WR_VAR(0, 11,  2), var_speed);
        } else {
            subset->store_variable_undef(WR_VAR(0,  4, 25));
            subset->store_variable_undef(WR_VAR(0, 11,  1));
            subset->store_variable_undef(WR_VAR(0, 11,  2));
        }
        subset->store_variable_undef(WR_VAR(0,  8, 21));

        add_wind_gust(c_gust1);
        add_wind_gust(c_gust2);
    } else {
        subset->store_variable_undef(WR_VAR(0,  7, 32));
        subset->store_variable_undef(WR_VAR(0,  2,  2));
        subset->store_variable_i(WR_VAR(0,  8, 21), 2);
        subset->store_variable_undef(WR_VAR(0,  4, 25));
        subset->store_variable_undef(WR_VAR(0, 11,  1));
        subset->store_variable_undef(WR_VAR(0, 11,  2));
        subset->store_variable_undef(WR_VAR(0,  8, 21));
        for (int i = 1; i <= 2; ++i)
        {
            subset->store_variable_undef(WR_VAR(0,  4, 25));
            subset->store_variable_undef(WR_VAR(0, 11, 43));
            subset->store_variable_undef(WR_VAR(0, 11, 41));
        }
    }
}

void CommonSynopExporter::add_D02059()
{
    if (c_wind || c_gust1 || c_gust2)
    {
        // Look for the sensor height in the context of any of the found levels
        const msg::Context* c_first = c_wind ? c_wind : c_gust1 ? c_gust1 : c_gust2;
        const Var* sample_var = c_first->values.maybe_var(WR_VAR(0, 11, 1));
        if (!sample_var) sample_var = c_first->values.maybe_var(WR_VAR(0, 11, 2));
        if (!sample_var) sample_var = c_first->values.maybe_var(WR_VAR(0, 11, 41));
        if (!sample_var) sample_var = c_first->values.maybe_var(WR_VAR(0, 11, 43));
        add_marine_sensor_height(*c_first, sample_var);

        add(WR_VAR(0, 2, 2), c_ana);
        subset->store_variable_i(WR_VAR(0, 8, 21), 2);

        if (c_wind)
        {
            // Compute time range from level and attrs
            const Var* var_dir = c_wind->values.maybe_var(WR_VAR(0, 11, 1));
            const Var* var_speed = c_wind->values.maybe_var(WR_VAR(0, 11, 2));
            int tr = MISSING_INT;
            if (c_wind->trange.pind == 200)
                tr = c_wind->trange.p2;
            tr = override_trange(var_dir, override_trange(var_speed, tr));
            if (tr == MISSING_INT)
                subset->store_variable_undef(WR_VAR(0,  4, 25));
            else
                subset->store_variable_d(WR_VAR(0,  4, 25), -tr / 60.0);

            add(WR_VAR(0, 11,  1), var_dir);
            add(WR_VAR(0, 11,  2), var_speed);
        } else {
            subset->store_variable_undef(WR_VAR(0,  4, 25));
            subset->store_variable_undef(WR_VAR(0, 11,  1));
            subset->store_variable_undef(WR_VAR(0, 11,  2));
        }
        subset->store_variable_undef(WR_VAR(0,  8, 21));

        add_wind_gust(c_gust1);
        add_wind_gust(c_gust2);
    } else {
        subset->store_variable_undef(WR_VAR(0,  7, 32));
        subset->store_variable_undef(WR_VAR(0,  7, 33));
        subset->store_variable_undef(WR_VAR(0,  2,  2));
        subset->store_variable_i(WR_VAR(0,  8, 21), 2);
        subset->store_variable_undef(WR_VAR(0,  4, 25));
        subset->store_variable_undef(WR_VAR(0, 11,  1));
        subset->store_variable_undef(WR_VAR(0, 11,  2));
        subset->store_variable_undef(WR_VAR(0,  8, 21));
        for (int i = 1; i <= 2; ++i)
        {
            subset->store_variable_undef(WR_VAR(0,  4, 25));
            subset->store_variable_undef(WR_VAR(0, 11, 43));
            subset->store_variable_undef(WR_VAR(0, 11, 41));
        }
    }
}

void CommonSynopExporter::add_wind_gust(const msg::Context* c)
{
    if (c)
    {
        // Compute time range from level and attrs
        const Var* var_dir = c->values.maybe_var(WR_VAR(0, 11, 43));
        const Var* var_speed = c->values.maybe_var(WR_VAR(0, 11, 41));
        int tr = MISSING_INT;
        if (c->trange.pind == 205)
            tr = c->trange.p2;
        else if (c->trange.pind == 254)
            tr = 600;
        tr = override_trange(var_dir, override_trange(var_speed, tr));
        if (tr == MISSING_INT)
            subset->store_variable_undef(WR_VAR(0,  4, 25));
        else
            subset->store_variable_d(WR_VAR(0,  4, 25), -tr / 60.0);

        add(WR_VAR(0, 11, 43), var_dir);
        add(WR_VAR(0, 11, 41), var_speed);
    } else {
        subset->store_variable_undef(WR_VAR(0,  4, 25));
        subset->store_variable_undef(WR_VAR(0, 11, 43));
        subset->store_variable_undef(WR_VAR(0, 11, 41));
    }
}


void CommonSynopExporter::add_cloud_data()
{
    // Cloud data
    add(WR_VAR(0, 20, 10), c_cloud_cover);
    add(WR_VAR(0,  8,  2), c_cloud_data[0]);
    add(WR_VAR(0, 20, 11), c_cloud_data[0]);
    add(WR_VAR(0, 20, 13), c_cloud_data[0]);
    add(WR_VAR(0, 20, 12), c_cloud_data[1]);
    add(WR_VAR(0, 20, 12), c_cloud_data[2]);
    add(WR_VAR(0, 20, 12), c_cloud_data[3]);

    // Individual cloud layers or masses
    int count = 0;
    for (unsigned i = 0; i < sizeof(c_cloud_group) / sizeof(c_cloud_group[0]); ++i)
        if (c_cloud_group[i])
            ++count;

    // Number of individual cloud layers or masses
    subset->store_variable_i(WR_VAR(0, 1, 31), count);
    for (unsigned i = 0; i < sizeof(c_cloud_group) / sizeof(c_cloud_group[0]); ++i)
    {
        if (!c_cloud_group[i]) continue;

        add(WR_VAR(0,  8,  2), c_cloud_group[i]);
        add(WR_VAR(0, 20, 11), c_cloud_group[i]);
        add(WR_VAR(0, 20, 12), c_cloud_group[i]);
        add(WR_VAR(0, 20, 13), c_cloud_group[i]);
    }
}

void CommonSynopExporter::add_prec_group(const msg::Context* c)
{
    if (c)
    {
        if (c->trange.p2 != MISSING_INT)
            subset->store_variable_d(WR_VAR(0,  4, 24), -c->trange.p2 / 3600);
        else
            subset->store_variable_undef(WR_VAR(0,  4, 24));
        if (const Var* var = c->values.maybe_var(WR_VAR(0, 13, 11)))
            subset->store_variable(*var);
        else
            subset->store_variable_undef(WR_VAR(0, 13, 11));
    } else {
        subset->store_variable_undef(WR_VAR(0,  4, 24));
        subset->store_variable_undef(WR_VAR(0, 13, 11));
    }
}

void CommonSynopExporter::add_sensor_height(const msg::Context& c, const Var* sample_var)
{
    // Try with attributes first
    if (sample_var)
    {
        if (const Var* a = sample_var->enqa(WR_VAR(0, 7, 32)))
        {
            subset->store_variable_d(WR_VAR(0, 7, 32), a->enqd());
            return;
        }
    }

    // Use level
    if (c.level.ltype1 == 1)
        // Ground level
        subset->store_variable_d(WR_VAR(0, 7, 32), 0);
    else if (c.level.ltype1 == 103)
    {
        // Height above ground level
        if (c.level.l1 == MISSING_INT)
            subset->store_variable_undef(WR_VAR(0, 7, 32));
        else
            subset->store_variable_d(WR_VAR(0, 7, 32), double(c.level.l1) / 1000.0);
    }
    else
        error_consistency::throwf("Cannot add sensor height from unsupported level type %d", c.level.ltype1);
}

void CommonSynopExporter::add_marine_sensor_height(const msg::Context& c, const Var* sample_var)
{
    // FIXME: B07033 is hardcoded as undef for now, until we actually found
    //        bulletins in which it is set

    // Try with attributes first
    if (sample_var)
    {
        if (const Var* a = sample_var->enqa(WR_VAR(0, 7, 32)))
        {
            subset->store_variable_d(WR_VAR(0, 7, 32), a->enqd());
            subset->store_variable_undef(WR_VAR(0, 7, 33));
            return;
        }
    }

    // Use level
    if (c.level.ltype1 == 1)
    {
        // Ground level
        subset->store_variable_d(WR_VAR(0, 7, 32), 0);
        subset->store_variable_undef(WR_VAR(0, 7, 33));
    }
    else if (c.level.ltype1 == 103)
    {
        // Height above ground level
        if (c.level.l1 == MISSING_INT)
            subset->store_variable_undef(WR_VAR(0, 7, 32));
        else
            subset->store_variable_d(WR_VAR(0, 7, 32), double(c.level.l1) / 1000.0);
        subset->store_variable_undef(WR_VAR(0, 7, 33));
    }
    else
        error_consistency::throwf("Cannot add sensor height from unsupported level type %d", c.level.ltype1);
}

void CommonSynopExporter::add_xtemp_group(Varcode code, const msg::Context* c)
{
    if (c)
    {
        // Duration of statistical processing
        if (c->trange.p2 != MISSING_INT)
            subset->store_variable_d(WR_VAR(0,  4, 24), -c->trange.p2 / 3600);
        else
            subset->store_variable_undef(WR_VAR(0,  4, 24));

        // Offset from end of interval to synop reference time
        if (c->trange.p1 != 0 && c->trange.p1 != MISSING_INT)
            subset->store_variable_d(WR_VAR(0,  4, 24), c->trange.p1 / 3600);
        else if (c->trange.p1 == MISSING_INT || c->trange.p2 == MISSING_INT)
            subset->store_variable_undef(WR_VAR(0,  4, 24));
        else
            subset->store_variable_d(WR_VAR(0,  4, 24), 0);

        add(code, c, WR_VAR(0, 12, 101));
    } else {
        subset->store_variable_undef(WR_VAR(0,  4, 24));
        subset->store_variable_undef(WR_VAR(0,  4, 24));
        subset->store_variable_undef(code);
    }
}

void CommonSynopExporter::add_ecmwf_synop_weather()
{
    add(WR_VAR(0, 11, 11), c_wind, sc::wind_dir);
    add(WR_VAR(0, 11, 12), c_wind, sc::wind_speed);
    add(WR_VAR(0, 12,  4), c_thermo, sc::temp_2m);
    add(WR_VAR(0, 12,  6), c_thermo, sc::dewpoint_2m);
    add(WR_VAR(0, 13,  3), c_thermo, sc::humidity);
    add(WR_VAR(0, 20,  1), c_visib);
    add(WR_VAR(0, 20,  3), c_surface_instant, sc::pres_wtr);
    add(WR_VAR(0, 20,  4), c_past_wtr);
    add(WR_VAR(0, 20,  5), c_past_wtr);
}

void CommonSynopExporter::add_D02038()
{
    //   Present and past weather
    add(WR_VAR(0, 20,  3), c_surface_instant);
    if (c_past_wtr)
    {
        int hour = msg->get_datetime().hour;
        // Look for a p2 override in the attributes
        const Var* v = c_past_wtr->values.maybe_var(WR_VAR(0, 20, 4));
        if (!v) v = c_past_wtr->values.maybe_var(WR_VAR(0, 20, 5));
        add_time_period(WR_VAR(0, 4, 24), *c_past_wtr, v, hour % 6 == 0 ? tr_std_past_wtr6 : tr_std_past_wtr3);
    } else
        subset->store_variable_undef(WR_VAR(0, 4, 24));
    add(WR_VAR(0, 20, 4), c_past_wtr);
    add(WR_VAR(0, 20, 5), c_past_wtr);
}

void CommonSynopExporter::add_time_period(Varcode code, const msg::Context& c, const Var* sample_var, const Trange& tr_std)
{
    int p2;
    if (c.trange.pind == 254)
        p2 = tr_std.p2;
    else
        p2 = c.trange.p2;

    // Look for a p2 override in the attributes
    if (sample_var)
        if (const Var* a = sample_var->enqa(WR_VAR(0, 4, 194)))
            p2 = a->enqi();

    if (p2 == MISSING_INT)
    {
        subset->store_variable_undef(WR_VAR(0, 4, 24));
        return;
    }

    double res = -p2;
    switch (code)
    {
        case WR_VAR(0, 4, 24): res /= 3600.0; break;
        case WR_VAR(0, 4, 25): res /= 60.0; break;
    }
    subset->store_variable_d(code, res);
}


void CommonSynopExporter::add_D02035()
{
    // Temperature and humidity data
    add_D02032();

    //   Visibility data
    if (c_visib)
    {
        const Var* var = c_visib->values.maybe_var(sc::visibility.code);
        add_sensor_height(*c_visib, var);
    } else
        subset->store_variable_undef(WR_VAR(0, 7, 32));
    add(WR_VAR(0, 20, 1), c_visib, sc::visibility);

    // Precipitation past 24 hours
    add_D02034();

    // Cloud data
    add_cloud_data();
}

void CommonSynopExporter::add_D02053()
{
    //   Visibility data
    if (c_visib)
    {
        const Var* var = c_visib->values.maybe_var(sc::visibility.code);
        add_marine_sensor_height(*c_visib, var);
    } else {
        subset->store_variable_undef(WR_VAR(0, 7, 32));
        subset->store_variable_undef(WR_VAR(0, 7, 33));
    }
    add(WR_VAR(0, 20, 1), c_visib, sc::visibility);
    subset->store_variable_undef(WR_VAR(0, 7, 33));
}

void CommonSynopExporter::add_D02055()
{
    add(WR_VAR(0, 20,  31), c_surface_instant);
    add(WR_VAR(0, 20,  32), c_surface_instant);
    add(WR_VAR(0, 20,  33), c_surface_instant);
    add(WR_VAR(0, 20,  34), c_surface_instant);
    add(WR_VAR(0, 20,  35), c_surface_instant);
    add(WR_VAR(0, 20,  36), c_surface_instant);
    add(WR_VAR(0, 20,  37), c_surface_instant);
    add(WR_VAR(0, 20,  38), c_surface_instant);
}

void CommonSynopExporter::add_ecmwf_ice()
{
    add(WR_VAR(0, 20,  33), c_surface_instant);
    add(WR_VAR(0, 20,  31), c_surface_instant);
    add(WR_VAR(0, 20,  32), c_surface_instant);
    add(WR_VAR(0, 20,  34), c_surface_instant);
    add(WR_VAR(0, 20,  37), c_surface_instant);
    add(WR_VAR(0, 20,  38), c_surface_instant);
    add(WR_VAR(0, 20,  36), c_surface_instant);
}

void CommonSynopExporter::add_D02056()
{
    add(WR_VAR(0,  2,  38), c_ana);
    if (c_depth)
    {
        if (c_depth->level.ltype1 == 160)
            subset->store_variable_d(WR_VAR(0,  7, 63), (double)c_depth->level.l1 / 1000.0);
        else
            subset->store_variable_undef(WR_VAR(0,  7, 63));
        add(WR_VAR(0, 22, 43), c_depth);
    } else {
        subset->store_variable_undef(WR_VAR(0,  7, 63));
        subset->store_variable_undef(WR_VAR(0, 22, 43));
    }
    subset->store_variable_undef(WR_VAR(0,  7, 63));
}

void CommonSynopExporter::add_plain_waves()
{
    add(WR_VAR(0, 22,   1), c_surface_instant);
    add(WR_VAR(0, 22,  11), c_surface_instant);
    add(WR_VAR(0, 22,  21), c_surface_instant);
}

void CommonSynopExporter::add_D02024()
{
    // Wind waves
    add(WR_VAR(0, 22,   2), c_surface_instant);
    add(WR_VAR(0, 22,  12), c_surface_instant);
    add(WR_VAR(0, 22,  22), c_surface_instant);

    // 2 systems of swell waves
    unsigned src_idx = 0;
    for (int i = 0; i < 2; ++i)
    {
        // Look for the next available swell waves group
        const msg::Context* c = 0;
        for ( ; src_idx < sizeof(c_swell_waves) / sizeof(c_swell_waves[0]); ++src_idx)
            if (c_swell_waves[src_idx])
            {
                c = c_swell_waves[src_idx];
                break;
            }

        add(WR_VAR(0, 22,   3), c);
        add(WR_VAR(0, 22,  13), c);
        add(WR_VAR(0, 22,  23), c);
        ++src_idx;
    }
}

}
}
}
}
