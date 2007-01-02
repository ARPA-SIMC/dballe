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

#include "db.h"
#include "internals.h"
#include "repinfo.h"
#include "pseudoana.h"
#include "context.h"
#include "data.h"
#include "attr.h"
#include <dballe/core/csv.h>
#include <dballe/core/verbose.h>
#include <dballe/core/aliases.h>

#include <config.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>

#define DBA_ODBC_MISSING_TABLE_POSTGRES "42P01"
#define DBA_ODBC_MISSING_TABLE_MYSQL "42S01"
#define DBA_ODBC_MISSING_TABLE_SQLITE "HY000"

/*
 * Define to true to enable the use of transactions during writes
 */
#define DBA_USE_TRANSACTIONS

/* Define this to enable referential integrity */
#undef USE_REF_INT

static SQLHENV dba_od_env;
static int dba_od_env_initialized = 0;

static const char* init_tables[] = {
	"attr", "data", "context", "pseudoana", "repinfo"
};

#ifdef DBA_USE_TRANSACTIONS
#define TABLETYPE "TYPE=InnoDB;"
#else
#define TABLETYPE ";"
#endif
static const char* init_queries_mysql[] = {
	"CREATE TABLE repinfo ("
	"   id		     SMALLINT PRIMARY KEY,"
	"	memo	 	 VARCHAR(30) NOT NULL,"
	"	description	 VARCHAR(255) NOT NULL,"
	"   prio	     INTEGER NOT NULL,"
	"	descriptor	 CHAR(6) NOT NULL,"
	"	tablea		 INTEGER NOT NULL"
	") " TABLETYPE,
	"CREATE TABLE pseudoana ("
	"   id         INTEGER auto_increment PRIMARY KEY,"
	"   lat        INTEGER NOT NULL,"
	"   lon        INTEGER NOT NULL,"
	"   ident      CHAR(64),"
	"   UNIQUE INDEX(lat, lon, ident(8)),"
	"   INDEX(lon)"
	") " TABLETYPE,
	"CREATE TABLE context ("
	"   id			INTEGER auto_increment PRIMARY KEY,"
	"   id_ana		INTEGER NOT NULL,"
	"	id_report	SMALLINT NOT NULL,"
	"   datetime	DATETIME NOT NULL,"
	"	ltype		SMALLINT NOT NULL,"
	"	l1			INTEGER NOT NULL,"
	"	l2			INTEGER NOT NULL,"
	"	ptype		SMALLINT NOT NULL,"
	"	p1			INTEGER NOT NULL,"
	"	p2			INTEGER NOT NULL,"
	"   UNIQUE INDEX (id_ana, datetime, ltype, l1, l2, ptype, p1, p2, id_report),"
	"   INDEX (id_ana),"
	"   INDEX (id_report),"
	"   INDEX (datetime),"
	"   INDEX (ltype, l1, l2),"
	"   INDEX (ptype, p1, p2)"
#ifdef USE_REF_INT
	"   , FOREIGN KEY (id_ana) REFERENCES pseudoana (id) ON DELETE CASCADE,"
	"   FOREIGN KEY (id_report) REFERENCES repinfo (id) ON DELETE CASCADE"
#endif
	") " TABLETYPE,
	"CREATE TABLE data ("
	"   id_context	INTEGER NOT NULL,"
	"	id_var		SMALLINT NOT NULL,"
	"	value		VARCHAR(255) NOT NULL,"
	"	INDEX (id_context),"
	"   UNIQUE INDEX(id_var, id_context)"
#ifdef USE_REF_INT
	"   , FOREIGN KEY (id_context) REFERENCES context (id) ON DELETE CASCADE"
#endif
	") " TABLETYPE,
	"CREATE TABLE attr ("
	"   id_context	INTEGER NOT NULL,"
	"	id_var		SMALLINT NOT NULL,"
	"   type		SMALLINT NOT NULL,"
	"   value		VARCHAR(255) NOT NULL,"
	"   INDEX (id_context, id_var),"
	"   UNIQUE INDEX (id_context, id_var, type)"
#ifdef USE_REF_INT
	"   , FOREIGN KEY (id_context, id_var) REFERENCES data (id_context, id_var) ON DELETE CASCADE"
#endif
	") " TABLETYPE,
};

static const char* init_queries_postgres[] = {
	"CREATE TABLE repinfo ("
	"   id		     INTEGER PRIMARY KEY,"
	"	memo	 	 VARCHAR(30) NOT NULL,"
	"	description	 VARCHAR(255) NOT NULL,"
	"   prio	     INTEGER NOT NULL,"
	"	descriptor	 CHAR(6) NOT NULL,"
	"	tablea		 INTEGER NOT NULL"
	") ",
	"CREATE TABLE pseudoana ("
	"   id         SERIAL PRIMARY KEY,"
	"   lat        INTEGER NOT NULL,"
	"   lon        INTEGER NOT NULL,"
	"   ident      CHAR(64)"
	") ",
	"CREATE UNIQUE INDEX pa_uniq ON pseudoana(lat, lon, ident)",
	"CREATE INDEX pa_lon ON pseudoana(lon)",
	"CREATE TABLE context ("
	"   id			SERIAL PRIMARY KEY,"
	"   id_ana		INTEGER NOT NULL,"
	"	id_report	INTEGER NOT NULL,"
	"   datetime	TIMESTAMP NOT NULL,"
	"	ltype		INTEGER NOT NULL,"
	"	l1			INTEGER NOT NULL,"
	"	l2			INTEGER NOT NULL,"
	"	ptype		INTEGER NOT NULL,"
	"	p1			INTEGER NOT NULL,"
	"	p2			INTEGER NOT NULL"
#ifdef USE_REF_INT
	"   , FOREIGN KEY (id_ana) REFERENCES pseudoana (id) ON DELETE CASCADE,"
	"   FOREIGN KEY (id_report) REFERENCES repinfo (id) ON DELETE CASCADE"
#endif
	") ",
	"CREATE UNIQUE INDEX co_uniq ON context(id_ana, datetime, ltype, l1, l2, ptype, p1, p2, id_report)",
	"CREATE INDEX co_ana ON context(id_ana)",
	"CREATE INDEX co_report ON context(id_report)",
	"CREATE INDEX co_dt ON context(datetime)",
	"CREATE INDEX co_lt ON context(ltype, l1, l2)",
	"CREATE INDEX co_pt ON context(ptype, p1, p2)",
	"CREATE TABLE data ("
	"   id_context	INTEGER NOT NULL,"
	"	id_var		INTEGER NOT NULL,"
	"	value		VARCHAR(255) NOT NULL"
#ifdef USE_REF_INT
	"   , FOREIGN KEY (id_context) REFERENCES context (id) ON DELETE CASCADE"
#endif
	") ",
	"CREATE INDEX da_co ON data(id_context)",
	"CREATE UNIQUE INDEX da_uniq ON data(id_var, id_context)",
	"CREATE TABLE attr ("
	"   id_context	INTEGER NOT NULL,"
	"	id_var		INTEGER NOT NULL,"
	"   type		INTEGER NOT NULL,"
	"   value		VARCHAR(255) NOT NULL"
#ifdef USE_REF_INT
	"   , FOREIGN KEY (id_context, id_var) REFERENCES data (id_context, id_var) ON DELETE CASCADE"
#endif
	") ",
	"CREATE INDEX at_da ON attr(id_context, id_var)",
	"CREATE UNIQUE INDEX at_uniq ON attr(id_context, id_var, type)",
};

