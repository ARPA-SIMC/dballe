/** @file
 * Debugging tracing functions
 */
#ifndef DBALLE_CORE_TRACE_H
#define DBALLE_CORE_TRACE_H

/*
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
