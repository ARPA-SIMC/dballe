#include "dballe/msg/wr_codec.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include <wreport/bulletin.h>
#include <cstdlib>

using namespace wreport;
using namespace std;

#define METAR_NAME "metar"
#define METAR_DESC "Metar (0.140)"

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

namespace {

struct Metar : public Template
{
    bool is_crex;

    Metar(const ExporterOptions& opts, const Messages& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return METAR_NAME; }
    virtual const char* description() const { return METAR_DESC; }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        // Use old table for old templates
        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table_version_number = 13;
        }

        is_crex = dynamic_cast<CrexBulletin*>(&bulletin) != 0;

        bulletin.data_category = 0;
        bulletin.data_subcategory = 255;
        bulletin.data_subcategory_local = 140;

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  7,  11));
        if (!is_crex)
        {
                bulletin.datadesc.push_back(WR_VAR(2, 22,   0));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  21));
                bulletin.datadesc.push_back(WR_VAR(0, 31,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  31));
                bulletin.datadesc.push_back(WR_VAR(0,  1,  32));
                bulletin.datadesc.push_back(WR_VAR(1,  1,  21));
                bulletin.datadesc.push_back(WR_VAR(0, 33,   7));
        }

        bulletin.load_tables();
    }
    void to_subset(const Message& msg, wreport::Subset& subset) override
    {
        Template::to_subset(msg, subset);

        // Look for significant levels
        const msg::Context* c_wtr = NULL;
        for (const auto& ctx: msg.data)
        {
            if (ctx.values.maybe_var(WR_VAR(0, 20, 9)))
                c_wtr = &ctx;
        }

        /*  0 */ add(WR_VAR(0,  1, 63), DBA_MSG_ST_NAME_ICAO);
        /*  1 */ add(WR_VAR(0,  2,  1), DBA_MSG_ST_TYPE);
        do_D01011();
        do_D01012();
        /*  7 */ add(WR_VAR(0,  5,  2), DBA_MSG_LATITUDE);
        /*  8 */ add(WR_VAR(0,  6,  2), DBA_MSG_LONGITUDE);
        /*  9 */ add(WR_VAR(0,  7,  1), DBA_MSG_HEIGHT_STATION);
        /* 10 */ subset.store_variable_i(WR_VAR(0,  7,  6), 10);
        /* 11 */ add(WR_VAR(0, 11,  1), DBA_MSG_WIND_DIR);
        /* 12 */ add(WR_VAR(0, 11, 16), DBA_MSG_EX_CCW_WIND);
        /* 13 */ add(WR_VAR(0, 11, 17), DBA_MSG_EX_CW_WIND);
        /* 14 */ add(WR_VAR(0, 11,  2), DBA_MSG_WIND_SPEED);
        /* 15 */ add(WR_VAR(0, 11, 41), DBA_MSG_WIND_GUST_MAX_SPEED);
        /* 16 */ subset.store_variable_i(WR_VAR(0,  7,  6), 2);
        /* 15 */ add(WR_VAR(0, 12,  1), DBA_MSG_TEMP_2M);
        /* 16 */ add(WR_VAR(0, 12,  3), DBA_MSG_DEWPOINT_2M);
        /* 17 */ add(WR_VAR(0, 10, 52), DBA_MSG_QNH);
        /* 18 */ add(WR_VAR(0, 20,  9), c_wtr, DBA_MSG_METAR_WTR);

        if (!is_crex)
        {
            subset.append_fixed_dpb(WR_VAR(2, 22, 0), 21);
            if (opts.centre != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 31), opts.centre);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 31));
            if (opts.application != MISSING_INT)
                subset.store_variable_i(WR_VAR(0, 1, 32), opts.application);
            else
                subset.store_variable_undef(WR_VAR(0, 1, 32));
        }
    }
};


} // anonymous namespace

void register_metar(TemplateRegistry& r)
{
    r.register_factory(0, METAR_NAME, METAR_DESC,
            [](const ExporterOptions& opts, const Messages& msgs) {
                return unique_ptr<Template>(new Metar(opts, msgs));
            });
}

}
}
}
}