static const char* init_queries_sqlite[] = {
	"CREATE TABLE repinfo ("
	"   id		     INTEGER PRIMARY KEY,"
	"	memo	 	 VARCHAR(30) NOT NULL,"
	"	description	 VARCHAR(255) NOT NULL,"
	"   prio	     INTEGER NOT NULL,"
	"	descriptor	 CHAR(6) NOT NULL,"
	"	tablea		 INTEGER NOT NULL"
	") ",
	"CREATE TABLE pseudoana ("
	"   id         INTEGER PRIMARY KEY,"
	"   lat        INTEGER NOT NULL,"
	"   lon        INTEGER NOT NULL,"
	"   ident      CHAR(64),"
	"   UNIQUE (lat, lon, ident)"
	") ",
	"CREATE INDEX pa_lon ON pseudoana(lon)",
	"CREATE TABLE context ("
	"   id			INTEGER PRIMARY KEY,"
	"   id_ana		INTEGER NOT NULL,"
	"	id_report	INTEGER NOT NULL,"
	"   datetime	TEXT NOT NULL,"
	"	ltype		INTEGER NOT NULL,"
	"	l1			INTEGER NOT NULL,"
	"	l2			INTEGER NOT NULL,"
	"	ptype		INTEGER NOT NULL,"
	"	p1			INTEGER NOT NULL,"
	"	p2			INTEGER NOT NULL,"
	"   UNIQUE (id_ana, datetime, ltype, l1, l2, ptype, p1, p2, id_report)"
#ifdef USE_REF_INT
	"   , FOREIGN KEY (id_ana) REFERENCES pseudoana (id) ON DELETE CASCADE,"
	"   FOREIGN KEY (id_report) REFERENCES repinfo (id) ON DELETE CASCADE"
#endif
	") ",
	"CREATE INDEX co_ana ON context(id_ana)",
	"CREATE INDEX co_report ON context(id_report)",
	"CREATE INDEX co_dt ON context(datetime)",
	"CREATE INDEX co_lt ON context(ltype, l1, l2)",
	"CREATE INDEX co_pt ON context(ptype, p1, p2)",
	"CREATE TABLE data ("
	"   id_context	INTEGER NOT NULL,"
	"	id_var		INTEGER NOT NULL,"
	"	value		VARCHAR(255) NOT NULL,"
	"   UNIQUE (id_var, id_context)"
#ifdef USE_REF_INT
	"   , FOREIGN KEY (id_context) REFERENCES context (id) ON DELETE CASCADE"
#endif
	") ",
	"CREATE INDEX da_co ON data(id_context)",
	"CREATE TABLE attr ("
	"   id_context	INTEGER NOT NULL,"
	"	id_var		INTEGER NOT NULL,"
	"   type		INTEGER NOT NULL,"
	"   value		VARCHAR(255) NOT NULL,"
	"   UNIQUE (id_context, id_var, type)"
#ifdef USE_REF_INT
	"   , FOREIGN KEY (id_context, id_var) REFERENCES data (id_context, id_var) ON DELETE CASCADE"
#endif
	") ",
	"CREATE INDEX at_da ON attr(id_context, id_var)",
};



/**
 * Get the report id from this record.
 *
 * If rep_memo is specified instead, the corresponding report id is queried in
 * the database and set as "rep_cod" in the record.
 */
static dba_err dba_db_get_rep_cod(dba_db db, dba_record rec, int* id)
{
	const char* rep;
	DBA_RUN_OR_RETURN(dba_db_need_repinfo(db));
	if ((rep = dba_record_key_peek_value(rec, DBA_KEY_REP_MEMO)) != NULL)
		DBA_RUN_OR_RETURN(dba_db_repinfo_get_id(db->repinfo, rep, id));
	else if ((rep = dba_record_key_peek_value(rec, DBA_KEY_REP_COD)) != NULL)
	{
		int exists;
		*id = strtol(rep, 0, 10);
		DBA_RUN_OR_RETURN(dba_db_repinfo_has_id(db->repinfo, *id, &exists));
		if (!exists)
			return dba_error_notfound("rep_cod %d does not exist in the database", *id);
	}
	else
		return dba_error_notfound("looking for report type in rep_cod or rep_memo");
	return dba_error_ok();
}		


