#include "config.h"
#include "dballe/simple/msgapi.h"
#include "dballe/simple/dbapi.h"
#include "dballe/db/db.h"

#include <cstring>  // memset
#include <limits.h>
#include <float.h>
#include "common.h"
#include "handles.h"
#include "error.h"
#include "trace.h"

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

/** @file
 * @ingroup fortran
 * Simplified interface for Dballe.
 *
 * Every function returns an error indicator, which is 0 if no error happened,
 * or 1 if there has been an error.
 *
 * When an error happens, the functions in error.c can be used
 * to get detailed informations about it.
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

static int usage_refcount = 0;

static void lib_init()
{
    if (usage_refcount > 0)
        return;

    fortran::trace_init();
    fortran::error_init();
    hsess.init("DB-All.e database sessions", "MAX_CALLBACKS");
    hsimp.init("DB-All.e work sessions", "MAX_SIMPLE");

    ++usage_refcount;
}

extern "C" {

/**@name Session routines
 * @anchor idba_enq
 *
 * These routines are used to begin and end working sessions with DB-All.e.
 * @{
 */

/**
 * Connect to the database
 *
 * This function can be called more than once once to connect to different
 * databases at the same time.
 *
 * @param url
 *   The URL of the database to use
 * @param user
 *   Used in the past, now it is ignored.
 * @param password
 *   Used in the past, now it is ignored.
 * @retval dbahandle
 *   The database handle that can be passed to idba_preparati to work with the
 *   database.
 * @return
 *   The error indication for the function.
 */
int idba_presentati(int* dbahandle, const char* url, const char* user, const char* password)
{
    try {
        /* Initialize the library if needed */
        lib_init();

        /* Allocate and initialize a new handle */
        *dbahandle = hsess.request();
        HSession& hs = hsess.get(*dbahandle);

        /* Open the DB-All.e session */

        /* If url is missing, look in the environment */
        if (url[0] == 0)
        {
            url = getenv("DBA_DB");
            if (url == NULL) url = "";
        }

        IF_TRACING(fortran::log_presentati_url(*dbahandle, url));
        hs.db = DB::connect_from_url(url).release();

        /* Open the database session */
        return fortran::success();
    } catch (error& e) {
        hsess.release(*dbahandle);
        return fortran::error(e);
    }
}

/**
 * Disconnect from the database.
 *
 * @param dbahandle
 *   The database handle to close.
 */
int idba_arrivederci(int *dbahandle)
{
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

    //  return fortran::success();
    // } catch (error& e) {
    //  return fortran::error(e);
    // }
    return fortran::success();
}


/**
 * Open a new session.
 *
 * You can call idba_preparati() many times and get more handles.  This allows
 * to perform many operations on the database at the same time.
 *
 * idba_preparati() has three extra parameters that can be used to limit
 * write operations on the database, as a limited protection against
 * programming errors:
 *
 * `anaflag` controls access to station value records and can have these values:
 *
 * \li \c "read" station records cannot be inserted.
 * \li \c "write" it is possible to insert and delete pseudoana records.
 *
 * `dataflag` controls access to observed data and can have these values:
 *
 * \li \c "read" data cannot be modified in any way.
 * \li \c "add" data can be added to the database, but existing data cannot be
 * modified.  Deletions are disabled.  This is used to insert new data in the
 * database while preserving the data that was already present in it.
 * \li \c "write" data can freely be added, overwritten and deleted.
 *
 * `attrflag` controls access to data attributes and can have these values:
 *
 * \li \c "read" attributes cannot be modified in any way.
 * \li \c "write" attributes can freely be added, overwritten and deleted.
 *
 * Note that some combinations of parameters are illegal, such as anaflag=read
 * and dataflag=add (when adding a new data, it's sometimes necessary to insert
 * new pseudoana records), or dataflag=rewrite and attrflag=read (when deleting
 * data, their attributes are deleted as well).
 *
 * @param dbahandle
 *   The main DB-ALLe connection handle
 * @retval handle
 *   The session handle created by the function
 * @param anaflag station values access level
 * @param dataflag data values access level
 * @param attrflag attribute access level
 * @return
 *   The error indication for the function.
 */
