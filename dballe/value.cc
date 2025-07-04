#include "value.h"
#include "dballe/core/var.h"
#include <ostream>
#include <wreport/var.h>

namespace dballe {

Value::Value(const Value& o)
    : m_var(o.m_var ? new wreport::Var(*o.m_var) : nullptr)
{
}

Value::Value(const wreport::Var& var) : m_var(new wreport::Var(var)) {}

Value::~Value() { delete m_var; }

Value& Value::operator=(const Value& o)
{
    if (this == &o)
        return *this;
    delete m_var;
    m_var = o.m_var ? new wreport::Var(*o.m_var) : nullptr;
    return *this;
}

Value& Value::operator=(Value&& o)
{
    if (this == &o)
        return *this;
    delete m_var;
    m_var   = o.m_var;
    o.m_var = nullptr;
    return *this;
}

bool Value::operator==(const Value& o) const
{
    if (m_var == o.m_var)
        return true;
    if (!m_var || !o.m_var)
        return false;
    return *m_var == *o.m_var;
}

bool Value::operator!=(const Value& o) const
{
    if (m_var == o.m_var)
        return false;
    if (!m_var || !o.m_var)
        return true;
    return *m_var != *o.m_var;
}

void Value::print(FILE* out) const
{
    if (!m_var)
        fputs("-\n", out);
    else
        m_var->print(out);
}

wreport::Varcode Value::code() const { return m_var ? m_var->code() : 0; }

void Value::reset(const wreport::Var& var)
{
    delete m_var;
    m_var = new wreport::Var(var);
}

void Value::reset(std::unique_ptr<wreport::Var>&& var)
{
    delete m_var;
    m_var = var.release();
}

std::unique_ptr<wreport::Var> Value::release()
{
    std::unique_ptr<wreport::Var> res(m_var);
    m_var = nullptr;
    return res;
}

bool DBValue::operator==(const DBValue& o) const
{
    return Value::operator==(o);
}

bool DBValue::operator!=(const DBValue& o) const
{
    return Value::operator!=(o);
}

void DBValue::print(FILE* out) const
{
    if (data_id == MISSING_INT)
        fputs("-------- ", out);
    else
        fprintf(out, "%8d ", data_id);
    if (!m_var)
        fputs("-\n", out);
    else
        m_var->print(out);
}

std::ostream& operator<<(std::ostream& o, const Value& val)
{
    if (!val.get())
        return o << "-";
    char bcode[7];
    format_bcode(val.code(), bcode);
    return o << bcode << ":" << val->format();
}

std::ostream& operator<<(std::ostream& o, const DBValue& val)
{
    if (val.data_id != MISSING_INT)
        o << val.data_id << ":";
    else
        o << "-:";
    if (!val.get())
        return o << "-";
    char bcode[7];
    format_bcode(val.code(), bcode);
    return o << bcode << ":" << val->format();
}

} // namespace dballe