dba_err dba_db_create(const char* dsn, const char* user, const char* password, dba_db* db)
{
	dba_err err = DBA_OK;
	char drivername[50];
	int sqlres;
	SQLSMALLINT len;

	if (dba_od_env_initialized == 0)
	{
		// Allocate ODBC environment handle and register version 
		int res = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &dba_od_env);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			return dba_db_error_odbc(SQL_HANDLE_ENV, dba_od_env, "Allocating main environment handle");

		res = SQLSetEnvAttr(dba_od_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			dba_err res = dba_db_error_odbc(SQL_HANDLE_ENV, dba_od_env, "Asking for ODBC version 3");
			SQLFreeHandle(SQL_HANDLE_ENV, dba_od_env);
			return res;
		}
		dba_od_env_initialized = 1;

		// One could close the main handle with this:
		// SQLFreeHandle(SQL_HANDLE_ENV, dba_od_env);
		// but there does not seem to be a big need of it.
	}


	/* Allocate a new handle */
	if ((*db = (dba_db)calloc(1, sizeof(struct _dba_db))) == NULL)
		return dba_error_alloc("trying to allocate a new dba_db object");

	/* Allocate the ODBC connection handle */
	sqlres = SQLAllocHandle(SQL_HANDLE_DBC, dba_od_env, &((*db)->od_conn));
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_DBC, (*db)->od_conn,
				"Allocating new connection handle");
		goto fail;
	}

	/* Set the connection timeout */
	/* SQLSetConnectAttr(pc.od_conn, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0); */

	/* Connect to the DSN */
	sqlres = SQLConnect((*db)->od_conn,
						(SQLCHAR*)dsn, SQL_NTS,
						(SQLCHAR*)user, SQL_NTS,
						(SQLCHAR*)(password == NULL ? "" : password), SQL_NTS);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_DBC, (*db)->od_conn,
				"Connecting to DSN %s as user %s", dsn, user);
		goto fail;
	}
	(*db)->connected = 1;

	sqlres = SQLGetInfo((*db)->od_conn, SQL_DRIVER_NAME, (SQLPOINTER)drivername, 50, &len);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_DBC, (*db)->od_conn, "Getting ODBC driver name");
		goto fail;
	}
	if ((len >= 9 && (strncmp(drivername, "libmyodbc", 9) == 0))
	 || (len >= 6 && (strncmp(drivername, "myodbc", 6) == 0)))
		(*db)->server_type = MYSQL;
	else if (len >= 6 && strncmp(drivername, "sqlite", 6) == 0)
		(*db)->server_type = SQLITE;
	else
	{
		fprintf(stderr, "ODBC driver %.*s is unsupported: assuming it's MySQL", len, drivername);
		(*db)->server_type = MYSQL;
	}

#ifdef DBA_USE_TRANSACTIONS
	DBA_RUN_OR_GOTO(fail, dba_db_statement_create(*db, &((*db)->stm_begin)));
	sqlres = SQLPrepare((*db)->stm_begin, (unsigned char*)"BEGIN", SQL_NTS);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, (*db)->stm_begin, "compiling query for beginning a transaction");
		goto fail;
	}

	DBA_RUN_OR_GOTO(fail, dba_db_statement_create(*db, &((*db)->stm_commit)));
	sqlres = SQLPrepare((*db)->stm_commit, (unsigned char*)"COMMIT", SQL_NTS);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, (*db)->stm_commit, "compiling query for committing a transaction");
		goto fail;
	}

	DBA_RUN_OR_GOTO(fail, dba_db_statement_create(*db, &((*db)->stm_rollback)));
	sqlres = SQLPrepare((*db)->stm_rollback, (unsigned char*)"ROLLBACK", SQL_NTS);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, (*db)->stm_rollback, "compiling query for rolling back a transaction");
		goto fail;
	}
#endif

	DBA_RUN_OR_GOTO(fail, dba_db_statement_create(*db, &((*db)->stm_last_insert_id)));
	SQLBindCol((*db)->stm_last_insert_id, 1, SQL_C_SLONG, &((*db)->last_insert_id), sizeof(int), 0);
	switch ((*db)->server_type)
	{
		case MYSQL:
			sqlres = SQLPrepare((*db)->stm_last_insert_id, (unsigned char*)"SELECT LAST_INSERT_ID()", SQL_NTS);
			break;
		case SQLITE:
			sqlres = SQLPrepare((*db)->stm_last_insert_id, (unsigned char*)"SELECT LAST_INSERT_ROWID()", SQL_NTS);
			break;
	}
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, (*db)->stm_last_insert_id, "compiling query for querying the last insert id");
		goto fail;
	}

	return dba_error_ok();
	
fail:
	dba_db_delete(*db);
	*db = 0;
	return err;
}

void dba_db_delete(dba_db db)
{
	assert(db);

	if (db->attr != NULL)
		dba_db_attr_delete(db->attr);
	if (db->data != NULL)
		dba_db_data_delete(db->data);
	if (db->context != NULL)
		dba_db_context_delete(db->context);
	if (db->pseudoana != NULL)
		dba_db_pseudoana_delete(db->pseudoana);
	if (db->repinfo != NULL)
		dba_db_repinfo_delete(db->repinfo);
	if (db->od_conn != NULL)
	{
		if (db->connected)
			SQLDisconnect(db->od_conn);
		SQLFreeHandle(SQL_HANDLE_DBC, db->od_conn);
	}
	free(db);
}

