/*
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "config.h"
#include "dballe/core/verbose.h"
#include "dballe/simple/msgapi.h"
#include "dballe/simple/dbapi.h"
#include "dballe/db/db.h"

#include <cstring>	// memset
#include <limits.h>
#include <float.h>
#include "handles.h"
#include "error.h"
#include "trace.h"

extern "C" {
#include <f77.h>
}

//#define TRACEMISSING(type) fprintf(stderr, "SET TO MISSING (" type ")\n")
#define TRACEMISSING(type) do {} while(0)

/*
 * First attempt using constants
 */
//#define MISSING_STRING ""
// Largest signed one byte value
#define MISSING_BYTE SCHAR_MAX
// integer 2 byte   32767
// Largest signed int value
#define MISSING_INT INT_MAX
//#define MISSING_REAL (3.4028235E+38f)
// Largest positive float value
#define MISSING_REAL FLT_MAX
// Largest positive double value
#define MISSING_DOUBLE   DBL_MAX
//#define MISSING_DOUBLE   (1.79769E+308)
//#define MISSING_DOUBLE   (1.7976931348623167E+308)
//#define MISSING_DOUBLE   (1.797693134862316E+308)
//#define MISSING_DOUBLE   (1.79769313486E+308)
//#define MISSING_DOUBLE ((double)0x7FEFFFFFFFFFFFFF)

using namespace dballe;
using namespace wreport;

static inline void tofortran(int& val)
{
	if (val == fortran::API::missing_int)
		val = MISSING_INT;
}
static inline int fromfortran(int val)
{
	return val == MISSING_INT ? fortran::API::missing_int : val;
}

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
 *   call idba_preparati(dba, handle, "read", "add", "write")
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

using namespace std;

struct HSession : public fortran::HBase
{
	DB* db;

	void start()
	{
		fortran::HBase::start();
		db = 0;
	}

	void stop()
	{
        if (db) delete db;
		fortran::HBase::stop();
	}
};

struct fortran::Handler<HSession, MAX_SESSION> hsess;

struct HSimple : public fortran::HBase
{
	fortran::API* api;
    fortran::SessionTracer trace;

	void start()
	{
		fortran::HBase::start();
		api = 0;
	}
	void stop()
	{
		if (api) delete api;
		fortran::HBase::stop();
	}
};

struct fortran::Handler<HSimple, MAX_SIMPLE> hsimp;

// FDBA_HANDLE_BODY(simple, MAX_SIMPLE, "Dballe simple sessions")
// FDBA_HANDLE_BODY(session, MAX_SESSION, "Dballe sessions")

//#define STATE (simples.get(*handle))
//#define SESSION (sessions.get(simples.get(*handle).session))
//FDBA_HANDLE(session, FDBA_HANDLE(simple, *handle).session).session)

static int usage_refcount = 0;

static void lib_init()
{
    if (usage_refcount > 0)
        return;

    dba_verbose_init();

    fortran::trace_init();
    fortran::error_init();
    hsess.init("DB-All.e database sessions", "MAX_CALLBACKS");
    hsimp.init("DB-All.e work sessions", "MAX_SIMPLE");

	++usage_refcount;
}

