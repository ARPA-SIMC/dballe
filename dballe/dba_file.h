#ifndef DBA_FILE_H
#define DBA_FILE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup dballe
 * High-level access to files with encoded weather data.
 */

#include <dballe/io/dba_rawmsg.h>
#include <dballe/io/encoding.h>
#include <dballe/msg/dba_msg.h>

struct _dba_file;
typedef struct _dba_file* dba_file;

/**
 * Create a reader for data in BUFR format
 *
 * @retval file
 *   The new file, to be deallocated with dba_file_delete()
 * @param type
 *   The type of data contained in the file
 * @param name
 *   The name of the file to access
 * @param mode
 *   The opening mode of the file (@see fopen)
 * @returns
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err dba_file_create(dba_file* file, dba_encoding type, const char* name, const char* mode);

/**
 * Delete a dba_file
 *
 * @param file
 *   The file
 */
void dba_file_delete(dba_file file);

/**
 * Get the type of the dba_file
 */
dba_encoding dba_file_get_type(dba_file file);

/**
 * Read a message from the file.
 *
 * @param file
 *   ::dba_file to read from
 * @param msg
 *   The dba_rawmsg that will hold the data.
 * @retval found
 *   Will be set to true if a message has been found in the file, else to false.
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_file_read_raw(dba_file file, dba_rawmsg msg, int* found);

/**
 * Read and parse a message from the file.
 *
 * @param file
 *   ::dba_file to read from
 * @retval msg
 *   The resulting ::dba_msg
 * @retval found
 *   Will be set to true if a message has been found in the file, else to false.
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_file_read(dba_file file, dba_msg* msg, int* found);

/**
 * Write an encoded message to the file.
 *
 * @param file
 *   ::dba_file to write to
 * @param msg
 *   The dba_rawmsg with the data to write.
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_file_write_raw(dba_file file, dba_rawmsg msg);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