dba_err dba_db_delete_tables(dba_db db)
{
	dba_err err = DBA_OK;
	SQLHSTMT stm = NULL;
	int i, res;

	/* Allocate statement handle */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

	/* Drop existing tables */
	for (i = 0; i < sizeof(init_tables) / sizeof(init_tables[0]); i++)
	{
		char buf[100];
		int len;

		switch (db->server_type)
		{
			case MYSQL:
				len = snprintf(buf, 100, "DROP TABLE IF EXISTS %s", init_tables[i]);
				res = SQLExecDirect(stm, (unsigned char*)buf, len);
				if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
				{
					err = dba_db_error_odbc(SQL_HANDLE_STMT, stm,
							"Removing old table %s", init_tables[i]);
					if (err != DBA_OK)
						goto cleanup;
				}
				break;
			case SQLITE:
				len = snprintf(buf, 100, "DROP TABLE %s", init_tables[i]);
				res = SQLExecDirect(stm, (unsigned char*)buf, len);
				if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
				{
					err = dba_db_error_odbc_except(DBA_ODBC_MISSING_TABLE_SQLITE, SQL_HANDLE_STMT, stm,
							"Removing old table %s", init_tables[i]);
					if (err != DBA_OK)
						goto cleanup;
				}
				break;
			default:
				len = snprintf(buf, 100, "DROP TABLE %s", init_tables[i]);
				res = SQLExecDirect(stm, (unsigned char*)buf, len);
				if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
				{
					err = dba_db_error_odbc_except(DBA_ODBC_MISSING_TABLE_POSTGRES, SQL_HANDLE_STMT, stm,
							"Removing old table %s", init_tables[i]);
					if (err != DBA_OK)
						goto cleanup;
				}
				break;
		}
	}

cleanup:
	if (stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_reset(dba_db db, const char* deffile)
{
	int res;
	int i;
	SQLHSTMT stm;
	dba_err err;
	const char** queries = NULL;
	int query_count = 0;

	assert(db);

	if (deffile == 0)
	{
		deffile = getenv("DBA_REPINFO");
		if (deffile == 0 || deffile[0] == 0)
			deffile = TABLE_DIR "/repinfo.csv";
	}

	/* Open the input CSV file */
	FILE* in = fopen(deffile, "r");
	if (in == NULL)
		return dba_error_system("opening file %s", deffile);

	/* Drop existing tables */
	DBA_RUN_OR_GOTO(fail0, dba_db_delete_tables(db));

	/* Allocate statement handle */
	DBA_RUN_OR_GOTO(fail0, dba_db_statement_create(db, &stm));

	switch (db->server_type)
	{
		case MYSQL:
			queries = init_queries_mysql;
			query_count = sizeof(init_queries_mysql) / sizeof(init_queries_mysql[0]); break;
		case SQLITE:
			queries = init_queries_sqlite;
			query_count = sizeof(init_queries_sqlite) / sizeof(init_queries_sqlite[0]); break;
		default:
			queries = init_queries_mysql;
			query_count = sizeof(init_queries_mysql) / sizeof(init_queries_mysql[0]); break;
	}
	/* Create tables */
	for (i = 0; i < query_count; i++)
	{
		/* Casting out 'const' because ODBC API is not const-conscious */
		res = SQLExecDirect(stm, (unsigned char*)queries[i], SQL_NTS);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm,
					"Executing database-initialization query %s", queries[i]);
			goto fail1;
		}
	}

	/* Populate the tables with values */
	{
		unsigned long id;
		char memo[30];
		char description[255];
		long prio;
		char descriptor[6];
		unsigned long tablea;
		int i;
		char* columns[7];
		int line;

		res = SQLPrepare(stm, (unsigned char*)
				"INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)"
				"     VALUES (?, ?, ?, ?, ?, ?)", SQL_NTS);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "compiling query to insert into 'repinfo'");
			goto fail1;
		}

		SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &id, 0, 0);
		SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, memo, 0, 0);
		SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, description, 0, 0);
		SQLBindParameter(stm, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &prio, 0, 0);
		SQLBindParameter(stm, 5, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, descriptor, 0, 0);
		SQLBindParameter(stm, 6, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &tablea, 0, 0);

		for (line = 0; (i = dba_csv_read_next(in, columns, 7)) != 0; line++)
		{
			if (i != 6)
			{
				err = dba_error_parse(deffile, line, "Expected 6 columns, got %d", i);
				goto fail1;
			}
				
			id = strtol(columns[0], 0, 10);
			strncpy(memo, columns[1], 30); memo[29] = 0;
			strncpy(description, columns[2], 255); description[254] = 0;
			prio = strtol(columns[3], 0, 10);
			strncpy(descriptor, columns[4], 6); descriptor[5] = 0;
			tablea = strtol(columns[5], 0, 10);

			res = SQLExecute(stm);
			if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			{
				err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into 'repinfo'");
				goto fail1;
			}

			for (i = 0; i < 6; i++)
				free(columns[i]);
		}
	}

	SQLFreeHandle(SQL_HANDLE_STMT, stm);

	return dba_error_ok();

fail1:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
fail0:
	fclose(in);
	return err;
}

dba_err dba_db_update_repinfo(dba_db db, const char* repinfo_file, int* added, int* deleted, int* updated)
{
	DBA_RUN_OR_RETURN(dba_db_need_repinfo(db));
	return dba_db_repinfo_update(db->repinfo, repinfo_file, added, deleted, updated);
}

dba_err dba_db_rep_cod_from_memo(dba_db db, const char* memo, int* rep_cod)
{
	DBA_RUN_OR_RETURN(dba_db_need_repinfo(db));
	return dba_db_repinfo_get_id(db->repinfo, memo, rep_cod);
}

dba_err dba_db_check_rep_cod(dba_db db, int rep_cod, int* valid)
{
	DBA_RUN_OR_RETURN(dba_db_need_repinfo(db));
	return dba_db_repinfo_has_id(db->repinfo, rep_cod, valid);
}

