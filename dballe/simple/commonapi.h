#ifndef FDBA_COMMONAPI_H
#define FDBA_COMMONAPI_H

#include "simple.h"
#include <dballe/cursor.h>
#include <dballe/values.h>
#include <dballe/core/query.h>
#include <dballe/core/data.h>
#include <functional>
#include <cstring>

namespace dballe {
namespace fortran {

/**
 * Storage for currently queried attributes
 */
struct Attributes
{
    Values values;
    Values::const_iterator current;
    bool valid = false;

    /// Return the next attribute in the result set
    wreport::Varcode next();

    /// Mark the result set as invalid
    void invalidate();

    /**
     * Mark that there is a new set of values in values.
     *
     * The result set will be marked as valid, and the iteration will start
     * from the beginning
     */
    void has_new_values();
};

/**
 * Operation-specific behaviour for the API
 */
class Operation
{
public:
    virtual ~Operation();
    virtual void set_varcode(wreport::Varcode varcode);
    virtual void query_attributes(Attributes& dest) = 0;
    virtual void insert_attribute(Values& qcinput) = 0;
    virtual void remove_attributes() = 0;
    virtual bool next_station();
    virtual wreport::Varcode next_data();

    virtual int enqi(const char* param) const = 0;
    virtual signed char enqb(const char* param) const;
    virtual float enqr(const char* param) const;
    virtual double enqd(const char* param) const = 0;
    virtual bool enqc(const char* param, std::string& res) const = 0;
    virtual void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) const = 0;
    virtual void enqtimerange(int& ptype, int& p1, int& p2) const = 0;
    virtual void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec) const = 0;
};

namespace {

inline Level cursor_get_level(const CursorStation& c) { return Level(); }
inline Level cursor_get_level(const CursorStationData& c) { return Level(); }
inline Level cursor_get_level(const CursorData& c) { return c.get_level(); }
inline Level cursor_get_level(const CursorSummary& c) { return c.get_level(); }
inline Trange cursor_get_trange(const CursorStation& c) { return Trange(); }
inline Trange cursor_get_trange(const CursorStationData& c) { return Trange(); }
inline Trange cursor_get_trange(const CursorData& c) { return c.get_trange(); }
inline Trange cursor_get_trange(const CursorSummary& c) { return c.get_trange(); }
inline Datetime cursor_get_datetime(const CursorStation& c) { return Datetime(); }
inline Datetime cursor_get_datetime(const CursorSummary& c) { return Datetime(); }
inline Datetime cursor_get_datetime(const CursorStationData& c) { return Datetime(); }
inline Datetime cursor_get_datetime(const CursorData& c) { return c.get_datetime(); }

}

template<typename Cursor>
struct CursorOperation : public Operation
{
    std::shared_ptr<Cursor> cursor;

    ~CursorOperation()
    {
        if (cursor) cursor->discard();
    }

    int enqi(const char* param) const override
    {
        if (!cursor)
            throw wreport::error_consistency("enqi called before running a query");
        int res;
        if (!cursor->enqi(param, strlen(param), res))
            return API::missing_int;
        return res;
    }
    double enqd(const char* param) const override
    {
        if (!cursor)
            throw wreport::error_consistency("enqd called before running a query");
        double res;
        if (!cursor->enqd(param, strlen(param), res))
        {
            return API::missing_double;
        }
        return res;
    }
    bool enqc(const char* param, std::string& res) const override
    {
        if (!cursor)
            throw wreport::error_consistency("enqc called before running a query");
        return cursor->enqs(param, strlen(param), res);
    }
    void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) const override
    {
        Level lev = cursor_get_level(*cursor);
        ltype1 = lev.ltype1 != MISSING_INT ? lev.ltype1 : API::missing_int;
        l1     = lev.l1     != MISSING_INT ? lev.l1     : API::missing_int;
        ltype2 = lev.ltype2 != MISSING_INT ? lev.ltype2 : API::missing_int;
        l2     = lev.l2     != MISSING_INT ? lev.l2     : API::missing_int;
    }
    void enqtimerange(int& ptype, int& p1, int& p2) const override
    {
        Trange tr = cursor_get_trange(*cursor);
        ptype = tr.pind != MISSING_INT ? tr.pind : API::missing_int;
        p1    = tr.p1   != MISSING_INT ? tr.p1   : API::missing_int;
        p2    = tr.p2   != MISSING_INT ? tr.p2   : API::missing_int;
    }
    void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec) const override
    {
        Datetime dt = cursor_get_datetime(*cursor);
        year = dt.year != 0xffff ? dt.year : API::missing_int;
        month = dt.month != 0xff ? dt.month : API::missing_int;
        day = dt.day != 0xff ? dt.day : API::missing_int;
        hour = dt.hour != 0xff ? dt.hour : API::missing_int;
        min = dt.minute != 0xff ? dt.minute : API::missing_int;
        sec = dt.second != 0xff ? dt.second : API::missing_int;
    }
};

