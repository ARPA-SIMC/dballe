/*
 * dballe/msg - Hold an interpreted weather bulletin
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "msg.h"
#include "context.h"
#include "vars.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sstream>

using namespace wreport;
using namespace std;

namespace dballe {

const char* msg_type_name(MsgType type)
{
    switch (type)
    {
        case MSG_GENERIC: return "generic";
        case MSG_SYNOP: return "synop";
        case MSG_PILOT: return "pilot";
        case MSG_TEMP: return "temp";
        case MSG_TEMP_SHIP: return "temp_ship";
        case MSG_AIREP: return "airep";
        case MSG_AMDAR: return "amdar";
        case MSG_ACARS: return "acars";
        case MSG_SHIP: return "ship";
        case MSG_BUOY: return "buoy";
        case MSG_METAR: return "metar";
        case MSG_SAT: return "sat";
        case MSG_POLLUTION: return "pollution";
    }
    return "(unknown)";
}

Msg::Msg()
{
    type = MSG_GENERIC;
}

Msg::~Msg()
{
    for (vector<msg::Context*>::iterator i = data.begin(); i != data.end(); ++i)
        delete *i;
}

Msg::Msg(const Msg& m)
    : type(m.type)
{
    // Reserve space for the new contexts
    data.reserve(m.data.size());
    
    // Copy the contexts
    for (vector<msg::Context*>::const_iterator i = m.data.begin();
            i != m.data.end(); ++i)
        data.push_back(new msg::Context(**i));
}

Msg& Msg::operator=(const Msg& m)
{
    // Manage a = a
    if (this == &m) return *this;

    type = m.type;

    // Delete existing vars
    for (vector<msg::Context*>::iterator i = data.begin();
            i != data.end(); ++i)
        delete *i;
    data.clear();

    // Reserve space for the new contexts
    data.reserve(m.data.size());

    // Copy the contexts
    for (vector<msg::Context*>::const_iterator i = m.data.begin();
            i != m.data.end(); ++i)
        data.push_back(new msg::Context(**i));
    return *this;
}

void Msg::clear()
{
    type = MSG_GENERIC;
    for (vector<msg::Context*>::iterator i = data.begin(); i != data.end(); ++i)
        delete *i;
    data.clear();
}

int Msg::find_index(const Level& lev, const Trange& tr) const
{
    /* Binary search */
    int low = 0, high = data.size() - 1;
    while (low <= high)
    {
        int middle = low + (high - low)/2;
//fprintf(stderr, "DMFC lo %d hi %d mid %d\n", low, high, middle);
        int cmp = -data[middle]->compare(lev, tr);
        if (cmp < 0)
            high = middle - 1;
        else if (cmp > 0)
            low = middle + 1;
        else
            return middle;
    }
    return -1;
}

const msg::Context* Msg::find_context(const Level& lev, const Trange& tr) const
{
    int pos = find_index(lev, tr);
    if (pos == -1)
        return NULL;
    return data[pos];
}

const msg::Context* Msg::find_station_context() const
{
    return find_context(Level::ana(), Trange::ana());
}

msg::Context* Msg::edit_context(const Level& lev, const Trange& tr)
{
    int pos = find_index(lev, tr);
    if (pos == -1)
        return NULL;
    return data[pos];
}

msg::Context& Msg::obtain_context(const Level& lev, const Trange& tr)
{
    int pos = find_index(lev, tr);
    if (pos == -1)
    {
        auto_ptr<msg::Context> c(new msg::Context(lev, tr));
        msg::Context* res = c.get();
        add_context(c);
        return *res;
    }
    return *data[pos];
}

void Msg::add_context(auto_ptr<msg::Context> ctx)
{
    // Enlarge the data
    data.resize(data.size() + 1);

    /* Insertionsort.  Crude, but our datasets should be too small for an
     * RB-Tree to be worth */
    int pos;
    for (pos = data.size() - 1; pos > 0; --pos)
    {
        int cmp = data[pos - 1]->compare(*ctx);
        if (cmp > 0)
            data[pos] = data[pos - 1];
        else if (cmp == 0)
        {
            data.erase(data.begin() + pos);
            throw error_consistency("attempting to add a context that already exists in the message");
        }
        else
            break;
    }
    data[pos] = ctx.release();
}

bool Msg::remove_context(const Level& lev, const Trange& tr)
{
    int pos = find_index(lev, tr);
    if (pos == -1)
        return false;
    delete data[pos];
    data.erase(data.begin() + pos);
    return true;
}