#if 0
static dba_err update_pseudoana_extra_info(dba_db db, dba_record rec, int id_ana)
{
	dba_var var;
	
	/* Don't do anything if rec doesn't have any extra data */
	if (	dba_record_key_peek_value(rec, DBA_KEY_HEIGHT) == NULL
		&&	dba_record_key_peek_value(rec, DBA_KEY_HEIGHTBARO) == NULL
		&&	dba_record_key_peek_value(rec, DBA_KEY_NAME) == NULL
		&&	dba_record_key_peek_value(rec, DBA_KEY_BLOCK) == NULL
		&&	dba_record_key_peek_value(rec, DBA_KEY_STATION) == NULL)
		return dba_error_ok();

	/* Get the id of the ana context */
	db->context->id_ana = id_ana;
	db->context->id_report = -1;
	DBA_RUN_OR_RETURN(dba_db_context_obtain_ana(db->context, &(db->data->id_context)));

	/* Insert or update the data that we find in record */
	if ((var = dba_record_key_peek(rec, DBA_KEY_BLOCK)) != NULL)
	{
		db->data->id_var = DBA_VAR(0, 1, 1);
		dba_db_data_set_value(db->data, dba_var_value(var));
		DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
	}
	if ((var = dba_record_key_peek(rec, DBA_KEY_STATION)) != NULL)
	{
		db->data->id_var = DBA_VAR(0, 1, 2);
		dba_db_data_set_value(db->data, dba_var_value(var));
		DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
	}
	if ((var = dba_record_key_peek(rec, DBA_KEY_NAME)) != NULL)
	{
		db->data->id_var = DBA_VAR(0, 1, 19);
		dba_db_data_set_value(db->data, dba_var_value(var));
		DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
	}
	if ((var = dba_record_key_peek(rec, DBA_KEY_HEIGHT)) != NULL)
	{
		db->data->id_var = DBA_VAR(0, 7, 1);
		dba_db_data_set_value(db->data, dba_var_value(var));
		DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
	}
	if ((var = dba_record_key_peek(rec, DBA_KEY_HEIGHTBARO)) != NULL)
	{
		db->data->id_var = DBA_VAR(0, 7, 31);
		dba_db_data_set_value(db->data, dba_var_value(var));
		DBA_RUN_OR_RETURN(dba_db_data_insert(db->data, 1));
	}

	return dba_error_ok();
}
#endif

/*
 * Insert or replace data in pseudoana taking the values from rec.
 * If rec did not contain ana_id, it will be set by this function.
 */
static dba_err dba_insert_pseudoana(dba_db db, dba_record rec, int can_add, int* id)
{
	dba_err err = DBA_OK;
	int mobile;
	const char* val;
	dba_db_pseudoana a;

	assert(db);
	DBA_RUN_OR_RETURN(dba_db_need_pseudoana(db));
	a = db->pseudoana;

	/* Look for an existing ID if provided */
	if ((val = dba_record_key_peek_value(rec, DBA_KEY_ANA_ID)) != NULL)
		*id = strtol(val, 0, 10);
	else
		*id = -1;

	/* If we don't need to rewrite, we are done */
	if (*id != -1)
		return dba_error_ok();

	/* Look for the key data in the record */
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_LAT, &(a->lat)));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_LON, &(a->lon)));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_MOBILE, &mobile));
	if (mobile)
	{
		DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqc(rec, DBA_KEY_IDENT, &val));
		dba_db_pseudoana_set_ident(a, val);
	} else {
		a->ident_ind = SQL_NULL_DATA;
	}

	/* Check for an existing pseudoana with these data */
	if (*id == -1)
	{
		DBA_RUN_OR_GOTO(cleanup, dba_db_pseudoana_get_id(a, id));

		/* If not found, insert a new one */
		if (*id == -1)
		{
			if (can_add)
				DBA_RUN_OR_GOTO(cleanup, dba_db_pseudoana_insert(a, id));
			else
			{
				err = dba_error_consistency(
						"trying to insert a pseudoana entry when it is forbidden");
				goto cleanup;
			}
		}
	}

cleanup:
	return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err dba_insert_context(dba_db db, dba_record rec, int id_ana, int* id)
{
	const char *year, *month, *day, *hour, *min, *sec;
	dba_db_context c;
	assert(db);
	DBA_RUN_OR_RETURN(dba_db_need_context(db));
	c = db->context;

	/* Retrieve data */

	c->id_ana = id_ana;

	/* Get the ID of the report */
	DBA_RUN_OR_RETURN(dba_db_get_rep_cod(db, rec, &(c->id_report)));

	/* Also input the seconds, defaulting to 0 if not found */
	sec = dba_record_key_peek_value(rec, DBA_KEY_SEC);
	/* Datetime needs to be computed */
	if ((year  = dba_record_key_peek_value(rec, DBA_KEY_YEAR)) != NULL &&
		(month = dba_record_key_peek_value(rec, DBA_KEY_MONTH)) != NULL &&
		(day   = dba_record_key_peek_value(rec, DBA_KEY_DAY)) != NULL &&
		(hour  = dba_record_key_peek_value(rec, DBA_KEY_HOUR)) != NULL &&
		(min   = dba_record_key_peek_value(rec, DBA_KEY_MIN)) != NULL)
	{
		c->date_ind = snprintf(c->date, 25,
				"%04ld-%02ld-%02ld %02ld:%02ld:%02ld",
					strtol(year, 0, 10),
					strtol(month, 0, 10),
					strtol(day, 0, 10),
					strtol(hour, 0, 10),
					strtol(min, 0, 10),
					sec != NULL ? strtol(sec, 0, 10) : 0);
	}
	else
		return dba_error_notfound("looking for datetime informations");

	DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_LEVELTYPE,	&(c->ltype)));
	DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_L1,			&(c->l1)));
	DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_L2,			&(c->l2)));
	DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_PINDICATOR,	&(c->pind)));
	DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_P1,			&(c->p1)));
	DBA_RUN_OR_RETURN(dba_record_key_enqi(rec, DBA_KEY_P2,			&(c->p2)));

	/* Check for an existing context with these data */
	DBA_RUN_OR_RETURN(dba_db_context_get_id(c, id));

	/* If there is an existing record, use its ID and don't do an INSERT */
	if (*id != -1)
		return dba_error_ok();

	/* Else, insert a new record */
	return dba_db_context_insert(c, id);
}


/*
 * If can_replace, then existing data can be rewritten, else it can only add new data
 *
 * If update_pseudoana, then the pseudoana informations are overwritten using
 * information from `rec'; else data from `rec' is written into pseudoana only
 * if there is no suitable anagraphical data for it.
 */
