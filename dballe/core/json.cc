#include "json.h"
#include "dballe/values.h"
#include <cctype>
#include <cmath>

using namespace std;

namespace dballe {
namespace core {

JSONWriter::JSONWriter(std::ostream& out) : out(out) {}
JSONWriter::~JSONWriter() {}

void JSONWriter::reset()
{
    stack.clear();
}

void JSONWriter::val_head()
{
    if (!stack.empty())
    {
        switch (stack.back())
        {
            case LIST_FIRST: stack.back() = LIST; break;
            case LIST: out << ','; break;
            case MAPPING_KEY_FIRST: stack.back() = MAPPING_VAL; break;
            case MAPPING_KEY: out << ','; stack.back() = MAPPING_VAL; break;
            case MAPPING_VAL: out << ':'; stack.back() = MAPPING_KEY; break;
        }
    }
}

void JSONWriter::add_null()
{
    val_head();
    out << "null";
}

void JSONWriter::add_bool(bool val)
{
    val_head();
    out << (val ? "true" : "false" );
}

void JSONWriter::add_int(int val)
{
    val_head();
    out << to_string(val);
}

void JSONWriter::add_double(double val)
{
    val_head();

    double vint, vfrac;
    vfrac = modf(val, &vint);
    if (vfrac == 0.0)
    {
        out << to_string((int)vint);
        out << ".0";
    }
    else
        out << to_string(val);
}

void JSONWriter::add_cstring(const char* val)
{
    val_head();
    out << '"';
    for ( ; *val; ++val)
        switch (*val)
        {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << *val; break;
        }
    out << '"';
}

void JSONWriter::add_string(const std::string& val)
{
    add_cstring(val.c_str());
}

void JSONWriter::add_level(const Level& val)
{
    start_list();
    if (val.ltype1 != MISSING_INT) add(val.ltype1); else add_null();
    if (val.l1 != MISSING_INT) add(val.l1); else add_null();
    if (val.ltype2 != MISSING_INT) add(val.ltype2); else add_null();
    if (val.l2 != MISSING_INT) add(val.l2); else add_null();
    end_list();
}

void JSONWriter::add_trange(const Trange& val)
{
    start_list();
    if (val.pind != MISSING_INT) add(val.pind); else add_null();
    if (val.p1 != MISSING_INT) add(val.p1); else add_null();
    if (val.p2 != MISSING_INT) add(val.p2); else add_null();
    end_list();
}

void JSONWriter::add_coords(const Coords& val)
{
    start_list();
    if (val.lat != MISSING_INT) add(val.lat); else add_null();
    if (val.lon != MISSING_INT) add(val.lon); else add_null();
    end_list();
}

void JSONWriter::add_station(const Station& s)
{
    start_mapping();
    add("r", s.report);
    add("c", s.coords);
    if (s.ident) add("i", s.ident);
    end_mapping();
}

void JSONWriter::add_dbstation(const DBStation& s)
{
    start_mapping();
    if (s.id != MISSING_INT) add("id", s.id);
    add("r", s.report);
    add("c", s.coords);
    if (s.ident) add("i", s.ident);
    end_mapping();
}

void JSONWriter::add_values(const Values& values)
{
    start_mapping();
    for (const auto& var: values) {
        add(wreport::varcode_format(var->code()));
        start_mapping();
        add("v");
        add(*var);
        if (var->next_attr()) {
            add("a");
            start_mapping();
            for (const wreport::Var* attr = var->next_attr(); attr; attr = attr->next_attr()) {
                add(wreport::varcode_format(attr->code()));
                add(*attr);
            }
            end_mapping();
        }
        end_mapping();
    }
    end_mapping();
}

void JSONWriter::add_dbvalues(const DBValues& values)
{
}

void JSONWriter::add_datetime(const Datetime& val)
{
    if (val.is_missing())
        add_null();
    else
    {
        start_list();
        if (val.year != 0xffff) add(val.year); else add_null();
        if (val.month != 0xff) add(val.month); else add_null();
        if (val.day != 0xff) add(val.day); else add_null();
        if (val.hour != 0xff) add(val.hour); else add_null();
        if (val.minute != 0xff) add(val.minute); else add_null();
        if (val.second != 0xff) add(val.second); else add_null();
        end_list();
    }
}

void JSONWriter::add_datetimerange(const DatetimeRange& val)
{
    start_list();
    add(val.min);
    add(val.max);
    end_list();
}

void JSONWriter::add_number(const std::string& val) {
    val_head();
    out << val;
}

void JSONWriter::add_var(const wreport::Var& val) {
    if (val.isset()) {
        if (val.info()->type == wreport::Vartype::String ||
            val.info()->type == wreport::Vartype::Binary) {
            add_string(val.format());
        } else {
            add_number(val.format());
        }
    } else {
        add_null();
    }
}

void JSONWriter::add_ident(const Ident& val)
{
    if (val.is_missing())
        add_null();
    else
        add_cstring(val);
}

void JSONWriter::add_break() {
    out << "\n";
}

void JSONWriter::start_list()
{
    val_head();
    out << '[';
    stack.push_back(LIST_FIRST);
}

void JSONWriter::end_list()
{
    out << ']';
    stack.pop_back();
}

void JSONWriter::start_mapping()
{
    val_head();
    out << '{';
    stack.push_back(MAPPING_KEY_FIRST);
}

void JSONWriter::end_mapping()
{
    out << '}';
    stack.pop_back();
}


namespace json {

void Stream::expect_token(const char* token)
{
    for (const char* s = token; *s; ++s)
    {
        int c = in.get();
        if (c != *s)
        {
            if (c == EOF)
                // throw JSONParseException(str::fmtf("end of file reached looking for %s in %s", s, expected));
                throw JSONParseException("unexpected end of file reached");
            else
                // throw JSONParseException(str::fmtf("unexpected character '%c' looking for %s in %s", c, s, expected));
                throw JSONParseException("unexpected character");
        }
    }
}

void Stream::skip_spaces()
{
    while (isspace(in.peek()))
        in.get();
}

double Stream::parse_double()
{
    string val;
    std::tie(val, std::ignore) = parse_number();
    return std::stod(val);
}

std::tuple<std::string, bool> Stream::parse_number()
{
    string num;
    bool done = false;
    bool is_double = false;
    while (!done)
    {
        int c = in.peek();
        switch (c)
        {
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                num.append(1, in.get());
                break;
            case '.':
            case 'e':
            case 'E':
            case '+':
                is_double = true;
                num.append(1, in.get());
                break;
            default:
                done = true;
        }
    }

    skip_spaces();

    return std::make_tuple(num, is_double);
}

std::string Stream::parse_string()
{
    string res;
    int c = in.get(); // Eat the leading '"'
    if (c != '"')
        throw JSONParseException("expected string does not begin with '\"'");
    bool done = false;
    while (!done)
    {
        int c = in.get();
        switch (c)
        {
            case '\\':
                c = in.get();
                if (c == EOF)
                    throw JSONParseException("unterminated string");
                switch (c)
                {
                    case 'b': res.append(1, '\b'); break;
                    case 'f': res.append(1, '\f'); break;
                    case 'n': res.append(1, '\n'); break;
                    case 'r': res.append(1, '\r'); break;
                    case 't': res.append(1, '\t'); break;
                    default: res.append(1, c); break;
                }
                break;
            case '"':
                done = true;
                break;
            case EOF:
                throw JSONParseException("unterminated string");
            default:
                res.append(1, c);
                break;
        }
    }
    skip_spaces();
    return res;
}

Coords Stream::parse_coords()
{
    Coords res;
    unsigned idx = 0;
    parse_array([&]{
        switch (identify_next())
        {
            case JSON_NUMBER:
                switch (idx)
                {
                    case 0: res.lat = parse_signed<int>(); break;
                    case 1: res.lon = parse_signed<int>(); break;
                    default: throw JSONParseException("extra element in Coords array");
                }
                break;
            case JSON_NULL: expect_token("null"); break;
            default:
                throw JSONParseException("unexpected element in Coords array");
        }
        ++idx;
    });
    return res;
}

Ident Stream::parse_ident()
{
    switch (identify_next())
    {
        case JSON_STRING: return Ident(parse_string());
        case JSON_NULL: expect_token("null"); return Ident();
        default: throw JSONParseException("unexpected element for Ident");
    }
}

Station Stream::parse_station()
{
    Station res;
    parse_object([&](const std::string& key) {
        if (key == "r")
            res.report = parse_string();
        else if (key == "c")
            res.coords = parse_coords();
        else if (key == "i")
            res.ident = parse_ident();
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for Station");
    });
    return res;
}

DBStation Stream::parse_dbstation()
{
    DBStation res;
    parse_object([&](const std::string& key) {
        if (key == "id")
            res.id = parse_unsigned<int>();
        else if (key == "r")
            res.report = parse_string();
        else if (key == "c")
            res.coords = parse_coords();
        else if (key == "i")
            res.ident = parse_ident();
        else
            throw core::JSONParseException("unsupported key \"" + key + "\" for DBStation");
    });
    return res;
}


Level Stream::parse_level()
{
    Level res;
    unsigned idx = 0;
    parse_array([&]{
        switch (identify_next())
        {
            case JSON_NUMBER:
                switch (idx)
                {
                    case 0: res.ltype1 = parse_signed<int>(); break;
                    case 1: res.l1 = parse_signed<int>(); break;
                    case 2: res.ltype2 = parse_signed<int>(); break;
                    case 3: res.l2 = parse_signed<int>(); break;
                    default: throw JSONParseException("extra element in Level array");
                }
                break;
            case JSON_NULL: expect_token("null"); break;
            default:
                throw JSONParseException("unexpected element in Level array");
        }
        ++idx;
    });
    return res;
}

Trange Stream::parse_trange()
{
    Trange res;
    unsigned idx = 0;
    parse_array([&]{
        switch (identify_next())
        {
            case JSON_NUMBER:
                switch (idx)
                {
                    case 0: res.pind = parse_signed<int>(); break;
                    case 1: res.p1 = parse_signed<int>(); break;
                    case 2: res.p2 = parse_signed<int>(); break;
                    default: throw JSONParseException("extra element in Trange array");
                }
                break;
            case JSON_NULL: expect_token("null"); break;
            default:
                throw JSONParseException("unexpected element in Trange array");
        }
        ++idx;
    });
    return res;
}

Datetime Stream::parse_datetime()
{
    switch (identify_next())
    {
        case JSON_NULL: expect_token("null"); return Datetime();
        case JSON_ARRAY:
        {
            Datetime res;
            unsigned idx = 0;
            parse_array([&]{
                switch (identify_next())
                {
                    case JSON_NUMBER:
                        switch (idx)
                        {
                            case 0: res.year = parse_unsigned<unsigned short>(); break;
                            case 1: res.month = parse_signed<unsigned char>(); break;
                            case 2: res.day = parse_signed<unsigned char>(); break;
                            case 3: res.hour = parse_signed<unsigned char>(); break;
                            case 4: res.minute = parse_signed<unsigned char>(); break;
                            case 5: res.second = parse_signed<unsigned char>(); break;
                            default: throw JSONParseException("extra element in Datetime array");
                        }
                        break;
                    case JSON_NULL: expect_token("null"); break;
                    default:
                        throw JSONParseException("unexpected element in Datetime array");
                }
                ++idx;
            });
            return res;
        }
        default: throw JSONParseException("unexpected element for Datetime");
    }
}

DatetimeRange Stream::parse_datetimerange()
{
    DatetimeRange res;
    unsigned idx = 0;
    parse_array([&]{
        switch (idx)
        {
            case 0: res.min = parse_datetime(); break;
            case 1: res.max = parse_datetime(); break;
            default: throw JSONParseException("extra element in DatetimeRange array");
        }
        ++idx;
    });
    return res;
}


void Stream::parse_array(std::function<void()> on_element)
{
    if (in.get() != '[')
        throw JSONParseException("expected array does not begin with '['");
    skip_spaces();
    while (in.peek() != ']')
    {
        on_element();
        if (in.peek() == ',')
            in.get();
        skip_spaces();
    }
    if (in.get() != ']')
        throw JSONParseException("array does not end with '['");
    skip_spaces();
}

void Stream::parse_object(std::function<void(const std::string& key)> on_value)
{
    if (in.get() != '{')
        throw JSONParseException("expected object does not begin with '{'");
    skip_spaces();
    while (in.peek() != '}')
    {
        if (in.peek() != '"')
            throw JSONParseException("expected a string as object key");
        std::string key = parse_string();
        skip_spaces();
        if (in.peek() == ':')
            in.get();
        else
            throw JSONParseException("':' expected after object key");
        on_value(key);
        if (in.peek() == ',')
            in.get();
        skip_spaces();
    }
    if (in.get() != '}')
        throw JSONParseException("expected object does not end with '}'");
    skip_spaces();
}

Element Stream::identify_next()
{
    skip_spaces();
    switch (in.peek())
    {
        case EOF:
            throw JSONParseException("JSON string is truncated");
        case '{': return JSON_OBJECT;
        case '[': return JSON_ARRAY;
        case '"': return JSON_STRING;
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': return JSON_NUMBER;
        case 't': return JSON_TRUE;
        case 'f': return JSON_FALSE;
        case 'n': return JSON_NULL;
        default:
            // throw JSONParseException(str::fmtf("unexpected character '%c'", in.in.peek()));
            throw JSONParseException("unexpected character");
    }
}

}

static void parse_value(json::Stream& in, JSONReader& e)
{
    switch (in.identify_next())
    {
        case json::JSON_OBJECT:
            e.on_start_mapping();
            in.parse_object([&](const std::string& key) {
                e.on_add_string(key);
                parse_value(in, e);
            });
            e.on_end_mapping();
            break;
        case json::JSON_ARRAY:
            e.on_start_list();
            in.parse_array([&]{
                parse_value(in, e);
            });
            e.on_end_list();
            break;
        case json::JSON_STRING:
            e.on_add_string(in.parse_string());
            break;
        case json::JSON_NUMBER: {
            std::string val;
            bool is_double;
            std::tie(val, is_double) = in.parse_number();

            if (is_double)
                e.on_add_double(std::stod(val));
            else
                e.on_add_int(stoi(val));
            break;
        }
        case json::JSON_TRUE:
            in.expect_token("true");
            e.on_add_bool(true);
            in.skip_spaces();
            break;
        case json::JSON_FALSE:
            in.expect_token("false");
            e.on_add_bool(false);
            in.skip_spaces();
            break;
        case json::JSON_NULL:
            in.expect_token("null");
            e.on_add_null();
            in.skip_spaces();
            break;
        default:
            // throw JSONParseException(str::fmtf("unexpected character '%c'", in.in.peek()));
            throw JSONParseException("unexpected character");
    }
    in.skip_spaces();
}

void JSONReader::parse(std::istream& in) {
    json::Stream jstream(in);
    parse_value(jstream, *this);
}

}
}
