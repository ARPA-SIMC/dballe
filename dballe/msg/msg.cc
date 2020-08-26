#include "msg.h"
#include "context.h"
#include "dballe/cursor.h"
#include "dballe/core/shortcuts.h"
#include "dballe/msg/cursor.h"
#include "dballe/core/var.h"
#include "dballe/core/csv.h"
#include <wreport/codetables.h>
#include <wreport/notes.h>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <iostream>

using namespace wreport;
using namespace std;

namespace dballe {
namespace impl {

namespace msg {

Contexts::const_iterator Contexts::find(const Level& level, const Trange& trange) const
{
    /* Binary search */
    if (m_contexts.empty())
        return m_contexts.end();

    const_iterator low = m_contexts.begin(), high = (m_contexts.end() - 1);
    while (low <= high)
    {
        const_iterator middle = low + (high - low) / 2;
        int cmp = middle->compare(level, trange);
        if (cmp > 0)
            high = middle - 1;
        else if (cmp < 0)
            low = middle + 1;
        else
            return middle;
    }
    return m_contexts.end();
}

Contexts::iterator Contexts::find(const Level& level, const Trange& trange)
{
    /* Binary search */
    if (m_contexts.empty())
        return m_contexts.end();

    iterator low = m_contexts.begin(), high = (m_contexts.end() - 1);
    while (low <= high)
    {
        iterator middle = low + (high - low) / 2;
        int cmp = middle->compare(level, trange);
        if (cmp > 0)
            high = middle - 1;
        else if (cmp < 0)
            low = middle + 1;
        else
            return middle;
    }
    return m_contexts.end();
}

Contexts::iterator Contexts::insert_new(const Level& level, const Trange& trange)
{
    // Enlarge the buffer
    m_contexts.emplace_back(level, trange);

    // Insertionsort
    iterator pos;
    for (pos = m_contexts.end() - 1; pos > m_contexts.begin(); --pos)
    {
        if ((pos - 1)->compare(*pos) > 0)
            std::swap(*pos, *(pos - 1));
        else
            break;
    }
    return pos;
}

Contexts::iterator Contexts::obtain(const Level& level, const Trange& trange)
{
    iterator pos = find(level, trange);
    if (pos != end())
        return pos;
    return insert_new(level, trange);
}

bool Contexts::drop(const Level& level, const Trange& trange)
{
    iterator pos = find(level, trange);
    if (pos == end())
        return false;
    m_contexts.erase(pos);
    return true;
}


Messages messages_from_csv(CSVReader& in)
{
    Messages res;
    string old_rep;
    bool first = true;
    while (true)
    {
        // Seek to beginning, skipping empty lines
        if (!in.move_to_data())
            return res;

        if (in.cols.size() != 13)
            error_consistency::throwf("cannot parse CSV line has %zd fields instead of 13", in.cols.size());
        if (first)
        {
            // If we are the first run, initialse old_* markers with the contents of this line
            old_rep = in.cols[2];
            first = false;
        } else if (old_rep != in.cols[2])
            // If Report changes, we are done
            break;

        auto msg = make_shared<impl::Message>();
        bool has_next = msg->from_csv(in);
        res.emplace_back(std::move(msg));
        if (!has_next)
            break;
    }
    return res;
}

void messages_to_csv(const Messages& msgs, CSVWriter& out)
{
    for (const auto& i: msgs)
        impl::Message::downcast(i)->to_csv(out);
}

unsigned messages_diff(const Messages& msgs1, const Messages& msgs2)
{
    unsigned diffs = 0;
    if (msgs1.size() != msgs2.size())
    {
        notes::logf("the message groups contain a different number of messages (first is %zd, second is %zd)\n",
                msgs1.size(), msgs2.size());
        ++diffs;
    }
    size_t count = min(msgs1.size(), msgs2.size());
    for (size_t i = 0; i < count; ++i)
        diffs += msgs1[i]->diff(*msgs2[i]);
    return diffs;
}

void messages_print(const Messages& msgs, FILE* out)
{
    for (unsigned i = 0; i < msgs.size(); ++i)
    {
        fprintf(out, "Subset %d:\n", i);
        msgs[i]->print(out);
    }
}

}


const Message& Message::downcast(const dballe::Message& o)
{
    const Message* ptr = dynamic_cast<const Message*>(&o);
    if (!ptr)
        throw error_consistency("Message given is not an impl::Message");
    return *ptr;
}

Message& Message::downcast(dballe::Message& o)
{
    Message* ptr = dynamic_cast<Message*>(&o);
    if (!ptr)
        throw error_consistency("Message given is not an impl::Message");
    return *ptr;
}

std::shared_ptr<Message> Message::downcast(std::shared_ptr<dballe::Message> o)
{
    auto ptr = dynamic_pointer_cast<Message>(o);
    if (!ptr)
        throw error_consistency("Message given is not a Message");
    return ptr;
}

std::unique_ptr<dballe::Message> Message::clone() const
{
    return unique_ptr<dballe::Message>(new Message(*this));
}

Datetime Message::get_datetime() const
{
    int ye = MISSING_INT, mo=MISSING_INT, da=MISSING_INT, ho=MISSING_INT, mi=MISSING_INT, se=MISSING_INT;
    if (const Var* v = station_data.maybe_var(sc::year.code))
        ye = v->enqi();
    if (const Var* v = station_data.maybe_var(sc::month.code))
        mo = v->enqi();
    if (const Var* v = station_data.maybe_var(sc::day.code))
        da = v->enqi();
    if (const Var* v = station_data.maybe_var(sc::hour.code))
        ho = v->enqi();
    if (const Var* v = station_data.maybe_var(sc::minute.code))
        mi = v->enqi();
    if (const Var* v = station_data.maybe_var(sc::second.code))
        se = v->enqi();

    if (ye == MISSING_INT)
        return Datetime();

    if (mo == MISSING_INT)
        throw error_consistency("no month information found in message");
    if (da == MISSING_INT)
        throw error_consistency("no day information found in message");
    if (ho == MISSING_INT)
        throw error_consistency("no hour information found in message");
    if (mi == MISSING_INT)
        throw error_consistency("no minute information found in message");
    if (se == MISSING_INT)
        se = 0;

    // Accept an hour of 24:00:00 and move it to 00:00:00 of the following
    // day
    Datetime::normalise_h24(ye, mo, da, ho, mi, se);

    return Datetime(ye, mo, da, ho, mi, se);
}

Coords Message::get_coords() const
{
    const Var* lat = station_data.maybe_var(sc::latitude.code);
    const Var* lon = station_data.maybe_var(sc::longitude.code);
    if (lat && lon)
        return Coords(lat->enqd(), lon->enqd());
    else
        return Coords();
}

Ident Message::get_ident() const
{
    const Var* ident = station_data.maybe_var(sc::ident.code);
    if (ident)
        return Ident(ident->enqc());
    else
        return Ident();
}

std::string Message::get_report() const
{
    // Postprocess extracting rep_memo information
    const Var* rep_memo = station_data.maybe_var(sc::rep_memo.code);
    if (rep_memo)
        return rep_memo->enqc();
    else
        return repmemo_from_type(type);
}

void Message::clear()
{
    type = MessageType::GENERIC;
    station_data.clear();
    data.clear();
}

const msg::Context* Message::find_context(const Level& lev, const Trange& tr) const
{
    if (lev.is_missing() && tr.is_missing())
        throw std::runtime_error("find_contexts called for station level, but this is no longer supported");

    auto i = data.find(lev, tr);
    if (i == data.end())
        return nullptr;
    return &*i;
}

const Values& Message::find_station_context() const
{
    return station_data;
}

msg::Context* Message::edit_context(const Level& lev, const Trange& tr)
{
    if (lev.is_missing() && tr.is_missing())
        throw std::runtime_error("find_contexts called for station level, but this is no longer supported");

    auto i = data.find(lev, tr);
    if (i == data.end())
        return nullptr;
    return &*i;
}

msg::Context& Message::obtain_context(const Level& lev, const Trange& tr)
{
    if (lev.is_missing() && tr.is_missing())
        throw std::runtime_error("find_contexts called for station level, but this is no longer supported");

    auto i = data.obtain(lev, tr);
    return *i;
}

bool Message::remove_context(const Level& lev, const Trange& tr)
{
    return data.drop(lev, tr);
}

const Var* Message::get_impl(const Level& lev, const Trange& tr, Varcode code) const
{
    if (lev.is_missing() && tr.is_missing())
        return station_data.maybe_var(code);

    auto ctx = data.find(lev, tr);
    if (ctx == data.end())
        return nullptr;
    return ctx->values.maybe_var(code);
}

bool Message::foreach_var(std::function<bool(const Level&, const Trange&, const wreport::Var&)> dest) const
{
    for (const auto& var: station_data)
        if (!dest(Level(), Trange(), *var))
            return false;

    for (const auto& ctx: data)
        for (const auto& var: ctx.values)
            if (!dest(ctx.level, ctx.trange, *var))
                return false;

    return true;
}

wreport::Var* Message::edit(wreport::Varcode code, const Level& lev, const Trange& tr)
{
    if (lev.is_missing() && tr.is_missing())
        throw std::runtime_error("find_contexts called for station level, but this is no longer supported");

    auto ctx = data.find(lev, tr);
    if (ctx == data.end())
        return nullptr;

    return ctx->values.maybe_var(code);
}

const Var* Message::get(const Shortcut& shortcut) const
{
    if (shortcut.station_data)
        return station_data.maybe_var(shortcut.code);
    return get(shortcut.level, shortcut.trange, shortcut.code);
}

std::shared_ptr<dballe::CursorStation> Message::query_stations(const Query& query) const
{
    return std::make_shared<msg::CursorStation>(*this);
}

std::shared_ptr<dballe::CursorStationData> Message::query_station_data(const Query& query) const
{
    return std::make_shared<msg::CursorStationData>(*this);
}

std::shared_ptr<dballe::CursorData> Message::query_data(const Query& query) const
{
    return std::make_shared<msg::CursorData>(*this);
}

std::shared_ptr<dballe::CursorData> Message::query_station_and_data(const Query& query) const
{
    return std::make_shared<msg::CursorData>(*this, true);
}

namespace {

struct VarContext
{
    const impl::Message& msg;
    // Extract datetime, lat, lon
    const Var* lat;
    const Var* lon;
    const Var* memo;
    const char* rep_memo;

