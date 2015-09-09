#ifndef DBALLE_CORE_JSON_H
#define DBALLE_CORE_JSON_H

#include <wreport/varinfo.h>
#include <wreport/var.h>
#include <dballe/core/defs.h>
#include <vector>
#include <ostream>

namespace dballe {
namespace core {

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
    void add_level(const Level& val);
    void add_trange(const Trange& val);
    void add_datetime(const Datetime& val);
    void add_coords(const Coords& val);
    void add_number(const std::string& val);
    void add_var(const wreport::Var& val);

    void add(const std::string& val) { add_string(val); }
    void add(const char* val) { add_cstring(val); }
    void add(double val) { add_double(val); }
    void add(int val) { add_int(val); }
    void add(wreport::Varcode val) { add_int(val); }
    void add(bool val) { add_bool(val); }
    void add(const Level& val) { add_level(val); }
    void add(const Trange& val) { add_trange(val); }
    void add(const Datetime& val) { add_datetime(val); }
    void add(const Coords& val) { add_coords(val); }
    void add(const wreport::Var& val) { add_var(val); }

    // Shortcut to add a mapping, which also ensures that the key is a string
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

}
}
#endif
