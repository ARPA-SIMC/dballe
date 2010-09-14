/*
 * wreport/vartable - Load variable information from on-disk tables
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

#ifndef WREPORT_VARTABLE_H
#define WREPORT_VARTABLE_H

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
 *    The ::WR_VAR macro can be used to construct wreport::Varcode values, and the
 *    ::WR_VAR_F, ::WR_VAR_X and ::WR_VAR_Y macros can be used to access the
 *    various parts of the wreport::Varcode.
 * \li \b wreport::Varinfo contains all the expanded information about a variable:
 *    its wreport::Varcode, description, measurement units, significant digits,
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
 * To construct a wreport::Varcode value one needs to provide three numbers: F, X
 * and Y.
 *
 * \li \b F (2 bits) identifies the type of table entry represented by the
 * wreport::Varcode, and is always 0 for B tables.  Different values are only used
 * during encoding and decoding of BUFR and CREX messages and are not in use in
 * other parts of DB-All.e.
 * \li \b X (6 bits) identifies a section of the table.
 * \li \b Y (8 bits) identifies the value within the section.  
 *
 * The normal text representation of a wreport::Varcode for a WMO B table uses the
 * format Bxxyyy.
 *
 * See @ref local_b_table for the contents of the local B table and their
 * relative wreport::Varcode values.
 */


#include <wreport/varinfo.h>
#include <vector>
#include <string>

namespace wreport {

/**
 * Holds a variable information table
 *
 * It never needs to be deallocated, as all the Vartable returned by
 * DB-ALLe are pointers to memory-cached versions that are guaranteed to exist
 * for all the lifetime of the program.
 */
class Vartable : public std::vector<_Varinfo>
{
protected:
	std::string m_id;

public:
	Vartable();
	~Vartable();

	/// Return the Vartable ID
	const std::string& id() const { return m_id; }

	/// Return true if the Vartable has been loaded
	bool loaded() const { return !m_id.empty(); }

	/// Load contents from the table with the given ID
	void load(const char* id);

	/**
	 * Query the Vartable. Throws an exception if not found.
	 *
	 * @param code
	 *   wreport::Varcode to query
	 * @return
	 *   the wreport::varinfo with the results of the query.
	 */
	Varinfo query(Varcode code) const;

	/**
	 * Query an altered version of the vartable
	 *
	 * @param var wreport::Varcode to query
	 * @param change
	 *   WMO C table entry specify a change on the variable characteristics
	 * @return
	 *   the ::dba_varinfo structure with the results of the query.  The returned
	 *   dba_varinfo is stored inside the dba_vartable, can be freely copied around
	 *   and does not need to be deallocated.
	 */
	Varinfo query_altered(Varcode var, Alteration change) const;

	/**
	 * Return a Vartable by id, loading it if necessary
	 *
	 * Once loaded, the table will be cached in memory for reuse, and
	 * further calls to get() will return the cached version.
	 *
	 * The cached tables are never deallocated, so the returned pointer is
	 * valid through the whole lifetime of the program.
	 *
	 * @param id
	 *   ID of the Vartable data to access
	 */
	static const Vartable* get(const char* id);

#if 0
	/**
	 * Query informations about the DBALLE variable definitions
	 * 
	 * @param code
	 *   The wreport::Varcode of the variable to query
	 * @retval info
	 *   the wreport::Varinfo structure with the results of the query.  The returned
	 *   Varinfo is stored inside the Vartable, can be freely copied around
	 *   and does not need to be deallocated.
	 * @return
	 *   The error indicator for this function (See @ref error.h)
	 */
	static Varinfo query_local(Varcode code);
#endif

	/// Convert a Vartable ID to a pathname
	static std::string id_to_pathname(const char* id);

	/**
	 * Check if the given Vartable exists on disk
	 * 
	 * @param id
	 *   ID of the Vartable data to check
	 * @return
	 *   true if the Vartable file exists, else false
	 */
	static bool exists(const char* id);
};

#if 0

/**
 * Query informations about the DBALLE variable definitions
 * 
 * @param code
 *   The ::dba_varcode of the variable to query
 * @param change
 *   WMO C table entry specify a change on the variable characteristics
 * @retval info
 *   the ::dba_varinfo structure with the results of the query.  The returned
 *   dba_varinfo is stored inside the Vartable, can be freely copied around
 *   and does not need to be deallocated.
 * @return
 *   The error indicator for this function (See @ref error.h)
 */
dba_err dba_varinfo_query_local_altered(dba_varcode code, dba_alteration change, const dba_varinfo* info);

/**
 * Get a reference to the local B table
 *
 * @retval table
 *   The local B table.  Vartable structures are never deallocated: the
 *   pointer will be valid through the entire lifetime of the program, and can
 *   be freely copied.
 * @return
 *   The error indicator for this function (See @ref error.h)
 */
dba_err dba_varinfo_get_local_table(Vartable* table);

/**
 * Return the ID for the given table
 *
 * @param table
 *   The table to query
 * @return
 *   The table id.  It is a pointer to the internal value, and does not need to
 *   be deallocated.
 */
const char* dba_vartable_id(dba_vartable table);

/**
 * Type of callback called by dba_vartable_iterate() on every member of a dba_vartable
 *
 * @param info
 *   Element of the table that is currently visited.
 * @param data
 *   Arbitrary user-supplied data given as the data parameter in dba_vartable_iterate()
 */
typedef void (*dba_vartable_iterator)(dba_varinfo info, void* data);

/**
 * Iterate through all elements in a dba_vartable
 *
 * @param table
 *   Table to iterate.
 * @param func
 *   Callback to be called for every item of the table.
 * @param data
 *   Arbitrary value passed as-is to the callback.
 * @return
 *   The error status (See @ref error.h)
 */
dba_err dba_vartable_iterate(dba_vartable table, dba_vartable_iterator func, void* data);
#endif

}

#endif
/* vim:set ts=4 sw=4: */
