#ifndef FDBA_COMMONAPI_H
#define FDBA_COMMONAPI_H

#include "simple.h"
#include <dballe/core/record.h>
#include <dballe/core/query.h>
#include <dballe/core/data.h>
#include <dballe/db/fwd.h>
#include <functional>

namespace dballe {
namespace fortran {

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
    virtual void voglioancora(db::Transaction& tr, std::vector<wreport::Var>& dest) = 0;
    virtual void critica(db::Transaction& tr, const core::Values& qcinput) = 0;
    virtual void scusa(db::Transaction& tr) = 0;
    virtual wreport::Varcode dammelo(dballe::Record& output);
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

protected:
    unsigned perms;
    core::Query input_query;
    core::Data input_data;
    bool station_context = false;
    core::Record output;
    core::Values qcinput;
    // TODO: replace with Values, reimplementing Values merging with wreport's sorted vector
    std::vector<wreport::Var> qcoutput;
    int qc_iter;
    int qc_count;

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
    const char* ancora() override;
    void fatto() override;

    const Operation* test_get_operation() const { return operation; }

    const core::Query& test_get_input_query() const { return input_query; }
    const core::Data& test_get_input_data() const { return input_data; }
    const core::Record& test_get_output() const { return output; }
    const core::Values& test_get_qcinput() const { return qcinput; }
    // const core::Record& test_get_qcoutput() const { return qcoutput; }
};

}
}
#endif
