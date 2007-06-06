/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#define _GNU_SOURCE
/* _GNU_SOURCE is defined to have asprintf */

#include <dballe/core/verbose.h>
#include <dballe/core/aliases.h>
#include <dballe/db/db.h>
#include <dballe/db/cursor.h>
#include <dballe/db/internals.h>
#include <dballe/msg/formatter.h>

#include <f77.h>
#include <limits.h>
#include <float.h>

#include <stdio.h>	// snprintf
#include <string.h>	// strncpy
#include <math.h>	// strncpy

#include "handles.h"

//#define MISSING_STRING ""
#define MISSING_BYTE 0x7f
// integer 2 byte   32767
#define MISSING_INT 0x7fffffff
#define MISSING_REAL (3.4028235E+38)
#define MISSING_DOUBLE (1.7976931348623167E+308)

#define PERM_ANA_RO		(1 << 0)
#define PERM_ANA_WRITE		(1 << 1)
#define PERM_DATA_RO		(1 << 2)
#define PERM_DATA_ADD		(1 << 3)
#define PERM_DATA_WRITE		(1 << 4)
#define PERM_ATTR_RO		(1 << 5)
#define PERM_ATTR_ADD		(1 << 6)
#define PERM_ATTR_WRITE		(1 << 7)

/** @defgroup fortransimple Simplified interface for Dballe
 * @ingroup fortran
 *
 * This module provides a simplified fortran API to Dballe.  The interface is
 * simplified by providing functions with fewer parameters than their
 * counterparts in the full interface, and the omitted parameters are replaced
 * by useful defaults.
 *
 * The resulting interface is optimized for the common usage, making it faster
 * and less error prone.  However, when creating complicated code with more
 * parallel reads and writes, it may be useful to use the functions in
 * ::fortranfull instead, because all parameters are explicit and their precise
 * semantics is always evident.
 *
 * This is a sample code for a query session with the simplified interface:
 * \code
 *   call idba_presentati(dba, "myDSN", "mariorossi", "CippoLippo")
 *   call idba_preparati(dba, handle, "read", "read", "read")
 *   call idba_setr(handle, "latmin", 30.)
 *   call idba_setr(handle, "latmax", 50.)
 *   call idba_setr(handle, "lonmin", 10.)
 *   call idba_setr(handle, "lonmax", 20.)
 *   call idba_voglioquesto(handle, count)
 *   do i=1,count
 *      call idba_dammelo(handle, param)
 *      call idba_enqd(handle, param, ...)
 *      call idba_enqi(handle, ...)
 *      call idba_enqr(handle, ...)
 *      call idba_enqd(handle, ...)
 *      call idba_enqc(handle, ...)
 *      call idba_voglioancora(handle, countancora)
 *      do ii=1,count
 *         call idba_ancora(handle, param)
 *         call idba_enqi(handle, param)
 *      enddo
 *   enddo
 *   call idba_fatto(handle)
 *   call idba_arrivederci(dba)
 * \endcode
 *
 * This is a sample code for a data insert session with the simplified interface:
 * \code
 *   call idba_presentati(dba, "myDSN", "mariorossi", "CippoLippo")
 *   call idba_preparati(dba, handle, "reuse", "add", "add")
 *   call idba_scopa(handle, "")
 *   call idba_setr(handle, "lat", 30.)
 *   call idba_setr(handle, "lon", 10.)
 *   call idba_seti(handle, .....)
 *   call idba_seti(handle, "B12011", 5)
 *   call idba_seti(handle, "B12012", 10)
 *   call idba_prendilo(handle)
 *   call idba_setc(handle, "*var", "B12012")
 *   call idba_seti(handle, "*B33101", 50)
 *   call idba_seti(handle, "*B33102", 75)
 *   call idba_critica(handle)
 *   call idba_setc(handle, "*var", "B12011")
 *   call idba_seti(handle, "*B33101", 50)
 *   call idba_seti(handle, "*B33102", 75)
 *   call idba_critica(handle)
 *   call idba_fatto(handle)
 *   call idba_arrivederci(dba)
 * \endcode
 */

/** @file
 * @ingroup fortransimple
 * Simplified interface for Dballe.
 *
 * Every function returns an error indicator, which is 0 if no error happened,
 * or 1 if there has been an error.
 *
 * When an error happens, the functions in fdba_error.c can be used
 * to get detailed informations about it.
 *
 * \par Internals of the simplified interface
 *
 * Behind the handle returned by idba_preparati() there are a set of variables
 * that are used as implicit parameters:
 *
 * \li \c query ::dba_record, used to set the parameters of the query made by idba_voglioquesto()
 * \li \c work ::dba_record, used by idba_dammelo() to return the parameters 
 * \li \c qc ::dba_record, used to manipulate qc data.  Every time the \ref idba_enq or \ref idba_set functions are used with a variable name starting with an asterisk, they will manipulate the \c qc record instead of the others.
 * \li \c ana ::dba_cursor, used to iterate on the results of idba_quantesono()
 * \li \c query ::dba_cursor, used to iterate on the results of idba_voglioquesto()
 *
 * The simplified interface has two possible states: \c QUERY and \c RESULT.
 * Then the interface is in the \c QUERY state, the \ref idba_enq and \ref
 * idba_set functions operate in the \c query ::dba_record, to set and check the
 * parameters of a query.  idba_voglioquesto() reads the parameters from the \c
 * query ::dba_record and switches the state to \c RESULT, and further calls to
 * idba_dammelo() will put the query results in the \c work ::dba_record, to be read by
 * the \ref idba_enq functions.
 *
 * In the \c RESULT state, the \ref idba_enq and \ref idba_set functions
 * operate on the \c work ::dba_record, to inspect the results of the queries.  A
 * call to idba_ricominciamo() terminates the current query and goes back to
 * the \c QUERY state, resetting the contents of all the ::dba_record of the interface.
 *
 * idba_prendilo() inserts in the database the data coming from the \c
 * QUERY ::dba_record if invoked in the \c query state, or the data coming from the
 * \c RESULT ::dba_record if invoked in the \c result state.  This is done
 * because inserting new values in the database should be independent from the
 * state.
 *
 * \ref qc functions instead always operate on the \c qc ::dba_record, which is
 * accessed with the \ref idba_enq and \ref idba_set functions by prefixing the
 * parameter name with an asterisk.
 */

/* Handles to give to Fortran */

#define MAX_SIMPLE 50
#define MAX_SESSION 10

FDBA_HANDLE_BODY(simple, MAX_SIMPLE, "Dballe simple sessions")
FDBA_HANDLE_BODY(session, MAX_SESSION, "Dballe sessions")

#define STATE (FDBA_HANDLE(simple, *handle))
#define SESSION (FDBA_HANDLE(session, FDBA_HANDLE(simple, *handle).session).session)

static int usage_refcount = 0;

/**
 * Start working with a DBALLE database.
 *
 * This function can be called more than once once to connect to different
 * databases at the same time.
 * 
 * @param dsn
 *   The ODBC DSN of the database to use
 * @param user
 *   The username used to connect to the database
 * @param password
 *   The username used to connect to the database
 * @retval dbahandle
 *   The database handle that can be passed to idba_preparati to work with the
 *   database.
 * @return
 *   The error indication for the function.
 */
