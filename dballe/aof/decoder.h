#ifndef DBALLE_AOF_DECODER_H
#define DBALLE_AOF_DECODER_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup aof
 *
 * AOF message decoder
 */

#include <dballe/io/dba_rawmsg.h>
#include <dballe/msg/dba_msg.h>

/**
 * Decode an AOF message
 *
 * @param msg
 *   The aof_message with the data to decode
 * @retval out
 *   The decoded message
 * @return
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err aof_decoder_decode(dba_rawmsg msg, dba_msg* out);

/**
 * Get category and subcategory of an AOF message
 */
dba_err aof_decoder_get_category(dba_rawmsg msg, int* category, int* subcategory);

/**
 * Encode an AOF message
 *
 * @param msg
 *   The aof_message with the data to encode
 * @param in
 *   The message to encode
 * @return
 *   The error indicator for the function.  @see ::dba_err
 */
/*
dba_err aof_message_encode(aof_message msg, dba_msg in);
*/

/**
 * Print the contents of the AOF message
 *
 * @param msg
 *   The encoded message to dump
 * @param out
 *   The stream to use to print the message
 */
void aof_decoder_dump(dba_rawmsg msg, FILE* out);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
