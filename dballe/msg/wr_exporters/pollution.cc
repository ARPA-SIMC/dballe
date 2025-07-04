#include "dballe/core/shortcuts.h"
#include "dballe/msg/context.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/wr_codec.h"
#include <cmath>
#include <cstdlib>
#include <wreport/bulletin.h>

using namespace wreport;
using namespace std;

#define POLLUTION_NAME "pollution"
#define POLLUTION_DESC "Pollution (8.171)"

namespace dballe {
namespace impl {
namespace msg {
namespace wr {

namespace {

static double intexp10(unsigned x)
{
    switch (x)
    {
        case 0:  return 1.0;
        case 1:  return 10.0;
        case 2:  return 100.0;
        case 3:  return 1000.0;
        case 4:  return 10000.0;
        case 5:  return 100000.0;
        case 6:  return 1000000.0;
        case 7:  return 10000000.0;
        case 8:  return 100000000.0;
        case 9:  return 1000000000.0;
        case 10: return 10000000000.0;
        case 11: return 100000000000.0;
        case 12: return 1000000000000.0;
        case 13: return 10000000000000.0;
        case 14: return 100000000000000.0;
        case 15: return 1000000000000000.0;
        case 16: return 10000000000000000.0;
        default:
            error_domain::throwf(
                "computing double value of %u^10 is not yet supported", x);
    }
}

struct Pollution : public Template
{
    Pollution(const dballe::ExporterOptions& opts, const Messages& msgs)
        : Template(opts, msgs)
    {
    }

    const char* name() const override { return POLLUTION_NAME; }
    const char* description() const override { return POLLUTION_DESC; }

    void setupBulletin(wreport::Bulletin& bulletin) override
    {
        Template::setupBulletin(bulletin);

        bulletin.data_category          = 8;
        bulletin.data_subcategory       = 255;
        bulletin.data_subcategory_local = 171;
        bulletin.originating_centre     = 98;
        bulletin.originating_subcentre  = 0;

        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->master_table_version_number       = 13;
            b->master_table_version_number_local = 102;
        }

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3, 7, 11));
        bulletin.datadesc.push_back(WR_VAR(0, 1, 19));
        bulletin.datadesc.push_back(WR_VAR(0, 1, 212));
        bulletin.datadesc.push_back(WR_VAR(0, 1, 213));
        bulletin.datadesc.push_back(WR_VAR(0, 1, 214));
        bulletin.datadesc.push_back(WR_VAR(0, 1, 215));
        bulletin.datadesc.push_back(WR_VAR(0, 1, 216));
        bulletin.datadesc.push_back(WR_VAR(0, 1, 217));
        bulletin.datadesc.push_back(WR_VAR(3, 1, 11));
        bulletin.datadesc.push_back(WR_VAR(3, 1, 13));
        bulletin.datadesc.push_back(WR_VAR(3, 1, 21));
        bulletin.datadesc.push_back(WR_VAR(0, 7, 30));
        bulletin.datadesc.push_back(WR_VAR(0, 7, 32));
        bulletin.datadesc.push_back(WR_VAR(0, 8, 21));
        bulletin.datadesc.push_back(WR_VAR(0, 4, 25));
        bulletin.datadesc.push_back(WR_VAR(0, 8, 43));
        bulletin.datadesc.push_back(WR_VAR(0, 8, 44));
        bulletin.datadesc.push_back(WR_VAR(0, 8, 45));
        bulletin.datadesc.push_back(WR_VAR(0, 8, 90));
        bulletin.datadesc.push_back(WR_VAR(0, 15, 23));
        bulletin.datadesc.push_back(WR_VAR(0, 8, 90));
        bulletin.datadesc.push_back(WR_VAR(0, 33, 3));