F77_INTEGER_FUNCTION(idba_presentati)(
		INTEGER(dbahandle),
		CHARACTER(dsn),
		CHARACTER(user),
		CHARACTER(password)
		TRAIL(dsn)
		TRAIL(user)
		TRAIL(password))
{
	GENPTR_INTEGER(dbahandle)
	GENPTR_CHARACTER(dsn)
	GENPTR_CHARACTER(user)
	GENPTR_CHARACTER(password)
	char s_dsn[50];
	char s_user[20];
	char s_password[20];
	dba_err err;

	/* Import input string parameters */
	cnfImpn(dsn, dsn_length, 49, s_dsn); s_dsn[49] = 0;
	cnfImpn(user, user_length, 19, s_user); s_user[19] = 0;
	cnfImpn(password, password_length, 19, s_password); s_password[19] = 0;

	/* Initialize the library if needed */
	if (usage_refcount == 0)
	{
		fdba_handle_init_session();
		fdba_handle_init_simple();
	}
	++usage_refcount;

	/* Allocate and initialize a new handle */
	DBA_RUN_OR_RETURN(fdba_handle_alloc_session(dbahandle));

	/* Open the DBALLE session */
	DBA_RUN_OR_GOTO(fail, dba_db_create(s_dsn, s_user, s_password,
				&(FDBA_HANDLE(session, *dbahandle).session)));

	/* Open the database session */
	return dba_error_ok();

fail:
	return err;
}

/**
 * Stop working with a DBALLE database
 *
 * @param dbahandle
 *   The database handle to close.
 */
F77_SUBROUTINE(idba_arrivederci)(INTEGER(dbahandle))
{
	GENPTR_INTEGER(dbahandle)

	dba_db_delete(FDBA_HANDLE(session, *dbahandle).session);
	FDBA_HANDLE(session, *dbahandle).session = NULL;
	fdba_handle_release_session(*dbahandle);

	/*
	dba_shutdown does not exist anymore, but I keep this code commented out
	here as a placeholder if in the future we'll need to hook actions when the
	usage refcount goes to 0

	if (--usage_refcount == 0)
		dba_shutdown();
	*/
}

static int check_flag(const char* val, const char* buf, int len)
{
	int val_len = strlen(val);
	if (len < val_len)
		return 0;
	if (strncasecmp(val, buf, val_len) != 0)
		return 0;
	return len == val_len || buf[val_len] == ' ';
}

/**
 * Starts a session with dballe.
 *
 * You can call idba_preparati() many times and get more handles.  This allows
 * to perform many operations on the database at the same time.
 *
 * idba_preparati() has three extra parameters that can be used to limit
 * write operations on the database, as a limited protection against
 * programming errors.
 *
 * Note that some combinations of parameters are illegal, such as anaflag=read
 * and dataflag=add (when adding a new data, it's sometimes necessary to insert
 * new pseudoana records), or dataflag=rewrite and qcflag=read (when deleting
 * data, their attributes are deleted as well).
 *
 * @param dbahandle
 *   The main DB-ALLe connection handle
 * @retval handle
 *   The session handle returned by the function
 * @param anaflag
 *   Controls access to pseudoana records and can have these values:
 *   \li \c "read" pseudoana records cannot be inserted.
 *   \li \c "write" it is possible to insert and delete pseudoana records.
 * @param dataflag
 *   Controls access to observed data and can have these values:
 *    \li \c "read" data cannot be modified in any way.
 *    \li \c "add" data can be added to the database, but existing data cannot be
 *    modified.  Deletions are disabled.  This is used to insert new data in the
 *    database while preserving the data that was already present in it.
 *    \li \c "write" data can freely be added, overwritten and deleted.
 * @param qcflag
 *    Controls access to data attributes and can have these values:
 *    \li \c "read" attributes cannot be modified in any way.
 *    \li \c "add" attributes can can be added to the database, but existing
 *    attributes cannot be modified.  Deletion of attributes is disabled.  This is
 *    used to insert new attribute in the database while preserving the attributes
 *    that were already present in it.
 *    \li \c "write" attributes can freely be added, overwritten and deleted.
 * @return
 *   The error indication for the function.
 */
F77_INTEGER_FUNCTION(idba_preparati)(
		INTEGER(dbahandle),
		INTEGER(handle),
		CHARACTER(anaflag),
		CHARACTER(dataflag),
		CHARACTER(attrflag)
		TRAIL(anaflag)
		TRAIL(dataflag)
		TRAIL(attrflag))
{
	GENPTR_INTEGER(dbahandle)
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(anaflag)
	GENPTR_CHARACTER(dataflag)
	GENPTR_CHARACTER(attrflag)
	dba_err err;

	/* Check here to warn users of the introduction of idba_presentati */
	/*
	if (session == NULL)
		return dba_error_consistency("idba_presentati should be called before idba_preparati");
	*/

	/* Allocate and initialize a new handle */
	DBA_RUN_OR_RETURN(fdba_handle_alloc_simple(handle));

	STATE.session = *dbahandle;
	STATE.perms = 0;
	STATE.input = NULL;
	STATE.output = NULL;
	STATE.qcinput = NULL;
	STATE.qcoutput = NULL;
	STATE.ana_cur = NULL;
	STATE.query_cur = NULL;

	if (check_flag("read",	anaflag,  anaflag_length))
		STATE.perms |= PERM_ANA_RO;
	if (check_flag("write",	anaflag,  anaflag_length))
		STATE.perms |= PERM_ANA_WRITE;
	if (check_flag("read",	dataflag, dataflag_length))
		STATE.perms |= PERM_DATA_RO;
	if (check_flag("add",	dataflag, dataflag_length))
		STATE.perms |= PERM_DATA_ADD;
	if (check_flag("write",	dataflag, dataflag_length))
		STATE.perms |= PERM_DATA_WRITE;
	if (check_flag("read",	attrflag,   attrflag_length))
		STATE.perms |= PERM_ATTR_RO;
	if (check_flag("add",	attrflag,   attrflag_length))
		STATE.perms |= PERM_ATTR_ADD;
	if (check_flag("write",	attrflag,   attrflag_length))
		STATE.perms |= PERM_ATTR_WRITE;

	if ((STATE.perms & (PERM_ANA_RO | PERM_ANA_WRITE)) == 0)
		return dba_error_consistency("pseudoana should be opened in either 'read' or 'write' mode");
	if ((STATE.perms & (PERM_DATA_RO | PERM_DATA_ADD | PERM_DATA_WRITE)) == 0)
		return dba_error_consistency("data should be opened in one of 'read', 'add' or 'write' mode");
	if ((STATE.perms & (PERM_ATTR_RO | PERM_ATTR_ADD | PERM_ATTR_WRITE)) == 0)
		return dba_error_consistency("attr should be opened in one of 'read', 'add' or 'write' mode");

	if (STATE.perms & PERM_ANA_RO && STATE.perms & PERM_DATA_WRITE)
		return dba_error_consistency("when data is 'write' ana must also be set to 'write', because deleting data can potentially also delete pseudoana");
	if (STATE.perms & PERM_ATTR_RO && STATE.perms & PERM_DATA_WRITE)
		return dba_error_consistency("when data is 'write' attr must also be set to 'write', because deleting data also delete its attributes");
	
	/* Allocate the records */
	DBA_RUN_OR_GOTO(fail, dba_record_create(&(STATE.input)));
	DBA_RUN_OR_GOTO(fail, dba_record_create(&(STATE.output)));
	DBA_RUN_OR_GOTO(fail, dba_record_create(&(STATE.qcinput)));
	DBA_RUN_OR_GOTO(fail, dba_record_create(&(STATE.qcoutput)));

	return dba_error_ok();

fail:
	if (STATE.qcoutput != NULL)
		dba_record_delete(STATE.qcoutput);
	if (STATE.qcinput != NULL)
		dba_record_delete(STATE.qcinput);
	if (STATE.output != NULL)
		dba_record_delete(STATE.output);
	if (STATE.input != NULL)
		dba_record_delete(STATE.input);

	fdba_handle_release_simple(*handle);

	return err;
}