    VarContext(const impl::Message& m) : msg(m)
    {
        // Extract datetime, lat, lon
        lat = m.station_data.maybe_var(sc::latitude.code);
        lon = m.station_data.maybe_var(sc::longitude.code);
        memo = m.station_data.maybe_var(sc::rep_memo.code);
        if (memo)
            rep_memo = memo->enqc();
        else
            rep_memo = impl::Message::repmemo_from_type(m.type);
    }

    void print(CSVWriter& out, const Level& level, const Trange& trange)
    {
        // Longitude
        if (lon)
            out.add_var_value_formatted(*lon);
        else
            out.add_value_empty();

        // Latitude
        if (lat)
            out.add_var_value_formatted(*lat);
        else
            out.add_value_empty();

        // Report type
        out.add_value(rep_memo);

        if (level != Level())
        {
            // Datetime
            msg.get_datetime().to_csv_iso8601(out, ' ');

            // Level
            level.to_csv(out);

            // Time range
            trange.to_csv(out);
        } else {
            for (int i = 0; i < 8; ++i)
                out.add_value_empty();
        }
    }
};

}

void Message::to_csv(CSVWriter& out) const
{
    VarContext vc(*this);

    for (const auto& val: station_data)
    {
        vc.print(out, Level(), Trange());

        out.add_value(val->code()); // B code
        out.add_var_value_formatted(*val);
        out.flush_row();

        // Add attribute columns
        for (const Var* a = val->next_attr(); a != NULL; a = a->next_attr())
        {
            vc.print(out, Level(), Trange());
            out.add_value(varcode_format(val->code()) + "." + varcode_format(a->code())); // B code
            out.add_var_value_formatted(*a);
            out.flush_row();
        }
    }

    for (const auto& ctx: data)
    {
        for (const auto& val: ctx.values)
        {
            const Var& v = *val;

            vc.print(out, ctx.level, ctx.trange);

            out.add_value(v.code()); // B code
            out.add_var_value_formatted(v);
            out.flush_row();

            // Add attribute columns
            for (const Var* a = v.next_attr(); a != NULL; a = a->next_attr())
            {
                vc.print(out, ctx.level, ctx.trange);
                out.add_value(varcode_format(v.code()) + "." + varcode_format(a->code())); // B code
                out.add_var_value_formatted(*a);
                out.flush_row();
            }
        }
    }
}

void Message::csv_header(CSVWriter& out)
{
    out.add_value("longitude");
    out.add_value("latitude");
    out.add_value("report");
    out.add_value("date");
    out.add_value("level");
    out.add_value("l");
    out.add_value("level");
    out.add_value("l");
    out.add_value("time rang");
    out.add_value("p");
    out.add_value("p");
    out.add_value("varcod");
    out.add_value("value");
    out.flush_row();
}

namespace {
// Convert a string to an integer value, returning MISSING_INT if the string is
// empty or "-"
int str_to_int(const std::string& str)
{
    if (str.empty() || str == "-")
        return MISSING_INT;
    else
        return stoi(str);
}
}

bool Message::from_csv(CSVReader& in)
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
                set_datetime(Datetime::from_iso8601(old_date.c_str()));
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
                set_datetime(Datetime::from_iso8601(old_date.c_str()));
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
        Level lev(str_to_int(in.cols[4]), str_to_int(in.cols[5]), str_to_int(in.cols[6]), str_to_int(in.cols[7]));
        if (in.cols[3].empty())
            // If we have station info, set level accordingly
            lev = Level();
        Trange tr(str_to_int(in.cols[8]), str_to_int(in.cols[9]), str_to_int(in.cols[10]));

