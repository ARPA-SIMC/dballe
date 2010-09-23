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

#ifndef FDBA_HANDLES_H
#define FDBA_HANDLES_H

#include <wreport/error.h>
#include <cassert>

namespace dballe {
struct DB;

namespace fortran {
struct API;

#if 0
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
#endif

struct HBase
{
	bool used;

	HBase() : used(false) {}

	void start() { used = true; }
	void stop() { used = false; }
};

template<typename T, int MAX>
struct Handler
{
	T* records;
	int next;
	const char* name;
	const char* def;
	size_t in_use;

	void init(const char* name, const char* def)
	{
		records = new T[MAX];
		next = 0;
		in_use = 0;
	}

	T& get(int id)
	{
		assert(records[id].used);
		return records[id];
	}

	int request()
	{
		for (int i = 0; i < MAX && (records[next].used); ++i)
			next = (next + 1) % MAX;
		if (records[next].used)
			wreport::error_handles::throwf("No more handles for %s. The maximum limit is %d: to increase it, recompile DB-All.e setting %s to a higher value", name, MAX, def);
		/* Setup the new handle */
		records[next].start();
		++in_use;
		return next;
	}

	void release(int h)
	{
		assert(in_use);
		assert(records[h].used);
		if (records[h].used)
		{
			records[h].stop();
			--in_use;
		}
	}
};

/// Initialise error handlers
void error_init();

}
}

#endif