/**
 * Ends a session with DBALLE
 *
 * @param handle
 *   Handle to the session to be closed.
 */
F77_INTEGER_FUNCTION(idba_fatto)(INTEGER(handle))
{
	GENPTR_INTEGER(handle)

	if (STATE.ana_cur != NULL)
		dba_db_cursor_delete(STATE.ana_cur);
	if (STATE.query_cur != NULL)
		dba_db_cursor_delete(STATE.query_cur);
	if (STATE.qcoutput != NULL)
		dba_record_delete(STATE.qcoutput);
	if (STATE.qcinput != NULL)
		dba_record_delete(STATE.qcinput);
	if (STATE.output != NULL)
		dba_record_delete(STATE.output);
	if (STATE.input != NULL)
		dba_record_delete(STATE.input);

	STATE.ana_cur = NULL;
	STATE.query_cur = NULL;
	STATE.qcoutput = NULL;
	STATE.qcinput = NULL;
	STATE.output = NULL;
	STATE.input = NULL;

	fdba_handle_release_simple(*handle);
	return dba_error_ok();
}

/**
 * Reset the database contents, loading default report informations from a file.
 *
 * It only works in rewrite mode.
 *
 * @param handle
 *   Handle to a DBALLE session
 * @param repinfofile
 *   CSV file with the default report informations.  See dba_reset()
 *   documentation for the format of the file.
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_scopa)(INTEGER(handle), CHARACTER(repinfofile) TRAIL(repinfofile))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(repinfofile)
	char fname[PATH_MAX];

	if (!(STATE.perms & PERM_DATA_WRITE))
		return dba_error_consistency(
			"idba_scopa must be run with the database open in data write mode");

	cnfImpn(repinfofile, repinfofile_length,  PATH_MAX, fname); fname[PATH_MAX - 1] = 0;

	return dba_db_reset(SESSION, fname[0] == 0 ? NULL : fname);
}

/**
 * Stop reading query results and start with a new query.
 *
 * @note The query parameters of the last query are preserved for the next one.
 *
 * @param handle
 *   Handle to a DBALLE session
 */
/*
F77_SUBROUTINE(idba_ricominciamo)(INTEGER(handle))
{
	GENPTR_INTEGER(handle)
	
	if (STATE.ana_cur != NULL)
		dba_cursor_delete(STATE.ana_cur);
	STATE.ana_cur = NULL;

	if (STATE.query_cur != NULL)
		dba_cursor_delete(STATE.query_cur);
	STATE.query_cur = NULL;

	STATE.state = QUERY;

	dba_record_clear(STATE.query);
	dba_record_clear(STATE.work);
	dba_record_clear(STATE.qc);
}
*/

/**@name idba_enq*
 * @anchor idba_enq
 * Functions used to read the output values of the DBALLE action routines
 * @{
 */

/**
 * Read one integer value from the output record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_enqi)(
		INTEGER(handle),
		CHARACTER(parameter),
		INTEGER(value)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_INTEGER(value)
	char parm[20];
	char* p;
	int found;
	dba_record rec;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	switch (parm[0])
	{
		case '*':
			rec = STATE.qcoutput;
			p = parm + 1;
			break;
		default:
			rec = STATE.output;
			p = parm;
			break;
	}

	DBA_RUN_OR_RETURN(dba_record_enqi(rec, p, value, &found));
	if (!found)
		*value = MISSING_INT;
	return dba_error_ok();
}

/**
 * Read one byte value from the output record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_enqb)(
		INTEGER(handle),
		CHARACTER(parameter),
		BYTE(value)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_BYTE(value)
	char parm[20];
	char* p;
	int ival, found;
	dba_record rec;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	switch (parm[0])
	{
		case '*':
			rec = STATE.qcoutput;
			p = parm + 1;
			break;
		default:
			rec = STATE.output;
			p = parm;
			break;
	}

	DBA_RUN_OR_RETURN(dba_record_enqi(rec, p, &ival, &found));
	if (!found)
		*value = MISSING_BYTE;
	else
	{
		if (ival < SCHAR_MIN || ival > SCHAR_MAX)
			return dba_error_consistency("value queried (%d) does not fit in a byte", ival);
		*value=(char)ival;
	}
	return dba_error_ok();
}


/**
 * Read one real value from the output record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_enqr)(
		INTEGER(handle),
		CHARACTER(parameter),
		REAL(value)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_REAL(value)
	char parm[20];
	char* p;
	double dval;
	int found;
	dba_record rec;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	if (parm[0] == '*')
	{
		rec = STATE.qcoutput;
		p = parm + 1;
	} else {
		rec = STATE.output;
		p = parm;
	}

	DBA_RUN_OR_RETURN(dba_record_enqd(rec, p, &dval, &found));
	if (!found)
	{
		*value = MISSING_REAL;
	} else {
		if (dval < -FLT_MAX || dval > FLT_MAX)
			return dba_error_consistency("value queried (%f) does not fit in a real", dval);
		*value = dval;
	}
	return dba_error_ok();
}

/**
 * Read one real*8 value from the output record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_enqd)(
		INTEGER(handle),
		CHARACTER(parameter),
		DOUBLE(value)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_DOUBLE(value)
	char parm[20];
	char* p;
	int found;
	dba_record rec;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	if (parm[0] == '*')
	{
		rec = STATE.qcoutput;
		p = parm + 1;
	} else {
		rec = STATE.output;
		p = parm;
	}

	DBA_RUN_OR_RETURN(dba_record_enqd(rec, p, value, &found));
	if (!found)
		*value = MISSING_DOUBLE;
	return dba_error_ok();
}

/**
 * Read one character value from the output record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_enqc)(
		INTEGER(handle),
		CHARACTER(parameter),
		CHARACTER(value)
		TRAIL(parameter)
		TRAIL(value))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_CHARACTER(value)
	char parm[20];
	char* p;
	dba_record rec;
	const char* strval;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	if (parm[0] == '*')
	{
		rec = STATE.qcoutput;
		p = parm + 1;
	} else {
		rec = STATE.output;
		p = parm;
	}

	DBA_RUN_OR_RETURN(dba_record_enqc(rec, p, &strval));
	if (strval == NULL)
		bzero(value, value_length);
	else
		cnfExprt(strval, value, value_length);
	return dba_error_ok();
}

/*@}*/

/**@name idba_set*
 * @anchor idba_set
 * Functions used to read the input values for the DBALLE action routines
 *@{*/