dba_err dba_db_insert(dba_db db, dba_record rec, int can_replace, int pseudoana_can_add, int* ana_id, int* context_id)
{
	dba_err err = DBA_OK;
	dba_db_data d;
	dba_record_cursor item;
	int id_pseudoana;
	
	assert(db);
	DBA_RUN_OR_RETURN(dba_db_need_data(db));
	d = db->data;

	/* Check for the existance of non-context data, otherwise it's all useless */
	if (dba_record_iterate_first(rec) == NULL)
		return dba_error_consistency("looking for data to insert");

	/* Begin the transaction */
	DBA_RUN_OR_RETURN(dba_db_begin(db));

	/* Insert the pseudoana data, and get the ID */
	DBA_RUN_OR_GOTO(fail, dba_insert_pseudoana(db, rec, pseudoana_can_add, &id_pseudoana));

	/* Insert the context data, and get the ID */
	DBA_RUN_OR_GOTO(fail, dba_insert_context(db, rec, id_pseudoana, &(d->id_context)));

	/* Insert all found variables */
	for (item = dba_record_iterate_first(rec); item != NULL;
			item = dba_record_iterate_next(rec, item))
	{
		/* Datum to be inserted, linked to id_pseudoana and all the other IDs */
		dba_db_data_set(d, dba_record_cursor_variable(item));
		DBA_RUN_OR_GOTO(fail, dba_db_data_insert(d, can_replace));
	}

	if (ana_id != NULL)
		*ana_id = id_pseudoana;
	if (context_id != NULL)
		*context_id = d->id_context;

	DBA_RUN_OR_GOTO(fail, dba_db_commit(db));

	return dba_error_ok();

	/* Exits with cleanup after error */
fail:
	dba_db_rollback(db);
	return err;
}

dba_err dba_db_ana_query(dba_db db, dba_record query, dba_db_cursor* cur, int* count)
{
	dba_err err;

	/* Allocate a new cursor */
	DBA_RUN_OR_RETURN(dba_db_cursor_create(db, cur));

	/* Perform the query, limited to pseudoana values */
	DBA_RUN_OR_GOTO(failed, dba_db_cursor_query(*cur, query,
				DBA_DB_WANT_ANA_ID | DBA_DB_WANT_COORDS | DBA_DB_WANT_IDENT,
				DBA_DB_MODIFIER_ANAEXTRA | DBA_DB_MODIFIER_DISTINCT));

	/* Get the number of results */
	*count = dba_db_cursor_remaining(*cur);

	/* Retrieve results will happen in dba_db_cursor_next() */

	/* Done.  No need to deallocate the statement, it will be done by
	 * dba_db_cursor_delete */
	return dba_error_ok();

	/* Exit point with cleanup after error */
failed:
	dba_db_cursor_delete(*cur);
	*cur = 0;
	return err;
}

dba_err dba_db_query(dba_db db, dba_record rec, dba_db_cursor* cur, int* count)
{
	dba_err err;

	/* Allocate a new cursor */
	DBA_RUN_OR_RETURN(dba_db_cursor_create(db, cur));

	/* Perform the query */
	DBA_RUN_OR_GOTO(failed, dba_db_cursor_query(*cur, rec,
				DBA_DB_WANT_ANA_ID | DBA_DB_WANT_CONTEXT_ID |
				DBA_DB_WANT_COORDS | DBA_DB_WANT_IDENT | DBA_DB_WANT_LEVEL |
				DBA_DB_WANT_TIMERANGE | DBA_DB_WANT_DATETIME |
				DBA_DB_WANT_VAR_NAME | DBA_DB_WANT_VAR_VALUE |
				DBA_DB_WANT_REPCOD,
				0));

	/* Get the number of results */
	*count = dba_db_cursor_remaining(*cur);

	/* Retrieve results will happen in dba_db_cursor_next() */

	/* Done.  No need to deallocate the statement, it will be done by
	 * dba_db_cursor_delete */
	return dba_error_ok();

	/* Exit point with cleanup after error */
failed:
	dba_db_cursor_delete(*cur);
	*cur = 0;
	return err;
}

dba_err dba_db_remove_orphans(dba_db db)
{
	static const char* cclean_mysql = "delete c from context as c left join data as d on d.id_context = c.id where d.id_context is NULL";
	static const char* pclean_mysql = "delete p from pseudoana as p left join context as c on c.id_ana = p.id where c.id is NULL";
	static const char* cclean_sqlite = "delete from context where id in (select c.id from context as c left join data as d on d.id_context = c.id where d.id_context is NULL)";
	static const char* pclean_sqlite = "delete from pseudoana where id in (select p.id from pseudoana as p left join context as c on c.id_ana = p.id where c.id is NULL)";
	static const char* cclean = NULL;
	static const char* pclean = NULL;
	dba_err err = DBA_OK;
	SQLHSTMT stm = NULL;
	int res;

	switch (db->server_type)
	{
		case MYSQL: cclean = cclean_mysql; pclean = pclean_mysql; break;
		case SQLITE: cclean = cclean_sqlite; pclean = pclean_sqlite; break;
		default: cclean = cclean_mysql; pclean = pclean_mysql; break;
	}

	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stm));

	/* Delete orphan contexts */
	res = SQLExecDirect(stm, (unsigned char*)cclean, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", cclean);
		goto cleanup;
	}

	/* Done with context */
	res = SQLCloseCursor(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_remove_orphans cursor");

	/* Delete orphan pseudoanas */
	res = SQLExecDirect(stm, (unsigned char*)pclean, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", pclean);
		goto cleanup;
	}

cleanup:
	if (stm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_remove(dba_db db, dba_record rec)
{
	dba_err err = DBA_OK;
	dba_db_cursor cur = NULL;
	SQLHSTMT stmd = NULL;
	SQLHSTMT stma = NULL;
	int res;

	/* Allocate statement handle */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stmd));
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &stma));

	/* Compile the DELETE query for the data */
	res = SQLPrepare(stmd, (unsigned char*)"DELETE FROM data WHERE data.id_context=? AND data.id_var=?", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stmd, "compiling query to delete data entries");
		goto cleanup;
	}
	/* Compile the DELETE query for the attributes */
	res = SQLPrepare(stma, (unsigned char*)"DELETE FROM attr WHERE attr.id_context=? AND attr.id_var=?", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stma, "compiling query to delete attribute entries");
		goto cleanup;
	}

	/* Allocate a new cursor */
	DBA_RUN_OR_RETURN(dba_db_cursor_create(db, &cur));

	/* Bind parameters */
	SQLBindParameter(stmd, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(cur->out_context_id), 0, 0);
	SQLBindParameter(stmd, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(cur->out_idvar), 0, 0);
	SQLBindParameter(stma, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(cur->out_context_id), 0, 0);
	SQLBindParameter(stma, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(cur->out_idvar), 0, 0);

	/* Get the list of data to delete */
	DBA_RUN_OR_GOTO(cleanup, dba_db_cursor_query(cur, rec,
				DBA_DB_WANT_CONTEXT_ID | DBA_DB_WANT_VAR_NAME,
				DBA_DB_MODIFIER_UNSORTED | DBA_DB_MODIFIER_STREAM));

	/* Iterate all the results, deleting them */
	while (1)
	{
		int has_data;
		DBA_RUN_OR_RETURN(dba_db_cursor_next(cur, &has_data));
		if (!has_data)
			break;

		res = SQLExecute(stmd);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stmd, "deleting data entry %d/B%02d%03d",
					cur->out_context_id, DBA_VAR_X(cur->out_idvar), DBA_VAR_Y(cur->out_idvar));
			goto cleanup;
		}
		res = SQLExecute(stma);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stma, "deleting attribute entries for %d/B%02d%03d",
					cur->out_context_id, DBA_VAR_X(cur->out_idvar), DBA_VAR_Y(cur->out_idvar));
			goto cleanup;
		}
	}