int idba_preparati(int dbahandle, int* handle, const char* anaflag, const char* dataflag, const char* attrflag)
{
    try {
        /* Check here to warn users of the introduction of idba_presentati */
        /*
        if (session == NULL)
            return dba_error_consistency("idba_presentati should be called before idba_preparati");
        */

        /* Allocate and initialize a new handle */
        *handle = hsimp.request();
        HSession& hs = hsess.get(dbahandle);
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_preparati(dbahandle, *handle, anaflag, dataflag, attrflag));
        h.api = new fortran::DbAPI(*hs.db, anaflag, dataflag, attrflag);

        return fortran::success();
    } catch (error& e) {
        hsimp.release(*handle);
        return fortran::error(e);
    }
}

/**
 * Start working with a message file.
 *
 * @retval handle
 *   The session handle returned by the function
 * @param filename
 *   Name of the file to open
 * @param mode
 *   File open mode.  It can be `"r"` for read, `"w"` for write (the old file
 *   is deleted), `"a"` for append
 * @param type
 *   Format of the data in the file.  It can be: `"BUFR"`, `"CREX"`, `"AOF"`
 *   (read only), `"AUTO"` (autodetect, read only)
 * @return
 *   The error indication for the function.
 */
int idba_messaggi(int* handle, const char* filename, const char* mode, const char* type)
{
    try {
        lib_init();

        *handle = hsimp.request();
        //HSession& hs = hsess.get(*dbahandle);
        HSimple& h = hsimp.get(*handle);

        IF_TRACING(h.trace.log_messaggi(*handle, filename, mode, type));

        h.api = new fortran::MsgAPI(filename, mode, type);

        return fortran::success();
    } catch (error& e) {
        hsimp.release(*handle);
        return fortran::error(e);
    }
}


/**
 * Close a session.
 *
 * @param handle
 *   Handle to the session to be closed.
 */
