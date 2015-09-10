#include "json.h"
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
            case '/': out << "\\/"; break;
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

void JSONWriter::add_datetime(const Datetime& val)
{
    if (val.is_missing())
        add_null();
    else
    {
        start_list();
        if (val.year != MISSING_INT) add(val.year); else add_null();
        if (val.month != MISSING_INT) add(val.month); else add_null();
        if (val.day != MISSING_INT) add(val.day); else add_null();
        if (val.hour != MISSING_INT) add(val.hour); else add_null();
        if (val.minute != MISSING_INT) add(val.minute); else add_null();
        if (val.second != MISSING_INT) add(val.second); else add_null();
        end_list();
    }
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

struct JSONParseException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

static void parse_spaces(std::istream& in)
{
    while (isspace(in.peek()))
        in.get();
}

static void parse_fixed(std::istream& in, const char* expected)
{
    const char* s = expected;
    while (*s)
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
        ++s;
    }
}

static void parse_number(std::istream& in, JSONReader& e)
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

    if (is_double)
    {
        e.on_add_double(strtod(num.c_str(), NULL));
    } else {
        e.on_add_int(strtoll(num.c_str(), NULL, 10));
    }

    parse_spaces(in);
}

static void parse_string(std::istream& in, JSONReader& e)
{
    string res;
    in.get(); // Eat the leading '"'
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
    parse_spaces(in);
    e.on_add_string(res);
}

static void parse_value(std::istream& in, JSONReader& e);

static void parse_array(std::istream& in, JSONReader& e)
{
    e.on_start_list();
    in.get(); // Eat the leading '['
    parse_spaces(in);
    while (in.peek() != ']')
    {
        parse_value(in, e);
        if (in.peek() == ',')
            in.get();
        parse_spaces(in);
    }
    in.get(); // Eat the trailing ']'
    parse_spaces(in);
    e.on_end_list();
}

static void parse_object(std::istream& in, JSONReader& e)
{
    e.on_start_mapping();
    in.get(); // Eat the leading '{'
    parse_spaces(in);
    while (in.peek() != '}')
    {
        if (in.peek() != '"')
            throw JSONParseException("expected a string as object key");
        parse_string(in, e);
        parse_spaces(in);
        if (in.peek() == ':')
            in.get();
        else
            throw JSONParseException("':' expected after object key");
        parse_value(in, e);
        if (in.peek() == ',')
            in.get();
        parse_spaces(in);
    }
    in.get(); // Eat the trailing '}'
    parse_spaces(in);
    e.on_end_mapping();
}

static void parse_value(std::istream& in, JSONReader& e)
{
    parse_spaces(in);
    switch (in.peek())
    {
        case EOF:
            throw JSONParseException("JSON string is truncated");
        case '{': parse_object(in, e); break;
        case '[': parse_array(in, e); break;
        case '"': parse_string(in, e); break;
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
        case '9': parse_number(in, e); break;
        case 't':
            parse_fixed(in, "true");
            e.on_add_bool(true);
            parse_spaces(in);
            break;
        case 'f':
            parse_fixed(in, "false");
            e.on_add_bool(false);
            parse_spaces(in);
            break;
        case 'n':
            parse_fixed(in, "null");
            e.on_add_null();
            parse_spaces(in);
            break;
        default:
            // throw JSONParseException(str::fmtf("unexpected character '%c'", in.peek()));
            throw JSONParseException("unexpected character");
    }
    parse_spaces(in);
}

void JSONReader::parse(std::istream& in) {
    parse_value(in, *this);
}

}
}