const Var* Msg::find(Varcode code, const Level& lev, const Trange& tr) const
{
    const msg::Context* ctx = find_context(lev, tr);
    if (ctx == NULL) return NULL;
    return ctx->find(code);
}

wreport::Var* Msg::edit(wreport::Varcode code, const Level& lev, const Trange& tr)
{
    msg::Context* ctx = edit_context(lev, tr);
    if (ctx == NULL) return NULL;
    return ctx->edit(code);
}

const Var* Msg::find_by_id(int id) const
{
    const MsgVarShortcut& v = shortcutTable[id];
    return find(v.code, Level(v.ltype1, v.l1, v.ltype2, v.l2), Trange(v.pind, v.p1, v.p2));
}

Var* Msg::edit_by_id(int id)
{
    const MsgVarShortcut& v = shortcutTable[id];
    return edit(v.code, Level(v.ltype1, v.l1, v.ltype2, v.l2), Trange(v.pind, v.p1, v.p2));
}

void Msg::print(FILE* out) const
{
    fprintf(out, "%s message ", msg_type_name(type));

    if (data.empty())
    {
        fprintf(stderr, "(empty)\n");
        return;
    }
    fprintf(out, "with %zd contexts:\n", data.size());

    switch (type)
    {
        case MSG_PILOT:
        case MSG_TEMP:
        case MSG_TEMP_SHIP:
            for (vector<msg::Context*>::const_iterator i = data.begin(); i != data.end(); ++i)
            {
                const Var* vsig = (*i)->find_vsig();
                if (vsig != NULL)
                {
                    int vs = vsig->enqi();
                    const int VSIG_EXTRA = 128;
                    const int VSIG_SURFACE = 64;
                    const int VSIG_STANDARD = 32;
                    const int VSIG_TROPOPAUSE = 16;
                    const int VSIG_MAXWIND = 8;
                    const int VSIG_SIGTEMP = 4;
                    const int VSIG_SIGWIND = 2;
                    const int VSIG_MISSING = 1;

                    fprintf(out, "Sounding #%zd (level %d -", (i - data.begin()) + 1, vs);
                    if (vs & VSIG_EXTRA)
                        fprintf(out, " extra");
                    if (vs & VSIG_SURFACE)
                        fprintf(out, " surface");
                    if (vs & VSIG_STANDARD)
                        fprintf(out, " standard");
                    if (vs & VSIG_TROPOPAUSE)
                        fprintf(out, " tropopause");
                    if (vs & VSIG_MAXWIND)
                        fprintf(out, " maxwind");
                    if (vs & VSIG_SIGTEMP)
                        fprintf(out, " sigtemp");
                    if (vs & VSIG_SIGWIND)
                        fprintf(out, " sigwind");
                    if (vs & VSIG_MISSING)
                        fprintf(out, " missing");
                    fprintf(out, ") ");
                }
                (*i)->print(out);
            }
            break;
        default:
            for (vector<msg::Context*>::const_iterator i = data.begin(); i != data.end(); ++i)
                (*i)->print(out);
            break;
    }
}

static void context_summary(const msg::Context& c, FILE* out)
{
    stringstream str;
    str << c.level << ", " << c.trange;
    fprintf(out, "c(%s)", str.str().c_str());
}

unsigned Msg::diff(const Msg& msg, FILE* out) const
{
    unsigned diffs = 0;
    if (type != msg.type)
    {
        fprintf(out, "the messages have different type (first is %s (%d), second is %s (%d))\n",
                msg_type_name(type), type, msg_type_name(msg.type), msg.type);
        ++diffs;
    }
    
    size_t i1 = 0, i2 = 0;
    while (i1 < data.size() || i2 < msg.data.size())
    {
        if (i1 == data.size())
        {
            fprintf(out, "Context "); context_summary(*msg.data[i2], out);
            fprintf(out, " exists only in the second message\n");
            ++i2;
            ++diffs;
        } else if (i2 == msg.data.size()) {
            fprintf(out, "Context "); context_summary(*data[i1], out);
            fprintf(out, " exists only in the first message\n");
            ++i1;
            ++diffs;
        } else {
            int cmp = data[i1]->compare(*msg.data[i2]);
            if (cmp == 0)
            {
                diffs += data[i1]->diff(*msg.data[i2], out);
                ++i1;
                ++i2;
            } else if (cmp < 0) {
                if (data[i1]->data.size() != 0)
                {
                    fprintf(out, "Context "); context_summary(*data[i1], out);
                    fprintf(out, " exists only in the first message\n");
                    ++diffs;
                }
                ++i1;
            } else {
                if (msg.data[i2]->data.size() != 0)
                {
                    fprintf(out, "Context "); context_summary(*msg.data[i2], out);
                    fprintf(out, " exists only in the second message\n");
                    ++diffs;
                }
                ++i2;
            }
        }
    }
    return diffs;
}