/**
 * Set one integer value into the input record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_seti)(
		INTEGER(handle),
		CHARACTER(parameter),
		INTEGER(value)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_INTEGER(value)
	GENPTR_INTEGER(err)
	char parm[20];
	char* p;
	dba_record rec;
	dba_varcode code = 0;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	switch (parm[0])
	{
		case '*':
			rec = STATE.qcinput;
			p = parm + 1;
			break;
		default:
			rec = STATE.input;
			p = parm;
			break;
	}

	if (p[0] != 'B' && (code = dba_varcode_alias_resolve(p)) == 0)
	{
		dba_keyword param = dba_record_keyword_byname(p);
		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled parameter \"%s\"", p);

		if (*value == MISSING_INT)
			return dba_record_key_unset(rec, param);
		else
			switch (param)
			{
				case DBA_KEY_LAT:
				case DBA_KEY_LON:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_ANA_ID));
					break;
				case DBA_KEY_ANA_ID:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LAT));
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LON));
					break;
				default: break;
			}

		return dba_record_key_seti(rec, param, *value);
	} else {
		if (code == 0)
			code = DBA_STRING_TO_VAR(p + 1);

		if (*value == MISSING_INT)
			return dba_record_var_unset(rec, code);

		return dba_record_var_seti(rec, code, *value);
	}
}

/**
 * Set one byte value into the input record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_setb)(
		INTEGER(handle),
		CHARACTER(parameter),
		BYTE(value)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_BYTE(value)
	GENPTR_INTEGER(err)
	char parm[20];
	char* p;
	dba_record rec;
	dba_varcode code = 0;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	switch (parm[0])
	{
		case '*':
			rec = STATE.qcinput;
			p = parm + 1;
			break;
		default:
			rec = STATE.input;
			p = parm;
			break;
	}

	if (p[0] != 'B' && (code = dba_varcode_alias_resolve(p)) == 0)
	{
		dba_keyword param = dba_record_keyword_byname(p);
		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled parameter \"%s\"", p);

		if (*value == MISSING_BYTE)
			return dba_record_key_unset(rec, param);
		else
			switch (param)
			{
				case DBA_KEY_LAT:
				case DBA_KEY_LON:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_ANA_ID));
					break;
				case DBA_KEY_ANA_ID:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LAT));
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LON));
					break;
				default: break;
			}

		return dba_record_key_seti(rec, param, *value);
	} else {
		if (code == 0)
			code = DBA_STRING_TO_VAR(p + 1);

		if (*value == MISSING_BYTE)
			return dba_record_var_unset(rec, code);

		return dba_record_var_seti(rec, code, *value);
	}
}


/**
 * Set one real value into the input record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_setr)(
		INTEGER(handle),
		CHARACTER(parameter),
		REAL(value)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_REAL(value)
	GENPTR_INTEGER(err)
	char parm[20];
	char* p;
	dba_record rec;
	dba_varcode code = 0;

	cnfImpn(parameter, parameter_length, 19, parm);

	switch (parm[0])
	{
		case '*':
			rec = STATE.qcinput;
			p = parm + 1;
			break;
		default:
			rec = STATE.input;
			p = parm;
			break;
	}

	if (p[0] != 'B' && (code = dba_varcode_alias_resolve(p)) == 0)
	{
		dba_keyword param = dba_record_keyword_byname(p);
		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled parameter \"%s\"", p);

		if (*value == MISSING_REAL)
			return dba_record_key_unset(rec, param);
		else
			switch (param)
			{
				case DBA_KEY_LAT:
				case DBA_KEY_LON:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_ANA_ID));
					break;
				case DBA_KEY_ANA_ID:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LAT));
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LON));
					break;
				default: break;
			}

		return dba_record_key_setd(rec, param, *value);
	} else {
		if (code == 0)
			code = DBA_STRING_TO_VAR(p + 1);

		if (*value == MISSING_REAL)
			return dba_record_var_unset(rec, code);

		return dba_record_var_setd(rec, code, *value);
	}
}

/**
 * Set one real*8 value into the input record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_setd)(
		INTEGER(handle),
		CHARACTER(parameter),
		DOUBLE(value)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_DOUBLE(value)
	GENPTR_INTEGER(err)
	char parm[20];
	char* p;
	dba_record rec;
	dba_varcode code = 0;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	switch (parm[0])
	{
		case '*':
			rec = STATE.qcinput;
			p = parm + 1;
			break;
		default:
			rec = STATE.input;
			p = parm;
			break;
	}

	if (p[0] != 'B' && (code = dba_varcode_alias_resolve(p)) == 0)
	{
		dba_keyword param = dba_record_keyword_byname(p);
		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled parameter \"%s\"", p);

		if (*value == MISSING_DOUBLE)
			return dba_record_key_unset(rec, param);
		else
			switch (param)
			{
				case DBA_KEY_LAT:
				case DBA_KEY_LON:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_ANA_ID));
					break;
				case DBA_KEY_ANA_ID:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LAT));
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LON));
					break;
				default: break;
			}

		return dba_record_key_setd(rec, param, *value);
	} else {
		if (code == 0)
			code = DBA_STRING_TO_VAR(p + 1);

		if (*value == MISSING_DOUBLE)
			return dba_record_var_unset(rec, code);

		return dba_record_var_setd(rec, code, *value);
	}
}

/**
 * Set one character value into the input record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_setc)(
		INTEGER(handle),
		CHARACTER(parameter),
		CHARACTER(value)
		TRAIL(parameter)
		TRAIL(value))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_CHARACTER(value)
	GENPTR_INTEGER(err)
	char parm[20];
	char* p;
	char val[255];
	dba_record rec;
	dba_varcode code = 0;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;
	cnfImpn(value, value_length, 254, val); val[254] = 0;

	switch (parm[0])
	{
		case '*':
			rec = STATE.qcinput;
			p = parm + 1;
			break;
		default:
			rec = STATE.input;
			p = parm;
			break;
	}

	if (p[0] != 'B' && (code = dba_varcode_alias_resolve(p)) == 0)
	{
		dba_keyword param = dba_record_keyword_byname(p);
		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled parameter \"%s\"", p);

		if (val[0] == 0)
			return dba_record_key_unset(rec, param);
		else
			switch (param)
			{
				case DBA_KEY_LAT:
				case DBA_KEY_LON:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_ANA_ID));
					break;
				case DBA_KEY_ANA_ID:
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LAT));
					DBA_RUN_OR_RETURN(dba_record_key_unset(rec, DBA_KEY_LON));
					break;
				default: break;
			}

		return dba_record_key_setc(rec, param, val);
	} else {
		if (code == 0)
			code = DBA_STRING_TO_VAR(p + 1);

		if (val[0] == 0)
			return dba_record_var_unset(rec, code);

		return dba_record_var_setc(rec, code, val);
	}
}

/**
 * Shortcut function to set query parameters to the anagraphical context
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_setcontextana)(
		INTEGER(handle))
{
	GENPTR_INTEGER(handle)
	return dba_record_set_ana_context(STATE.input);
}

/**
 * Shortcut function to read level data.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @retval ltype
 *   Level type from the output record
 * @retval l1
 *   L1 from the output record
 * @retval l2
 *   L2 from the output record
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_enqlevel)(
		INTEGER(handle),
		INTEGER(ltype),
		INTEGER(l1),
		INTEGER(l2))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(ltype)
	GENPTR_INTEGER(l1)
	GENPTR_INTEGER(l2)
	int found;

	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_LEVELTYPE, ltype, &found));
	if (!found) *ltype = MISSING_INT;
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_L1, l1, &found));
	if (!found) *l1 = MISSING_INT;
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_L2, l2, &found));
	if (!found) *l2 = MISSING_INT;
	return dba_error_ok();
}

/**
 * Shortcut function to set level data.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param ltype
 *   Level type to set in the input record
 * @param l1
 *   L1 to set in the input record
 * @param l2
 *   L2 to set in the input record
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_setlevel)(
		INTEGER(handle),
		INTEGER(ltype),
		INTEGER(l1),
		INTEGER(l2))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(ltype)
	GENPTR_INTEGER(l1)
	GENPTR_INTEGER(l2)

	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_LEVELTYPE, *ltype));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_L1, *l1));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_L2, *l2));
	return dba_error_ok();
}

/**
 * Shortcut function to read time range data.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @retval ptype
 *   P indicator from the output record
 * @retval p1
 *   P1 from the output record
 * @retval p2
 *   P2 from the output record
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_enqtimerange)(
		INTEGER(handle),
		INTEGER(ptype),
		INTEGER(p1),
		INTEGER(p2))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(ptype)
	GENPTR_INTEGER(p1)
	GENPTR_INTEGER(p2)
	int found;

	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_PINDICATOR, ptype, &found));
	if (!found) *ptype = MISSING_INT;
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_P1, p1, &found));
	if (!found) *p1 = MISSING_INT;
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_P2, p2, &found));
	if (!found) *p2 = MISSING_INT;
	return dba_error_ok();
}

/**
 * Shortcut function to set time range data.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param ptype
 *   P indicator to set in the input record
 * @param p1
 *   P1 to set in the input record
 * @param p2
 *   P2 to set in the input record
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_settimerange)(
		INTEGER(handle),
		INTEGER(ptype),
		INTEGER(p1),
		INTEGER(p2))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(ptype)
	GENPTR_INTEGER(p1)
	GENPTR_INTEGER(p2)

	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_PINDICATOR, *ptype));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_P1, *p1));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_P2, *p2));
	return dba_error_ok();
}

/**
 * Shortcut function to read date information.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @retval year
 *   Year from the output record
 * @retval month
 *   Month the output record
 * @retval day
 *   Day the output record
 * @retval hour
 *   Hour the output record
 * @retval min
 *   Minute the output record
 * @retval sec
 *   Second the output record
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_enqdate)(
		INTEGER(handle),
		INTEGER(year),
		INTEGER(month),
		INTEGER(day),
		INTEGER(hour),
		INTEGER(min),
		INTEGER(sec))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(year)
	GENPTR_INTEGER(month)
	GENPTR_INTEGER(day)
	GENPTR_INTEGER(hour)
	GENPTR_INTEGER(min)
	GENPTR_INTEGER(sec)
	int found;

	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_YEAR, year, &found));
	if (!found) *year = MISSING_INT;
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_MONTH, month, &found));
	if (!found) *month = MISSING_INT;
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_DAY, day, &found));
	if (!found) *day = MISSING_INT;
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_HOUR, hour, &found));
	if (!found) *hour = MISSING_INT;
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_MIN, min, &found));
	if (!found) *min = MISSING_INT;
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_SEC, sec, &found));
	if (!found) *sec = MISSING_INT;
	return dba_error_ok();
}

/**
 * Shortcut function to set date information.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param year
 *   Year to set in the input record
 * @param month
 *   Month to set in the input
 * @param day
 *   Day to set in the input
 * @param hour
 *   Hour to set in the input
 * @param min
 *   Minute to set in the input
 * @param sec
 *   Second to set in the input
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_setdate)(
		INTEGER(handle),
		INTEGER(year),
		INTEGER(month),
		INTEGER(day),
		INTEGER(hour),
		INTEGER(min),
		INTEGER(sec))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(year)
	GENPTR_INTEGER(month)
	GENPTR_INTEGER(day)
	GENPTR_INTEGER(hour)
	GENPTR_INTEGER(min)
	GENPTR_INTEGER(sec)

	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_YEAR, *year));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_MONTH, *month));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_DAY, *day));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_HOUR, *hour));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_MIN, *min));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_SEC, *sec));
	return dba_error_ok();
}

/**
 * Shortcut function to set minimum date for a query.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param year
 *   Minimum year to set in the query
 * @param month
 *   Minimum month to set in the query
 * @param day
 *   Minimum day to set in the query
 * @param hour
 *   Minimum hour to set in the query
 * @param min
 *   Minimum minute to set in the query
 * @param sec
 *   Minimum second to set in the query
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_setdatemin)(
		INTEGER(handle),
		INTEGER(year),
		INTEGER(month),
		INTEGER(day),
		INTEGER(hour),
		INTEGER(min),
		INTEGER(sec))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(year)
	GENPTR_INTEGER(month)
	GENPTR_INTEGER(day)
	GENPTR_INTEGER(hour)
	GENPTR_INTEGER(min)
	GENPTR_INTEGER(sec)

	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_YEARMIN, *year));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_MONTHMIN, *month));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_DAYMIN, *day));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_HOURMIN, *hour));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_MINUMIN, *min));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_SECMIN, *sec));
	return dba_error_ok();
}

/**
 * Shortcut function to set maximum date for a query.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param year
 *   Maximum year to set in the query
 * @param month
 *   Maximum month to set in the query
 * @param day
 *   Maximum day to set in the query
 * @param hour
 *   Maximum hour to set in the query
 * @param min
 *   Maximum minute to set in the query
 * @param sec
 *   Maximum second to set in the query
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_setdatemax)(
		INTEGER(handle),
		INTEGER(year),
		INTEGER(month),
		INTEGER(day),
		INTEGER(hour),
		INTEGER(min),
		INTEGER(sec))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(year)
	GENPTR_INTEGER(month)
	GENPTR_INTEGER(day)
	GENPTR_INTEGER(hour)
	GENPTR_INTEGER(min)
	GENPTR_INTEGER(sec)

	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_YEARMAX, *year));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_MONTHMAX, *month));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_DAYMAX, *day));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_HOURMAX, *hour));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_MINUMAX, *min));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.input, DBA_KEY_SECMAX, *sec));
	return dba_error_ok();
}

/*@}*/

