#ifndef BUFREX_EXPORTERS_H
#define BUFREX_EXPORTERS_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <dballe/core/dba_var.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/bufrex/bufrex_raw.h>
	
/**
 * Type can be:
 *  0: encode a BUFR
 *  1: encode a CREX
 */
typedef dba_err (*bufrex_exporter_func)(dba_msg src, bufrex_raw dst, int type);

struct _bufrex_exporter
{
	int type;
	int subtype;

	/* dba_msg type it can convert from */
	dba_msg_type accept_type;

	/* Data descriptor section */
	dba_varcode* ddesc;

	/* Exporter function */
	bufrex_exporter_func exporter;
};

typedef struct _bufrex_exporter bufrex_exporter;

/* ID of originating center to put in encoded messages */
#define ORIG_CENTRE_ID 255
/* ID of originating application to put in encoded messages */
#define ORIG_APP_ID 0

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