cleanup:
	if (stmd != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stmd);
	if (stma != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, stma);
	if (cur != NULL)
		dba_db_cursor_delete(cur);
	return err == DBA_OK ? dba_error_ok() : err;
}
#if 0
#ifdef DBA_USE_DELETE_USING
dba_err dba_db_remove(dba_db db, dba_record rec)
{
	const char* query =
		"DELETE FROM d, a"
		" USING pseudoana AS pa, context AS c, repinfo AS ri, data AS d"
		"  LEFT JOIN attr AS a ON a.id_context = d.id_context AND a.id_var = d.id_var"
		" WHERE d.id_context = c.id AND c.id_ana = pa.id AND c.id_report = ri.id";
	dba_err err;
	SQLHSTMT stm;
	int res;
	int pseq = 1;

	assert(db);

	/* Allocate statement handle */
	DBA_RUN_OR_RETURN(dba_db_statement_create(db, &stm));

	/* Write the SQL query */

	/* Initial query */
	dba_querybuf_reset(db->querybuf);
	DBA_RUN_OR_GOTO(dba_delete_failed, dba_querybuf_append(db->querybuf, query));

	/* Bind select fields */
	DBA_RUN_OR_GOTO(dba_delete_failed, dba_db_prepare_select(db, rec, stm, &pseq));

	/*fprintf(stderr, "QUERY: %s\n", db->querybuf);*/

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)dba_querybuf_get(db->querybuf), dba_querybuf_size(db->querybuf));
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", dba_querybuf_get(db->querybuf));
		goto dba_delete_failed;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return dba_error_ok();

	/* Exit point with cleanup after error */
dba_delete_failed:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err;
}
#else
dba_err dba_db_remove(dba_db db, dba_record rec)
{
	const char* query =
		"SELECT d.id FROM pseudoana AS pa, context AS c, data AS d, repinfo AS ri"
		" WHERE d.id_context = c.id AND c.id_ana = pa.id AND c.id_report = ri.id";
	dba_err err = DBA_OK;
	SQLHSTMT stm;
	SQLHSTMT stm1;
	SQLHSTMT stm2;
	SQLINTEGER id;
	int res;

	assert(db);

	/* Allocate statement handles */
	DBA_RUN_OR_RETURN(dba_db_statement_create(db, &stm));

	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm1);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm1, "Allocating new statement handle");
	}
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm2);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
		SQLFreeHandle(SQL_HANDLE_STMT, stm1);
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm2, "Allocating new statement handle");
	}

	/* Write the SQL query */

	/* Initial query */
	dba_querybuf_reset(db->querybuf);
	DBA_RUN_OR_GOTO(cleanup, dba_querybuf_append(db->querybuf, query));

	/* Bind select fields */
	DBA_RUN_OR_GOTO(cleanup, dba_db_prepare_select(db, rec, stm));

	/* Bind output field */
	SQLBindCol(stm, 1, SQL_C_SLONG, &id, sizeof(id), NULL);

	/*fprintf(stderr, "QUERY: %s\n", db->querybuf);*/

	/* Perform the query */
	TRACE("Performing query %s\n", dba_querybuf_get(db->querybuf));
	res = SQLExecDirect(stm, dba_querybuf_get(db->querybuf), dba_querybuf_size(db->querybuf));
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", dba_querybuf_get(db->querybuf));
		goto cleanup;
	}

	/* Compile the DELETE query for the data */
	res = SQLPrepare(stm1, (unsigned char*)"DELETE FROM data WHERE id=?", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm1, "compiling query to delete data entries");
		goto cleanup;
	}
	/* Bind parameters */
	SQLBindParameter(stm1, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id, 0, 0);

	/* Compile the DELETE query for the associated QC */
	res = SQLPrepare(stm2, (unsigned char*)"DELETE FROM attr WHERE id_data=?", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm2, "compiling query to delete entries related to QC data");
		goto cleanup;
	}
	/* Bind parameters */
	SQLBindParameter(stm2, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id, 0, 0);

	/* Fetch the IDs and delete them */
	while (SQLFetch(stm) != SQL_NO_DATA)
	{
		/*fprintf(stderr, "Deleting %d\n", id);*/
		res = SQLExecute(stm1);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm1, "deleting entry %d from the 'data' table", id);
			goto cleanup;
		}
		res = SQLExecute(stm2);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm2, "deleting QC data related to 'data' entry %d", id);
			goto cleanup;
		}
	}

	/* Exit point with cleanup after error */
cleanup:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	SQLFreeHandle(SQL_HANDLE_STMT, stm1);
	SQLFreeHandle(SQL_HANDLE_STMT, stm2);
	return err == DBA_OK ? dba_error_ok() : err;
}
#endif
#endif

