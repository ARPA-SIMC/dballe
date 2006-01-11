#include <dballe/db/dba_export.h>
#include <dballe/msg/dba_msg.h>

#include <stdlib.h>
#include <string.h>

dba_err dba_db_export_generic(dba db, dba_msg** msgs, dba_record query)
{
	return dba_error_unimplemented("database export of generic data");
}

dba_err dba_db_export(dba db, dba_msg_type type, dba_msg** msgs, dba_record query)
{
	switch (type)
	{
		case MSG_GENERIC: return dba_db_export_generic(db, msgs, query);
		case MSG_SYNOP: return dba_db_export_synop(db, msgs, query);
		case MSG_TEMP: 
		case MSG_TEMP_SHIP: return dba_db_export_sounding(db, msgs, query);
		case MSG_AIREP:
		case MSG_AMDAR:
		case MSG_ACARS: return dba_db_export_flight(db, msgs, query);
		case MSG_SHIP:
		case MSG_BUOY: return dba_db_export_sea(db, msgs, query);
	}
	return dba_error_consistency("unhandled message type %d", type);
}

/* vim:set ts=4 sw=4: */