/**
 * Remove one parameter from the input record
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param parameter
 *   Parameter to remove.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in \ref
 *   dba_record_keywords
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_unset)(
		INTEGER(handle),
		CHARACTER(parameter)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	GENPTR_INTEGER(err)
	char parm[20];
	char* p;
	dba_record rec;

	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	if (parm[0] == '*')
	{
		rec = STATE.qcinput;
		p = parm + 1;
	} else {
		rec = STATE.input;
		p = parm;
	}

	if (p[0] != 'B')
	{
		dba_keyword param = dba_record_keyword_byname(p);
		if (param == DBA_KEY_ERROR)
			return dba_error_notfound("looking for misspelled parameter \"%s\"", p);

		return dba_record_key_unset(rec, param);
	} else
		return dba_record_var_unset(rec, DBA_STRING_TO_VAR(p + 1));
}

static void clear_attr_rec(dba_record rec)
{
	const char* val;
	int saved_context_id = -1;
	char saved_varname[8];

	/* Copy the values to be preserved */
	if ((val = dba_record_key_peek_value(rec, DBA_KEY_CONTEXT_ID)) != NULL)
		saved_context_id = strtol(val, NULL, 10);
	if ((val = dba_record_key_peek_value(rec, DBA_KEY_VAR_RELATED)) != NULL)
	{
		strncpy(saved_varname, val, 7);
		saved_varname[6] = 0;
	}
	else
		saved_varname[0] = 0;

	dba_record_clear(rec);

	if (saved_context_id != -1)
		dba_record_key_seti(rec, DBA_KEY_CONTEXT_ID, saved_context_id);
	if (saved_varname[0] != 0)
		dba_record_key_setc(rec, DBA_KEY_VAR_RELATED, saved_varname);
}

