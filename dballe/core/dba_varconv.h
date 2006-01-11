#ifndef DBA_VARCONV_H
#define DBA_VARCONV_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/core/dba_var.h>

/**
 * Convert between CREX or BUFR variables
 *
 * @param bufrex
 *   The ::dba_varcode of the variable as used in BUFR of CREX messages
 * @retval local
 *   The corresponding DBALLE ::dba_varcode
 * @returns
 *   The error indicator for the function (@see ::dba_err)
 */
dba_err dba_convert_vars(dba_varcode bufrex, dba_varcode* local);

#ifdef  __cplusplus
}
#endif

#endif