extern "C" {

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
	const char* chosen_dsn;
	char s_dsn[256];
	char s_user[20];
	char s_password[20];

	try {
		/* Import input string parameters */
		cnfImpn(dsn, dsn_length, 255, s_dsn); s_dsn[255] = 0;
		cnfImpn(user, user_length, 19, s_user); s_user[19] = 0;
		cnfImpn(password, password_length, 19, s_password); s_password[19] = 0;

		/* Initialize the library if needed */
		lib_init();

		/* Allocate and initialize a new handle */
		*dbahandle = hsess.request();
		HSession& hs = hsess.get(*dbahandle);

		/* Open the DBALLE session */

		/* If dsn is missing, look in the environment */
		if (s_dsn[0] == 0)
		{
			chosen_dsn = getenv("DBA_DB");
			if (chosen_dsn == NULL) chosen_dsn = "";
		} else
			chosen_dsn = s_dsn;

        /* If dsn looks like a url, treat it accordingly */
        if (DB::is_url(chosen_dsn))
        {
            IF_TRACING(fortran::log_presentati_url(*dbahandle, chosen_dsn));
            hs.db = DB::connect_from_url(chosen_dsn).release();
        }
        else
        {
            IF_TRACING(fortran::log_presentati_dsn(*dbahandle, chosen_dsn, s_user, s_password));
            hs.db = DB::connect(chosen_dsn, s_user, s_password).release();
        }

		/* Open the database session */
		return fortran::success();
	} catch (error& e) {
        hsess.release(*dbahandle);
		return fortran::error(e);
	}
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

    IF_TRACING(fortran::log_arrivederci(*dbahandle));

    // try {
        hsess.release(*dbahandle);

		/*
		dba_shutdown does not exist anymore, but I keep this code commented out
		here as a placeholder if in the future we'll need to hook actions when the
		usage refcount goes to 0

		if (--usage_refcount == 0)
			dba_shutdown();
		*/

	// 	return fortran::success();
	// } catch (error& e) {
	// 	return fortran::error(e);
	// }
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
	char c_anaflag[10];
	char c_dataflag[10];
	char c_attrflag[10];
	cnfImpn(anaflag, anaflag_length,  10, c_anaflag);
	cnfImpn(dataflag, dataflag_length,  10, c_dataflag);
	cnfImpn(attrflag, attrflag_length,  10, c_attrflag);

	try {
		/* Check here to warn users of the introduction of idba_presentati */
		/*
		if (session == NULL)
			return dba_error_consistency("idba_presentati should be called before idba_preparati");
		*/

		/* Allocate and initialize a new handle */
		*handle = hsimp.request();
		HSession& hs = hsess.get(*dbahandle);
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_preparati(*dbahandle, *handle, c_anaflag, c_dataflag, c_attrflag));
		h.api = new fortran::DbAPI(*hs.db, c_anaflag, c_dataflag, c_attrflag);

		return fortran::success();
	} catch (error& e) {
		hsimp.release(*handle);
		return fortran::error(e);
	}
}

/**
 * Access a file with weather messages
 *
 * @retval handle
 *   The session handle returned by the function
 * @param filename
 *   Name of the file to open
 * @param mode
 *   File open mode.  It can be:
 *   \li \c r for read
 *   \li \c w for write (the old file is deleted)
 *   \li \c a for append
 * @param type
 *   Format of the data in the file.  It can be:
 *   \li \c "BUFR"
 *   \li \c "CREX"
 *   \li \c "AOF" (read only)
 *   \li \c "AUTO" (autodetect, read only)
 * @param force_report
 *   if 0, nothing happens; otherwise, choose the output message template
 *   using this report type instead of the one in the message
 * @return
 *   The error indication for the function.
 */
