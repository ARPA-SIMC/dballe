#ifndef DBA_RAWFILE_H
#define DBA_RAWFILE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/io/dba_rawmsg.h>
#include <stdio.h>

struct _dba_rawfile
{
	char* name;
	FILE* fd;
	int close_on_exit;
	int idx;
};
typedef struct _dba_rawfile* dba_rawfile;

dba_err dba_rawfile_create(dba_rawfile* file, const char* name, const char* mode);
void dba_rawfile_delete(dba_rawfile file);

/*
 * Write the encoded message data to the file
 *
 * @param file
 *   The file to write to
 * @param msg
 *   The ::dba_rawmsg with the encoded data to write
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_rawfile_write(dba_rawfile file, dba_rawmsg msg);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
