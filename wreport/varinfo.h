/*
 * wreport/varinfo - Variable information
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef WREPORT_VARINFO_H
#define WREPORT_VARINFO_H

#include <stdint.h>

namespace wreport {

/** @file
 * @ingroup core
 * Implement fast access to information about WMO variables.
 *
 * The measured value of a physical quantity has little meaning without
 * specifying what quantity it represents, what units are used to measure it,
 * and how many digits are significant for the value.
 *
 * This module provides access to all this metadata:
 *
 * \li \b wreport::Varcode represents what is the quantity measured, and takes
 *    values from the WMO B tables used for BUFR and CREX encodings.
 *    The ::WR_VAR macro can be used to construct ::dba_varcode values, and the
 *    ::WR_VAR_F, ::WR_VAR_X and ::WR_VAR_Y macros can be used to access the
 *    various parts of the dba_varcode.
 * \li \b wreport::::Varinfo contains all the expanded information about a variable:
 *    its wreport::::Varcode, description, measurement units, significant digits,
 *    minimum and maximum values it can have and other information useful for
 *    serialisation and deserialisation of values.
 *
 * There are many B tables with slight differences used by different
 * meteorological centre or equipment.  This module allows to access 
 * different vartables using dba_vartable_create().
 *
 * DB-All.e provides a default B table called the "local B table" which is used
 * for the encoding-independent values.  The local B table has the desirable
 * property of having unambiguous entries for the various physical values:
 * normal B tables can have more than one, for example low accuracy and
 * high accuracy latitudes.  The local B table can be queried using
 * dba_varinfo_query_local().
 *
 * ::dba_vartable and ::dba_varinfo have special memory management: they are never
 * deallocated.  This is a precise design choice to speed up passing and
 * copying ::dba_varinfo values, that are used very intensely as they accompany
 * all the physical values processed by DB-All.e and its components.
 * This behaviour should not be a cause of memory leaks, since a software would
 * only need to access a limited amount of B tables during its lifetime.
 *
 * To construct a ::dba_varcode value one needs to provide three numbers: F, X
 * and Y.
 *
 * \li \b F (2 bits) identifies the type of table entry represented by the
 * dba_varcode, and is always 0 for B tables.  Different values are only used
 * during encoding and decoding of BUFR and CREX messages and are not in use in
 * other parts of DB-All.e.
 * \li \b X (6 bits) identifies a section of the table.
 * \li \b Y (8 bits) identifies the value within the section.  
 *
 * The normal text representation of a ::dba_varcode for a WMO B table uses the
 * format Bxxyyy.
 *
 * See @ref local_b_table for the contents of the local B table and their
 * relative ::dba_varcode values.
 */

/**
 * Holds the WMO variable code of a variable
 */
typedef short unsigned int Varcode;

/**
 * Create a WMO variable code from its F, X and Y components.
 */
#define WR_VAR(f, x, y) ((wreport::Varcode)( ((unsigned)(f)<<14) | ((unsigned)(x)<<8) | (unsigned)(y) ))

/**
 * Convert a XXYYY string to a WMO variable code.
 *
 * This is useful only in rare cases, such as when parsing tables; use
 * dba_descriptor_code to parse proper entry names such as "B01003" or "D21301".
 */
#define DBA_STRING_TO_VAR(str) ((wreport::Varcode)( \
		(( ((str)[0] - '0')*10 + ((str)[1] - '0') ) << 8) | \
		( ((str)[2] - '0')*100 + ((str)[3] - '0')*10 + ((str)[4] - '0') ) \
))

/**
 * Get the F part of a WMO variable code.
 */
#define WR_VAR_F(code) (((code) >> 14) & 0x3)
/**
 * Get the X part of a WMO variable code.
 */
#define WR_VAR_X(code) ((code) >> 8 & 0x3f)
/**
 * Get the Y part of a WMO variable code.
 */
#define WR_VAR_Y(code) ((code) & 0xff)

/**
 * Convert a FXXYYY string descriptor code into its short integer
 * representation.
 *
 * @param desc
 *   The 6-byte string descriptor as FXXYYY
 *
 * @return
 *   The short integer code that can be queried with the DBA_GET_* macros
 */
Varcode descriptor_code(const char* desc);


/**
 * Describes how a wreport::Varinfo has been altered: it is used for supporting
 * variables coming from BUFR and CREX messages that use C codes to alter
 * variable information.
 */
typedef short unsigned int Alteration;

/**
 * Create a variable alteration value
 */
#define DBA_ALT(width, scale) (((width)+128) << 8 | ((scale)+128))

/**
 * Read the width part of a variable alteration value
 */
#define DBA_ALT_WIDTH(code) (((code) >> 8) - 128)

/**
 * Read the scale part of a variable alteration value
 */
#define DBA_ALT_SCALE(code) (((code) & 0xff) - 128)

/**
 * Varinfo flags
 */
#define VARINFO_FLAG_STRING	0x01


/**
 * Holds the information about a DBALLE variable.
 *
 * It never needs to be deallocated, as all the Varinfo returned by DB-ALLe
 * are pointers to memory-cached versions that are guaranteed to exist for all
 * the lifetime of the program.
 */
