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

#ifndef FDBA_HANDLES_H
#define FDBA_HANDLES_H

#include <dballe/core/record.h>
#include <dballe/db/db.h>
#include <assert.h>

#define FDBA_HANDLE_START_DECL(name) \
	struct fdba_handle_##name { \
		int used;

#define FDBA_HANDLE_END_DECL(name) }; \
	extern struct fdba_handle_##name _##name[]; \
	void fdba_handle_init_##name(); \
	dba_err fdba_handle_alloc_##name(int* res); \
	void fdba_handle_release_##name(int hnd);


#define FDBA_HANDLE_BODY(name, size, desc) \
	struct fdba_handle_##name _##name[size]; \
	static int _##name##_last = 0; \
	/* Initialize the handle storage */ \
	void fdba_handle_init_##name() { \
		int i; \
		for (i = 0; i < size; i++) \
			_##name[i].used = 0; \
	} \
	/* Allocate a new handle */ \
	dba_err fdba_handle_alloc_##name(int* res) { \
		if (_##name##_last < size) { \
			*res = _##name##_last++; \
		} else { \
			int i, found = 0; \
			for (i = 0; i < size && !found; i++) \
				if (_##name[i].used == 0) { \
					*res = i; \
					found = 1; \
				} \
			if (!found) \
				return dba_error_handles("No more handles for " desc ". The maximum limit is %d: to increase it, recompile dballe setting " #size " to a higher value", size); \
		} \
		/* Setup the new handle */ \
		_##name[*res].used = 1; \
		return dba_error_ok(); \
	} \
	/* Release a handle */ \
	void fdba_handle_release_##name(int hnd) { \
		assert(hnd < size); \
		assert(_##name[hnd].used == 1); \
		_##name[hnd].used = 0; \
		if (hnd == _##name##_last - 1) \
			_##name##_last--; \
	}

#define FDBA_HANDLE(name, hnd) (_##name[hnd])

FDBA_HANDLE_START_DECL(session)
	dba_db session;
FDBA_HANDLE_END_DECL(session)

FDBA_HANDLE_START_DECL(simple)
	int session;
	int perms;
	dba_record input;
	dba_record output;
	dba_record qcinput;
	dba_record qcoutput;
	dba_db_cursor ana_cur;
	dba_db_cursor query_cur;
	dba_record_cursor qc_iter;
	int qc_count;
FDBA_HANDLE_END_DECL(simple)

typedef void (*fdba_error_callback)(INTEGER(data));

FDBA_HANDLE_START_DECL(errcb)
	int error;
	fdba_error_callback cb;
	int data;
FDBA_HANDLE_END_DECL(errcb)

#endif