        // Parse variable code
        if (in.cols[11].size() == 13)
        {
            // Bxxyyy.Bxxyyy: attribute
            Varcode vcode = varcode_parse(in.cols[11].substr(0, 6).c_str());
            // Find master variable
            wreport::Var* var = edit(vcode, lev, tr);
            if (var == NULL)
                error_consistency::throwf("cannot find corresponding variable for attribute %s", in.cols[11].c_str());

            Varcode acode = varcode_parse(in.cols[11].substr(7).c_str());
            auto attr = newvar(acode);
            attr->setf(in.cols[12].c_str());
            var->seta(move(attr));
        } else if (in.cols[11].size() == 6) {
            // Bxxyyy: variable
            Varcode vcode = varcode_parse(in.cols[11].c_str());
            unique_ptr<Var> var = newvar(vcode);
            var->setf(in.cols[12].c_str());
            if (lev.is_missing() && tr.is_missing())
                station_data.set(std::move(var));
            else
                set(lev, tr, std::move(var));
        } else
            error_consistency::throwf("cannot parse variable code %s", in.cols[11].c_str());

        if (!in.next())
            break;
    }
    return true;
}

void Message::print(FILE* out) const
{
    fprintf(out, "%s message, ", format_message_type(type));
    get_coords().print(out, ", ");

    auto ident = get_ident();
    if (!ident.is_missing())
        fprintf(out, "ident: %s, ", (const char*)ident);

    auto dt = get_datetime();
    if (dt.is_missing())
        fprintf(out, "dt: missing, ");
    else
    {
        fprintf(out, "dt: ");
        dt.print_iso8601(out, 'T', ", ");
    }

    if (data.empty())
        fprintf(out, "(no data)\n");
    else
        fprintf(out, "%zd contexts:\n", data.size() + 1);

    fprintf(out, "Level ");
    Level().print(out, "-", " tr ");
    Trange().print(out, "-", "\n");
    station_data.print(out);

    switch (type)
    {
        case MessageType::PILOT:
        case MessageType::TEMP:
        case MessageType::TEMP_SHIP:
        {
            unsigned sounding_idx = 0;
            for (auto i = data.cbegin(); i != data.cend(); ++i)
            {
                if (const Var* vsig = i->find_vsig())
                {
                    int vs = vsig->enqi();
                    fprintf(out, "Sounding #%u (level %d -", ++sounding_idx, vs);
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
                i->print(out);
            }
            break;
        }
        default:
            for (const auto& ctx: data)
                ctx.print(out);
            break;
    }
}

static void context_summary(const msg::Context& c, ostream& out)
{
    out << "c(" << c.level << ", " << c.trange << ")";
}

static void station_data_summary(const Var& var, ostream& out)
{
    out << "Station variable ";
    out << varcode_format(var.code()) << "[" << var.info()->desc << "]";
}


unsigned Message::diff(const dballe::Message& o) const
{
    const Message& msg = downcast(o);

    unsigned diffs = 0;
    if (type != msg.type)
    {
        notes::logf("the messages have different type (first is %s (%d), second is %s (%d))\n",
                format_message_type(type), static_cast<int>(type), format_message_type(msg.type), static_cast<int>(msg.type));
        ++diffs;
    }

    // Compare station data
    auto v1 = station_data.cbegin();
    auto v2 = msg.station_data.cbegin();
    while (v1 != station_data.cend() && v2 != msg.station_data.cend())
    {
        // Skip second=0 in station context
        if (v1->code() == WR_VAR(0, 4, 6) && (*v1)->enqi() == 0) ++v1;
        if (v2->code() == WR_VAR(0, 4, 6) && (*v2)->enqi() == 0) ++v2;
        if (v1 == station_data.end() || v2 == msg.station_data.end())
            break;

        int cmp = (int)v1->code() - (int)v2->code();
        if (cmp == 0)
        {
            diffs += (*v1)->diff(**v2);
            ++v1;
            ++v2;
        } else if (cmp < 0) {
            if (!(*v1)->isset())
            {
                station_data_summary(**v1, notes::log());
                notes::log() << " exists only in the first message" << endl;
                ++diffs;
            }
            ++v1;
        } else {
            if (!(*v2)->isset())
            {
                station_data_summary(**v2, notes::log());
                notes::log() << " exists only in the second message" << endl;
                ++diffs;
            }
            ++v2;
        }
    }
    while (v1 != station_data.end())
    {
        station_data_summary(**v1, notes::log());
        notes::log() << " exists only in the first message" << endl;
        ++v1;
        ++diffs;
    }
    while (v2 != msg.station_data.end())
    {
        station_data_summary(**v2, notes::log());
        notes::log() << " exists only in the second message" << endl;
        ++v2;
        ++diffs;
    }

    auto i1 = data.cbegin();
    auto i2 = msg.data.cbegin();
    while (i1 != data.cend() && i2 != msg.data.cend())
    {
        int cmp = i1->compare(*i2);
        if (cmp == 0)
        {
            diffs += i1->diff(*i2);
            ++i1;
            ++i2;
        } else if (cmp < 0) {
            if (!i1->values.empty())
            {
                notes::log() << "Context ";
                context_summary(*i1, notes::log());
                notes::log() << " exists only in the first message" << endl;
                ++diffs;
            }
            ++i1;
        } else {
            if (!i2->values.empty())
            {
                notes::log() << "Context ";
                context_summary(*i2, notes::log());
                notes::log() << " exists only in the second message" << endl;
                ++diffs;
            }
            ++i2;
        }
    }

    while (i1 != data.end())
    {
        notes::log() << "Context ";
        context_summary(*i1, notes::log());
        notes::log() << " exists only in the first message" << endl;
        ++i1;
        ++diffs;
    }


    while (i2 != msg.data.end())
    {
        notes::log() << "Context ";
        context_summary(*i2, notes::log());
        notes::log() << " exists only in the second message" << endl;
        ++i2;
        ++diffs;
    }

    return diffs;
}

void Message::set(const Shortcut& shortcut, const wreport::Var& var)
{
    if (shortcut.station_data)
    {
        if (shortcut.code == var.code())
            station_data.set(var);
        else
            station_data.set(var_copy_without_unset_attrs(var, shortcut.code));
    }
    else
        set(shortcut.level, shortcut.trange, shortcut.code, var);
}

void Message::set_impl(const Level& lev, const Trange& tr, std::unique_ptr<Var> var)
{
    if (lev.is_missing() && tr.is_missing())
        station_data.set(std::move(var));
    else
    {
        msg::Context& ctx = obtain_context(lev, tr);
        ctx.values.set(std::move(var));
    }
}

void Message::seti(const Level& lev, const Trange& tr, Varcode code, int val, int conf)
{
    unique_ptr<Var> var(newvar(code, val));
    if (conf != -1)
        var->seta(newvar(WR_VAR(0, 33, 7), conf));
    if (lev.is_missing() && tr.is_missing())
        station_data.set(std::move(var));
    else
        set(lev, tr, std::move(var));
}

void Message::setd(const Level& lev, const Trange& tr, Varcode code, double val, int conf)
{
    unique_ptr<Var> var(newvar(code, val));
    if (conf != -1)
        var->seta(newvar(WR_VAR(0, 33, 7), conf));
    if (lev.is_missing() && tr.is_missing())
        station_data.set(std::move(var));
    else
        set(lev, tr, std::move(var));
}

void Message::setc(const Level& lev, const Trange& tr, Varcode code, const char* val, int conf)
{
    unique_ptr<Var> var(newvar(code, val));
    if (conf != -1)
        var->seta(newvar(WR_VAR(0, 33, 7), conf));
    if (lev.is_missing() && tr.is_missing())
        station_data.set(std::move(var));
    else
        set(lev, tr, std::move(var));
}

MessageType Message::type_from_repmemo(const char* repmemo)
{
    if (repmemo == NULL || repmemo[0] == 0) return MessageType::GENERIC;
    switch (tolower(repmemo[0]))
    {
        case 'a':
            if (strcasecmp(repmemo+1, "cars")==0) return MessageType::ACARS;
            if (strcasecmp(repmemo+1, "irep")==0) return MessageType::AIREP;
            if (strcasecmp(repmemo+1, "mdar")==0) return MessageType::AMDAR;
            break;
        case 'b':
            if (strcasecmp(repmemo+1, "uoy")==0) return MessageType::BUOY;
            break;
        case 'm':
            if (strcasecmp(repmemo+1, "etar")==0) return MessageType::METAR;
            break;
        case 'p':
            if (strcasecmp(repmemo+1, "ilot")==0) return MessageType::PILOT;
            if (strcasecmp(repmemo+1, "ollution")==0) return MessageType::POLLUTION;
            break;
        case 's':
            if (strcasecmp(repmemo+1, "atellite")==0) return MessageType::SAT;
            if (strcasecmp(repmemo+1, "hip")==0) return MessageType::SHIP;
            if (strcasecmp(repmemo+1, "ynop")==0) return MessageType::SYNOP;
            break;
        case 't':
            if (strcasecmp(repmemo+1, "emp")==0) return MessageType::TEMP;
            if (strcasecmp(repmemo+1, "empship")==0) return MessageType::TEMP_SHIP;
            break;
    }
    return MessageType::GENERIC;
}

const char* Message::repmemo_from_type(MessageType type)
{
    switch (type)
    {
        case MessageType::SYNOP:     return "synop";
        case MessageType::METAR:     return "metar";
        case MessageType::SHIP:      return "ship";
        case MessageType::BUOY:      return "buoy";
        case MessageType::AIREP:     return "airep";
        case MessageType::AMDAR:     return "amdar";
        case MessageType::ACARS:     return "acars";
        case MessageType::PILOT:     return "pilot";
        case MessageType::TEMP:      return "temp";
        case MessageType::TEMP_SHIP: return "tempship";
        case MessageType::SAT:       return "satellite";
        case MessageType::POLLUTION: return "pollution";
        case MessageType::GENERIC:
        default:            return "generic";
    }
}

void Message::sounding_pack_levels()
{
    msg::Contexts new_data;
    for (auto& ctx: data)
    {
        if (ctx.find_vsig())
            // FIXME: shouldn't this also set significance bits in the output level?
            new_data.obtain(Level(ctx.level.ltype1, ctx.level.l1), ctx.trange)->values.merge(std::move(ctx.values));
        else
            // If it is not a sounding level, just copy it
            new_data.obtain(ctx.level, ctx.trange)->values = std::move(ctx.values);
    }
    data = std::move(new_data);
}

void Message::set_datetime(const Datetime& dt)
{
    set_year(dt.year);
    set_month(dt.month);
    set_day(dt.day);
    set_hour(dt.hour);
    set_minute(dt.minute);
    set_second(dt.second);
}


MatchedMsg::MatchedMsg(const Message& m)
    : m(m)
{
}

MatchedMsg::~MatchedMsg()
{
}

matcher::Result MatchedMsg::match_var_id(int value) const
{
    for (const auto& val: m.station_data)
        if (const Var* a = val->enqa(WR_VAR(0, 33, 195)))
            if (a->enqi() == value)
                return matcher::MATCH_YES;

    for (const auto& ctx: m.data)
        for (const auto& val: ctx.values)
            if (const Var* a = val->enqa(WR_VAR(0, 33, 195)))
                if (a->enqi() == value)
                    return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsg::match_station_id(int val) const
{
    if (const wreport::Var* var = m.station_data.maybe_var(WR_VAR(0, 1, 192)))
    {
        return var->enqi() == val ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}

matcher::Result MatchedMsg::match_station_wmo(int block, int station) const
{
    if (const wreport::Var* var = m.station_data.maybe_var(WR_VAR(0, 1, 1)))
    {
        // Match block
        if (var->enqi() != block) return matcher::MATCH_NO;

        // If station was not requested, we are done
        if (station == -1) return matcher::MATCH_YES;

        // Match station
        if (const wreport::Var* var = m.station_data.maybe_var(WR_VAR(0, 1, 2)))
        {
            if (var->enqi() != station) return matcher::MATCH_NO;
            return matcher::MATCH_YES;
        }
    }
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsg::match_datetime(const DatetimeRange& range) const
{
    Datetime dt = m.get_datetime();
    if (dt.is_missing()) return matcher::MATCH_NA;
    return range.contains(dt) ? matcher::MATCH_YES : matcher::MATCH_NO;
}

matcher::Result MatchedMsg::match_coords(const LatRange& latrange, const LonRange& lonrange) const
{
    Coords coords = m.get_coords();
    if (coords.is_missing())
    {
        matcher::Result r1 = latrange.is_missing() ? matcher::MATCH_YES : matcher::MATCH_NA;
        matcher::Result r2 = lonrange.is_missing() ? matcher::MATCH_YES : matcher::MATCH_NA;
        if (r1 == matcher::MATCH_YES && r2 == matcher::MATCH_YES)
            return matcher::MATCH_YES;
        return matcher::MATCH_NA;
    }

    matcher::Result r1 = latrange.contains(coords.lat) ? matcher::MATCH_YES : matcher::MATCH_NO;
    matcher::Result r2 = lonrange.contains(coords.lon) ? matcher::MATCH_YES : matcher::MATCH_NO;

    if (r1 == matcher::MATCH_YES && r2 == matcher::MATCH_YES)
        return matcher::MATCH_YES;
    if (r1 == matcher::MATCH_NO || r2 == matcher::MATCH_NO)
        return matcher::MATCH_NO;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsg::match_rep_memo(const char* memo) const
{
    if (const Var* var = m.station_data.maybe_var(sc::rep_memo.code))
    {
        if (!var->isset()) return matcher::MATCH_NA;
        return strcmp(memo, var->enqc()) == 0 ? matcher::MATCH_YES : matcher::MATCH_NO;
    } else
        return matcher::MATCH_NA;
}


MatchedMessages::MatchedMessages(const Messages& m)
    : m(m)
{
}
MatchedMessages::~MatchedMessages()
{
}

matcher::Result MatchedMessages::match_var_id(int val) const
{
    for (const auto& i: m)
        if (MatchedMsg(*impl::Message::downcast(i)).match_var_id(val) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMessages::match_station_id(int val) const
{
    for (const auto& i: m)
        if (MatchedMsg(*impl::Message::downcast(i)).match_station_id(val) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMessages::match_station_wmo(int block, int station) const
{
    for (const auto& i: m)
        if (MatchedMsg(*impl::Message::downcast(i)).match_station_wmo(block, station) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMessages::match_datetime(const DatetimeRange& range) const
{
    for (const auto& i: m)
        if (MatchedMsg(*impl::Message::downcast(i)).match_datetime(range) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMessages::match_coords(const LatRange& latrange, const LonRange& lonrange) const
{
    for (const auto& i: m)
        if (MatchedMsg(*impl::Message::downcast(i)).match_coords(latrange, lonrange) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMessages::match_rep_memo(const char* memo) const
{
    for (const auto& i: m)
        if (MatchedMsg(*impl::Message::downcast(i)).match_rep_memo(memo) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

}
}