/**
 * Remove all parameters from the input record
 * 
 * @param handle
 *   Handle to a DBALLE session
 */
F77_SUBROUTINE(idba_unsetall)(
		INTEGER(handle))
{
	GENPTR_INTEGER(handle)

	clear_attr_rec(STATE.qcinput);
	dba_record_clear(STATE.input);
}

/**
 * Count the number of elements in the anagraphical storage, and start a new
 * anagraphical query.
 *
 * Resulting anagraphical data can be retrieved with idba_elencamele()
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @param count
 *   The count of elements
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_quantesono)(
		INTEGER(handle),
		INTEGER(count))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(count)

	if (STATE.ana_cur != NULL)
	{
		dba_db_cursor_delete(STATE.ana_cur);
		STATE.ana_cur = NULL;
	}

	if (dba_verbose_is_allowed(DBA_VERB_DB_INPUT))
	{
		dba_verbose(DBA_VERB_DB_INPUT,
				"invoking dba_db_ana_query(%d, <input>).  <input> is:\n",
				*handle);
		dba_record_print(STATE.input, DBA_VERBOSE_STREAM);
	}

	DBA_RUN_OR_RETURN(dba_db_ana_query(
			SESSION,
			STATE.input,
			&(STATE.ana_cur),
			count));

	return dba_error_ok();
}

/**
 * Iterate through the anagraphical data.
 *
 * Every invocation of this function will return a new anagraphical data, or
 * fill fail with code DBA_ERR_NOTFOUND when there are no more anagraphical
 * data available.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_elencamele)(INTEGER(handle))
{
	GENPTR_INTEGER(handle)
	int has_data;

	if (STATE.ana_cur == NULL)
		return dba_error_consistency("idba_elencamele called without a previous idba_quantesono");

	DBA_RUN_OR_RETURN(dba_db_cursor_next(STATE.ana_cur, &has_data));
	if (!has_data)
	{
		dba_db_cursor_delete(STATE.ana_cur);
		STATE.ana_cur = NULL;
		dba_record_clear(STATE.output);
		return dba_error_ok();
	} else
		return dba_db_cursor_to_record(STATE.ana_cur, STATE.output);
}

/**
 * Submit a query to the database.
 *
 * The query results can be accessed with calls to idba_dammelo.
 *
 * @param handle
 *   Handle to a DBALLE session
 * @retval count
 *   Number of values returned by the function
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_voglioquesto)(
		INTEGER(handle),
		INTEGER(count))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(count)

	if (STATE.query_cur != NULL)
	{
		dba_db_cursor_delete(STATE.query_cur);
		STATE.query_cur = NULL;
	}

	if (dba_verbose_is_allowed(DBA_VERB_DB_INPUT))
	{
		dba_verbose(DBA_VERB_DB_INPUT,
				"invoking dba_query(%d, <input>).  <input> is:\n",
				*handle);
		dba_record_print(STATE.input, DBA_VERBOSE_STREAM);
	}

	DBA_RUN_OR_RETURN(dba_db_query(SESSION, STATE.input, &(STATE.query_cur), count));

	return dba_error_ok();
}

/**
 * Iterate through the query results data.
 *
 * Every invocation of this function will return a new result, or fill fail
 * with code DBA_ERR_NOTFOUND when there are no more results available.
 * 
 * @param handle
 *   Handle to a DBALLE session
 * @retval parameter
 *   Contains the ID of the parameter retrieved by this fetch
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_dammelo)(
		INTEGER(handle),
		CHARACTER(parameter)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	const char* varstr;
	int has_data;

	if (STATE.query_cur == NULL)
		return dba_error_consistency("idba_dammelo called without a previous idba_voglioquesto");

	/* Reset qc record iterator, so that idba_ancora will not return
	 * leftover QC values from a previous query */
	STATE.qc_iter = 0;

	DBA_RUN_OR_RETURN(dba_db_cursor_next(STATE.query_cur, &has_data));
	if (!has_data)
	{
		dba_db_cursor_delete(STATE.query_cur);
		STATE.query_cur = NULL;
		cnfExprt("", parameter, parameter_length);
		dba_record_clear(STATE.output);
	} else {
		dba_record_clear(STATE.output);
		DBA_RUN_OR_RETURN(dba_db_cursor_to_record(STATE.query_cur, STATE.output));
		DBA_RUN_OR_RETURN(dba_record_key_enqc(STATE.output, DBA_KEY_VAR, &varstr));

		/* Set context id and variable name on qcinput so that
		 * attribute functions will refer to the last variable read */
		DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.qcinput, DBA_KEY_CONTEXT_ID,
							STATE.query_cur->out_context_id));
		DBA_RUN_OR_RETURN(dba_record_key_setc(STATE.qcinput, DBA_KEY_VAR_RELATED, varstr));

		cnfExprt(varstr, parameter, parameter_length);
	}

	return dba_error_ok();
}