F77_INTEGER_FUNCTION(idba_messaggi)(
		INTEGER(handle),
		CHARACTER(filename),
		CHARACTER(mode),
		CHARACTER(type)
		TRAIL(filename)
		TRAIL(mode)
		TRAIL(type))
{
	GENPTR_INTEGER(handle)
	GENPTR_CHARACTER(filename)
	GENPTR_CHARACTER(mode)
	GENPTR_CHARACTER(type)
	char c_filename[512];
	char c_mode[10];
	char c_type[10];

	cnfImpn(filename, filename_length,  512, c_filename);
	cnfImpn(mode, mode_length,  10, c_mode);
	cnfImpn(type, type_length,  10, c_type);

	try {
		lib_init();

		*handle = hsimp.request();
		//HSession& hs = hsess.get(*dbahandle);
		HSimple& h = hsimp.get(*handle);

        IF_TRACING(h.trace.log_messaggi(*handle, c_filename, c_mode, c_type));

		h.api = new fortran::MsgAPI(c_filename, c_mode, c_type);

		return fortran::success();
	} catch (error& e) {
		hsimp.release(*handle);
		return fortran::error(e);
	}
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

    try {
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_fatto());
		hsimp.release(*handle);
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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

	cnfImpn(repinfofile, repinfofile_length,  PATH_MAX, fname); fname[PATH_MAX - 1] = 0;

	try {
        HSimple& h = hsimp.get(*handle);
        if (fname[0] == 0)
        {
            IF_TRACING(h.trace.log_scopa());
            h.api->scopa(0);
        } else {
            IF_TRACING(h.trace.log_scopa(fname));
            h.api->scopa(fname);
        }

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
}

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
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		*value = h.api->enqi(parm);
		tofortran(*value);
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		*value = h.api->enqb(parm);
		if (*value == fortran::API::missing_byte)
			*value = MISSING_BYTE;
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		*value = h.api->enqr(parm);
		if (*value == fortran::API::missing_float)
			*value = MISSING_REAL;
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		*value = h.api->enqd(parm);
		if (*value == fortran::API::missing_double)
			*value = MISSING_DOUBLE;
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		const char* v = h.api->enqc(parm);
		if (!v)
		{
			if (value_length > 0)
			{
				// The missing string value has been defined as a
				// null byte plus blank padding.
				value[0] = 0;
				memset(value+1, ' ', value_length - 1);
			}
		} else
			cnfExprt(v, value, value_length);
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	char parm[20];
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		if (*value == MISSING_INT)
		{
			TRACEMISSING("int");
            IF_TRACING(h.trace.log_unset(parm));
			h.api->unset(parm);
		}
		else
        {
            IF_TRACING(h.trace.log_set(parm, *value));
			h.api->seti(parm, *value);
        }
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
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
	char parm[20];
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		if (*value == MISSING_BYTE)
		{
			TRACEMISSING("byte");
            IF_TRACING(h.trace.log_unset(parm));
			h.api->unset(parm);
		}
		else
        {
            IF_TRACING(h.trace.log_set(parm, *value));
			h.api->setb(parm, *value);
        }
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
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
	char parm[20];
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		if (*value == MISSING_REAL)
		{
			TRACEMISSING("real");
            IF_TRACING(h.trace.log_unset(parm));
			h.api->unset(parm);
		}
		else
        {
            IF_TRACING(h.trace.log_set(parm, *value));
			h.api->setr(parm, *value);
        }
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
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
	char parm[20];
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		if (*value == MISSING_DOUBLE)
		{
			TRACEMISSING("double");
            IF_TRACING(h.trace.log_unset(parm));
			h.api->unset(parm);
		}
		else
        {
            IF_TRACING(h.trace.log_set(parm, *value));
			h.api->setd(parm, *value);
        }
		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
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
	char parm[20];
	char val[255];
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;
	cnfImpn(value, value_length, 254, val); val[254] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		if (val[0] == 0)
		{
			TRACEMISSING("char");
            IF_TRACING(h.trace.log_unset(parm));
			h.api->unset(parm);
		}
		else
        {
            IF_TRACING(h.trace.log_set(parm, val));
			h.api->setc(parm, val);
        }

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
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
	try {
		HSimple& h = hsimp.get(*handle);
		IF_TRACING(h.trace.log_func("setcontextana"));
		h.api->setcontextana();

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
		INTEGER(ltype1),
		INTEGER(l1),
		INTEGER(ltype2),
		INTEGER(l2))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(ltype1)
	GENPTR_INTEGER(l1)
	GENPTR_INTEGER(ltype2)
	GENPTR_INTEGER(l2)
	try {
		HSimple& h = hsimp.get(*handle);
		h.api->enqlevel(*ltype1, *l1, *ltype2, *l2);
		tofortran(*ltype1); tofortran(*l1);
		tofortran(*ltype2); tofortran(*l2);

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
		INTEGER(ltype1),
		INTEGER(l1),
		INTEGER(ltype2),
		INTEGER(l2))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(ltype1)
	GENPTR_INTEGER(l1)
	GENPTR_INTEGER(ltype2)
	GENPTR_INTEGER(l2)
	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_setlevel(
                fromfortran(*ltype1), fromfortran(*l1),
                fromfortran(*ltype2), fromfortran(*l2)));
		h.api->setlevel(
			fromfortran(*ltype1), fromfortran(*l1),
			fromfortran(*ltype2), fromfortran(*l2));

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	try {
		HSimple& h = hsimp.get(*handle);
		h.api->enqtimerange(*ptype, *p1, *p2);
		tofortran(*ptype); tofortran(*p1); tofortran(*p2);

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_settimerange(fromfortran(*ptype), fromfortran(*p1), fromfortran(*p2)));
		h.api->settimerange(
			fromfortran(*ptype), fromfortran(*p1), fromfortran(*p2));

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	try {
		HSimple& h = hsimp.get(*handle);
		h.api->enqdate(*year, *month, *day, *hour, *min, *sec);
		tofortran(*year), tofortran(*month), tofortran(*day);
		tofortran(*hour), tofortran(*min), tofortran(*sec);

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_setdate(
                fromfortran(*year), fromfortran(*month), fromfortran(*day),
                fromfortran(*hour), fromfortran(*min), fromfortran(*sec)));

		h.api->setdate(
			fromfortran(*year), fromfortran(*month), fromfortran(*day),
			fromfortran(*hour), fromfortran(*min), fromfortran(*sec));

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_setdate(
                fromfortran(*year), fromfortran(*month), fromfortran(*day),
                fromfortran(*hour), fromfortran(*min), fromfortran(*sec), "min"));
		h.api->setdatemin(
			fromfortran(*year), fromfortran(*month), fromfortran(*day),
			fromfortran(*hour), fromfortran(*min), fromfortran(*sec));

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_setdate(
                fromfortran(*year), fromfortran(*month), fromfortran(*day),
                fromfortran(*hour), fromfortran(*min), fromfortran(*sec), "max"));
		h.api->setdatemax(
			fromfortran(*year), fromfortran(*month), fromfortran(*day),
			fromfortran(*hour), fromfortran(*min), fromfortran(*sec));

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	cnfImpn(parameter, parameter_length, 19, parm); parm[19] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_unset(parm));
		h.api->unset(parm);

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
}

