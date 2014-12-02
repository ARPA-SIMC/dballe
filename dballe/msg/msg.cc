/*
 * dballe/msg - Hold an interpreted weather bulletin
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include "dballe/msg/vars.h"
#include <dballe/core/csv.h>
#include <wreport/codetables.h>
#include <wreport/notes.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sstream>
#include <iomanip>
#include <iostream>

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

msg::Context& Msg::obtain_station_context()
{
    return obtain_context(Level::ana(), Trange::ana());
}

msg::Context& Msg::obtain_context(const Level& lev, const Trange& tr)
{
    int pos = find_index(lev, tr);
    if (pos == -1)
    {
        unique_ptr<msg::Context> c(new msg::Context(lev, tr));
        msg::Context* res = c.get();
        add_context(std::move(c));
        return *res;
    }
    return *data[pos];
}

void Msg::add_context(unique_ptr<msg::Context> ctx)
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

const msg::Context* Msg::find_context_by_id(int id) const
{
    const MsgVarShortcut& v = shortcutTable[id];
    return find_context(Level(v.ltype1, v.l1, v.ltype2, v.l2), Trange(v.pind, v.p1, v.p2));
}

Var* Msg::edit_by_id(int id)
{
    const MsgVarShortcut& v = shortcutTable[id];
    return edit(v.code, Level(v.ltype1, v.l1, v.ltype2, v.l2), Trange(v.pind, v.p1, v.p2));
}

namespace {

struct VarContext
{
    // Extract datetime, lat, lon
    const Var* lat;
    const Var* lon;
    const Var* year;
    const Var* month;
    const Var* day;
    const Var* hour;
    const Var* minute;
    const Var* second;
    const Var* memo;
    const char* rep_memo;

    VarContext(const Msg& m)
    {
        // Extract datetime, lat, lon
        lat = m.get_latitude_var();
        lon = m.get_longitude_var();
        year = m.get_year_var();
        month = m.get_month_var();
        day = m.get_day_var();
        hour = m.get_hour_var();
        minute = m.get_minute_var();
        second = m.get_second_var();
        memo = m.get_rep_memo_var();
        if (memo)
            rep_memo = memo->enqc();
        else
            rep_memo = Msg::repmemo_from_type(m.type);
    }

    void print(ostream& out, msg::Context& c)
    {
        // Longitude
        if (lon)
            out << setprecision(5) << lon->enqd() << ",";
        else
            out << ",";

        // Latitude
        if (lat)
            out << setprecision(5) << lat->enqd() << ",";
        else
            out << ",";

        // Report type
        out << rep_memo << ",";

        if (c.level != Level::ana())
        {
            // Datetime
            out << setfill('0') << setw(4) << (year ? year->enq(0) : 0) << "-";
            out << setfill('0') << setw(2) << (month ? month->enq(0) : 0) << "-";
            out << setfill('0') << setw(2) << (day ? day->enq(0) : 0) << " ";
            out << setfill('0') << setw(2) << (hour ? hour->enq(0) : 0) << ":";
            out << setfill('0') << setw(2) << (minute ? minute->enq(0) : 0) << ":";
            out << setfill('0') << setw(2) << (second ? second->enq(0) : 0) << ",";

            // Level
            c.level.format(out, "");
            out << ",";

            // Time range
            c.trange.format(out, "");
            out << ",";
        } else
            out << ",,,,,,,,";
    }
};

}

void Msg::to_csv(std::ostream& out) const
{
    VarContext vc(*this);

    for (std::vector<msg::Context*>::const_iterator ci = data.begin();
            ci != data.end(); ++ci)
    {
        msg::Context& c = **ci;
        for (std::vector<wreport::Var*>::const_iterator vi = c.data.begin();
                vi != c.data.end(); ++vi)
        {
            Var& v = **vi;

            vc.print(out, c);

            out << format_code(v.code()) << ","; // B code
            csv_output_quoted_string(out, v.format(""));
            out << endl;

            // Add attribute columns
            for (const Var* a = v.next_attr(); a != NULL; a = a->next_attr())
            {
                vc.print(out, c);
                out << format_code(v.code()) << "." << format_code(a->code()) << ","; // B code
                csv_output_quoted_string(out, a->format(""));
                out << endl;
            }
        }
    }
}

void Msg::csv_header(std::ostream& out)
{
    out << "Longitude,Latitude,Report,Date,Level1,L1,Level2,L2,Time range,P1,P2,Varcode,Value" << endl;
}

bool Msg::from_csv(CSVReader& in)
{
    // Seek to beginning, skipping empty lines
    if (!in.move_to_data())
        return false;

    string old_lat, old_lon, old_rep, old_date;
    bool first = true;
    while (true)
    {
        // If there are empty lines, use them as separators
        if (in.cols.empty())
            break;
        if (in.cols.size() != 13)
            error_consistency::throwf("cannot parse CSV line has %zd fields instead of 13", in.cols.size());
        if (first)
        {
            // If we are the first run, initialse old_* markers with the contents of this line
            old_lon = in.cols[0];
            old_lat = in.cols[1];
            old_rep = in.cols[2];
            old_date = in.cols[3];
            set_latitude(strtod(old_lat.c_str(), NULL));
            set_longitude(strtod(old_lon.c_str(), NULL));
            set_rep_memo(old_rep.c_str());
            if (!old_date.empty())
                set_date(old_date.c_str());
            type = type_from_repmemo(old_rep.c_str());
            first = false;
        } else if (old_lon != in.cols[0] || old_lat != in.cols[1] || old_rep != in.cols[2]) {
            // If Longitude, Latitude or Report change, we are done
            break;
        } else if (old_date != in.cols[3]) {
            // In case of Date differences, we need to deal with station
            // information for which the date is left empty
            if (old_date.empty())
            {
                // previous lines were station information, next line is data
                old_date = in.cols[3];
                set_date(old_date.c_str());
            }
            else if (in.cols[3].empty())
                // previous lines were data, next line is station information
                ; // Keep the old date
            else
                // The date has changed, we are done.
                break;
        }

        //         0         1        2      3    4      5  6      7  8          9  10 11      12
        // out << "Longitude,Latitude,Report,Date,Level1,L1,Level2,L2,Time range,P1,P2,Varcode,Value" << endl;

        // Acquire the data
        Level lev(in.cols[4].c_str(), in.cols[5].c_str(), in.cols[6].c_str(), in.cols[7].c_str());
        if (in.cols[3].empty())
            // If we have station info, set level accordingly
            lev = Level::ana();
        Trange tr(in.cols[8].c_str(), in.cols[9].c_str(), in.cols[10].c_str());

        // Parse variable code
        if (in.cols[11].size() == 13)
        {
            // Bxxyyy.Bxxyyy: attribute
            Varcode vcode = descriptor_code(in.cols[11].substr(0, 6).c_str());
            // Find master variable
            wreport::Var* var = edit(vcode, lev, tr);
            if (var == NULL)
                error_consistency::throwf("cannot find corresponding variable for attribute %s", in.cols[11].c_str());

            Varcode acode = descriptor_code(in.cols[11].substr(7).c_str());
            auto_ptr<Var> attr(ap_newvar(acode));
            Varinfo info = attr->info();
            attr->set_from_formatted(in.cols[12].c_str());
            var->seta(attr);
        } else if (in.cols[11].size() == 6) {
            // Bxxyyy: variable
            Varcode vcode = descriptor_code(in.cols[11].c_str());
            unique_ptr<Var> var = newvar(vcode);
            var->set_from_formatted(in.cols[12].c_str());
            set(std::move(var), lev, tr);
        } else
            error_consistency::throwf("cannot parse variable code %s", in.cols[11].c_str());

        if (!in.next())
            break;
    }
    return true;
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

                    fprintf(out, "Sounding #%zd (level %d -", (i - data.begin()) + 1, vs);
                    if (vs & BUFR08042::MISSING) fprintf(out, " missing");
                    if (vs & BUFR08042::H2PRESS) fprintf(out, " h2press");
                    if (vs & BUFR08042::RESERVED) fprintf(out, " reserved");
                    if (vs & BUFR08042::REGIONAL) fprintf(out, " regional");
                    if (vs & BUFR08042::TOPWIND) fprintf(out, " topwind");
                    if (vs & BUFR08042::ENDMISSW) fprintf(out, " endmissw");
                    if (vs & BUFR08042::BEGMISSW) fprintf(out, " begmissw");
                    if (vs & BUFR08042::ENDMISSH) fprintf(out, " endmissh");
                    if (vs & BUFR08042::BEGMISSH) fprintf(out, " begmissh");
                    if (vs & BUFR08042::ENDMISST) fprintf(out, " endmisst");
                    if (vs & BUFR08042::BEGMISST) fprintf(out, " begmisst");
                    if (vs & BUFR08042::SIGWIND) fprintf(out, " sigwind");
                    if (vs & BUFR08042::SIGHUM) fprintf(out, " sighum");
                    if (vs & BUFR08042::SIGTEMP) fprintf(out, " sigtemp");
                    if (vs & BUFR08042::MAXWIND) fprintf(out, " maxwind");
                    if (vs & BUFR08042::TROPO) fprintf(out, " tropo");
                    if (vs & BUFR08042::STD) fprintf(out, " std");
                    if (vs & BUFR08042::SURFACE) fprintf(out, " surface");
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

static void context_summary(const msg::Context& c, ostream& out)
{
    out << "c(" << c.level << ", " << c.trange << ")";
}

unsigned Msg::diff(const Msg& msg) const
{
    unsigned diffs = 0;
    if (type != msg.type)
    {
        notes::logf("the messages have different type (first is %s (%d), second is %s (%d))\n",
                msg_type_name(type), type, msg_type_name(msg.type), msg.type);
        ++diffs;
    }

    size_t i1 = 0, i2 = 0;
    while (i1 < data.size() || i2 < msg.data.size())
    {
        if (i1 == data.size())
        {
            notes::log() << "Context ";
            context_summary(*msg.data[i2], notes::log());
            notes::log() << " exists only in the second message" << endl;
            ++i2;
            ++diffs;
        } else if (i2 == msg.data.size()) {
            notes::log() << "Context ";
            context_summary(*data[i1], notes::log());
            notes::log() << " exists only in the first message" << endl;
            ++i1;
            ++diffs;
        } else {
            int cmp = data[i1]->compare(*msg.data[i2]);
            if (cmp == 0)
            {
                diffs += data[i1]->diff(*msg.data[i2]);
                ++i1;
                ++i2;
            } else if (cmp < 0) {
                if (data[i1]->data.size() != 0)
                {
                    notes::log() << "Context ";
                    context_summary(*data[i1], notes::log());
                    notes::log() << " exists only in the first message" << endl;
                    ++diffs;
                }
                ++i1;
            } else {
                if (msg.data[i2]->data.size() != 0)
                {
                    notes::log() << "Context ";
                    context_summary(*msg.data[i2], notes::log());
                    notes::log() << " exists only in the second message" << endl;
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
    set(var_copy_without_unset_attrs(var, code), lev, tr);
}

void Msg::set(std::unique_ptr<Var> var, const Level& lev, const Trange& tr)
{
    msg::Context& ctx = obtain_context(lev, tr);
    ctx.set(std::move(var));
}

void Msg::seti(Varcode code, int val, int conf, const Level& lev, const Trange& tr)
{
    unique_ptr<Var> var(newvar(code, val));
    if (conf != -1)
        var->seta(ap_newvar(WR_VAR(0, 33, 7), conf));
    set(std::move(var), lev, tr);
}

void Msg::setd(Varcode code, double val, int conf, const Level& lev, const Trange& tr)
{
    unique_ptr<Var> var(newvar(code, val));
    if (conf != -1)
        var->seta(ap_newvar(WR_VAR(0, 33, 7), conf));
    set(std::move(var), lev, tr);
}

void Msg::setc(Varcode code, const char* val, int conf, const Level& lev, const Trange& tr)
{
    unique_ptr<Var> var(newvar(code, val));
    if (conf != -1)
        var->seta(ap_newvar(WR_VAR(0, 33, 7), conf));
    set(std::move(var), lev, tr);
}

void Msg::set_date(const char* date)
{
    int ye, mo, da, ho, mi, se;
    if (sscanf(date, "%04d-%02d-%02d %02d:%02d:%02d", &ye, &mo, &da, &ho, &mi, &se) != 6)
        error_consistency::throwf("cannot parse date/time string \"%s\"", date);
    set_year(ye);
    set_month(mo);
    set_day(da);
    set_hour(ho);
    set_minute(mi);
    set_second(se);
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
            unique_ptr<msg::Context> newctx(new msg::Context(ctx));
            dst.add_context(std::move(newctx));
            continue;
        }

        // FIXME: shouldn't this also set significance bits in the output level?
        for (size_t j = 0; j < ctx.data.size(); ++j)
        {
            unique_ptr<Var> copy(new Var(*ctx.data[j]));
            dst.set(std::move(copy), Level(ctx.level.ltype1, ctx.level.l1), ctx.trange);
        }
    }
}

#if 0
void Msg::sounding_unpack_levels(Msg& dst) const
{
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
#endif

void Msg::parse_date(int* values) const
{
    const msg::Context* c = find_station_context();
    if (!c)
    {
        for (int i = 0; i < 6; ++i)
            values[i] = -1;
        return;
    }

    int macros[] = { DBA_MSG_YEAR, DBA_MSG_MONTH, DBA_MSG_DAY, DBA_MSG_HOUR, DBA_MSG_MINUTE, DBA_MSG_SECOND };
    const char* names[] = { "year", "month", "day", "hour", "minute", "second" };

    for (int i = 0; i < 6; i++)
    {
        const wreport::Var* v = c->find_by_id(macros[i]);
        if (v)
            values[i] = v->enq(-1);
        else
            values[i] = -1;

        if (i > 0 && (values[i-1] == -1 && values[i] != -1))
            error_consistency::throwf("%s is unset but %s is set",
                    names[i-1], names[i]);
    }

    /* Now values is either 6 times -1, 6 values, or X values followed by 6-X times -1 */

    /* If one of the extremes has been selected, fill in the blanks */

    if (values[0] != -1)
    {
        values[1] = values[1] != -1 ? values[1] : 1;
        values[2] = values[2] != -1 ? values[2] : 1;
        values[3] = values[3] != -1 ? values[3] : 0;
        values[4] = values[4] != -1 ? values[4] : 0;
        values[5] = values[5] != -1 ? values[5] : 0;
    }
}

