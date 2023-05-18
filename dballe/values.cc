#include "values.h"
#include "core/values.h"
#include <ostream>

using namespace std;
using namespace wreport;

namespace dballe {
namespace impl {

template<typename Value>
bool ValuesBase<Value>::operator==(const ValuesBase<Value>& o) const
{
    return m_values == o.m_values;
}

template<typename Value>
bool ValuesBase<Value>::operator!=(const ValuesBase<Value>& o) const
{
    return m_values != o.m_values;
}

template<typename Value>
typename ValuesBase<Value>::iterator ValuesBase<Value>::find(wreport::Varcode code) noexcept
{
    /* Binary search */
    if (m_values.empty())
        return m_values.end();

    iterator low = m_values.begin(), high = (m_values.end() - 1);
    while (low <= high)
    {
        iterator middle = low + (high - low) / 2;
        int cmp = (int)code - (int)(middle->code());
        if (cmp < 0)
            high = middle - 1;
        else if (cmp > 0)
            low = middle + 1;
        else
            return middle;
    }
    return m_values.end();
}

template<typename Value>
typename ValuesBase<Value>::const_iterator ValuesBase<Value>::find(wreport::Varcode code) const noexcept
{
    /* Binary search */
    if (m_values.empty())
        return m_values.end();

    const_iterator low = m_values.cbegin(), high = (m_values.cend() - 1);
    while (low <= high)
    {
        const_iterator middle = low + (high - low) / 2;
        int cmp = (int)code - (int)middle->code();
        if (cmp < 0)
            high = middle - 1;
        else if (cmp > 0)
            low = middle + 1;
        else
            return middle;
    }
    return m_values.end();
}

template<typename Value>
typename ValuesBase<Value>::iterator ValuesBase<Value>::insert_new(Value&& val)
{
    // Insertionsort, since the common case is to work with small arrays
    wreport::Varcode key = val.code();

    // Enlarge the buffer
    m_values.resize(m_values.size() + 1);

    // Insertionsort
    iterator pos;
    for (pos = m_values.end() - 1; pos > m_values.begin(); --pos)
    {
        if ((pos - 1)->code() > key)
            *pos = std::move(*(pos - 1));
        else
            break;
    }
    *pos = std::move(val);
    return pos;
}

template<typename Value>
void ValuesBase<Value>::unset(wreport::Varcode code)
{
    iterator pos = find(code);
    if (pos == end())
        return;
    m_values.erase(pos);
}

template<typename Value>
void ValuesBase<Value>::set(Value&& val)
{
    auto i = find(val.code());
    if (i == end())
        insert_new(std::move(val));
    else
        *i = std::move(val);
}

template<typename Value>
void ValuesBase<Value>::set(const wreport::Var& v)
{
    auto i = find(v.code());
    if (i == end())
        insert_new(Value(v));
    else
        i->reset(v);
}

template<typename Value>
void ValuesBase<Value>::set(std::unique_ptr<wreport::Var>&& v)
{
    auto code = v->code();
    auto i = find(code);
    if (i == end())
        insert_new(Value(move(v)));
    else
        i->reset(std::move(v));
}

template<typename Value>
void ValuesBase<Value>::merge(const ValuesBase<Value>& vals)
{
    for (const auto& vi: vals)
        set(*vi);
}

template<typename Value>
void ValuesBase<Value>::merge(ValuesBase<Value>&& vals)
{
    if (empty())
        operator=(std::move(vals));
    else
    {
        for (const auto& vi: vals)
            set(std::move(*vi));
        vals.clear();
    }
}

template<typename Value>
const Value& ValuesBase<Value>::value(wreport::Varcode code) const
{
    auto i = find(code);
    if (i == end())
        error_notfound::throwf("variable %01d%02d%03d not found",
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));
    return *i;
}

template<typename Value>
const wreport::Var& ValuesBase<Value>::var(wreport::Varcode code) const
{
    auto i = find(code);
    if (i == end())
        error_notfound::throwf("variable %01d%02d%03d not found",
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));

    if (!i->get())
        error_notfound::throwf("variable %01d%02d%03d not set",
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));

    return **i;
}

template<typename Value>
wreport::Var& ValuesBase<Value>::var(wreport::Varcode code)
{
    auto i = find(code);
    if (i == end())
        error_notfound::throwf("variable %01d%02d%03d not found",
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));

    if (!i->get())
        error_notfound::throwf("variable %01d%02d%03d not set",
                WR_VAR_F(code), WR_VAR_X(code), WR_VAR_Y(code));

    return **i;
}