        bulletin.load_tables();
    }
    void to_subset(const Message& msg, wreport::Subset& subset) override
    {
        Template::to_subset(msg, subset);

        // Get the variable out of msg
        const Var* mainvar = NULL;
        int l1 = -1, p1 = -1;
        for (const auto& ctx : msg.data)
        {
            if (ctx.level.ltype1 != 103)
                continue;
            if (ctx.trange.pind != 0)
                continue;
            for (const auto& val : ctx.values)
            {
                const Var& var = *val;
                if (var.code() < WR_VAR(0, 15, 193) ||
                    var.code() > WR_VAR(0, 15, 198))
                    continue;
                if (mainvar != NULL)
                    error_consistency::throwf(
                        "found more than one variable to export in one "
                        "template: B%02d%03d and B%02d%03d",
                        WR_VAR_X(mainvar->code()), WR_VAR_Y(mainvar->code()),
                        WR_VAR_X(var.code()), WR_VAR_Y(var.code()));
                mainvar = &var;
                l1      = ctx.level.l1 / 1000;
                p1      = ctx.trange.p1;
            }
        }

        if (mainvar == NULL)
            throw error_consistency("found no pollution value to export");

        // Extract the various attributes
        const Var* attr_conf = mainvar->enqa(WR_VAR(0, 33, 3));
        const Var* attr_cas  = mainvar->enqa(WR_VAR(0, 8, 44));
        const Var* attr_pmc  = mainvar->enqa(WR_VAR(0, 8, 45));

        // Compute the constituent type
        int constituent;
        switch (mainvar->code())
        {
            case WR_VAR(0, 15, 193): constituent = 5; break;
            case WR_VAR(0, 15, 194): constituent = 0; break;
            case WR_VAR(0, 15, 195): constituent = 27; break;
            case WR_VAR(0, 15, 196): constituent = 4; break;
            case WR_VAR(0, 15, 197): constituent = 8; break;
            case WR_VAR(0, 15, 198): constituent = 26; break;
            default:
                error_consistency::throwf(
                    "found unknown variable type B%02d%03d when getting "
                    "constituent type",
                    WR_VAR_X(mainvar->code()), WR_VAR_Y(mainvar->code()));
        }

        // Compute the decimal scaling factor
        double value = mainvar->enqd();
        int scaled   = 0;
        int decscale = 0;

        if (value < 0)
            // Negative values are meaningless
            scaled = 0;
        else if (value == 0)
        {
            // Prevent divide by zero
            scaled = 0;
        }
        else
        {
            // static const int bits = 24;
            static const int maxscale = 126, minscale = -127;
            static const double numerator =
                (double)((1 << 24 /* aka, bits */) - 2);
            int factor = (int)floor(log10(numerator / value));

            if (factor > maxscale)
                // Factor is too big: collapse to 0
                scaled = 0;
            else if (factor < minscale)
                // Factor is too small
                error_consistency::throwf(
                    "scale factor is too small (%d): cannot encode because "
                    "value would not fit in",
                    factor);
            else
            {
                decscale = -factor;
                // fprintf(stderr, "scale: %d, unscaled: %e, scaled: %f\n",
                // decscale, value, rint(value * exp10(factor)));
                scaled   = (int)rint(value * intexp10(factor));
            }
        }

        // Add the variables to the subset

        /*  0 */ add(WR_VAR(0, 1, 19), sc::st_name);
        /*  1 */ add(WR_VAR(0, 1, 212), sc::poll_lcode);
        /*  2 */ add(WR_VAR(0, 1, 213), sc::poll_scode);
        /*  3 */ add(WR_VAR(0, 1, 214), sc::poll_gemscode);
        /*  4 */ add(WR_VAR(0, 1, 215), sc::poll_source);
        /*  5 */ add(WR_VAR(0, 1, 216), sc::poll_atype);
        /*  6 */ add(WR_VAR(0, 1, 217), sc::poll_ttype);
        do_D01011();
        do_D01013();
        /* 13 */ add(WR_VAR(0, 5, 1), sc::latitude);
        /* 14 */ add(WR_VAR(0, 6, 1), sc::longitude);
        /* 15 */ add(WR_VAR(0, 7, 30), sc::height_station);

        /* 16 */ subset.store_variable_i(WR_VAR(0, 7, 32), l1);
        /* 17 */ subset.store_variable_i(WR_VAR(0, 8, 21), 2);
        /* 18 */ subset.store_variable_i(WR_VAR(0, 4, 25), p1 / 60);
        /* 19 */ subset.store_variable_i(WR_VAR(0, 8, 43), constituent);
        if (attr_cas)
            /* 20 */ subset.store_variable(WR_VAR(0, 8, 44), *attr_cas);
        else
            /* 20 */ subset.store_variable_undef(WR_VAR(0, 8, 44));
        if (attr_pmc)
            /* 21 */ subset.store_variable(WR_VAR(0, 8, 45), *attr_pmc);
        else
            /* 21 */ subset.store_variable_undef(WR_VAR(0, 8, 45));
        /* 22 */ subset.store_variable_i(WR_VAR(0, 8, 90), decscale);
        /* 23 */ subset.store_variable_i(WR_VAR(0, 15, 23), scaled);
        /* Here it should have been bufrex_subset_store_variable_undef, but
         * instead someone decided that we have to store the integer value -127
         * instead */
        /* 24 */ subset.store_variable_i(WR_VAR(0, 8, 90), -127);
        if (attr_conf)
            /* 25 */ subset.store_variable(WR_VAR(0, 33, 3), *attr_conf);
        else
            /* 25 */ subset.store_variable_undef(WR_VAR(0, 33, 3));
    }
};

} // anonymous namespace

void register_pollution(TemplateRegistry& r);

void register_pollution(TemplateRegistry& r)
{
    r.register_factory(
        8, POLLUTION_NAME, POLLUTION_DESC,
        [](const dballe::ExporterOptions& opts, const Messages& msgs) {
            return unique_ptr<Template>(new Pollution(opts, msgs));
        });
}

} // namespace wr
} // namespace msg
} // namespace impl
} // namespace dballe