/**
 * Remove all B* parameters from the input record
 * 
 * @param handle
 *   Handle to a DBALLE session
 */
F77_INTEGER_FUNCTION(idba_unsetb)(
        INTEGER(handle))
{
    GENPTR_INTEGER(handle)

    try {
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_func("unsetb"));
        h.api->unsetb();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Remove all parameters from the input record
 *
 * @param handle
 *   Handle to a DBALLE session
 */
F77_INTEGER_FUNCTION(idba_unsetall)(
        INTEGER(handle))
{
    GENPTR_INTEGER(handle)

    try {
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_func("unsetall"));
        h.api->unsetall();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
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

	try {
		HSimple& h = hsimp.get(*handle);
		IF_TRACING(h.trace.log_quantesono());
		*count = h.api->quantesono();
        IF_TRACING(fortran::log_result(*count));

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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

	try {
		HSimple& h = hsimp.get(*handle);
		IF_TRACING(h.trace.log_func("elencamele"));
		h.api->elencamele();

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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

	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_voglioquesto());
        *count = h.api->voglioquesto();
        IF_TRACING(fortran::log_result(*count));

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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

	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_dammelo());
        const char* res = h.api->dammelo();
        IF_TRACING(fortran::log_result(res));
		if (!res)
			cnfExprt("", parameter, parameter_length);
		else
			cnfExprt(res, parameter, parameter_length);

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	try {
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_func("prendilo"));
        h.api->prendilo();

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	try {
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_func("dimenticami"));
        h.api->dimenticami();

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_voglioancora());
        *count = h.api->voglioancora();
        IF_TRACING(fortran::log_result(*count));

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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

	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_ancora());
        const char* res = h.api->ancora();
        IF_TRACING(fortran::log_result(res));
		if (!res)
			cnfExprt("", parameter, parameter_length);
		else
			cnfExprt(res, parameter, parameter_length);

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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

	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_func("critica"));
		h.api->critica();

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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

	try {
		HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_func("scusa"));
		h.api->scusa();

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
}

