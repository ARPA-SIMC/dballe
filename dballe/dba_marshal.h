#ifndef DBA_MARSHAL_H
#define DBA_MARSHAL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/io/dba_rawmsg.h>
#include <dballe/msg/dba_msg.h>

/**
 * Decode a message from its raw encoded representation
 *
 * @param rmsg
 *   Encoded message
 * @retval msg
 *   The resulting ::dba_msg
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_marshal_decode(dba_rawmsg rmsg, dba_msg *msg);

/**
 * Encode a message into its raw encoded representation
 *
 * @param msg
 *   Message to encode
 * @param type
 *   Format to use for encoding
 * @retval rmsg
 *   The resulting ::dba_rawmsg
 * @return
 *   The error indicator for the function. @see dba_err
 */
dba_err dba_marshal_encode(dba_msg msg, dba_encoding type, dba_rawmsg *rmsg);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
