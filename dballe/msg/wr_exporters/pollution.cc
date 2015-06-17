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

#include "msg/wr_codec.h"
#include "msg/msg.h"
#include "msg/context.h"
#include <wreport/bulletin.h>
#include <cstdlib>
#include <cmath>

using namespace wreport;
using namespace std;

#define POLLUTION_NAME "pollution"
#define POLLUTION_DESC "Pollution (8.171)"

namespace dballe {
namespace msg {
namespace wr {

namespace {

static double intexp10(unsigned x)
{
    switch (x)
    {
        case  0: return 1.0;
        case  1: return 10.0;
        case  2: return 100.0;
        case  3: return 1000.0;
        case  4: return 10000.0;
        case  5: return 100000.0;
        case  6: return 1000000.0;
        case  7: return 10000000.0;
        case  8: return 100000000.0;
        case  9: return 1000000000.0;
        case 10: return 10000000000.0;
        case 11: return 100000000000.0;
        case 12: return 1000000000000.0;
        case 13: return 10000000000000.0;
        case 14: return 100000000000000.0;
        case 15: return 1000000000000000.0;
        case 16: return 10000000000000000.0;
        default:
                 error_domain::throwf("computing double value of %u^10 is not yet supported", x);
    }
}

struct Pollution : public Template
{
    Pollution(const Exporter::Options& opts, const Messages& msgs)
        : Template(opts, msgs) {}

    virtual const char* name() const { return POLLUTION_NAME; }
    virtual const char* description() const { return POLLUTION_DESC; }

