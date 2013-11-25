/*
 * core/trace - Debugging trace functions
 *
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#ifndef DBALLE_CORE_TRACE_H
#define DBALLE_CORE_TRACE_H

/**
 * Include this file if you want to enable trace functions in a source
 *
 * The trace functions are not compiled unless you #define TRACE_SOURCE
 * before including this header.
 */
#ifdef TRACE_SOURCE
#include <cstdio>
// Output a trace message
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
// Prefix a block of code to compile only if trace is enabled
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif

#endif