int idba_fatto(int* handle)
{
    try {
        HSimple& h = hsimp.get(*handle);
        IF_TRACING(h.trace.log_fatto());
        h.api->fatto();
        hsimp.release(*handle);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/*@}*/


/**@name Input/output routines
 * These routines are used to set the input and read the output of action routines.
 * @{
 */

/**
 * Set an integer value in input
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in fapi_parms.md
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
int idba_seti(int handle, const char* parameter, const int* value)
{
    try {
        HSimple& h = hsimp.get(handle);
        if (*value == MISSING_INT)
        {
            TRACEMISSING("int");
            IF_TRACING(h.trace.log_unset(parameter));
            h.api->unset(parameter);
        }
        else
        {
            IF_TRACING(h.trace.log_set(parameter, *value));
            h.api->seti(parameter, *value);
        }
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Set a byte value in input
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in fapi_parms.md
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
int idba_setb(int handle, const char* parameter, const unsigned char* value)
{
    try {
        HSimple& h = hsimp.get(handle);
        if (*value == MISSING_BYTE)
        {
            TRACEMISSING("byte");
            IF_TRACING(h.trace.log_unset(parameter));
            h.api->unset(parameter);
        }
        else
        {
            IF_TRACING(h.trace.log_set(parameter, *value));
            h.api->setb(parameter, *value);
        }
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}


/**
 * Set a real value in input
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in
 *   fapi_parms.md
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
int idba_setr(int handle, const char* parameter, const float* value)
{
    try {
        HSimple& h = hsimp.get(handle);
        if (*value == MISSING_REAL)
        {
            TRACEMISSING("real");
            IF_TRACING(h.trace.log_unset(parameter));
            h.api->unset(parameter);
        }
        else
        {
            IF_TRACING(h.trace.log_set(parameter, *value));
            h.api->setr(parameter, *value);
        }
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Set a `real*8` value in input
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in fapi_parms.md
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
int idba_setd(int handle, const char* parameter, const double* value)
{
    try {
        HSimple& h = hsimp.get(handle);
        if (*value == MISSING_DOUBLE)
        {
            TRACEMISSING("double");
            IF_TRACING(h.trace.log_unset(parameter));
            h.api->unset(parameter);
        }
        else
        {
            IF_TRACING(h.trace.log_set(parameter, *value));
            h.api->setd(parameter, *value);
        }
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Set a character value in input
 * 
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to set.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in fapi_parms.md
 * @param value
 *   The value to assign to the parameter
 * @return
 *   The error indicator for the function
 */
int idba_setc(int handle, const char* parameter, const char* value)
{
    try {
        HSimple& h = hsimp.get(handle);
        if (value[0] == 0)
        {
            TRACEMISSING("char");
            IF_TRACING(h.trace.log_unset(parameter));
            h.api->unset(parameter);
        }
        else
        {
            IF_TRACING(h.trace.log_set(parameter, value));
            h.api->setc(parameter, value);
        }

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Read an integer value from the output
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in fapi_parms.md
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
int idba_enqi(int handle, const char* parameter, int* value)
{
    try {
        HSimple& h = hsimp.get(handle);
        *value = h.api->enqi(parameter);
        tofortran(*value);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Read a byte value from the output
 * 
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in fapi_parms.md
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
int idba_enqb(int handle, const char* parameter, unsigned char* value)
{
    try {
        HSimple& h = hsimp.get(handle);
        *value = h.api->enqb(parameter);
        if (*value == fortran::API::missing_byte)
            *value = MISSING_BYTE;
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Read a real value from the output
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in fapi_parms.md
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
int idba_enqr(int handle, const char* parameter, float* value)
{
    try {
        HSimple& h = hsimp.get(handle);
        *value = h.api->enqr(parameter);
        if (*value == fortran::API::missing_float)
            *value = MISSING_REAL;
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Read a `real*8` value from the output
 * 
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in fapi_parms.md
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
int idba_enqd(int handle, const char* parameter, double* value)
{
    try {
        HSimple& h = hsimp.get(handle);
        *value = h.api->enqd(parameter);
        if (*value == fortran::API::missing_double)
            *value = MISSING_DOUBLE;
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Read a character value from the output
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to query.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of an attribute prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in
 *   fapi_parms.md
 * @param value
 *   Where the value will be returned
 * @return
 *   The error indicator for the function
 */
int idba_enqc(int handle, const char* parameter, char* value, unsigned value_len)
{
    try {
        HSimple& h = hsimp.get(handle);
        const char* v = h.api->enqc(parameter);

        // Copy the result values
        fortran::cstring_to_fortran(v, value, value_len);

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Remove one value from the input
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param parameter
 *   Parameter to remove.  It can be the code of a WMO variable prefixed by \c
 *   "B" (such as \c "B01023"); the code of a QC value prefixed by \c "*B"
 *   (such as \c "*B01023") or a keyword among the ones defined in fapi_parms.md
 * @return
 *   The error indicator for the function
 */
int idba_unset(int handle, const char* parameter)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_unset(parameter));
        h.api->unset(parameter);

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Remove all Bxxyyy values from the input
 * 
 * @param handle
 *   Handle to a DB-All.e session
 */
int idba_unsetb(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_func("unsetb"));
        h.api->unsetb();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Completely clear the input, removing all values.
 *
 * @param handle
 *   Handle to a DB-All.e session
 */
int idba_unsetall(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_func("unsetall"));
        h.api->unsetall();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Signal that the input values that are set are related to station values
 * instead of normal variables.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @return
 *   The error indicator for the function
 */
int idba_setcontextana(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_func("setcontextana"));
        h.api->setcontextana();

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}


/// @}


/**@name Input/output shortcuts
 *
 * The following routines are shortcuts for common combinations of Input/Output routines.
 * @{
 */

/**
 * Set all level information.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param ltype1
 *   Level type to set in the input record
 * @param l1
 *   L1 to set in the input record
 * @param ltype2
 *   Level type to set in the input record
 * @param l2
 *   L2 to set in the input record
 * @return
 *   The error indicator for the function
 */
int idba_setlevel(int handle, int ltype1, int l1, int ltype2, int l2)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_setlevel(
                fromfortran(ltype1), fromfortran(l1),
                fromfortran(ltype2), fromfortran(l2)));
        h.api->setlevel(
            fromfortran(ltype1), fromfortran(l1),
            fromfortran(ltype2), fromfortran(l2));

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Set all time range information.
 * 
 * @param handle
 *   Handle to a DB-All.e session
 * @param ptype
 *   P indicator to set in the input record
 * @param p1
 *   P1 to set in the input record
 * @param p2
 *   P2 to set in the input record
 * @return
 *   The error indicator for the function
 */
int idba_settimerange(int handle, int ptype, int p1, int p2)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_settimerange(fromfortran(ptype), fromfortran(p1), fromfortran(p2)));
        h.api->settimerange(fromfortran(ptype), fromfortran(p1), fromfortran(p2));

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Set all date information.
 *
 * @param handle
 *   Handle to a DB-All.e session
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
int idba_setdate(int handle,
        int year, int month, int day,
        int hour, int min, int sec)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_setdate(
                fromfortran(year), fromfortran(month), fromfortran(day),
                fromfortran(hour), fromfortran(min), fromfortran(sec)));

        h.api->setdate(
            fromfortran(year), fromfortran(month), fromfortran(day),
            fromfortran(hour), fromfortran(min), fromfortran(sec));

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Set the minimum date for a query.
 * 
 * @param handle
 *   Handle to a DB-All.e session
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
int idba_setdatemin(int handle,
        int year, int month, int day,
        int hour, int min, int sec)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_setdate(
                fromfortran(year), fromfortran(month), fromfortran(day),
                fromfortran(hour), fromfortran(min), fromfortran(sec), "min"));
        h.api->setdatemin(
            fromfortran(year), fromfortran(month), fromfortran(day),
            fromfortran(hour), fromfortran(min), fromfortran(sec));

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Set the maximum date for a query.
 *
 * @param handle
 *   Handle to a DB-All.e session
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
int idba_setdatemax(int handle,
        int year, int month, int day,
        int hour, int min, int sec)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_setdate(
                fromfortran(year), fromfortran(month), fromfortran(day),
                fromfortran(hour), fromfortran(min), fromfortran(sec), "max"));
        h.api->setdatemax(
            fromfortran(year), fromfortran(month), fromfortran(day),
            fromfortran(hour), fromfortran(min), fromfortran(sec));

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Read all level information.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @retval ltype1
 *   Type of the first level from the output record
 * @retval l1
 *   L1 from the output record
 * @retval ltype2
 *   Type of the second level from the output record
 * @retval l2
 *   L2 from the output record
 * @return
 *   The error indicator for the function
 */
int idba_enqlevel(int handle, int* ltype1, int* l1, int* ltype2, int* l2)
{
    try {
        HSimple& h = hsimp.get(handle);
        h.api->enqlevel(*ltype1, *l1, *ltype2, *l2);
        tofortran(*ltype1); tofortran(*l1);
        tofortran(*ltype2); tofortran(*l2);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Read all time range information.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @retval ptype
 *   P indicator from the output record
 * @retval p1
 *   P1 from the output record
 * @retval p2
 *   P2 from the output record
 * @return
 *   The error indicator for the function
 */
int idba_enqtimerange(int handle, int* ptype, int* p1, int* p2)
{
    try {
        HSimple& h = hsimp.get(handle);
        h.api->enqtimerange(*ptype, *p1, *p2);
        tofortran(*ptype); tofortran(*p1); tofortran(*p2);

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Read all date information.
 *
 * @param handle
 *   Handle to a DB-All.e session
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
int idba_enqdate(int handle,
        int* year, int* month, int* day,
        int* hour, int* min, int* sec)
{
    try {
        HSimple& h = hsimp.get(handle);
        h.api->enqdate(*year, *month, *day, *hour, *min, *sec);
        tofortran(*year), tofortran(*month), tofortran(*day);
        tofortran(*hour), tofortran(*min), tofortran(*sec);

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}


/*@}*/


/**@name Action routines
 * @{
 */

/**
 * Reinitialize the database, removing all data and loading report information.
 *
 * It requires the database to be opened in rewrite mode.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param repinfofile
 *   CSV file with the default report informations.  See dba_reset()
 *   documentation for the format of the file.
 * @return
 *   The error indicator for the function
 */
int idba_scopa(int handle, const char* repinfofile)
{
    try {
        HSimple& h = hsimp.get(handle);
        if (repinfofile[0] == 0)
        {
            IF_TRACING(h.trace.log_scopa());
            h.api->scopa(nullptr);
        } else {
            IF_TRACING(h.trace.log_scopa(repinfofile));
            h.api->scopa(repinfofile);
        }

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Query the stations in the database.
 *
 * Results are retrieved using idba_elencamele().
 *
 * There is no guarantee on the ordering of results of quantesono/elencamele.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param count
 *   The count of elements
 * @return
 *   The error indicator for the function
 */
int idba_quantesono(int handle, int* count)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_quantesono());
        *count = h.api->quantesono();
        IF_TRACING(fortran::log_result(*count));
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Retrieve the data about one station.
 *
 * After invocation, the output record is filled with information about the
 * station and its station values.
 *
 * If there are no more stations to read, the function will fail with
 * DBA_ERR_NOTFOUND.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @return
 *   The error indicator for the function
 */
int idba_elencamele(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_func("elencamele"));
        h.api->elencamele();

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Query the data in the database.
 *
 * Results are retrieved using idba_dammelo().
 *
 * Results are sorted by (in order): latitude, longitude, ident, datetime,
 * level, time range, rep_memo, varcode. The latitude changes slowest, and the
 * varcode changes fastest.
 *
 * Note that in V6 databases the ana_id corresponds to (latitude, longitude,
 * ident), and therefore changes slowest, and in MEM and V7 databases the
 * ana_id corresponds to (latitude, longitude, ident, rep_memo), and therefore
 * changes almost the fastest.
 *
 * Sort order can change in the future, with the invariant that the slowest to
 * change remains (coordinates, ident) and the fastest to change remains the
 * varcode.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @retval count
 *   Number of values returned by the function
 * @return
 *   The error indicator for the function
 */
int idba_voglioquesto(int handle, int* count)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_voglioquesto());
        *count = h.api->voglioquesto();
        IF_TRACING(fortran::log_result(*count));
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Retrieve the data about one value.
 *
 * After invocation, the output record is filled with information about the
 * value.
 *
 * If there are no more values to read, the function will fail with
 * DBA_ERR_NOTFOUND.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @retval parameter
 *   Contains the ID of the parameter retrieved by this fetch
 * @return
 *   The error indicator for the function
 */
int idba_dammelo(int handle, char* parameter, int parameter_len)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_dammelo());
        const char* res = h.api->dammelo();
        IF_TRACING(fortran::log_result(res));
        fortran::cstring_to_fortran(res, parameter, parameter_len);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Insert a new value in the database.
 *
 * This function will fail if the database is open in data readonly mode, and
 * it will refuse to overwrite existing values if the database is open in data
 * add mode.
 *
 * If the database is open in station reuse mode, the station values provided
 * on input will be used to create a station record if it is missing, but will
 * be ignored if it is already present.  If it is open in station rewrite mode
 * instead, the station values on input will be used to replace all the
 * existing station values.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @return
 *   The error indicator for the function
 */
int idba_prendilo(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_func("prendilo"));
        h.api->prendilo();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Remove from the database all values that match the query.
 *
 * This function will fail unless the database is open in data rewrite mode.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @return
 *   The error indicator for the function
 */
int idba_dimenticami(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_func("dimenticami"));
        h.api->dimenticami();

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Remove all values from the database.
 *
 * The difference with idba_scopa() is that it preserves the existing report
 * information.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @return
 *   The error indicator for the function
 */
int idba_remove_all(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_func("remove_all"));
        h.api->remove_all();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}


/**
 * Query attributes about a variable.
 *
 * The variable queried is either:
 *
 * @li the last variable returned by `idba_dammelo()`
 * @li the last variable inserted by `idba_prendilo()`
 * @li the variable selected by settings `*context_id` and `*var_related`.
 *
 * Results are retrieved using idba_ancora().
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @retval count
 *   Number of values returned by the function
 * @return
 *   The error indicator for the function
 */
int idba_voglioancora(int handle, int* count)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_voglioancora());
        *count = h.api->voglioancora();
        IF_TRACING(fortran::log_result(*count));
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Retrieve one attribute from the result of idba_voglioancora().
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @retval parameter
 *   Contains the ID of the parameter retrieved by this fetch
 * @return
 *   The error indicator for the function
 */
int idba_ancora(int handle, char* parameter, unsigned parameter_len)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_ancora());
        const char* res = h.api->ancora();
        IF_TRACING(fortran::log_result(res));
        fortran::cstring_to_fortran(res, parameter, parameter_len);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Insert new attributes for a variable.
 *
 * The variable is either:
 *
 * @li the last variable returned by `idba_dammelo()`
 * @li the last variable inserted by `idba_prendilo()`
 * @li the variable selected by settings `*context_id` and `*var_related`.
 *
 * The attributes that will be inserted are all those set by the functions
 * idba_seti(), idba_setc(), idba_setr(), idba_setd(), using an asterisk in
 * front of the variable name.
 *
 * Contrarily to idba_prendilo(), this function resets all the attribute
 * information (and only attribute information) previously set in input, so the
 * values to be inserted need to be explicitly set every time.
 *
 * This function will fail if the database is open in attribute readonly mode,
 * and it will refuse to overwrite existing values if the database is open in
 * attribute add mode.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @return
 *   The error indicator for the function
 */
int idba_critica(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_func("critica"));
        h.api->critica();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Remove attribute information for a variable.
 *
 * The variable is either:
 *
 * @li the last variable returned by `idba_dammelo()`
 * @li the last variable inserted by `idba_prendilo()`
 * @li the variable selected by settings `*context_id` and `*var_related`.
 *
 * The attribute informations to be removed are selected with:
 * \code
 *   idba_setc(handle, "*varlist", "*B33021,*B33003");
 * \endcode
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @return
 *   The error indicator for the function
 */
int idba_scusa(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_func("scusa"));
        h.api->scusa();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/// @}


/**@name Message routines
 * @{
 */

/**
 * Open a BUFR, CREX, or AOF file for reading.
 *
 * Each session can only have one open input file: if one was previously open,
 * it is closed before opening the new one.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param filename
 *   The file name
 * @param mode
 *   The opening mode. See the mode parameter of libc's fopen() call for details.
 * @param format
 *   The file format ("BUFR", "CREX", or "AOF")
 * @param simplified
 *   true if the file is imported in simplified mode, false if it is imported
 *   in precise mode. This controls approximating levels and time ranges to
 *   standard values.
 * @return
 *   The error indication for the function.
 */
int idba_messages_open_input(
        int handle,
        const char* filename,
        const char* mode,
        const char* format,
        int simplified)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_messages_open_input(filename, mode, format, simplified));
        h.api->messages_open_input(filename, mode, File::parse_encoding(format), simplified);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Open a BUFR, CREX, or AOF file for writing.
 *
 * Each session can only have one open input file: if one was previously open,
 * it is closed before opening the new one.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param filename
 *   The file name
 * @param mode
 *   The opening mode. See the mode parameter of libc's fopen() call for details.
 * @param format
 *   The file format ("BUFR", "CREX", or "AOF")
 * @return
 *   The error indication for the function.
 */
int idba_messages_open_output(
        int handle,
        const char* filename,
        const char* mode,
        const char* format)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_messages_open_output(filename, mode, format));
        h.api->messages_open_output(filename, mode, File::parse_encoding(format));
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Read the next message and import it in the database.
 *
 * The access mode of the session controls how data is imported:
 *
 * @li station and data mode cannot be "read".
 * @li if data mode is "add", existing data will not be overwritten.
 * @li if attribute mode is "read", attributes will not be imported.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @retval found
 *   True if a message has been imported, false if we are at the end of the
 *   input file.
 * @return
 *   The error indication for the function.
 */
int idba_messages_read_next(int handle, int *found)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_messages_read_next());
        *found = h.api->messages_read_next();
        IF_TRACING(fortran::log_result(*found));
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Export the data from the database that match the current query and add them
 * to the current message.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param template_name
 *   The template name used to decide the layout of variables in the messages
 *   that are exported.
 * @return
 *   The error indication for the function.
 */
int idba_messages_write_next(int handle, const char* template_name)
{
    try {
        HSimple& h = hsimp.get(handle);
        IF_TRACING(h.trace.log_messages_write_next(template_name));
        h.api->messages_write_next(template_name);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}


/// @}


/**@name Pretty-printing routines
 * @{
 */

/**
 * Format the description of a level given its value.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param ltype1
 *   Level type to set in the input record
 * @param l1
 *   L1 to set in the input record
 * @param ltype2
 *   Level type to set in the input record
 * @param l2
 *   L2 to set in the input record
 * @retval result
 *   The string with the description of the level.
 * @return
 *   The error indication for the function.
 */
int idba_spiegal(
        int handle,
        int ltype1, int l1, int ltype2, int l2,
        char* result, unsigned result_len)
{
    try {
        HSimple& h = hsimp.get(handle);
        const char* res = h.api->spiegal(ltype1, l1, ltype2, l2);
        fortran::cstring_to_fortran(res, result, result_len);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Format the description of a time range given its value.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param ptype
 *   P indicator to set in the input record
 * @param p1
 *   P1 to set in the input record
 * @param p2
 *   P2 to set in the input record
 * @retval result
 *   The string with the description of the time range.
 * @return
 *   The error indication for the function.
 */
int idba_spiegat(
        int handle,
        int ptype, int p1, int p2,
        char* result, unsigned result_len)
{
    try {
        HSimple& h = hsimp.get(handle);
        const char* res = h.api->spiegat(ptype, p1, p2);
        fortran::cstring_to_fortran(res, result, result_len);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Format the description of a variable given its varcode and its value.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param varcode
 *   B table code of the variable (`"Bxxyyy"`)
 * @param value
 *   Value of the variable, as read with idba_enqc()
 * @retval result
 *   The string with the description of the time range.
 * @return
 *   The error indication for the function.
 */
int idba_spiegab(
        int handle,
        const char* varcode,
        const char* value,
        char* result, unsigned result_len)
{
    try {
        HSimple& h = hsimp.get(handle);
        const char* res = h.api->spiegab(varcode, value);
        fortran::cstring_to_fortran(res, result, result_len);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/*@}*/

int idba_test_input_to_output(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        h.api->test_input_to_output();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}


}
