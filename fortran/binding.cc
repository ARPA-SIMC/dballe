#include "config.h"
#include "dballe/simple/msgapi.h"
#include "dballe/simple/dbapi.h"
#include "dballe/simple/traced.h"
#include "dballe/core/var.h"
#include "dballe/core/string.h"
#include "dballe/db/db.h"

#include <cstring>  // memset
#include <limits.h>
#include <float.h>
#include "common.h"
#include "handles.h"
#include "error.h"

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
    std::string url;
};

struct fortran::Handler<HSession, MAX_SESSION> hsess;

struct HSimple : public fortran::HBase
{
    fortran::API* api;

    void start()
    {
        fortran::HBase::start();
        api = 0;
    }
    void stop()
    {
        delete api;
        api = 0;
        fortran::HBase::stop();
    }
};

struct fortran::Handler<HSimple, MAX_SIMPLE> hsimp;

static int usage_refcount = 0;

static dballe::fortran::Tracer* tracer = nullptr;

static void lib_init()
{
    if (usage_refcount > 0)
        return;

    tracer = dballe::fortran::Tracer::create().release();
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
 * This function can be called more than once to connect to different
 * databases at the same time.
 *
 * The function expects to find a properly initialised DB-All.e database.
 * Append `&wipe=true` to the end of the url to wipe any existing DB-All.e
 * information from the database if it exists, then recreate it from scratch.
 *
 * @param url
 *   The URL of the database to use
 * @param user
 *   Used in the past, now it is ignored.
 * @param password
 *   Used in the past, now it is ignored.
 * @retval dbahandle
 *   The database handle that can be passed to idba_begin to work with the
 *   database.
 * @return
 *   The error indication for the function.
 */
int idba_connect(int* dbahandle, const char* url)
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

        tracer->log_connect_url(*dbahandle, url);

        hs.url = url;

        std::string wipe = url_pop_query_string(hs.url, "wipe");
        if (!wipe.empty())
        {
            // Disappear then reset, to allow migrating a db from V6 to V7
            auto db = dynamic_pointer_cast<db::DB>(DB::connect_from_url(hs.url.c_str()));
            db->disappear();
            db = dynamic_pointer_cast<db::DB>(DB::connect_from_url(hs.url.c_str()));
            db->reset();
        }

        /* Open the database session */
        return fortran::success();
    } catch (error& e) {
        hsess.release(*dbahandle);
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_connect()
int idba_presentati(int* dbahandle, const char* url)
{
    return idba_connect(dbahandle, url);
}

/**
 * Disconnect from the database.
 *
 * @param dbahandle
 *   The database handle to close.
 */
int idba_disconnect(int *dbahandle)
{
    tracer->log_disconnect(*dbahandle);

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

/// Deprecated compatibility version of idba_disconnect()
int idba_arrivederci(int *dbahandle)
{
    return idba_disconnect(dbahandle);
}


/**
 * Open a new session.
 *
 * You can call idba_begin() many times and get more handles.  This allows
 * to perform many operations on the database at the same time.
 *
 * idba_begin() has three extra parameters that can be used to limit
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
int idba_begin(int dbahandle, int* handle, const char* anaflag, const char* dataflag, const char* attrflag)
{
    try {
        /* Allocate and initialize a new handle */
        *handle = hsimp.request();
        HSession& hs = hsess.get(dbahandle);
        HSimple& h = hsimp.get(*handle);

        std::unique_ptr<dballe::fortran::API> api = tracer->begin(dbahandle, *handle, hs.url.c_str(), anaflag, dataflag, attrflag);
        h.api = api.release();

        return fortran::success();
    } catch (error& e) {
        hsimp.release(*handle);
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_begin()
int idba_preparati(int dbahandle, int* handle, const char* anaflag, const char* dataflag, const char* attrflag)
{
    return idba_begin(dbahandle, handle, anaflag, dataflag, attrflag);
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
 *   Format of the data in the file.  It can be: `"BUFR"`, `"CREX"`, `"AUTO"` (autodetect, read only)
 * @return
 *   The error indication for the function.
 */
int idba_begin_messages(int* handle, const char* filename, const char* mode, const char* type)
{
    try {
        lib_init();

        *handle = hsimp.request();
        //HSession& hs = hsess.get(*dbahandle);
        HSimple& h = hsimp.get(*handle);

        std::unique_ptr<dballe::fortran::API> api = tracer->begin_messages(*handle, filename, mode, type);
        h.api = api.release();

        return fortran::success();
    } catch (error& e) {
        hsimp.release(*handle);
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_begin_messages()
int idba_messaggi(int* handle, const char* filename, const char* mode, const char* type)
{
    return idba_begin_messages(handle, filename, mode, type);
}


/**
 * Close a session.
 *
 * @param handle
 *   Handle to the session to be closed.
 */
int idba_commit(int* handle)
{
    try {
        HSimple& h = hsimp.get(*handle);
        h.api->commit();
        hsimp.release(*handle);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_commit()
int idba_fatto(int* handle)
{
    return idba_commit(handle);
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
            h.api->unset(parameter);
        else
            h.api->seti(parameter, *value);
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
            h.api->unset(parameter);
        else
            h.api->setb(parameter, *value);
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
            h.api->unset(parameter);
        else
            h.api->setr(parameter, *value);
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
            h.api->unset(parameter);
        else
            h.api->setd(parameter, *value);
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
            h.api->unset(parameter);
        else
            h.api->setc(parameter, value);
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
        std::string v;
        bool found = h.api->enqc(parameter, v);

        // Copy the result values
        if (found)
            fortran::cstring_to_fortran(v, value, value_len);
        else
            fortran::cstring_to_fortran(nullptr, value, value_len);

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
int idba_reinit_db(int handle, const char* repinfofile)
{
    try {
        HSimple& h = hsimp.get(handle);
        if (repinfofile[0] == 0)
            h.api->reinit_db(nullptr);
        else
            h.api->reinit_db(repinfofile);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_reinit_db()
int idba_scopa(int handle, const char* repinfofile)
{
    return idba_reinit_db(handle, repinfofile);
}

/**
 * Query the stations in the database.
 *
 * Results are retrieved using idba_next_station().
 *
 * There is no guarantee on the ordering of results of query_stations/next_station.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @param count
 *   The count of elements
 * @return
 *   The error indicator for the function
 */
int idba_query_stations(int handle, int* count)
{
    try {
        HSimple& h = hsimp.get(handle);
        *count = h.api->query_stations();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_query_stations()
int idba_quantesono(int handle, int* count)
{
    return idba_query_stations(handle, count);
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
int idba_next_station(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        h.api->next_station();

        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_next_station()
int idba_elencamele(int handle)
{
    return idba_next_station(handle);
}

/**
 * Query the data in the database.
 *
 * Results are retrieved using idba_next_data().
 *
 * Results are sorted by (in order): ana_id, datetime, level, time range,
 * varcode. The ana_id changes slowest, and the varcode changes fastest.
 *
 * Ordering by ana_id effectively does grouping by station rather than
 * ordering.
 *
 * Sort order can change in the future, with the invariant that the slowest to
 * change remains ana_id, followed by datetime, and the fastest to change
 * remains the varcode.
 *
 * @param handle
 *   Handle to a DB-All.e session
 * @retval count
 *   Number of values returned by the function
 * @return
 *   The error indicator for the function
 */
int idba_query_data(int handle, int* count)
{
    try {
        HSimple& h = hsimp.get(handle);
        *count = h.api->query_data();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_query_data()
int idba_voglioquesto(int handle, int* count)
{
    return idba_query_data(handle, count);
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
 *   Contains the variable code of the parameter retrieved by this fetch
 * @return
 *   The error indicator for the function
 */
int idba_next_data(int handle, char* parameter, int parameter_len)
{
    try {
        HSimple& h = hsimp.get(handle);
        wreport::Varcode res = h.api->next_data();
        char buf[8];
        format_bcode(res, buf);
        fortran::cstring_to_fortran(buf, parameter, parameter_len);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_next_data()
int idba_dammelo(int handle, char* parameter, int parameter_len)
{
    return idba_next_data(handle, parameter, parameter_len);
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
int idba_insert_data(int handle)
{
    try {
        HSimple& h = hsimp.get(handle);
        h.api->insert_data();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/// Deprecated compatibility version of idba_insert_data()
int idba_prendilo(int handle)
{
    return idba_insert_data(handle);
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
        h.api->dimenticami();
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Remove all values from the database.
 *
 * The difference with idba_reinit_db() is that it preserves the existing report
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
 * @li the last variable returned by `idba_next_data()`
 * @li the last variable inserted by `idba_insert_data()`
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
        *count = h.api->voglioancora();
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
        const char* res = h.api->ancora();
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
 * @li the last variable returned by `idba_next_data()`
 * @li the last variable inserted by `idba_insert_data()`
 * @li the variable selected by settings `*context_id` and `*var_related`.
 *
 * The attributes that will be inserted are all those set by the functions
 * idba_seti(), idba_setc(), idba_setr(), idba_setd(), using an asterisk in
 * front of the variable name.
 *
 * Contrarily to idba_insert_data(), this function resets all the attribute
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
 * @li the last variable returned by `idba_next_data()`
 * @li the last variable inserted by `idba_insert_data()`
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
 * Open a BUFR, or CREX file for reading.
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
 *   The file format ("BUFR", or "CREX")
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
        h.api->messages_open_input(filename, mode, File::parse_encoding(format), simplified);
        return fortran::success();
    } catch (error& e) {
        return fortran::error(e);
    }
}

/**
 * Open a BUFR, or CREX file for writing.
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
 *   The file format ("BUFR", or "CREX")
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
        *found = h.api->messages_read_next();
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

}
