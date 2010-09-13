/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "config.h"

#include "opcode.h"

#include <stdlib.h>	/* malloc */
#include <string.h>	/* memcpy */
#include <assert.h>

/*
struct _bufrex_opcode
{
	char val[7];
	struct _bufrex_opcode* next;
};
*/

void bufrex_opcode_delete(bufrex_opcode* entry)
{
	/* If we have an empty chain, we have nothing to delete */
	if (*entry == NULL)
		return;
	
	/* Delete the subchain, if it exists */
	if ((*entry)->next != NULL)
		bufrex_opcode_delete(&((*entry)->next));

	/* Lastly, delete the node itself */
	free(*entry);
	*entry = 0;
}

dba_err bufrex_opcode_append(bufrex_opcode* entry, dba_varcode value)
{
	if (*entry == NULL)
	{
		*entry = (bufrex_opcode)malloc(sizeof(struct _bufrex_opcode));
		if (*entry == NULL)
			return dba_error_alloc("creating new BUFREX opcode");
		(*entry)->val = value;
		(*entry)->next = 0;
		return dba_error_ok();
	} else
		return bufrex_opcode_append(&((*entry)->next), value);
}

dba_err bufrex_opcode_prepend(bufrex_opcode* dest, bufrex_opcode src)
{
	if (src == NULL)
		return dba_error_ok();

	/* Duplicate src and prepend it to 'dest' */
	bufrex_opcode next;
	next = (bufrex_opcode)malloc(sizeof(struct _bufrex_opcode));
	if (next == NULL)
		return dba_error_alloc("creating new BUFREX opcode");
	next->val = src->val;
	next->next = (*dest);
	(*dest) = next;

	return bufrex_opcode_prepend(&((*dest)->next), src->next);
}

dba_err bufrex_opcode_join(bufrex_opcode* op1, bufrex_opcode op2)
{
	if (*op1 == NULL)
	{
		*op1 = op2;
		return dba_error_ok();
	}
	else
		return bufrex_opcode_join(&((*op1)->next), op2);
}

dba_err bufrex_opcode_pop(bufrex_opcode* chain, bufrex_opcode* head)
{
	if (*chain == NULL)
	{
		*head = NULL;
		return dba_error_ok();
	}
	
	*head = *chain;
	*chain = (*chain)->next;
	(*head)->next = NULL;

	return dba_error_ok();
}

dba_err bufrex_opcode_pop_n(bufrex_opcode* chain, bufrex_opcode* head, int length)
{
	if (length == 0 || *chain == NULL)
		return dba_error_ok();

	DBA_RUN_OR_RETURN(bufrex_opcode_pop(chain, head));

	return bufrex_opcode_pop_n(chain, &((*head)->next), length - 1);
}


#if 0

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
bufrex_opcode bufrex_opcode_copy_n(bufrex_opcode entry, int length)
{
	bufrex_opcode res = bufrex_opcode_create(entry->val);
	res->refval = entry->refval;

	if (entry->next != NULL && length > 1)
		res->next = bufrex_opcode_copy_n(entry->next, length - 1);

	return res;
}

#endif

#include <stdio.h>
void bufrex_opcode_print(bufrex_opcode entry, void* outstream)
{
	FILE* out = (FILE*)outstream;

	if (entry == NULL)
	{
		fprintf(out, "(null)");
		return;
	}
	fprintf(out, "%d%02d%03d ",
			DBA_VAR_F(entry->val),
			DBA_VAR_X(entry->val),
			DBA_VAR_Y(entry->val));

	if (entry->next != NULL)
		bufrex_opcode_print(entry->next, out);
}

/* vim:set ts=4 sw=4: */