/**
 * Common implementation of the set* and enq* machinery using input and output
 * records.
 */
class CommonAPIImplementation : public API
{
public:
    enum Permissions {
        PERM_ANA_RO =       (1 << 0),
        PERM_ANA_WRITE =    (1 << 1),
        PERM_DATA_RO =      (1 << 2),
        PERM_DATA_ADD =     (1 << 3),
        PERM_DATA_WRITE =   (1 << 4),
        PERM_ATTR_RO =      (1 << 5),
        PERM_ATTR_WRITE =   (1 << 6)
    };

    /**
     * Set the permission bits, parsing the flags and doing consistency checks
     */
    static unsigned compute_permissions(const char* anaflag, const char* dataflag, const char* attrflag);

    unsigned perms = 0;

    core::Query input_query;
    /*
     * Fortran code wants to do something like set("var", …); unset("varlist").
     *
     * If both var and varlist edit input_query.varcodes, the unset of varlist
     * will also unset the previous set of var, with unexpected results.
     *
     * To work around this, var and varlist are stored in the following
     * members, and merged into input_query when validate_input_query() is
     * called.
     */
    wreport::Varcode input_query_var = 0;
    std::set<wreport::Varcode> input_query_varlist;

    core::Data input_data;
    /// Selected attribute varcodes (*varlist)
    std::vector<wreport::Varcode> selected_attr_codes;
    bool station_context = false;
    Values qcinput;
    Attributes qcoutput;

protected:
    Operation* operation = nullptr;

    // Last string returned by one of the spiega* functions, held here so
    // that we can deallocate it when needed.
    std::string cached_spiega;

    bool _seti(const char* key, unsigned len, int val);
    bool _setd(const char* key, unsigned len, double val);
    bool _setc(const char* key, unsigned len, const char* val);
    bool _unset(const char* key, unsigned len);
    void validate_input_query();

public:
    CommonAPIImplementation();
    CommonAPIImplementation(const CommonAPIImplementation&) = delete;
    CommonAPIImplementation(CommonAPIImplementation&&) = delete;
    virtual ~CommonAPIImplementation();
    CommonAPIImplementation& operator=(const CommonAPIImplementation&) = delete;
    CommonAPIImplementation& operator=(CommonAPIImplementation&&) = delete;

    template<typename Operation>
    auto reset_operation(Operation* op) -> decltype(op->run())
    {
        delete operation;
        operation = op;
        qcoutput.invalidate();
        return op->run();
    }

    void reset_operation()
    {
        delete operation;
        operation = nullptr;
        qcoutput.invalidate();
    }

    int enqi(const char* param) override;
    signed char enqb(const char* param) override;
    float enqr(const char* param) override;
    double enqd(const char* param) override;
    bool enqc(const char* param, std::string& res) override;
    void seti(const char* param, int value) override;
    void setb(const char* param, signed char value) override;
    void setr(const char* param, float value) override;
    void setd(const char* param, double value) override;
    void setc(const char* param, const char* value) override;
    void set_station_context() override;
    void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) override;
    void setlevel(int ltype1, int l1, int ltype2, int l2) override;
    void enqtimerange(int& ptype, int& p1, int& p2) override;
    void settimerange(int ptype, int p1, int p2) override;
    void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec) override;
    void setdate(int year, int month, int day, int hour, int min, int sec) override;
    void setdatemin(int year, int month, int day, int hour, int min, int sec) override;
    void setdatemax(int year, int month, int day, int hour, int min, int sec) override;
    void unset(const char* param) override;
    void unsetall() override;
    void unsetb() override;
    const char* describe_level(int ltype1, int l1, int ltype2, int l2) override;
    const char* describe_timerange(int ptype, int p1, int p2) override;
    const char* describe_var(const char* varcode, const char* value) override;
    void next_station() override;
    wreport::Varcode next_data() override;
    int query_attributes() override;
    const char* next_attribute() override;
    void insert_attribute() override;
    void remove_attributes() override;
    void commit() override;

    const Operation* test_get_operation() const { return operation; }

    const core::Query& test_get_input_query() const { return input_query; }
    const core::Data& test_get_input_data() const { return input_data; }
    const Values& test_get_qcinput() const { return qcinput; }

    friend class Operation;
};

}
}
#endif