/**
 * Insert a new item in the database.
 *
 * This function will fail if the database is open in data readonly mode, and
 * it will refuse to overwrite existing values if the database is open in data
 * add mode.
 *
 * If the database is open in pseudoana reuse mode, the pseudoana values
 * provided on input will be used to create a pseudoana record if it is
 * missing, but will be ignored if it is already present.  If it is open in
 * pseudoana rewrite mode instead, the pseudoana values on input will be used
 * to replace all the existing pseudoana values.
 *
 * @param handle
 *   Handle to a DBALLE session
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_prendilo)(
		INTEGER(handle))
{
	GENPTR_INTEGER(handle)
	int ana_id, context_id;
	dba_record_cursor cur;
	dba_var var = NULL;

	if (STATE.perms & PERM_DATA_RO)
		return dba_error_consistency(
			"idba_prendilo cannot be called with the database open in data readonly mode");

	if (dba_verbose_is_allowed(DBA_VERB_DB_INPUT))
	{
		dba_verbose(DBA_VERB_DB_INPUT,
				"invoking dba_insert_or_replace(%d, <input>, %d, %d).  <input> is:\n",
				*handle,
				STATE.perms & PERM_DATA_WRITE ? 1 : 0,
				STATE.perms & PERM_ANA_WRITE ? 1 : 0);
		dba_record_print(STATE.input, DBA_VERBOSE_STREAM);
	}

	DBA_RUN_OR_RETURN(dba_db_insert(
				SESSION, STATE.input,
				STATE.perms & PERM_DATA_WRITE ? 1 : 0,
				STATE.perms & PERM_ANA_WRITE ? 1 : 0,
				&ana_id, &context_id));

	/* Set the values in the output */
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.output, DBA_KEY_ANA_ID, ana_id));
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.output, DBA_KEY_CONTEXT_ID, context_id));

	/* Set context id and variable name on qcinput so that
	 * attribute functions will refer to what has been written */
	DBA_RUN_OR_RETURN(dba_record_key_seti(STATE.qcinput, DBA_KEY_CONTEXT_ID, context_id));

	/* If there was only one variable in the input, we can pass it on as a
	 * default for attribute handling routines; otherwise we unset to mark
	 * the ambiguity */
	if ((cur = dba_record_iterate_first(STATE.input)) != NULL &&
			dba_record_iterate_next(STATE.input, cur) == NULL)
		var = dba_record_cursor_variable(cur);
	
	if (var != NULL)
	{
		dba_varcode code = dba_var_code(var);
		char varname[8];
		snprintf(varname, 7, "B%02d%03d", DBA_VAR_X(code), DBA_VAR_Y(code));
		DBA_RUN_OR_RETURN(dba_record_key_setc(STATE.qcinput, DBA_KEY_VAR_RELATED, varname));
	}
	else
		DBA_RUN_OR_RETURN(dba_record_key_unset(STATE.qcinput, DBA_KEY_VAR_RELATED));

	/* Copy the input on the output, so that QC functions can find the data
	 * they need */
	return dba_error_ok();
}

/**
 * Remove all selected items from the database.
 *
 * This function will fail unless the database is open in data rewrite mode.
 *
 * @param handle
 *   Handle to a DBALLE session
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_dimenticami)(
		INTEGER(handle))
{
	GENPTR_INTEGER(handle)

	if (! (STATE.perms & PERM_DATA_WRITE))
		return dba_error_consistency(
			"idba_dimenticami must be called with the database open in data write mode");

	return dba_db_remove(SESSION, STATE.input);
}

/*
 * Look for the ID of the data which a critica or scusa operation are
 * supposed to operate on.
 */
static dba_err get_referred_data_id(int* handle, int* id_context, dba_varcode* id_var)
{
	const char* val;
	int found;

	/* Read context ID */
	DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.qcinput, DBA_KEY_CONTEXT_ID, id_context, &found));
	if (!found)
		return dba_error_notfound("looking for variable context id");

	*id_var = 0;

#if 0
	/* First try with *data_id */
	if (*id == 0 && (val = dba_record_key_peek_value(STATE.qcinput, DBA_KEY_DATA_ID)) != NULL)
	{
		*id = strtol(val, 0, 10);
		if (*id == 0)
			return dba_error_consistency("checking *data_id is set to non-zero");
	}
#endif
	/* Then with *var */
	if (*id_var == 0 && (val = dba_record_key_peek_value(STATE.qcinput, DBA_KEY_VAR_RELATED)) != NULL)
		*id_var = DBA_STRING_TO_VAR(val + 1);
#if 0
	/* Lastly, with the data_id from last idba_dammelo */
	if (*id == 0)
	{
		DBA_RUN_OR_RETURN(dba_record_key_enqi(STATE.output, DBA_KEY_DATA_ID, id));
		if (*id == 0)
			return dba_error_consistency("checking that data_id left by previous idba_dammelo is set to non-zero");
	}
#endif
	/*return dba_error_consistency("looking for data ID (no id specified, and no variable previously selected with idba_setc(handle, \"*b\", \"Bxxyyy\"))");*/

	return dba_error_ok();
}

/* Reads the list of QC values to operate on, for dba_voglioancora and dba_scusa */
static dba_err read_qc_list(int* handle, dba_varcode** res_arr, size_t* res_len)
{
	dba_err err = DBA_OK;
	dba_varcode* arr = NULL;
	size_t arr_len = 0;
	const char* val;

	/* Retrieve the varcodes of the wanted QC values */
	if ((val = dba_record_key_peek_value(STATE.qcinput, DBA_KEY_VAR)) != NULL)
	{
		/* Get only the QC values in *varlist */
		if (*val != '*')
		{
			err = dba_error_consistency("QC values to delete must start with '*'");
			goto cleanup;
		}
		if ((arr = (dba_varcode*)malloc(1 * sizeof(dba_varcode))) == NULL)
		{
			err = dba_error_alloc("allocating the dba_varcode array to pass to dba_qc_query");
			goto cleanup;
		}
		arr_len = 1;
		arr[0] = DBA_STRING_TO_VAR(val + 2);
	}
	else if ((val = dba_record_key_peek_value(STATE.qcinput, DBA_KEY_VARLIST)) != NULL)
	{
		/* Get only the QC values in *varlist */
		size_t pos;
		size_t len;
		const char* s;
		int i;

		/* Count the number of commas (and therefore of items in the
		 * list) to decide the size of arr */
		for (s = val, arr_len = 1; *s; ++s)
			++arr_len;
		if ((arr = (dba_varcode*)malloc(arr_len * sizeof(dba_varcode))) == NULL)
		{
			err = dba_error_alloc("allocating the dba_varcode array to pass to dba_qc_query");
			goto cleanup;
		}

		for (pos = 0, i = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
		{
			/*
			fprintf(stderr, "str: \"%s\", str+pos: \"%s\", str+pos+len: \"%s\"\n",
					val, val+pos, val+pos+len);
			*/
			if (*(val+pos) != '*')
			{
				err = dba_error_consistency("QC values to delete must start with '*'");
				goto cleanup;
			}
			arr[i++] = DBA_STRING_TO_VAR(val + pos + 1);
		}
	}

	*res_arr = arr;
	*res_len = arr_len;
	arr = NULL;
	
cleanup:
	if (arr != NULL)
		free(arr);
	return err == DBA_OK ? dba_error_ok() : err;
}

/**@name QC functions
 * @anchor qc
 * Functions used to manipulate QC data.
 *
 * All these functions require some context data about the variable, which is
 * automatically available when the variable just came as the result of an
 * idba_dammelo() or has just been inserted with an idba_prendilo().
 *@{
 */

F77_INTEGER_FUNCTION(idba_voglioancora)(INTEGER(handle), INTEGER(count))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(count)
	dba_err err = DBA_OK;
	dba_varcode* arr = NULL;
	size_t arr_len = 0;
	int id_context;
	dba_varcode id_var;

	/* Retrieve the ID of the data to query */
	DBA_RUN_OR_RETURN(get_referred_data_id(handle, &id_context, &id_var));

	/* Retrieve the varcodes of the wanted QC values */
	DBA_RUN_OR_RETURN(read_qc_list(handle, &arr, &arr_len));

	/* Do QC query */
	DBA_RUN_OR_GOTO(cleanup, dba_db_qc_query(SESSION, id_context, id_var, 
				arr == NULL ? NULL : arr,
				arr == NULL ? 0 : arr_len,
				STATE.qcoutput, &(STATE.qc_count)));

	STATE.qc_iter = dba_record_iterate_first(STATE.qcoutput);

	*count = STATE.qc_count;

	clear_attr_rec(STATE.qcinput);

cleanup:
	if (arr != NULL)
		free(arr);
	return err == DBA_OK ? dba_error_ok() : err;
}