void Msg::set_by_id(const wreport::Var& var, int shortcut)
{
    const MsgVarShortcut& v = shortcutTable[shortcut];
    return set(var, v.code, Level(v.ltype1, v.l1, v.ltype2, v.l2), Trange(v.pind, v.p1, v.p2));
}

void Msg::set(const Var& var, Varcode code, const Level& lev, const Trange& tr)
{
    auto_ptr<Var> copy(newvar(code));
    copy->copy_val(var); // Copy value performing conversions
    set(copy, lev, tr);
}

void Msg::set(std::auto_ptr<Var> var, const Level& lev, const Trange& tr)
{
    msg::Context& ctx = obtain_context(lev, tr);
    ctx.set(var);
}

void Msg::seti(Varcode code, int val, int conf, const Level& lev, const Trange& tr)
{
    auto_ptr<Var> var(newvar(code, val));
    if (conf != -1)
        var->seta(auto_ptr<Var>(newvar(WR_VAR(0, 33, 7), conf)));
    set(var, lev, tr);
}

void Msg::setd(Varcode code, double val, int conf, const Level& lev, const Trange& tr)
{
    auto_ptr<Var> var(newvar(code, val));
    if (conf != -1)
        var->seta(auto_ptr<Var>(newvar(WR_VAR(0, 33, 7), conf)));
    set(var, lev, tr);
}

void Msg::setc(Varcode code, const char* val, int conf, const Level& lev, const Trange& tr)
{
    auto_ptr<Var> var(newvar(code, val));
    if (conf != -1)
        var->seta(auto_ptr<Var>(newvar(WR_VAR(0, 33, 7), conf)));
    set(var, lev, tr);
}

MsgType Msg::type_from_repmemo(const char* repmemo)
{
    if (repmemo == NULL || repmemo[0] == 0) return MSG_GENERIC;
    switch (tolower(repmemo[0]))
    {
        case 'a':
            if (strcasecmp(repmemo+1, "cars")==0) return MSG_ACARS;
            if (strcasecmp(repmemo+1, "irep")==0) return MSG_AIREP;
            if (strcasecmp(repmemo+1, "mdar")==0) return MSG_AMDAR;
            break;
        case 'b':
            if (strcasecmp(repmemo+1, "uoy")==0) return MSG_BUOY;
            break;
        case 'm':
            if (strcasecmp(repmemo+1, "etar")==0) return MSG_METAR;
            break;
        case 'p':
            if (strcasecmp(repmemo+1, "ilot")==0) return MSG_PILOT;
            if (strcasecmp(repmemo+1, "ollution")==0) return MSG_POLLUTION;
            break;
        case 's':
            if (strcasecmp(repmemo+1, "atellite")==0) return MSG_SAT;
            if (strcasecmp(repmemo+1, "hip")==0) return MSG_SHIP;
            if (strcasecmp(repmemo+1, "ynop")==0) return MSG_SYNOP;
            break;
        case 't':
            if (strcasecmp(repmemo+1, "emp")==0) return MSG_TEMP;
            if (strcasecmp(repmemo+1, "empship")==0) return MSG_TEMP_SHIP;
            break;
    }
    return MSG_GENERIC;
}

const char* Msg::repmemo_from_type(MsgType type)
{
    switch (type)
    {
        case MSG_SYNOP:     return "synop";
        case MSG_METAR:     return "metar";
        case MSG_SHIP:      return "ship";
        case MSG_BUOY:      return "buoy";
        case MSG_AIREP:     return "airep";
        case MSG_AMDAR:     return "amdar";
        case MSG_ACARS:     return "acars";
        case MSG_PILOT:     return "pilot";
        case MSG_TEMP:      return "temp";
        case MSG_TEMP_SHIP: return "tempship";
        case MSG_SAT:       return "satellite";
        case MSG_POLLUTION: return "pollution";
        case MSG_GENERIC:
        default:            return "generic";
    }
}

void Msg::sounding_pack_levels(Msg& dst) const
{
    dst.clear();
    dst.type = type;

    for (size_t i = 0; i < data.size(); ++i)
    {
        const msg::Context& ctx = *data[i];

        // If it is not a sounding level, just copy it
        if (ctx.find_vsig() == NULL)
        {
            auto_ptr<msg::Context> newctx(new msg::Context(ctx));
            dst.add_context(newctx);
            continue;
        }

        // FIXME: shouldn't this also set significance bits in the output level?
        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            auto_ptr<Var> copy(new Var(*ctx.data[j]));
            dst.set(copy, Level(ctx.level.ltype1, ctx.level.l1), ctx.trange);
        }
    }
}

