#ifndef DBALLE_CORE_VALUES_H
#define DBALLE_CORE_VALUES_H

/** @file
 * Structures used as input to database insert functions.
 */

#include <dballe/core/fwd.h>
#include <dballe/types.h>
#include <dballe/core/defs.h>
#include <dballe/core/var.h>
#include <wreport/varinfo.h>
#include <vector>
#include <map>
#include <functional>
#include <iosfwd>

namespace dballe {
namespace core {

/**
 * A station or measured value
 */
struct Value
{
    /// Database ID of the value
    int data_id = MISSING_INT;

    /// wreport::Var representing the value
    wreport::Var* var = nullptr;

    Value() = default;
    Value(const Value& o) : data_id(o.data_id), var(o.var ? new wreport::Var(*o.var) : nullptr) {}
    Value(Value&& o) : data_id(o.data_id), var(o.var) { o.var = nullptr; }

    /// Construct from a wreport::Var
    Value(const wreport::Var& var) : var(new wreport::Var(var)) {}

    /// Construct from a wreport::Var
    Value(int data_id, const wreport::Var& var) : data_id(data_id), var(new wreport::Var(var)) {}

    /// Construct from a wreport::Var, taking ownership of it
    Value(std::unique_ptr<wreport::Var>&& var) : var(var.release()) {}

    /// Construct from a wreport::Var, taking ownership of it
    Value(int data_id, std::unique_ptr<wreport::Var>&& var) : data_id(data_id), var(var.release()) {}

    ~Value() { delete var; }

    Value& operator=(const Value& o)
    {
        if (this == &o) return *this;
        data_id = o.data_id;
        delete var;
        var = o.var ? new wreport::Var(*o.var) : nullptr;
        return *this;
    }
    Value& operator=(Value&& o)
    {
        if (this == &o) return *this;
        data_id = o.data_id;
        delete var;
        var = o.var;
        o.var = nullptr;
        return *this;
    }

    bool operator==(const Value& o) const
    {
        if (data_id != o.data_id) return false;
        if (var == o.var) return true;
        if (!var || !o.var) return false;
        return *var == *o.var;
    }

    /// Return the varcode of the variable, or 0 if no variable has been set yet
    wreport::Varcode code() const { return var ? var->code() : 0; }

    /// Reset the database ID
    void clear_ids() { data_id = MISSING_INT; }

    /// Print the contents of this Value
    void print(FILE* out) const;

protected:
    /// Fill from a wreport::Var
    void set(const wreport::Var& v)
    {
        delete var;
        var = new wreport::Var(v);
    }

    /// Fill from a wreport::Var, taking ownership of it
    void set(std::unique_ptr<wreport::Var>&& v)
    {
        delete var;
        var = v.release();
    }

    friend class Values;
};

namespace value {

struct Encoder
{
    std::vector<uint8_t> buf;

    Encoder();
    void append_uint16(uint16_t val);
    void append_uint32(uint32_t val);
    void append_cstring(const char* val);
    void append(const wreport::Var& var);
    void append_attributes(const wreport::Var& var);
};

struct Decoder
{
    const uint8_t* buf;
    unsigned size;

    Decoder(const std::vector<uint8_t>& buf);
    uint16_t decode_uint16();
    uint32_t decode_uint32();
    const char* decode_cstring();
    std::unique_ptr<wreport::Var> decode_var();

    /**
     * Decode the attributes of var from a buffer
     */
    static void decode_attrs(const std::vector<uint8_t>& buf, wreport::Var& var);
};

}

/**
 * Collection of Value objects, indexed by wreport::Varcode
 */
class Values
{
public:
    typedef std::vector<Value>::const_iterator const_iterator;
    typedef std::vector<Value>::iterator iterator;

protected:
    std::vector<Value> m_values;

    iterator insert_new(Value&& val);

public:
    Values() = default;

    const_iterator begin() const { return m_values.begin(); }
    const_iterator end() const { return m_values.end(); }
    iterator begin() { return m_values.begin(); }
    iterator end() { return m_values.end(); }
    iterator find(wreport::Varcode code) noexcept;
    const_iterator find(wreport::Varcode code) const noexcept;
    size_t size() const { return m_values.size(); }
    bool empty() const { return m_values.empty(); }
    void clear() { return m_values.clear(); }
    bool operator==(const Values& o) const;
    bool operator!=(const Values& o) const;

    /// Check if the variables are the same, regardless of the data_id
    bool vars_equal(const Values& o) const;

    // const Value& operator[](size_t idx) const { return m_values[idx]; }

    /**
     * Lookup a value, throwing an exception if not found
     */
    const Value& want(wreport::Varcode code) const;
    const Value& want(const char* code) const { return want(resolve_varcode(code)); }
    const Value& want(const std::string& code) const { return want(resolve_varcode(code)); }

    /**
     * Lookup a value, returning nullptr if not found
     */
    const Value* get(wreport::Varcode code) const;
    const Value* get(const char* code) const { return get(resolve_varcode(code)); }
    const Value* get(const std::string& code) const { return get(resolve_varcode(code)); }

    /**
     * Lookup a variable, returning nullptr if not found
     */
    const wreport::Var* get_var(wreport::Varcode code) const;
    const wreport::Var* get_var(const char* code) const { return get_var(resolve_varcode(code)); }
    const wreport::Var* get_var(const std::string& code) const { return get_var(resolve_varcode(code)); }

    template<typename C, typename T> T enq(C code, const T& def)
    {
        if (const wreport::Var* var = get_var(code))
            return var->enq(def);
        return def;
    }

    /// Set from a wreport::Var
    void set(const wreport::Var&);

    /// Set from a wreport::Var, taking ownership of it
    void set(std::unique_ptr<wreport::Var>&&);

    /// Set with a Value
    void set(Value&& val);

    /// Set with all the variables from vals
    void set(const Values& vals);

    /// Set from a variable created by dballe::newvar()
    template<typename C, typename T> void set(C code, const T& val) { this->set(newvar(code, val)); }

    /// Remove one variable
    void unset(wreport::Varcode code);

    /// Set the database ID for the Value with this wreport::Varcode
    void set_data_id(wreport::Varcode code, int data_id);

    /// Reset all the database IDs
    void clear_ids()
    {
        for (auto& val : m_values)
            val.clear_ids();
    }

    /**
     * Move the Var contained in this Values as attributes to dest.
     *
     * After this function is called, this Values will be empty.
     */
    void move_to_attributes(wreport::Var& dest);

    void move_to(std::function<void(std::unique_ptr<wreport::Var>)> dest);

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
    static void decode(const std::vector<uint8_t>& buf, std::function<void(std::unique_ptr<wreport::Var>)> dest);

    /// Print the contents of this Values
    void print(FILE* out) const;
};

std::ostream& operator<<(std::ostream&, const Values&);

}
}

#endif
