/*
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "db/test-utils-db.h"
#include "db/internals.h"
#include "db/v5/db.h"
#include <wibble/sys/fs.h>
#include <sql.h>
#include <sqlext.h>

using namespace dballe;
using namespace wreport;
using namespace wibble;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct db_sqlite_shar
{
    db_sqlite_shar()
    {
    }
};
TESTGRP(db_sqlite);

// Test creating a sqlite database
template<> template<>
void to::test<1>()
{
    sys::fs::deleteIfExists("test.sqlite");

    // Allocate ODBC environment handle and register version 
    SQLHENV od_env;
    wassert(actual(SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &od_env)) == SQL_SUCCESS);
    wassert(actual(SQLSetEnvAttr(od_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0)) == SQL_SUCCESS);

    SQLHDBC od_conn;
    wassert(actual(SQLAllocHandle(SQL_HANDLE_DBC, od_env, &od_conn)) == SQL_SUCCESS);

    // Connect to the DSN
    char sdcout[1024];
    SQLSMALLINT outlen;
    wassert(actual(SQLDriverConnect(od_conn, NULL,
                    (SQLCHAR*)"Driver=SQLite3;Database=test.sqlite;", SQL_NTS,
                    (SQLCHAR*)sdcout, 1024, &outlen,
                    SQL_DRIVER_NOPROMPT)) == SQL_SUCCESS);

    // Create a statement
    SQLHSTMT stm;
    wassert(actual(SQLAllocHandle(SQL_HANDLE_STMT, od_conn, &stm)) == SQL_SUCCESS);


    // Prepare a query
    wassert(actual(SQLPrepare(stm, (SQLCHAR*)"SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name=?", SQL_NTS)) == SQL_SUCCESS);


    // All good, deallocate things
    SQLFreeHandle(SQL_HANDLE_STMT, stm);
    SQLFreeHandle(SQL_HANDLE_DBC, od_conn);
    SQLFreeHandle(SQL_HANDLE_ENV, od_env);
}

}
