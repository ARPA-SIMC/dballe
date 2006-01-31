#ifndef DBA_CONV
#define DBA_CONV

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup conv
 * Unit conversion functions.
 */

#include <dballe/err/dba_error.h>

/**
 * Convert between CREX or BUFR units
 *
 * @param from
 *   Unit of the value to convert (see ::dba_varinfo)
 * @param to
 *   Unit to convert to (see ::dba_varinfo)
 * @param val
 *   Value to convert
 * @retval res
 *   Converted value
 * @returns
 *   The error indicator for the function (@see ::dba_err)
 */
dba_err dba_convert_units(const char* from, const char* to, double val, double* res);

/**
 * Convert ICAO height (in meters) to pressure (in hpa) and back
 */
dba_err dba_convert_icao_to_press(double from, double* to);

/**
 * Convert pressure (in hpa) to ICAO height (in meters)
 */
dba_err dba_convert_press_to_icao(double from, double* to);

/**
 * Convert vertical sounding significance from the AOF encoding to BUFR code
 * table 08001.
 */
dba_err dba_convert_AOFVSS_to_BUFR08001(int from, int* to);

/**
 * Conversion functions between various code tables
 * @{ */
/* Cloud type */
dba_err dba_convert_WMO0500_to_BUFR20012(int from, int* to);
/* Cloud type (CH) */
dba_err dba_convert_WMO0509_to_BUFR20012(int from, int* to);
/* Cloud type (CM) */
dba_err dba_convert_WMO0515_to_BUFR20012(int from, int* to);
/* Cloud type (CL) */
dba_err dba_convert_WMO0513_to_BUFR20012(int from, int* to);
/* Present weather */
dba_err dba_convert_WMO4677_to_BUFR20003(int from, int* to);
/* Past weather */
dba_err dba_convert_WMO4561_to_BUFR20004(int from, int* to);

dba_err dba_convert_BUFR20012_to_WMO0500(int from, int* to);
dba_err dba_convert_BUFR20012_to_WMO0509(int from, int* to);
dba_err dba_convert_BUFR20012_to_WMO0515(int from, int* to);
dba_err dba_convert_BUFR20012_to_WMO0513(int from, int* to);
dba_err dba_convert_BUFR20003_to_WMO4677(int from, int* to);
dba_err dba_convert_BUFR20004_to_WMO4561(int from, int* to);
/* @} */

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
