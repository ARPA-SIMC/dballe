#ifndef AOF_CONV_H
#define AOF_CONV_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/message/dba_msg_buoy.h>
#include <dballe/message/dba_msg_synop.h>
#include <dballe/aof/aof_message.h>

dba_err aof_to_msg(dba_msg* msg, aof_message amsg);
dba_err aof_from_msg(dba_msg msg, aof_message* amsg);

dba_err aof_to_buoy(dba_msg_buoy* buoy, aof_message msg);
dba_err aof_from_buoy(dba_msg_buoy buoy, aof_message* msg);

dba_err aof_to_synop(dba_msg_synop* synop, aof_message msg);
dba_err aof_from_synop(dba_msg_synop synop, aof_message* msg);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