struct _Varinfo
{
	/** The variable code.  See @ref WR_VAR, WR_VAR_X, WR_VAR_Y. */
	Varcode var;
	/** The variable description. */
	char desc[64];
	/** The measurement unit of the variable. */
	char unit[24];
	/** The scale of the variable.  When the variable is represented as an
	 * integer, it is multiplied by 10**scale */
	int scale;
	/** The reference value for the variable.  When the variable is represented
	 * as an integer, and after scaling, it is added this value */
	int ref;
	/** The length in digits of the integer representation of this variable
	 * (after scaling and changing reference value) */
	int len;
	/** The reference value for bit-encoding.  When the variable is encoded in
	 * a bit string, it is added this value */
	int bit_ref;
	/** The length in bits of the variable when encoded in a bit string (after
	 * scaling and changing reference value) */
	int bit_len;
	/** True if the variable is a string; false if it is a numeric value */
	int flags;
	/** Minimum unscaled value the field can have */
	int imin;
	/** Maximum unscaled value the field can have */
	int imax;
	/** Minimum scaled value the field can have */
	double dmin;
	/** Maximum scaled value the field can have */
	double dmax;
	/** C-table alteration that has been applied to this entry */
	Alteration alteration;
	/** Other altered versions of this Varinfo */
	mutable struct _Varinfo* alterations;
	/** The measurement unit of the variable when encoded in BUFR. */
	char bufr_unit[24];
	/** The scale of the variable when encoded in BUFR. */
	int bufr_scale;

	/// Reference count
	mutable int _ref;

	_Varinfo();

        /// Increment the reference count to this Data object
        void do_ref() const { ++_ref; }

        /**
         * Decrement the reference count to this Data object, and return true
         * if the reference count went down to 0
         */
        bool do_unref() const { return (--_ref) == 0; }

	/// Check if we are a string value
	bool is_string() const
	{
		return (flags & VARINFO_FLAG_STRING) != 0;
	}
 
	/**
	 * Encode a double value into an integer value using Varinfo encoding
	 * informations
	 *
	 * @param fval
	 *   Value to encode
	 * @returns
	 *   The double value encoded as an integer
	 */
	int encode_int(double fval) const throw ();

	/**
	 * Decode a double value from integer value using Varinfo encoding
	 * informations
	 *
	 * @param val
	 *   Value to decode
	 * @returns
	 *   The decoded double value
	 */
	double decode_int(int val) const throw ();

	/**
	 * Decode a double value from integer value using Varinfo encoding
	 * informations for BUFR
	 *
	 * @param val
	 *   Value to decode
	 * @returns
	 *   The decoded double value
	 */
	double bufr_decode_int(uint32_t val) const throw ();

	/**
	 * Set all fields to 0, except the reference count
	 */
	void reset();

	/**
	 * Set the values all in one shot.
	 *
	 * It also calls compute_range
	 */
	void set(Varcode var, const char* desc, const char* unit, int scale = 0, int ref = 0, int len = 0, int bit_ref = 0, int bit_len = 0, int flags = 0, const char* bufr_unit = 0, int bufr_scale = 0);

	/**
	 * Compute the valid variable range and store it in the *min and *max
	 * fields
	 */
	void compute_range();
};

class Varinfo;

/// Smart pointer to handle/use varinfos
class MutableVarinfo
{
protected:
	_Varinfo* impl;

public:
	MutableVarinfo(_Varinfo* impl) : impl(impl) { impl->do_ref(); }
	MutableVarinfo(const MutableVarinfo& vi) : impl(vi.impl) { impl->do_ref(); }
	~MutableVarinfo() { if (impl->do_unref()) delete impl; }

	MutableVarinfo& operator=(const MutableVarinfo& vi)
	{
		vi.impl->do_ref();
		if (impl->do_unref()) delete impl;
		impl = vi.impl;
		return *this;
	}

	_Varinfo* operator->() { return impl; }
	_Varinfo& operator*() { return *impl; }

	/**
	 * Create a single use varinfo structure.
	 *
	 * A single use varinfo structure is not memory managed by a vartable
	 * and needs to be deallocated explicitly when it is not needed
	 * anymore.
	 *
	 * The various fields of the resulting varinfo will be zeroed, except var (set
	 * to \a code) and flags (which will have VARINFO_FLAG_SINGLEUSE set)
	 * 
	 * @param code
	 *   The wreport::Varcode of the variable to query
	 */
	static MutableVarinfo create_singleuse(Varcode code);

	friend class wreport::Varinfo;
};

/// Smart pointer to handle/use varinfos
class Varinfo
{
protected:
	const _Varinfo* m_impl;

public:
	Varinfo(const _Varinfo* impl) : m_impl(impl) { m_impl->do_ref(); }
	Varinfo(const Varinfo& vi) : m_impl(vi.m_impl) { m_impl->do_ref(); }
	Varinfo(const MutableVarinfo& vi) : m_impl(vi.impl) { m_impl->do_ref(); }
	~Varinfo() { if (m_impl->do_unref()) delete m_impl; }

	const Varinfo& operator=(const Varinfo& vi)
	{
		vi.m_impl->do_ref();
		if (m_impl->do_unref()) delete m_impl;
		m_impl = vi.m_impl;
		return *this;
	}

	const _Varinfo& operator*() const { return *m_impl; }
	const _Varinfo* operator->() const { return m_impl; }
	const _Varinfo* impl() const { return m_impl; }
};

}

#endif
/* vim:set ts=4 sw=4: */
