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
    out << val ? "true" : "false";
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

}
}
