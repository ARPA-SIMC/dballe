#ifndef DBA_CSV_H
#define DBA_CSV_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Routines to parse data in CSV format
 */

#include <stdio.h>

/**
 * Parse a CSV line.
 *
 * @param in
 *   The file where to read from
 * @param cols
 *   The array of char* that will hold the parsed columns.  If the line has
 *   more than `cols' columns, the exceeding ones will be ignored.
 *   Please note that you have to deallocate all the lines returned in cols.
 * @param col_max
 *   The size of cols
 * @return
 *   The number of columns found, or 0 if we hit the end of the file
 */
int dba_csv_read_next(FILE* in, char** cols, int col_max);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