void Msg::sounding_unpack_levels(Msg& dst) const
{
    const int VSIG_MISSING = 1;
    const int VSIG_SIGWIND = 2;     /* 6 */
    const int VSIG_SIGTEMP = 4;     /* 5 */
    const int VSIG_MAXWIND = 8;     /* 4 */
    const int VSIG_TROPOPAUSE = 16; /* 3 */
    const int VSIG_STANDARD = 32;   /* 2 */
    const int VSIG_SURFACE = 64;    /* 1 */

    dst.clear();
    dst.type = type;

    for (size_t i = 0; i < data.size(); ++i)
    {
        const msg::Context& ctx = *data[i];

        const Var* vsig_var = ctx.find_vsig();
        if (!vsig_var)
        {
            auto_ptr<msg::Context> newctx(new msg::Context(ctx));
            dst.add_context(newctx);
            continue;
        }

        int vsig = vsig_var->enqi();
        if (vsig & VSIG_MISSING)
        {
            // If there is no vsig, then we consider it a normal level
            auto_ptr<msg::Context> newctx(new msg::Context(ctx));
            dst.add_context(newctx);
            continue;
        }

        /* DBA_RUN_OR_GOTO(fail, dba_var_enqi(msg->data[i].var_press, &press)); */

        /* TODO: delete the dba_msg_datum that do not belong in that level */

        if (vsig & VSIG_SIGWIND)
        {
            auto_ptr<msg::Context> copy(new msg::Context(ctx));
            copy->level.l2 = 6;
            dst.add_context(copy);
        }
        if (vsig & VSIG_SIGTEMP)
        {
            auto_ptr<msg::Context> copy(new msg::Context(ctx));
            copy->level.l2 = 5;
            dst.add_context(copy);
        }
        if (vsig & VSIG_MAXWIND)
        {
            auto_ptr<msg::Context> copy(new msg::Context(ctx));
            copy->level.l2 = 4;
            dst.add_context(copy);
        }
        if (vsig & VSIG_TROPOPAUSE)
        {
            auto_ptr<msg::Context> copy(new msg::Context(ctx));
            copy->level.l2 = 3;
            dst.add_context(copy);
        }
        if (vsig & VSIG_STANDARD)
        {
            auto_ptr<msg::Context> copy(new msg::Context(ctx));
            copy->level.l2 = 2;
            dst.add_context(copy);
        }
        if (vsig & VSIG_SURFACE)
        {
            auto_ptr<msg::Context> copy(new msg::Context(ctx));
            copy->level.l2 = 1;
            dst.add_context(copy);
        }
    }
}


#if 0

static dba_err dba_msg_add_context(dba_msg msg, dba_msg_context ctx)
{
    dba_err err;
    dba_msg_context copy = NULL;
    DBA_RUN_OR_RETURN(dba_msg_context_copy(ctx, &copy));
    DBA_RUN_OR_GOTO(fail, dba_msg_add_context_nocopy(msg, copy));
    return dba_error_ok();

fail:
    if (copy)
        dba_msg_context_delete(copy);
    return err;
}

dba_err dba_msg_set_nocopy_by_id(dba_msg msg, dba_var var, int id)
{
    dba_msg_var v = &dba_msg_vartable[id];
    return dba_msg_set_nocopy(msg, var, v->ltype1, v->l1, v->ltype2, v->l2, v->pind, v->p1, v->p2);
}

dba_err dba_msg_set_by_id(dba_msg msg, dba_var var, int id)
{
    dba_msg_var v = &dba_msg_vartable[id];
    dba_err err;
    dba_var copy = NULL;

    /* Make a copy of the variable, to give it to dba_msg_add_nocopy */
    DBA_RUN_OR_RETURN(dba_var_create_local(v->code, &copy));
    /* Use copy_val to ensure we get the variable code we want */
    DBA_RUN_OR_GOTO(fail, dba_var_copy_val(copy, var));

    DBA_RUN_OR_GOTO(fail, dba_msg_set_nocopy(msg, copy, v->ltype1, v->l1, v->ltype2, v->l2, v->pind, v->p1, v->p2));

    return dba_error_ok();

fail:
    if (copy != NULL)
        dba_var_delete(copy);
    return err;
}



dba_msg_type dba_msg_get_type(dba_msg msg)
{
    return msg->type;
}



#endif

} // namespace dballe

/* vim:set ts=4 sw=4: */