dba_err dba_db_qc_query(dba_db db, int id_context, dba_varcode id_var, dba_varcode* qcs, int qcs_size, dba_record qc, int* count)
{
	char query[100 + 100*6];
	SQLHSTMT stm;
	dba_err err;
	int res;
	int out_type;
	const char out_value[255];

	assert(db);

	/* Create the query */
	if (qcs == NULL || qcs_size == 0)
		/* If qcs is null, query all QC data */
		strcpy(query,
				"SELECT type, value"
				"  FROM attr"
				" WHERE id_context = ? AND id_var = ?");
	else {
		int i, qs;
		char* q;
		if (qcs_size > 100)
			return dba_error_consistency("checking bound of 100 QC values to retrieve per query");
		strcpy(query,
				"SELECT type, value"
				"  FROM attr"
				" WHERE id_context = ? AND id_var = ? AND type IN (");
		qs = strlen(query);
		q = query + qs;
		for (i = 0; i < qcs_size; i++)
			if (q == query + qs)
				q += snprintf(q, 7, "%hd", qcs[i]);
			else
				q += snprintf(q, 8, ",%hd", qcs[i]);
		strcpy(q, ")");
	}

	/* Allocate statement handle */
	DBA_RUN_OR_RETURN(dba_db_statement_create(db, &stm));

	/* Bind input parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_context, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &id_var, 0, 0);

	/* Bind output fields */
	SQLBindCol(stm, 1, SQL_C_SLONG, &out_type, sizeof(out_type), 0);
	SQLBindCol(stm, 2, SQL_C_CHAR, &out_value, sizeof(out_value), 0);
	
	TRACE("QC read query: %s with id_data %d\n", query, id_data);

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", query);
		goto dba_qc_query_failed;
	}

	/* Retrieve results */
	dba_record_clear(qc);

	/* Fetch new data */
	*count = 0;
	while (SQLFetch(stm) != SQL_NO_DATA)
	{
		dba_record_var_setc(qc, out_type, out_value);		
		(*count)++;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return dba_error_ok();

	/* Exit point with cleanup after error */
dba_qc_query_failed:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err;
}

dba_err dba_db_qc_insert_or_replace(dba_db db, int id_context, dba_varcode id_var, dba_record qc, int can_replace)
{
	dba_err err;
	dba_record_cursor item;
	dba_db_attr a;
	
	assert(db);

	DBA_RUN_OR_RETURN(dba_db_need_attr(db));
	a = db->attr;

	a->id_context = id_context;
	a->id_var = id_var;

	/* Begin the transaction */
	DBA_RUN_OR_GOTO(fail, dba_db_begin(db));

	/* Insert all found variables */
	for (item = dba_record_iterate_first(qc); item != NULL;
			item = dba_record_iterate_next(qc, item))
	{
		dba_db_attr_set(a, dba_record_cursor_variable(item));
		DBA_RUN_OR_GOTO(fail, dba_db_attr_insert(a, can_replace));
	}
			
	DBA_RUN_OR_GOTO(fail, dba_db_commit(db));
	return dba_error_ok();

	/* Exits with cleanup after error */
fail:
	dba_db_rollback(db);
	return err;
}

dba_err dba_db_qc_insert(dba_db db, int id_context, dba_varcode id_var, dba_record qc)
{
	return dba_db_qc_insert_or_replace(db, id_context, id_var, qc, 1);
}

dba_err dba_db_qc_insert_new(dba_db db, int id_context, dba_varcode id_var, dba_record qc)
{
	return dba_db_qc_insert_or_replace(db, id_context, id_var, qc, 0);
}

dba_err dba_db_qc_remove(dba_db db, int id_context, dba_varcode id_var, dba_varcode* qcs, int qcs_size)
{
	char query[60 + 100*6];
	SQLHSTMT stm;
	dba_err err;
	int res;

	assert(db);

	// Create the query
	if (qcs == NULL)
		strcpy(query, "DELETE FROM attr WHERE id_context = ? AND id_var = ?");
	else {
		int i, qs;
		char* q;
		if (qcs_size > 100)
			return dba_error_consistency("checking bound of 100 QC values to delete per query");
		strcpy(query, "DELETE FROM attr WHERE id_context = ? AND id_var = ? AND type IN (");
		qs = strlen(query);
		q = query + qs;
		for (i = 0; i < qcs_size; i++)
			if (q == query + qs)
				q += snprintf(q, 7, "%hd", qcs[i]);
			else
				q += snprintf(q, 8, ",%hd", qcs[i]);
		strcpy(q, ")");
	}

	/* Allocate statement handle */
	DBA_RUN_OR_RETURN(dba_db_statement_create(db, &stm));

	/* Bind parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_context, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &id_var, 0, 0);

	dba_verbose(DBA_VERB_DB_SQL, "Performing query %s for id %d,B%02d%03d\n", query, id_context, DBA_VAR_X(id_var), DBA_VAR_Y(id_var));
	
	/* Execute the DELETE SQL query */
	/* Casting to char* because ODBC is unaware of const */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "deleting data from 'attr'");
		goto dba_qc_delete_failed;
	}
			
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return dba_error_ok();

	/* Exits with cleanup after error */
dba_qc_delete_failed:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err;
}


#if 0
	{
		/* List DSNs */
		char dsn[100], desc[100];
		short int len_dsn, len_desc, next;

		for (next = SQL_FETCH_FIRST;
				SQLDataSources(pc.od_env, next, dsn, sizeof(dsn),
					&len_dsn, desc, sizeof(desc), &len_desc) == SQL_SUCCESS;
				next = SQL_FETCH_NEXT)
			printf("DSN %s (%s)\n", dsn, desc);
	}
#endif

#if 0
	for (res = SQLFetch(pc.od_stm); res != SQL_NO_DATA; res = SQLFetch(pc.od_stm))
	{
		printf("Result: %d\n", i);
	}
#endif

/* vim:set ts=4 sw=4: */
