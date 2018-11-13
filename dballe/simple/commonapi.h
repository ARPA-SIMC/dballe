#ifndef FDBA_COMMONAPI_H
#define FDBA_COMMONAPI_H

#include "simple.h"
#include <dballe/cursor.h>
#include <dballe/core/record.h>
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
    core::Values values;
    core::Values::const_iterator current;
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
protected:
    /// Selected varcodes
    std::vector<wreport::Varcode> selected_attr_codes;

public:
    virtual ~Operation();
    virtual void set_varcode(wreport::Varcode varcode);
    virtual void select_attrs(const std::vector<wreport::Varcode>& varcodes)
    {
        selected_attr_codes = varcodes;
    }
    virtual void voglioancora(Attributes& dest) = 0;
    virtual void critica(core::Values& qcinput) = 0;
    virtual void scusa() = 0;
    virtual bool elencamele();
    virtual wreport::Varcode dammelo(dballe::Record& output);

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
    core::Data input_data;
    bool station_context = false;
    core::Record output;
    core::Values qcinput;
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
    void setcontextana() override;
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
    const char* spiegal(int ltype1, int l1, int ltype2, int l2) override;
    const char* spiegat(int ptype, int p1, int p2) override;
    const char* spiegab(const char* varcode, const char* value) override;
    void elencamele() override;
    wreport::Varcode dammelo() override;
    int voglioancora() override;
    const char* ancora() override;
    void critica() override;
    void scusa() override;
    void fatto() override;

    const Operation* test_get_operation() const { return operation; }

    const core::Query& test_get_input_query() const { return input_query; }
    const core::Data& test_get_input_data() const { return input_data; }
    const core::Record& test_get_output() const { return output; }
    const core::Values& test_get_qcinput() const { return qcinput; }
    // const core::Record& test_get_qcoutput() const { return qcoutput; }

    friend class Operation;
};

}
}
#endif