    void add(Varcode code, int shortcut)
    {
        const Var* var = msg->find_by_id(shortcut);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    void add(Varcode code, Varcode srccode, const Level& level, const Trange& trange)
    {
        const Var* var = msg->get(srccode, level, trange);
        if (var)
            subset->store_variable(code, *var);
        else
            subset->store_variable_undef(code);
    }

    virtual void setupBulletin(wreport::Bulletin& bulletin)
    {
        Template::setupBulletin(bulletin);

        bulletin.type = 8;
        bulletin.subtype = 255;
        bulletin.localsubtype = 171;

        if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
        {
            b->centre = 98;
            b->subcentre = 0;
            b->master_table = 13;
            b->local_table = 102;
        }

        // Data descriptor section
        bulletin.datadesc.clear();
        bulletin.datadesc.push_back(WR_VAR(3,  7,  11));
        bulletin.datadesc.push_back(WR_VAR(0,  1,  19));
        bulletin.datadesc.push_back(WR_VAR(0,  1, 212));
        bulletin.datadesc.push_back(WR_VAR(0,  1, 213));
        bulletin.datadesc.push_back(WR_VAR(0,  1, 214));
        bulletin.datadesc.push_back(WR_VAR(0,  1, 215));
        bulletin.datadesc.push_back(WR_VAR(0,  1, 216));
        bulletin.datadesc.push_back(WR_VAR(0,  1, 217));
        bulletin.datadesc.push_back(WR_VAR(3,  1,  11));
        bulletin.datadesc.push_back(WR_VAR(3,  1,  13));
        bulletin.datadesc.push_back(WR_VAR(3,  1,  21));
        bulletin.datadesc.push_back(WR_VAR(0,  7,  30));
        bulletin.datadesc.push_back(WR_VAR(0,  7,  32));
        bulletin.datadesc.push_back(WR_VAR(0,  8,  21));
        bulletin.datadesc.push_back(WR_VAR(0,  4,  25));
        bulletin.datadesc.push_back(WR_VAR(0,  8,  43));
        bulletin.datadesc.push_back(WR_VAR(0,  8,  44));
        bulletin.datadesc.push_back(WR_VAR(0,  8,  45));
        bulletin.datadesc.push_back(WR_VAR(0,  8,  90));
        bulletin.datadesc.push_back(WR_VAR(0, 15,  23));
        bulletin.datadesc.push_back(WR_VAR(0,  8,  90));
        bulletin.datadesc.push_back(WR_VAR(0, 33,   3));

        bulletin.load_tables();
    }
    virtual void to_subset(const Msg& msg, wreport::Subset& subset)
    {
        Template::to_subset(msg, subset);

        // Get the variable out of msg
        const Var* mainvar = NULL;
        int l1 = -1, p1 = -1;
        for (size_t i = 0; i < msg.data.size(); ++i)
        {
            const msg::Context& ctx = *msg.data[i];
            if (ctx.level.ltype1 != 103) continue;
            if (ctx.trange.pind != 0) continue;
            for (size_t j = 0; j < ctx.data.size(); ++j)
            {
                const Var& var = *ctx.data[j];
                if (var.code() < WR_VAR(0, 15, 193) || var.code() > WR_VAR(0, 15, 198)) continue;
                if (mainvar != NULL)
                    error_consistency::throwf("found more than one variable to export in one template: B%02d%03d and B%02d%03d",
                            WR_VAR_X(mainvar->code()), WR_VAR_Y(mainvar->code()),
                            WR_VAR_X(var.code()), WR_VAR_Y(var.code()));
                mainvar = &var;
                l1 = ctx.level.l1 / 1000;
                p1 = ctx.trange.p1;
            }
        }

        if (mainvar == NULL)
            throw error_consistency("found no pollution value to export");

        // Extract the various attributes
        const Var* attr_conf = mainvar->enqa(WR_VAR(0, 33,   3));
        const Var* attr_cas = mainvar->enqa(WR_VAR(0,  8,  44));
        const Var* attr_pmc = mainvar->enqa(WR_VAR(0,  8,  45));

        // Compute the constituent type
        int constituent;
        switch (mainvar->code())
        {
            case WR_VAR(0, 15, 193): constituent =  5; break;
            case WR_VAR(0, 15, 194): constituent =  0; break;
            case WR_VAR(0, 15, 195): constituent = 27; break;
            case WR_VAR(0, 15, 196): constituent =  4; break;
            case WR_VAR(0, 15, 197): constituent =  8; break;
            case WR_VAR(0, 15, 198): constituent = 26; break;
            default: error_consistency::throwf("found unknown variable type B%02d%03d when getting constituent type",
                             WR_VAR_X(mainvar->code()), WR_VAR_Y(mainvar->code()));
        }

        // Compute the decimal scaling factor
        double value = mainvar->enqd();
        int scaled = 0;
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
            //static const int bits = 24;
            static const int maxscale = 126, minscale = -127;
            static const double numerator = (double)((1 << 24 /* aka, bits */) - 2);
            int factor = (int)floor(log10(numerator/value));

            if (factor > maxscale)
                // Factor is too big: collapse to 0
                scaled = 0;
            else if (factor < minscale)
                // Factor is too small
                error_consistency::throwf("scale factor is too small (%d): cannot encode because value would not fit in", factor);
            else
            {
                decscale = -factor;
                //fprintf(stderr, "scale: %d, unscaled: %e, scaled: %f\n", decscale, value, rint(value * exp10(factor)));
                scaled = (int)rint(value * intexp10(factor));
            }
        }

        // Add the variables to the subset

        /*  0 */ add(WR_VAR(0,  1,  19), DBA_MSG_ST_NAME);
        /*  1 */ add(WR_VAR(0,  1, 212), DBA_MSG_POLL_LCODE);
        /*  2 */ add(WR_VAR(0,  1, 213), DBA_MSG_POLL_SCODE);
        /*  3 */ add(WR_VAR(0,  1, 214), DBA_MSG_POLL_GEMSCODE);
        /*  4 */ add(WR_VAR(0,  1, 215), DBA_MSG_POLL_SOURCE);
        /*  5 */ add(WR_VAR(0,  1, 216), DBA_MSG_POLL_ATYPE);
        /*  6 */ add(WR_VAR(0,  1, 217), DBA_MSG_POLL_TTYPE);
        do_D01011();
        do_D01013();
        /* 13 */ add(WR_VAR(0,  5,   1), DBA_MSG_LATITUDE);
        /* 14 */ add(WR_VAR(0,  6,   1), DBA_MSG_LONGITUDE);
        /* 15 */ add(WR_VAR(0,  7,  30), DBA_MSG_HEIGHT_STATION);


        /* 16 */ subset.store_variable_i(WR_VAR(0,  7, 32), l1);
        /* 17 */ subset.store_variable_i(WR_VAR(0,  8, 21), 2);
        /* 18 */ subset.store_variable_i(WR_VAR(0,  4, 25), p1/60);
        /* 19 */ subset.store_variable_i(WR_VAR(0,  8, 43), constituent);
        if (attr_cas)
            /* 20 */ subset.store_variable(WR_VAR(0,  8, 44), *attr_cas);
        else
            /* 20 */ subset.store_variable_undef(WR_VAR(0,  8, 44));
        if (attr_pmc)
            /* 21 */ subset.store_variable(WR_VAR(0,  8, 45), *attr_pmc);
        else
            /* 21 */ subset.store_variable_undef(WR_VAR(0,  8, 45));
        /* 22 */ subset.store_variable_i(WR_VAR(0,  8, 90), decscale);
        /* 23 */ subset.store_variable_i(WR_VAR(0, 15, 23), scaled);
        /* Here it should have been bufrex_subset_store_variable_undef, but
         * instead someone decided that we have to store the integer value -127 instead */
        /* 24 */ subset.store_variable_i(WR_VAR(0,  8, 90), -127);
        if (attr_conf)
            /* 25 */ subset.store_variable(WR_VAR(0, 33,  3), *attr_conf);
        else
            /* 25 */ subset.store_variable_undef(WR_VAR(0, 33,  3));
    }
};

struct PollutionFactory : public TemplateFactory
{
    PollutionFactory() { name = POLLUTION_NAME; description = POLLUTION_DESC; }

    std::unique_ptr<Template> make(const Exporter::Options& opts, const Messages& msgs) const
    {
        return unique_ptr<Template>(new Pollution(opts, msgs));
    }
};

} // anonymous namespace

void register_pollution(TemplateRegistry& r)
{
static const TemplateFactory* pollution = NULL;

    if (!pollution) pollution = new PollutionFactory;
 
    r.register_factory(pollution);
}

}
}
}

/* vim:set ts=4 sw=4: */
