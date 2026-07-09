#ifndef DBALLE_VALUE_H
#define DBALLE_VALUE_H

#include <dballe/fwd.h>
#include <iosfwd>
#include <memory>
#include <wreport/varinfo.h>

namespace wreport {
struct Var;
}

namespace dballe {

/**
 * Container for a wreport::Var pointer
 */
class Value
{
protected:
    wreport::Var* m_var = nullptr;

public:
    Value() = default;
    Value(const Value& o);
    Value(Value&& o) : m_var(o.m_var) { o.m_var = nullptr; }

    /// Construct from a wreport::Var
    Value(const wreport::Var& var);

    /// Construct from a wreport::Var, taking ownership of it
    Value(std::unique_ptr<wreport::Var>&& var) : m_var(var.release()) {}

    ~Value();

    Value& operator=(const Value& o);
    Value& operator=(Value&& o);

    bool operator==(const Value& o) const;
    bool operator!=(const Value& o) const;

    const wreport::Var* get() const { return m_var; }
    wreport::Var* get() { return m_var; }

    const wreport::Var* operator->() const { return m_var; }
    wreport::Var* operator->() { return m_var; }

    const wreport::Var& operator*() const { return *m_var; }
    wreport::Var& operator*() { return *m_var; }

    /// Return the varcode of the variable, or 0 if no variable has been set
    wreport::Varcode code() const;

    /// Fill from a wreport::Var
    void reset(const wreport::Var& var);

    /// Fill from a wreport::Var, taking ownership of it
    void reset(std::unique_ptr<wreport::Var>&& var);

    /// Return the Var pointer, setting the Value to undefined
    std::unique_ptr<wreport::Var> release();

    /// Print the contents of this Value
    void print(FILE* out) const;
};

/**
 * Container for a wreport::Var pointer, and its database ID
 */
struct DBValue : public Value
{
    using Value::Value;

    /// Database ID of the value
    int data_id = MISSING_INT;

    DBValue()                 = default;
    DBValue(const DBValue& o) = default;
    DBValue(DBValue&& o)      = default;

    /// Construct from a wreport::Var
    DBValue(int data_id, const wreport::Var& var) : Value(var), data_id(data_id)
    {
    }

    /// Construct from a wreport::Var, taking ownership of it
    DBValue(int data_id, std::unique_ptr<wreport::Var>&& var)
        : Value(std::move(var)), data_id(data_id)
    {
    }

    DBValue& operator=(const DBValue&) = default;
    DBValue& operator=(DBValue&&)      = default;

    bool operator==(const DBValue& o) const;
    bool operator!=(const DBValue& o) const;

    /// Print the contents of this Value
    void print(FILE* out) const;
};

std::ostream& operator<<(std::ostream&, const Value&);
std::ostream& operator<<(std::ostream&, const DBValue&);

} // namespace dballe

#endif
