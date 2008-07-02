/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "simple.h"
#include <dballe/core/record.h>
#include <dballe/core/error.h>

namespace dballef
{

/**
 * Simplified way to convey a DB-All.e error status as an exception
 */
struct APIException
{
	dba_err err;
	APIException(dba_err err) : err(err) {}
};

/**
 * Turn DB-All.e error codes into exceptions
 */
inline void checked(dba_err err)
{
	if (err != DBA_OK)
		throw APIException(err);
}



/**
 * Common implementation of the set* and enq* machinery using input and output
 * records.
 */
class CommonAPIImplementation : public API
{
protected:
	enum {
		PERM_ANA_RO =		(1 << 0),
		PERM_ANA_WRITE =	(1 << 1),
		PERM_DATA_RO =		(1 << 2),
		PERM_DATA_ADD =		(1 << 3),
		PERM_DATA_WRITE =	(1 << 4),
		PERM_ATTR_RO =		(1 << 5),
		PERM_ATTR_ADD =		(1 << 6),
		PERM_ATTR_WRITE	=	(1 << 7)
	};

	int perms;
	dba_record input;
	dba_record output;
	dba_record qcinput;
	dba_record qcoutput;
	dba_record_cursor qc_iter;
	int qc_count;
	// Last string returned by one of the spiega* functions, held here so
	// that we can deallocate it when needed.
	char* cached_spiega;

	/**
	 * Set the permission bits, parsing the flags and doing consistency checks
	 */
	void set_permissions(const char* anaflag, const char* dataflag, const char* attrflag);

	/**
	 * Choose the input record to use for param.  Also, adjust param to remove
	 * a leading '*' if present.
	 */
	dba_record choose_input_record(const char*& param);

	/**
	 * Choose the output record to use for param.  Also, adjust param to remove
	 * a leading '*' if present.
	 */
	dba_record choose_output_record(const char*& param);

	/**
	 * Look for the ID of the data which a critica or scusa operation are
	 * supposed to operate on.
	 */
	void get_referred_data_id(int* id_context, dba_varcode* id_var) const;

	/// Reads the list of QC values to operate on, for dba_voglioancora and dba_scusa
	void read_qc_list(dba_varcode** res_arr, size_t* res_len) const;

	/**
	 * Clear the qcinput record preserving DBA_KEY_CONTEXT_ID and
	 * DBA_KEY_VAR_RELATED
	 */
	void clear_qcinput();

public:
	CommonAPIImplementation();
	virtual ~CommonAPIImplementation();

	virtual int enqi(const char* param) = 0;
	virtual signed char enqb(const char* param) = 0;
	virtual float enqr(const char* param) = 0;
	virtual double enqd(const char* param) = 0;
	virtual const char* enqc(const char* param) = 0;

	virtual void seti(const char* param, int value) = 0;
	virtual void setb(const char* param, signed char value) = 0;
	virtual void setr(const char* param, float value) = 0;
	virtual void setd(const char* param, double value) = 0;
	virtual void setc(const char* param, const char* value) = 0;

	virtual void setcontextana() = 0;

	virtual void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) = 0;
	virtual void setlevel(int ltype1, int l1, int ltype2, int l2) = 0;

	virtual void enqtimerange(int& ptype, int& p1, int& p2) = 0;
	virtual void settimerange(int ptype, int p1, int p2) = 0;

	virtual void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec) = 0;
	virtual void setdate(int year, int month, int day, int hour, int min, int sec) = 0;

	virtual void setdatemin(int year, int month, int day, int hour, int min, int sec) = 0;
	virtual void setdatemax(int year, int month, int day, int hour, int min, int sec) = 0;

	virtual void unset(const char* param) = 0;
	virtual void unsetall() = 0;

	virtual const char* spiegal(int ltype1, int l1, int ltype2, int l2) = 0;
	virtual const char* spiegat(int ptype, int p1, int p2) = 0;
	virtual const char* spiegab(const char* varcode, const char* value) = 0;


	virtual const char* ancora();
};

}

/* vim:set ts=4 sw=4: */