/**
 * Retrieve QC informations from the last variable returned by idba_dammelo().
 *
 * @param handle
 *   Handle to a DBALLE session
 * @retval parameter
 *   Contains the ID of the parameter retrieved by this fetch
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_ancora)(
		INTEGER(handle),
		CHARACTER(parameter)
		TRAIL(parameter))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(parameter)
	dba_varcode var;
	char parm[20];

	if (STATE.qc_iter == NULL)
		return dba_error_notfound("reading a QC item");

	var = dba_var_code(dba_record_cursor_variable(STATE.qc_iter));
	snprintf(parm, 20, "*B%02d%03d", DBA_VAR_X(var), DBA_VAR_Y(var));
	cnfExprt(parm, parameter, parameter_length);

	/* Get next value from qc */
	STATE.qc_iter = dba_record_iterate_next(STATE.qcoutput, STATE.qc_iter);

	return dba_error_ok();
}

/**
 * Insert new QC informations for a variable of the current record.
 *
 * QC informations inserted are all those set by the functions idba_seti(),
 * idba_setc(), idba_setr(), idba_setd(), using an asterisk in front of the
 * variable name.
 *
 * Contrarily to idba_prendilo(), this function resets all the QC informations
 * (but only the QC informations) previously set in input, so the values to be
 * inserted need to be explicitly set every time.
 *
 * This function will fail if the database is open in QC readonly mode, and it
 * will refuse to overwrite existing values if the database is open in QC add
 * mode.
 *
 * The variable referred by the QC informations can be specified in three ways:
 * 
 * \li by variable code, using ::idba_setc(handle, "*var", "Bxxyyy")
 * \li by variable id, using ::idba_seti(handle, "*data_id", id)
 * \li unspecified, will use the last variable returned by ::idba_dammelo
 *
 * @param handle
 *   Handle to a DBALLE session
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_critica)(
		INTEGER(handle))
{
	GENPTR_INTEGER(handle)
	int id_context;
	dba_varcode id_var;

	if (STATE.perms & PERM_ATTR_RO)
		return dba_error_consistency(
			"idba_critica cannot be called with the database open in attribute readonly mode");

	DBA_RUN_OR_RETURN(get_referred_data_id(handle, &id_context, &id_var));

	DBA_RUN_OR_RETURN(dba_db_qc_insert_or_replace(
				SESSION, id_context, id_var, STATE.qcinput,
				STATE.perms & PERM_ATTR_WRITE ? 1 : 0));

	clear_attr_rec(STATE.qcinput);

	return dba_error_ok();
}

/**
 * Remove QC informations for a variable of the current record.
 *
 * The QC informations to be removed are set with:
 * \code
 *   idba_setc(handle, "*varlist", "*B33021,*B33003");
 * \endcode
 *
 * The variable referred by the QC informations can be specified in three ways:
 * 
 * \li by variable code, using ::idba_setc(handle, "*var", "Bxxyyy")
 * \li by variable id, using ::idba_seti(handle, "*data_id", id)
 * \li unspecified, will use the last variable returned by ::idba_dammelo
 *
 * @param handle
 *   Handle to a DBALLE session
 * @return
 *   The error indicator for the function
 */
F77_INTEGER_FUNCTION(idba_scusa)(INTEGER(handle))
{
	GENPTR_INTEGER(handle)
	dba_err err = DBA_OK;
	dba_varcode* arr = NULL;
	size_t arr_len = 0;
	int id_context;
	dba_varcode id_var;

	if (! (STATE.perms & PERM_ATTR_WRITE))
		return dba_error_consistency(
			"idba_scusa must be called with the database open in attribute write mode");
	
	DBA_RUN_OR_RETURN(get_referred_data_id(handle, &id_context, &id_var));

	/* Retrieve the varcodes of the wanted QC values */
	DBA_RUN_OR_RETURN(read_qc_list(handle, &arr, &arr_len));

	// If arr is still 0, then dba_qc_delete deletes all QC values
	DBA_RUN_OR_GOTO(cleanup,
			dba_db_qc_remove(
				SESSION, id_context, id_var,
				arr == NULL ? NULL : arr,
				arr == NULL ? 0 : arr_len));

	clear_attr_rec(STATE.qcinput);

cleanup:
	if (arr != NULL)
		free(arr);
	return err == DBA_OK ? dba_error_ok() : err;
}

F77_INTEGER_FUNCTION(idba_spiegal)(
		INTEGER(handle),
		INTEGER(ltype),
		INTEGER(l1),
		INTEGER(l2),
		CHARACTER(result)
		TRAIL(result))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(ltype)
	GENPTR_INTEGER(l1)
	GENPTR_INTEGER(l2)
	GENPTR_CHARACTER(result)
	char* res;

	DBA_RUN_OR_RETURN(dba_formatter_describe_level(*ltype, *l1, *l2, &res));
	cnfExprt(res, result, result_length);
	free(res);
	return dba_error_ok();
}

F77_INTEGER_FUNCTION(idba_spiegat)(
		INTEGER(handle),
		INTEGER(ptype),
		INTEGER(p1),
		INTEGER(p2),
		CHARACTER(result)
		TRAIL(result))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(ptype)
	GENPTR_INTEGER(p1)
	GENPTR_INTEGER(p2)
	GENPTR_CHARACTER(result)
	char* res;

	DBA_RUN_OR_RETURN(dba_formatter_describe_trange(*ptype, *p1, *p2, &res));
	cnfExprt(res, result, result_length);
	free(res);
	return dba_error_ok();
}

F77_INTEGER_FUNCTION(idba_spiegab)(
		INTEGER(handle),
		CHARACTER(varcode),
		CHARACTER(value),
		CHARACTER(result)
		TRAIL(varcode)
		TRAIL(value)
		TRAIL(result))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(varcode)
	GENPTR_CHARACTER(value)
	GENPTR_CHARACTER(result)
	dba_var var = NULL;
	char s_varcode[10];
	char s_value[300];
	char* res = NULL;
	dba_varinfo info;
	dba_err err = DBA_OK;

	cnfImpn(varcode, varcode_length, 9, s_varcode); s_varcode[9] = 0;
	cnfImpn(value, value_length, 299, s_value); s_value[299] = 0;

	DBA_RUN_OR_RETURN(dba_varinfo_query_local(DBA_STRING_TO_VAR(s_varcode + 1), &info));
	DBA_RUN_OR_RETURN(dba_var_createc(info, s_value, &var));

	if (info->is_string)
	{
		const char* s;
		DBA_RUN_OR_GOTO(cleanup, dba_var_enqc(var, &s));
		asprintf(&res, "%s (%s) %s", s, info->unit, info->desc);
	} else {
		double d;
		DBA_RUN_OR_GOTO(cleanup, dba_var_enqd(var, &d));
		asprintf(&res, "%.*f (%s) %s", info->scale > 0 ? info->scale : 0, d, info->unit, info->desc);
	}

	cnfExprt(res, result, result_length);

cleanup:
	if (var != NULL)
		dba_var_delete(var);
	if (res != NULL)
		free(res);
	return err != DBA_OK ? err : dba_error_ok();
}

/*@}*/

/* vim:set ts=4 sw=4: */