template<typename Value>
const Value* ValuesBase<Value>::maybe_value(wreport::Varcode code) const
{
    auto i = find(code);
    if (i == end())
        return nullptr;
    return &*i;
}

template<typename Value>
const wreport::Var* ValuesBase<Value>::maybe_var(wreport::Varcode code) const
{
    auto i = find(code);
    if (i == end())
        return nullptr;
    if (!i->get()) return nullptr;
    return i->get();
}

template<typename Value>
wreport::Var* ValuesBase<Value>::maybe_var(wreport::Varcode code)
{
    auto i = find(code);
    if (i == end())
        return nullptr;
    if (!i->get()) return nullptr;
    return i->get();
}

template<typename Value>
void ValuesBase<Value>::move_to(std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    for (auto& val: m_values)
        dest(val.release());
    m_values.clear();
}

template<typename Value>
void ValuesBase<Value>::move_to_attributes(wreport::Var& dest)
{
    for (auto& val: m_values)
        dest.seta(val.release());
    m_values.clear();
}

template<typename Value>
void ValuesBase<Value>::print(FILE* out) const
{
    for (const auto& val: *this)
        val.print(out);
}

template<typename Value>
std::vector<uint8_t> ValuesBase<Value>::encode() const
{
    core::value::Encoder enc;
    for (const auto& i: *this)
        enc.append(*i);
    return enc.buf;
}

template<typename Value>
std::vector<uint8_t> ValuesBase<Value>::encode_attrs(const wreport::Var& var)
{
    core::value::Encoder enc;
    for (const Var* a = var.next_attr(); a != nullptr; a = a->next_attr())
        enc.append(*a);
    return enc.buf;
}

template<typename Value>
void ValuesBase<Value>::decode(const std::vector<uint8_t>& buf, std::function<void(std::unique_ptr<wreport::Var>)> dest)
{
    core::value::Decoder dec(buf);
    while (dec.size)
        dest(dec.decode_var());
}

template struct ValuesBase<Value>;
template struct ValuesBase<DBValue>;

}

Values::Values(const DBValues& o)
{
    clear();
    reserve(o.size());
    for (const auto& val: o)
        if (const Var* var = val.get())
            m_values.emplace_back(*var);
}

Values::Values(DBValues&& o)
{
    clear();
    reserve(o.size());
    o.move_to([&](std::unique_ptr<wreport::Var> var) {
        m_values.emplace_back(std::move(var));
    });
}

Values& Values::operator=(const DBValues& o)
{
    clear();
    reserve(o.size());
    for (const auto& val: o)
        if (const Var* var = val.get())
            m_values.emplace_back(*var);
    return *this;
}

Values& Values::operator=(DBValues&& o)
{
    clear();
    reserve(o.size());
    o.move_to([&](std::unique_ptr<wreport::Var> var) {
        m_values.emplace_back(std::move(var));
    });
    return *this;
}

DBValues::DBValues(const Values& o)
{
    clear();
    reserve(o.size());
    for (const auto& val: o)
        if (const Var* var = val.get())
            m_values.emplace_back(*var);
}

DBValues::DBValues(Values&& o)
{
    clear();
    reserve(o.size());
    o.move_to([&](std::unique_ptr<wreport::Var> var) {
        m_values.emplace_back(std::move(var));
    });
}

DBValues& DBValues::operator=(const Values& o)
{
    clear();
    reserve(o.size());
    for (const auto& val: o)
        if (const Var* var = val.get())
            m_values.emplace_back(*var);
    return *this;
}

DBValues& DBValues::operator=(Values&& o)
{
    clear();
    reserve(o.size());
    o.move_to([&](std::unique_ptr<wreport::Var> var) {
        m_values.emplace_back(std::move(var));
    });
    return *this;
}

bool DBValues::vars_equal(const DBValues& o) const
{
    const_iterator a = begin();
    const_iterator b = o.begin();
    while (a != end() && b != o.end())
    {
        if (!a->get() && !b->get())
            ;
        else if (!a->get() || !b->get())
            return false;
        else if (*a->get() != *b->get())
            return false;
        ++a; ++b;
    }
    return a == end() && b == o.end();
}

void DBValues::set_data_id(wreport::Varcode code, int data_id)
{
    auto i = find(code);
    if (i == end()) return;
    i->data_id = data_id;
}

std::ostream& operator<<(std::ostream& o, const Values& values)
{
    for (const auto& val: values)
        o << val << endl;
    return o;
}

std::ostream& operator<<(std::ostream& o, const DBValues& values)
{
    for (const auto& val: values)
        o << val << endl;
    return o;
}

}
