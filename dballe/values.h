#ifndef DBALLE_VALUES_H
#define DBALLE_VALUES_H

/** @file
 * Structures used as input to database insert functions.
 */

#include <dballe/fwd.h>
#include <dballe/value.h>
#include <dballe/var.h>
#include <functional>
#include <iosfwd>
#include <vector>

namespace dballe {
namespace impl {

template <typename Value> class ValuesBase
{
public:
    typedef typename std::vector<Value>::const_iterator const_iterator;
    typedef typename std::vector<Value>::iterator iterator;

protected:
    std::vector<Value> m_values;

    iterator insert_new(Value&& val);

public:
    ValuesBase() {}
    ValuesBase(const ValuesBase&)            = default;
    ValuesBase(ValuesBase&&)                 = default;
    ValuesBase& operator=(const ValuesBase&) = default;
    ValuesBase& operator=(ValuesBase&&)      = default;

    const_iterator begin() const { return m_values.begin(); }
    const_iterator end() const { return m_values.end(); }
    const_iterator cbegin() const { return m_values.cbegin(); }
    const_iterator cend() const { return m_values.cend(); }
    iterator begin() { return m_values.begin(); }
    iterator end() { return m_values.end(); }
    iterator find(wreport::Varcode code) noexcept;
    const_iterator find(wreport::Varcode code) const noexcept;
    size_t size() const { return m_values.size(); }
    bool empty() const { return m_values.empty(); }
    void clear() { return m_values.clear(); }
    void reserve(typename std::vector<Value>::size_type size)
    {
        m_values.reserve(size);
    }
    bool operator==(const ValuesBase<Value>& o) const;
    bool operator!=(const ValuesBase<Value>& o) const;

    /// Set from a wreport::Var
    void set(const wreport::Var&);

    /// Set from a wreport::Var, taking ownership of it
    void set(std::unique_ptr<wreport::Var>&&);

    /// Set with a Value
    void set(Value&& val);

    /// Remove one variable
    void unset(wreport::Varcode code);

    /// Add all the variables from vals
    void merge(const ValuesBase<Value>& vals);

    /// Add all the variables from vals
    void merge(ValuesBase<Value>&& vals);

    /// Set a variable value, creating it if it does not exist
    template <typename C, typename T> void set(const C& code, const T& val)
    {
        this->set(newvar(code, val));
    }

    template <typename C, typename T> void setf(const C& code, const T& val)
    {
        auto var = newvar(code);
        var->setf(val);
        this->set(std::move(var));
    }

    /**
     * Lookup a value, throwing an exception if not found
     */
    const Value& value(wreport::Varcode code) const;
    const Value& value(const char* code) const
    {
        return value(resolve_varcode(code));
    }
    const Value& value(const std::string& code) const
    {
        return value(resolve_varcode(code));
    }

    /**
     * Lookup a wreport::Var, throwing an exception if not found
     */
    const wreport::Var& var(wreport::Varcode code) const;
    const wreport::Var& var(const char* code) const
    {
        return var(resolve_varcode(code));
    }
    const wreport::Var& var(const std::string& code) const
    {
        return var(resolve_varcode(code));
    }

    /**
     * Lookup a wreport::Var, throwing an exception if not found (non-const
     * version)
     */
    wreport::Var& var(wreport::Varcode code);
    wreport::Var& var(const char* code) { return var(resolve_varcode(code)); }
    wreport::Var& var(const std::string& code)
    {
        return var(resolve_varcode(code));
    }

    /**
     * Lookup a value, returning nullptr if not found
     */
    const Value* maybe_value(wreport::Varcode code) const;
    const Value* maybe_value(const char* code) const
    {
        return maybe_value(resolve_varcode(code));
    }
    const Value* maybe_value(const std::string& code) const
    {
        return maybe_value(resolve_varcode(code));
    }

    /**
     * Lookup a variable, returning nullptr if not found
     */
    const wreport::Var* maybe_var(wreport::Varcode code) const;
    const wreport::Var* maybe_var(const char* code) const
    {
        return maybe_var(resolve_varcode(code));
    }
    const wreport::Var* maybe_var(const std::string& code) const
    {
        return maybe_var(resolve_varcode(code));
    }

    /**
     * Lookup a variable, returning nullptr if not found (non-const version)
     */
    wreport::Var* maybe_var(wreport::Varcode code);
    wreport::Var* maybe_var(const char* code)
    {
        return maybe_var(resolve_varcode(code));
    }
    wreport::Var* maybe_var(const std::string& code)
    {
        return maybe_var(resolve_varcode(code));
    }

    /**
     * Get the value of a variable, or def if it is not set
     */
    template <typename C, typename T> T enq(C code, const T& def)
    {
        if (const wreport::Var* var = maybe_var(code))
            return var->enq(def);
        return def;
    }

    /**
     * Move all the Var as attributes to dest.
     *
     * After this method is called, this Values will be empty.
     */
    void move_to_attributes(wreport::Var& dest);

    /**
     * Move all the Var passing them to the given function.
     *
     * After this method is called, this Values will be empty.
     */
    void move_to(std::function<void(std::unique_ptr<wreport::Var>)> dest);

    /// Print the contents of this Values
    void print(FILE* out) const;

    /**
     * Encode these values in a DB-All.e specific binary representation
     */
    std::vector<uint8_t> encode() const;

    /**
     * Encode the attributes of var in a DB-All.e specific binary representation
     */
    static std::vector<uint8_t> encode_attrs(const wreport::Var& var);

    /**
     * Decode variables from a DB-All.e specific binary representation
     */
    static void decode(const std::vector<uint8_t>& buf,
                       std::function<void(std::unique_ptr<wreport::Var>)> dest);
};

extern template struct ValuesBase<Value>;
extern template struct ValuesBase<DBValue>;

} // namespace impl

struct DBValues;

/**
 * Collection of Value objects, indexed by wreport::Varcode
 */
struct Values : public impl::ValuesBase<Value>
{
    using ValuesBase<Value>::ValuesBase;
    Values() = default;
    explicit Values(const DBValues&);
    explicit Values(DBValues&&);

    Values& operator=(const DBValues&);
    Values& operator=(DBValues&&);
};

/**
 * Collection of DBValue objects, indexed by wreport::Varcode
 */
struct DBValues : public impl::ValuesBase<DBValue>
{
public:
    using ValuesBase<DBValue>::ValuesBase;
    DBValues() = default;
    explicit DBValues(const Values&);
    explicit DBValues(Values&&);

    DBValues& operator=(const Values&);
    DBValues& operator=(Values&&);

    /// Check if the variables are the same, regardless of the data_id
    bool vars_equal(const DBValues& o) const;

    /// Set the database ID for the Value with this wreport::Varcode
    void set_data_id(wreport::Varcode code, int data_id);

    /// Reset all the database IDs
    void clear_ids()
    {
        for (auto& val : m_values)
            val.data_id = MISSING_INT;
    }
};

std::ostream& operator<<(std::ostream&, const Values&);
std::ostream& operator<<(std::ostream&, const DBValues&);

} // namespace dballe

#endif
