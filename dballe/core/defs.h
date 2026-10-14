#ifndef DBA_MSG_DEFS_H
#define DBA_MSG_DEFS_H

/** @file
 * Common definitions
 */

#if defined(_DBALLE_TEST_CODE) || defined(_DBALLE_LIBRARY_CODE)
#define DBALLE_TEST_ONLY
#else
#define DBALLE_TEST_ONLY                                                       \
    [[deprecated("this is intended for DB-All.e unit tests only")]]
#endif

#endif
