#include "dballe/msg/msg.h"
#include "dballe/core/shortcuts.h"
#include "dballe/msg/wr_codec.h"
#include <wreport/bulletin.h>
#include <cstdlib>

using namespace wreport;
using namespace std;

#define BUOY_NAME "buoy"
#define BUOY_DESC "Buoy (1.21)"

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

namespace {

struct Buoy : public Template
{
    bool is_crex;

    Buoy(const ExporterOptions& opts, const Messages& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return BUOY_NAME; }
    virtual const char* description() const { return BUOY_DESC; }

    void add(Varcode code, const Shortcut& shortcut)
    {
        const Var* var = msg->get(shortcut);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    void add(Varcode code, Varcode srccode, const Level& level, const Trange& trange)
    {
        const Var* var = msg->get(level, trange, srccode);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        // Use old table for old templates
        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table_version_number = 13;
        }

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        bulletin.data_category = 1;
        bulletin.data_subcategory = 255;
        bulletin.data_subcategory_local = 21;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  8,   3));
        if (!is_crex)
        {
                bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  32));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1, 201));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  32));
                bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }

        bulletin.load_tables();
    }
    void to_subset(const Message& msg, wreport::Subset& subset) override
    {
        Template::to_subset(msg, subset);

        /*  0 */
        const Var* var = msg.get_ident_var();
        if (var && var->isset())
        {
            subset.store_variable_i(WR_VAR(0, 1, 5), strtol(var->enqc(), 0, 10));
            subset.back().setattrs(*var);
        } else
            subset.store_variable_undef(WR_VAR(0, 1, 5));
        /*  1 */ add(WR_VAR(0,  1, 12), sc::st_dir);
        /*  2 */ add(WR_VAR(0,  1, 13), sc::st_speed);
        /*  3 */ add(WR_VAR(0,  2,  1), sc::st_type);
        do_D01011();
        do_D01012();
        /*  9 */ add(WR_VAR(0,  5,  2), sc::latitude);
        /* 10 */ add(WR_VAR(0,  6,  2), sc::longitude);
        /* 11 */ add(WR_VAR(0, 10,  4), sc::press);
        /* 12 */ add(WR_VAR(0, 10, 51), sc::press_msl);
        /* 13 */ add(WR_VAR(0, 10, 61), sc::press_3h);
        /* 14 */ add(WR_VAR(0, 10, 63), sc::press_tend);
        /* 15 */ add(WR_VAR(0, 11, 11), sc::wind_dir);
        /* 16 */ add(WR_VAR(0, 11, 12), sc::wind_speed);
        /* 17 */ add(WR_VAR(0, 12,  4), sc::temp_2m);
        /* 18 */ add(WR_VAR(0, 12,  6), sc::dewpoint_2m);
        /* 19 */ add(WR_VAR(0, 13,  3), sc::humidity);
        /* 20 */ add(WR_VAR(0, 20,  1), sc::visibility);
        /* 21 */ add(WR_VAR(0, 20,  3), sc::pres_wtr);
        do_ecmwf_past_wtr();
        /* 24 */ add(WR_VAR(0, 20, 10), sc::cloud_n);
        /* 25 */ add(WR_VAR(0,  8,  2), WR_VAR(0, 8, 2), Level::cloud(258, 0), Trange::instant());
        /* 26 */ add(WR_VAR(0, 20, 11), sc::cloud_nh);
        /* 27 */ add(WR_VAR(0, 20, 13), sc::cloud_hh);
        /* 28 */ add(WR_VAR(0, 20, 12), sc::cloud_cl);
        /* 29 */ add(WR_VAR(0, 20, 12), sc::cloud_cm);
        /* 30 */ add(WR_VAR(0, 20, 12), sc::cloud_ch);
        /* 31 */ add(WR_VAR(0, 22, 42), sc::water_temp);

        if (!is_crex)
        {
            subset.append_fixed_dpb(WR_VAR(2, 22, 0), 32);
            if (opts.centre != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 31), opts.centre);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 31));
            if (opts.application != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 201), opts.application);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 201));
        }
    }
};

} // anonymous namespace

void register_buoy(TemplateRegistry& r)
{
    r.register_factory(1, BUOY_NAME, BUOY_DESC,
            [](const ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new Buoy(opts, msgs));
            });
}

}
}
}
}