MatchedMsg::MatchedMsg(const Msg& m)
    : m(m)
{
}

MatchedMsg::~MatchedMsg()
{
}

matcher::Result MatchedMsg::match_var_id(int val) const
{
    for (std::vector<msg::Context*>::const_iterator ci = m.data.begin();
            ci != m.data.end(); ++ci)
        for (std::vector<wreport::Var*>::const_iterator vi = (*ci)->data.begin();
                vi != (*ci)->data.end(); ++vi)
            if (const Var* a = (*vi)->enqa(WR_VAR(0, 33, 195)))
                if (a->enqi() == val)
                    return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsg::match_station_id(int val) const
{
    if (const wreport::Var* var = m.find(WR_VAR(0, 1, 192), Level::ana(), Trange::ana()))
    {
        return var->enqi() == val ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

matcher::Result MatchedMsg::match_station_wmo(int block, int station) const
{
    const msg::Context* c = m.find_station_context();
    if (!c) return matcher::MATCH_NA;

    if (const wreport::Var* var = c->find_by_id(DBA_MSG_BLOCK))
    {
        // Match block
        if (var->enqi() != block) return matcher::MATCH_NO;

        // If station was not requested, we are done
        if (station == -1) return matcher::MATCH_YES;

        // Match station
        if (const wreport::Var* var = c->find_by_id(DBA_MSG_STATION))
        {
            if (var->enqi() != station) return matcher::MATCH_NO;
            return matcher::MATCH_YES;
        }
    }
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsg::match_date(const int* min, const int* max) const
{
    int date[6];
    m.parse_date(date);
    if (date[0] == -1) return matcher::MATCH_NA;
    return Matched::date_in_range(date, min, max);
}

matcher::Result MatchedMsg::match_coords(int latmin, int latmax, int lonmin, int lonmax) const
{
    const msg::Context* c = m.find_station_context();
    if (!c) return matcher::MATCH_NA;

    matcher::Result r1 = matcher::MATCH_NA;
    if (const wreport::Var* var = c->find_by_id(DBA_MSG_LATITUDE))
        r1 = Matched::int_in_range(var->enqi(), latmin, latmax);
    else if (latmin == MISSING_INT && latmax == MISSING_INT)
        r1 = matcher::MATCH_YES;

    matcher::Result r2 = matcher::MATCH_NA;
    if (const wreport::Var* var = c->find_by_id(DBA_MSG_LONGITUDE))
        r2 = Matched::int_in_range(var->enqi(), lonmin, lonmax);
    else if (lonmin == MISSING_INT && lonmax == MISSING_INT)
        r2 = matcher::MATCH_YES;

    if (r1 == matcher::MATCH_YES && r2 == matcher::MATCH_YES)
        return matcher::MATCH_YES;
    if (r1 == matcher::MATCH_NO || r2 == matcher::MATCH_NO)
        return matcher::MATCH_NO;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsg::match_rep_memo(const char* memo) const
{
    if (const Var* var = m.get_rep_memo_var())
    {
        const char* val = var->value();
        if (!val) return matcher::MATCH_NA;
        return strcmp(memo, val) == 0 ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
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
