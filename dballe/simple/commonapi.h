#ifndef FDBA_COMMONAPI_H
#define FDBA_COMMONAPI_H

#include "simple.h"
#include <dballe/core/record.h>

namespace dballe {
namespace fortran {

/**
 * Common implementation of the set* and enq* machinery using input and output
 * records.
 */
class CommonAPIImplementation : public API
{
public:
    enum AttrState {
        /// After a dammelo
        ATTR_DAMMELO =   1,
        /// After a prendilo
        ATTR_PRENDILO =  2,
        /// After a set *context_id or *var_related
        ATTR_REFERENCE = 3,
    };

protected:
	enum {
		PERM_ANA_RO =		(1 << 0),
		PERM_ANA_WRITE =	(1 << 1),
		PERM_DATA_RO =		(1 << 2),
		PERM_DATA_ADD =		(1 << 3),
		PERM_DATA_WRITE =	(1 << 4),
		PERM_ATTR_RO =		(1 << 5),
		PERM_ATTR_WRITE	=	(1 << 6)
	};

	int perms;
    core::Record input;
    bool station_context = false;
    core::Record output;
    core::Record qcinput;
    core::Record qcoutput;
	int qc_iter;
	int qc_count;

    AttrState attr_state;

    // Varcode of the variable referred to by the next attribute operations
    wreport::Varcode attr_varid;

    // Reference ID of the variable referred to by the next attribute operations
    int attr_reference_id;

	// Last string returned by one of the spiega* functions, held here so
	// that we can deallocate it when needed.
	std::string cached_spiega;

	/**
	 * Set the permission bits, parsing the flags and doing consistency checks
	 */
	void set_permissions(const char* anaflag, const char* dataflag, const char* attrflag);

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

	virtual void test_input_to_output();

	virtual int enqi(const char* param);
	virtual signed char enqb(const char* param);
	virtual float enqr(const char* param);
	virtual double enqd(const char* param);
	virtual const char* enqc(const char* param);

	virtual void seti(const char* param, int value);
	virtual void setb(const char* param, signed char value);
	virtual void setr(const char* param, float value);
	virtual void setd(const char* param, double value);
	virtual void setc(const char* param, const char* value);

	virtual void setcontextana();

	virtual void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2);
	virtual void setlevel(int ltype1, int l1, int ltype2, int l2);

	virtual void enqtimerange(int& ptype, int& p1, int& p2);
	virtual void settimerange(int ptype, int p1, int p2);

	virtual void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec);
	virtual void setdate(int year, int month, int day, int hour, int min, int sec);

	virtual void setdatemin(int year, int month, int day, int hour, int min, int sec);
	virtual void setdatemax(int year, int month, int day, int hour, int min, int sec);

    virtual void unset(const char* param);
    virtual void unsetall();
    virtual void unsetb();

	virtual const char* spiegal(int ltype1, int l1, int ltype2, int l2);
	virtual const char* spiegat(int ptype, int p1, int p2);
	virtual const char* spiegab(const char* varcode, const char* value);


	virtual const char* ancora();

    AttrState test_get_attr_state() const { return attr_state; }
};

}
}
#endif
