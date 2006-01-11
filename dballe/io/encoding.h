#ifndef DBA_IO_ENCODING_H
#define DBA_IO_ENCODING_H

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * Supported encodings for report data
 */
typedef enum {
	BUFR = 0,
	CREX = 1,
	AOF = 2
} dba_encoding;

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