F77_INTEGER_FUNCTION(idba_messages_open)(
        INTEGER(handle),
        CHARACTER(filename),
        CHARACTER(mode),
        CHARACTER(format),
        CHARACTER(options)
        TRAIL(filename)
        TRAIL(mode)
        TRAIL(format)
        TRAIL(options))
{
    GENPTR_INTEGER(handle)
    GENPTR_CHARACTER(filename)
    GENPTR_CHARACTER(mode)
    GENPTR_CHARACTER(format)
    GENPTR_CHARACTER(options)
    char c_filename[512];
    char c_mode[10];
    char c_format[10];
    char c_options[512];
    cnfImpn(filename, filename_length,  511, c_filename); c_filename[511] = 0;
    cnfImpn(mode, mode_length,  9, c_mode); c_mode[9] = 0;
    cnfImpn(format, format_length,  9, c_format); c_format[9] = 0;
    cnfImpn(options, options_length,  511, c_options); c_options[511] = 0;

    try {
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_messages_open(c_filename, c_mode, c_format, c_options));
        h.api->messages_open(c_filename, c_mode, parse_encoding(c_format), c_options);

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

F77_INTEGER_FUNCTION(idba_messages_read_next)(INTEGER(handle), LOGICAL(found))
{
    GENPTR_INTEGER(handle)
    GENPTR_LOGICAL(found)
    try {
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_messages_read_next());
        *found = h.api->messages_read_next();
        IF_TRACING(fortran::log_result(*found));

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

F77_INTEGER_FUNCTION(idba_remove_all)(
        INTEGER(handle))
{
    GENPTR_INTEGER(handle)

    try {
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_func("remove_all"));
        h.api->remove_all();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

F77_INTEGER_FUNCTION(idba_spiegal)(
		INTEGER(handle),
		INTEGER(ltype1),
		INTEGER(l1),
		INTEGER(ltype2),
		INTEGER(l2),
		CHARACTER(result)
		TRAIL(result))
{
	GENPTR_INTEGER(handle)
	GENPTR_INTEGER(ltype1)
	GENPTR_INTEGER(l1)
	GENPTR_INTEGER(ltype2)
	GENPTR_INTEGER(l2)
	GENPTR_CHARACTER(result)

	try {
		HSimple& h = hsimp.get(*handle);
		const char* res = h.api->spiegal(*ltype1, *l1, *ltype2, *l2);
		cnfExprt(res, result, result_length);

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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

	try {
		HSimple& h = hsimp.get(*handle);
		const char* res = h.api->spiegat(*ptype, *p1, *p2);
		cnfExprt(res, result, result_length);

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
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
	char s_varcode[10];
	char s_value[300];
	cnfImpn(varcode, varcode_length, 9, s_varcode); s_varcode[9] = 0;
	cnfImpn(value, value_length, 299, s_value); s_value[299] = 0;

	try {
		HSimple& h = hsimp.get(*handle);
		const char* res = h.api->spiegab(s_varcode, s_value);
		cnfExprt(res, result, result_length);

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
}

F77_INTEGER_FUNCTION(idba_test_input_to_output)(
		INTEGER(handle))
{
	GENPTR_INTEGER(handle)

	try {
		HSimple& h = hsimp.get(*handle);
		h.api->test_input_to_output();

		return fortran::success();
	} catch (error& e) {
		return fortran::error(e);
	}
}


}

/*@}*/

/* vim:set ts=4 sw=4: */
