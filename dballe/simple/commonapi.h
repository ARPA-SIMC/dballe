#ifndef FDBA_COMMONAPI_H
#define FDBA_COMMONAPI_H

#include "simple.h"
#include <dballe/core/record.h>
#include <dballe/db/fwd.h>
#include <functional>

namespace dballe {
namespace fortran {

/**
 * Operation-specific behaviour for the API
 */
struct Operation
{
    virtual ~Operation();
    virtual void set_varcode(wreport::Varcode varcode);
    virtual void voglioancora(db::Transaction& tr, std::function<void(std::unique_ptr<wreport::Var>&&)> dest) = 0;
    virtual void critica(db::Transaction& tr, const core::Record& qcinput) = 0;
    virtual void scusa(db::Transaction& tr, const std::vector<wreport::Varcode>& attrs) = 0;
    virtual const char* dammelo(dballe::Record& output);
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
    core::Record input;
    bool station_context = false;
    core::Record output;
    core::Record qcinput;
    core::Record qcoutput;
    int qc_iter;
    int qc_count;

    Operation* operation = nullptr;

	// Last string returned by one of the spiega* functions, held here so
	// that we can deallocate it when needed.
	std::string cached_spiega;

	/**
	 * Choose the input record to use for param.  Also, adjust param to remove
	 * a leading '*' if present.
	 */
	Record& choose_input_record(const char*& param);

	/**
	 * Choose the output record to use for param.  Also, adjust param to remove
	 * a leading '*' if present.
	 */
	Record& choose_output_record(const char*& param);

	/// Reads the list of QC values to operate on, for dba_voglioancora and dba_scusa
	void read_qc_list(std::vector<wreport::Varcode>& res_arr) const;

public:
    CommonAPIImplementation();
    virtual ~CommonAPIImplementation();

    void test_input_to_output() override;
    int enqi(const char* param) override;
    signed char enqb(const char* param) override;
    float enqr(const char* param) override;
    double enqd(const char* param) override;
    const char* enqc(const char* param) override;
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

    const core::Record& test_get_input() const { return input; }
    const core::Record& test_get_output() const { return output; }
    const core::Record& test_get_qcinput() const { return qcinput; }
    const core::Record& test_get_qcoutput() const { return qcoutput; }
};

}
}
#endif
