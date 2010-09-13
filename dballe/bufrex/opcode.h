/*
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

#ifndef BUFREX_OPCODE_H
#define BUFREX_OPCODE_H

/** @file
 * @ingroup bufrex
 * Implementation of opcode chains, that are used to drive the encoding and
 * decoding process.
 */

#include <dballe/core/varinfo.h>
#include <vector>

namespace bufrex {

struct Opcodes
{
	const std::vector<dballe::Varcode>& vals;
	unsigned begin;
	unsigned end;

	Opcodes(const std::vector<dballe::Varcode>& vals) : vals(vals), begin(0), end(vals.size()) {}
	Opcodes(const std::vector<dballe::Varcode>& vals, unsigned begin, unsigned end)
		: vals(vals), begin(begin), end(end) {}
	Opcodes(const Opcodes& o) : vals(o.vals), begin(o.begin), end(o.end) {}

	/**
	 * Assignment only works if the Opcodes share the same vector.
	 *
	 * @warning: for efficiency reasons, we do not check for it
	 */
	Opcodes& operator=(const Opcodes& o)
	{
		begin = o.begin;
		end = o.end;
		return *this;
	}

	/// Return the i-th varcode in the chain
	dballe::Varcode operator[](unsigned i) const
	{
		if (begin + i > end)
			return 0;
		else
			return vals[begin + i];
	}

	/// Number of items in this opcode list
	unsigned size() const { return end - begin; }

	/// True if there are no opcodes
	bool empty() const { return begin == end; }

	/// First opcode in the list (0 if the list is empty)
	dballe::Varcode head() const
	{
		if (begin == end)
			return 0;
		return vals[begin];
	}

	/**
	 * List of all opcodes after the first one
	 *
	 * If the list is empty, return the empty list
	 */
	Opcodes next() const
	{
		if (begin == end)
			return *this;
		else
			return Opcodes(vals, begin+1, end);
	}

	Opcodes sub(unsigned skip) const
	{
		if (begin + skip > end)
			return Opcodes(vals, end, end);
		else
			return Opcodes(vals, begin + skip, end);
	}

	Opcodes sub(unsigned skip, unsigned len) const
	{
		if (begin + skip > end)
			return Opcodes(vals, end, end);
		else if (begin + skip + len > end)
			return Opcodes(vals, begin + skip, end);
		else
			return Opcodes(vals, begin + skip, begin + skip + len);
	}

	/**
	 * Print the contents of this opcode list
	 *
	 * This function is used mainly for debugging purposes.
	 *
	 * @param outstream
	 *   The output stream (a FILE* variable) to print to.  void* is used to avoid
	 *   including stdio.h just for this debugging function.
	 */
	void print(void* outstream) const;
};

#if 0

/**
 * List node containing an entry of a BUFR or CREX data descriptor section.
 */
struct _bufrex_opcode
{
	/** Data descriptor as a B, C or D table entry.  See @ref vartable.h */
	dballe::Varcode val;
	/** Next data descriptor in the list */
	struct _bufrex_opcode* next;
};
/** @copydoc _bufrex_opcode */
typedef struct _bufrex_opcode* bufrex_opcode;

/**
 * Delete a bufrex_opcode chain.
 *
 * @param entry
 *   The chain to be deleted.  After the call, entry will be NULL.
 */
void bufrex_opcode_delete(bufrex_opcode* entry);

/**
 * Append an entry to an opcode chain
 *
 * @param entry
 *   opcode entry to append the entry to
 * @param value
 *   Value to append to the entry.  See @ref vartable.h
 * @return
 *   The error indicator for the function
 */
dba_err bufrex_opcode_append(bufrex_opcode* entry, dballe::Varcode value);

/**
 * Duplicate an opcode chain and insert it before an existing bufrex_opcode
 *
 * To insert a chain in a specific point of an existing chain, do this:
 * \code
 *   DBA_RUN_OR_RETURN(bufrex_opcode_prepend(&(oldnode->next), newchain));
 * \endcode
 * 
 * This can be used to duplicate opcode chains, as in:
 * \code
 *   bufrex_opcode newchain = NULL;
 *   DBA_RUN_OR_RETURN(bufrex_opcode_prepend(&newchain, oldchain));
 * \endcode
 *
 * @param dest
 *   The bufrex_opcode before which the chain is to be copied
 *
 * @param src
 *   The bufrex_opcode chain to copy
 *
 * @returns
 *   The error indicator for the function
 */
dba_err bufrex_opcode_prepend(bufrex_opcode* dest, bufrex_opcode src);

/**
 * Attach a bufrex_opcode chain to the end of another
 *
 * @param op1
 *   The first bufrex_opcode chain to be joined
 * @param op2
 *   bufrex_opcode chain to be attached to the end of op1
 * @returns
 *   The error indicator for the function
 */
dba_err bufrex_opcode_join(bufrex_opcode* op1, bufrex_opcode op2);

/**
 * Detach the first element from a bufrex_opcode list
 *
 * @param chain
 *   The bufrex_opcode chain whose first element is to be detached.
 *
 * @param head
 *   Where will be stored the detached first element of the opcode list.
 *
 * @returns
 *   The error indicator for the function
 */
dba_err bufrex_opcode_pop(bufrex_opcode* chain, bufrex_opcode* head);

/**
 * Detach the first `length' elements from a bufrex_opcode list
 *
 * @param chain
 *   The bufrex_opcode chain whose first element is to be detached.
 *
 * @param head
 *   Where will be stored the detached first element of the opcode list.
 *
 * @param length
 *   Number of items to detach
 *
 * @returns
 *   The error indicator for the function
 */
dba_err bufrex_opcode_pop_n(bufrex_opcode* chain, bufrex_opcode* head, int length);

#if 0
/**
 * Return the next opcode in an opcode chain
 *
 * @param entry
 *   Element of an opcode chain
 *
 * @returns
 *   The element in the chain after `entry'
 */
#define bufrex_opcode_next(entry) ((entry)->next)
/*bufrex_opcode bufrex_opcode_next(bufrex_opcode entry);*/

/**
 * Copy the first `length' elements of an opcode chain
 *
 * @param entry
 *   The chain to be copied
 *
 * @param length
 *   Number of elements to copy
 * 
 * @returns
 *   The copy of the first `length' elements of the chain
 */
bufrex_opcode bufrex_opcode_copy_n(bufrex_opcode entry, int length);

/* void bufrex_opcode_print(bufrex_opcode entry, FILE* out); */
#endif

#endif

}

#endif
