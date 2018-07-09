#ifndef DBALLE_CORE_JSON_H
#define DBALLE_CORE_JSON_H

#include <wreport/varinfo.h>
#include <wreport/var.h>
#include <dballe/types.h>
#include <dballe/core/fwd.h>
#include <vector>
#include <ostream>
#include <istream>

namespace dballe {
namespace core {

struct JSONParseException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};


/**
 * JSON serializer
 *
 * It is called with a sequence of sax-like events, and appends the resulting
 * JSON to a string.
 *
 * The JSON output is all in one line, so that end of line can be used as
 * separator between distinct JSON records.
 */
class JSONWriter
{
protected:
    enum State {
        LIST_FIRST,
        LIST,
        MAPPING_KEY_FIRST,
        MAPPING_KEY,
        MAPPING_VAL,
    };
    std::ostream& out;
    std::vector<State> stack;

    /// Append whatever separator is needed (if any) before a new value
    void val_head();

    void jputc(char c);
    void jputs(const char* s);

public:
    JSONWriter(std::ostream& out);
    ~JSONWriter();

    /**
     * Reset the serializer state, to cancel the current output and prepare for
     * a new one
     */
    void reset();

    void start_list();
    void end_list();

    void start_mapping();
    void end_mapping();

    void add_null();
    void add_bool(bool val);
    void add_int(int val);
    void add_double(double val);
    void add_cstring(const char* val);
    void add_string(const std::string& val);
    template<typename T>
    void add_ostream(const T& val)
    {
        val_head();
        out << val;
    }

    void add_number(const std::string& val);
    void add_level(const Level& val);
    void add_trange(const Trange& val);
    void add_datetime(const Datetime& val);
    void add_datetime_range(const DatetimeRange& val);
    void add_coords(const Coords& val);
    void add_ident(const Ident& val);
    void add_var(const wreport::Var& val);
    void add_break();

    void add(const std::string& val) { add_string(val); }
    void add(const char* val) { add_cstring(val); }
    void add(double val) { add_double(val); }
    void add(int val) { add_int(val); }
    void add(bool val) { add_bool(val); }
    void add(size_t val) { add_ostream(val); }
    void add(wreport::Varcode val) { add_int(val); }
    void add(const Level& val) { add_level(val); }
    void add(const Trange& val) { add_trange(val); }
    void add(const Datetime& val) { add_datetime(val); }
    void add(const DatetimeRange& val) { add_datetime_range(val); }
    void add(const Coords& val) { add_coords(val); }
    void add(const Ident& val) { add_ident(val); }
    void add(const wreport::Var& val) { add_var(val); }

    template<typename T>
    void add(const char* a, T b)
    {
        add_cstring(a);
        add(b);
    }

    template<typename T>
    void add_list(const T& val)
    {
        start_list();
        for (const auto& i : val)
            add(i);
        end_list();
    }
};

/**
 * JSON sax-like parser.
 */
class JSONReader
{
public:
    virtual ~JSONReader() {}

    virtual void on_start_list() = 0;
    virtual void on_end_list() = 0;

    virtual void on_start_mapping() = 0;
    virtual void on_end_mapping() = 0;

    virtual void on_add_null() = 0;
    virtual void on_add_bool(bool val) = 0;
    virtual void on_add_int(int val) = 0;
    virtual void on_add_double(double val) = 0;
    virtual void on_add_string(const std::string& val) = 0;

    // Parse a stream
    void parse(std::istream& in);
};


namespace json {

enum Element
{
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_NUMBER,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL,
};

struct Stream
{
    std::istream& in;

    Stream(std::istream& in) : in(in) {}

    /// Raise a parse error if the stream does not yield this exact token
    void expect_token(const char* token);

    /// Consume and discard all spaces at the start of the stream
    void skip_spaces();

    /// Parse an unsigned integer
    template<typename T>
    T parse_unsigned()
    {
        T res = 0;
        while (true)
        {
            int c = in.peek();
            if (c >= '0' and c <= '9')
                res = res * 10 + in.get() - '0';
            else
                break;
        }
        skip_spaces();
        return res;
    }

    /// Parse a signed integer
    template<typename T>
    T parse_signed()
    {
        if (in.peek() == '-')
        {
            in.get();
            return -parse_unsigned<T>();
        } else
            return parse_unsigned<T>();
    }

    /// Parse a double
    double parse_double();

    /**
     * Parse a number, without converting it.
     *
     * Returns the number, and true if it looks like a floating point number,
     * false if it looks like an integer number
     */
    std::tuple<std::string, bool> parse_number();

    /// Parse a string from the start of the stream
    std::string parse_string();

    /// Parse a Coords object
    Coords parse_coords();

    /// Parse an Ident object
    Ident parse_ident();

    /// Parse a Level object
    Level parse_level();

    /// Parse a Trange object
    Trange parse_trange();

    /// Parse a Datetime object
    Datetime parse_datetime();

    /// Parse a DatetimeRange object
    DatetimeRange parse_datetime_range();

    /// Parse a JSON array, calling on_element to parse each element
    void parse_array(std::function<void()> on_element);

    /// Parse a JSON object, calling on_value to parse each value
    void parse_object(std::function<void(const std::string& key)> on_value);

    /// Identify the next element in the stream, without moving the stream
    /// position
    Element identify_next();
};

}

}
}
#endif
