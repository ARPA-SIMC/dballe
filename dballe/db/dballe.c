#define _GNU_SOURCE
#include <dballe/db/dballe.h>
#include <dballe/core/dba_record.h>
#include <dballe/core/dba_var.h>
#include <dballe/core/dba_csv.h>
#include <dballe/core/verbose.h>

#include <config.h>

#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>

/*
 * Define to true to enable the use of transactions during writes
 */
/*
*/
#ifdef USE_MYSQL4
#define DBA_USE_DELETE_USING
#define DBA_USE_TRANSACTIONS
#endif

/* #define TRACE_DB */

#ifdef TRACE_DB
#define TRACE(...) fprintf(stderr, __VA_ARGS__)
#define IFTRACE if (1)
#else
#define TRACE(...) do { } while (0)
#define IFTRACE if (0)
#endif


struct _dba
{
	SQLHDBC	od_conn;
	/*
	 * This is very conservative:
	 * The query size plus 30 possible select values, maximum of 30 characters each
	 * plus 400 characters for the various combinations of the two min and max datetimes,
	 * plus 255 for the blist
	 */
	char querybuf[170 + 30*30 + 400 + 255];
 	char sel_dtmin[25];
 	char sel_dtmax[25];
 	char sel_dtlike[25];
	int sel_latmin;
	int sel_lonmin;
	int sel_latmax;
	int sel_lonmax;
	const char* sel_ident;
	int sel_pindicator;
	int sel_p1;
	int sel_p2;
	int sel_leveltype;
	int sel_l1;
	int sel_l2;
	int sel_b;
	int sel_rep_cod;
	const char* sel_rep_memo;
	int sel_priority;
	int sel_priomin;
	int sel_priomax;
	int sel_ana_id;
	int sel_data_id;
	int sel_block;
	int sel_station;
};

struct _dba_cursor {
	dba db;
	enum { ANA, DATA, QC } type;
	SQLHSTMT stm;

	int count;

	/* Bound variables */
#define DB_CUR_VAR(type, name) type out_##name; SQLINTEGER out_##name##_ind
#define DB_CUR_CHARVAR(name, len) char out_##name[len]; SQLINTEGER out_##name##_ind
	DB_CUR_VAR(int, lat);
	DB_CUR_VAR(int, lon);
	DB_CUR_CHARVAR(ident, 64);
	DB_CUR_VAR(int, height);
	DB_CUR_VAR(int, heightbaro);
	DB_CUR_VAR(int, block);
	DB_CUR_VAR(int, station);
	DB_CUR_CHARVAR(name, 255);
	DB_CUR_VAR(int, leveltype);
	DB_CUR_VAR(int, l1);
	DB_CUR_VAR(int, l2);
	DB_CUR_VAR(int, pindicator);
	DB_CUR_VAR(int, p1);
	DB_CUR_VAR(int, p2);
	DB_CUR_VAR(int, idvar);
	DB_CUR_CHARVAR(datetime, 25);
	DB_CUR_CHARVAR(value, 255);
	/* DB_CUR_VAR(int, rep_id); */
	DB_CUR_VAR(int, rep_cod);
	DB_CUR_CHARVAR(rep_memo, 20);
	DB_CUR_VAR(int, priority);
	DB_CUR_VAR(int, ana_id);
	DB_CUR_VAR(int, data_id);
#undef DB_CUR_VAR
#undef DB_CUR_CHARVAR
};

static SQLHENV dba_od_env;

static const char* init_tables[] = {
	"repinfo", "pseudoana", "data", "context", "attr"
};

#ifdef DBA_USE_TRANSACTIONS
#define TABLETYPE "TYPE=InnoDB;"
#else
#define TABLETYPE ";"
#endif
static const char* init_queries[] = {
	"CREATE TABLE repinfo ("
	"   id		     INTEGER PRIMARY KEY,"
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
	"   height     INTEGER,"
	"   heightbaro INTEGER,"
	"   block      INTEGER,"
	"   station    INTEGER,"
	"   name       VARCHAR(255),"
	"   UNIQUE INDEX(lat, lon, ident(8))"
	") " TABLETYPE,
	"CREATE TABLE data ("
	"   id			INTEGER auto_increment PRIMARY KEY,"
	"   id_context	INTEGER NOT NULL,"
	"	id_var		INTEGER NOT NULL,"
	"	value		VARCHAR(255) NOT NULL,"
	"   UNIQUE INDEX(id_var, id_context)"
	") " TABLETYPE,
	"CREATE TABLE context ("
	"   id			INTEGER auto_increment PRIMARY KEY,"
	"   id_ana		INTEGER NOT NULL,"
	"   id_report	INTEGER NOT NULL,"
	"   datetime	DATETIME NOT NULL,"
	"	ltype		INTEGER NOT NULL,"
	"	l1			INTEGER NOT NULL,"
	"	l2			INTEGER NOT NULL,"
	"	ptype		INTEGER NOT NULL,"
	"	p1			INTEGER NOT NULL,"
	"	p2			INTEGER NOT NULL,"
	"   UNIQUE INDEX (id_ana, datetime, ltype, l1, l2, ptype, p1, p2, id_report)"
	") " TABLETYPE,
	"CREATE TABLE attr ("
	"   id_data INTEGER NOT NULL,"
	"   type    INTEGER NOT NULL,"
	"   value   VARCHAR(255) NOT NULL,"
	"   INDEX (id_data),"
	"   UNIQUE INDEX (id_data, type)"
	") " TABLETYPE,
};


/**
 * Copy informations from the ODBC diagnostic record to the dba error
 * report
 */
static dba_err dba_error_odbc(SQLSMALLINT handleType, SQLHANDLE handle, const char* fmt, ...)
{
	va_list ap;
	static const int strsize = 200;
	char stat[10], msg[strsize];
	char* context;
	SQLINTEGER err;
	SQLSMALLINT mlen;

	SQLGetDiagRec(handleType, handle, 1, (unsigned char*)stat, &err, (unsigned char*)msg, strsize, &mlen);

	va_start(ap, fmt);
	vasprintf(&context, fmt, ap);
	va_end(ap);

	return dba_error_generic0(DBA_ERR_ODBC, context, strndup(msg, mlen));
}

dba_err dba_db_init()
{
	// Allocate ODBC environment handle and register version 
	int res = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &dba_od_env);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_error_odbc(SQL_HANDLE_ENV, dba_od_env, "Allocating main environment handle");

	res = SQLSetEnvAttr(dba_od_env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		dba_err res = dba_error_odbc(SQL_HANDLE_ENV, dba_od_env, "Asking for ODBC version 3");
		SQLFreeHandle(SQL_HANDLE_ENV, dba_od_env);
		return res;
	}

	return dba_error_ok();
}

void dba_db_shutdown()
{
	SQLFreeHandle(SQL_HANDLE_ENV, dba_od_env);
	/* TODO: warn about all allocated resources and free them */
}

dba_err dba_open(const char* dsn, const char* user, const char* password, dba* db)
{
	int sqlres;

	/* Allocate a new handle */
	*db = (dba)calloc(1, sizeof(struct _dba));
	if (!*db)
		return dba_error_alloc("trying to allocate a new dba object");

	/* Allocate the ODBC connection handle */
	sqlres = SQLAllocHandle(SQL_HANDLE_DBC, dba_od_env, &((*db)->od_conn));
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
	{
		dba_err err = dba_error_odbc(SQL_HANDLE_DBC, (*db)->od_conn,
				"Allocating new connection handle");
		free(*db);
		*db = 0;
		return err;
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
		dba_err err = dba_error_odbc(SQL_HANDLE_DBC, (*db)->od_conn,
				"Connecting to DSN %s as user %s", dsn, user);
		SQLFreeHandle(SQL_HANDLE_DBC, (*db)->od_conn);
		free(*db);
		*db = 0;
		return err;
	}
	
	return dba_error_ok();
}

dba_err dba_reset(dba db, const char* deffile)
{
	int res;
	int i;
	SQLHSTMT stm;
	dba_err err;

	assert(db);

	if (deffile == 0)
	{
		deffile = getenv("DBA_REPINFO");
		if (deffile == 0 || deffile[0] == 0)
			deffile = CONF_DIR "/repinfo.csv";
	}

	/* Open the input CSV file */
	FILE* in = fopen(deffile, "r");
	if (in == NULL)
		return dba_error_system("opening file %s", deffile);

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
		goto fail0;
	}

	/* Drop existing tables */
	for (i = 0; i < sizeof(init_tables) / sizeof(init_tables[0]); i++)
	{
		char buf[100];
		int len = snprintf(buf, 100, "DROP TABLE IF EXISTS %s", init_tables[i]);
		res = SQLExecDirect(stm, (unsigned char*)buf, len);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm,
					"Removing old table %s", init_tables[i]);
			goto fail1;
		}
	}

	/* Create tables */
	for (i = 0; i < sizeof(init_queries) / sizeof(init_queries[0]); i++)
	{
		/* Casting out 'const' because ODBC API is not const-conscious */
		res = SQLExecDirect(stm, (unsigned char*)init_queries[i], SQL_NTS);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm,
					"Executing database-initialization query %s", init_queries[i]);
			goto fail1;
		}
	}

	/* Populate the tables with values */
	{
		int id;
		char memo[30];
		char description[255];
		int prio;
		char descriptor[6];
		int tablea;
		int i;
		char* columns[7];
		int line;

		res = SQLPrepare(stm, (unsigned char*)
				"INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)"
				"     VALUES (?, ?, ?, ?, ?, ?)", SQL_NTS);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm, "compiling query to insert into 'repinfo'");
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
				err = dba_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into 'data'");
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

void dba_close(dba db)
{
	assert(db);

	SQLDisconnect(db->od_conn);
	SQLFreeHandle(SQL_HANDLE_DBC, db->od_conn);
	free(db);
}

static dba_err dba_last_insert_id(SQLHDBC od_conn, int* id)
{
	SQLHSTMT stm;
	int res;
	SQLINTEGER id_ind;
	dba_err err;

	/* Allocate statement handle for select */
	res = SQLAllocHandle(SQL_HANDLE_STMT, od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement");

	/* Bind variable and indicator */
	SQLBindCol(stm, 1, SQL_C_SLONG, id, sizeof(*id), &id_ind);
	
	res = SQLExecDirect(stm, (unsigned char*)"SELECT LAST_INSERT_ID()", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "querying last inserted ID");
		goto dba_last_insert_id_failed;
	}

	if (SQLFetch(stm) == SQL_NO_DATA)
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "retrieving results of query for last inserted ID");
		goto dba_last_insert_id_failed;
	}

	if (id_ind != sizeof(*id))
	{
		err = dba_error_consistency("checking that the size of the last insert ID coming from the database is correct");
		goto dba_last_insert_id_failed;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return dba_error_ok();

dba_last_insert_id_failed:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err;
}

dba_err dba_rep_cod_from_memo(dba db, const char* memo, int* rep_cod)
{
	const char* query = "SELECT id FROM repinfo WHERE memo = ?";
	dba_err err = DBA_OK;
	SQLHSTMT stm;
	SQLINTEGER rep_cod_ind;
	int res;

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

	/* Bind input parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)memo, 0, 0);

	/* Bind variable and indicator for SELECT results */
	SQLBindCol(stm, 1, SQL_C_SLONG, rep_cod, sizeof(int), &rep_cod_ind);

	/* Casting to char* because ODBC is unaware of const */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "looking for report informations");
		goto cleanup;
	}

	/* Get one result */
	if (SQLFetch(stm) == SQL_NO_DATA)
	{
		err = dba_error_notfound("looking for report informations for '%s'", memo);
		goto cleanup;
	}

cleanup:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err;
}

/* Get the report id from this record.  If rep_memo is specified instead, the
 * corresponding report id is queried in the database and set as "rep_cod" in
 * the record. */
static dba_err dba_get_rep_cod(dba db, dba_record rec, int* id)
{
	const char* rep;
	if ((rep = dba_record_key_peek_value(rec, DBA_KEY_REP_COD)) != NULL)
		*id = strtol(rep, 0, 10);
	else if ((rep = dba_record_key_peek_value(rec, DBA_KEY_REP_MEMO)) != NULL)
	{
		DBA_RUN_OR_RETURN(dba_rep_cod_from_memo(db, rep, id));
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_REP_COD, *id));
	}
	else
		return dba_error_notfound("looking for report type in rep_cod or rep_memo");
	return dba_error_ok();
}		


/*
 * Insert or replace data in pseudoana taking the values from rec.
 * If rec did not contain ana_id, it will be set by this function.
 */
static dba_err dba_insert_pseudoana(dba db, dba_record rec, int* id, int rewrite)
{
	dba_err err = DBA_OK;
	const char* query_sel = NULL;
	const char* query_sel_fixed =
		"SELECT id FROM pseudoana WHERE lat=? AND lon=? AND ident IS NULL";
	const char* query_sel_mobile =
		"SELECT id FROM pseudoana WHERE lat=? AND lon=? AND ident=?";
	const char* query_repl =
		"UPDATE pseudoana SET lat=?, lon=?, ident=?, height=?, heightbaro=?,"
		"                     block=?, station=?, name=? WHERE id=?";
	const char* query =
		"INSERT INTO pseudoana (lat, lon, ident, height, heightbaro, block, station, name)"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
	int lat;
	int lon;
	int mobile;
	const char* ident;
	SQLINTEGER ident_ind;
	const char* val;
	int height;
	SQLINTEGER height_ind;
	int heightbaro;
	SQLINTEGER heightbaro_ind;
	int block;
	SQLINTEGER block_ind;
	int station;
	SQLINTEGER station_ind;
	const char* name;
	SQLINTEGER name_ind;
	SQLHSTMT stm;
	int res;
	int old_id;
	SQLINTEGER old_id_ind;
	int has_data = 0;

	assert(db);

	/* Look for an existing ID if provided */
	{
		const char* val;
		if ((val = dba_record_key_peek_value(rec, DBA_KEY_ANA_ID)) != NULL)
			*id = strtol(val, 0, 10);
		else
			*id = -1;
	}

	/* If we don't need to rewrite, we are done */
	if (*id != -1 && !rewrite)
		return dba_error_ok();

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

	/* Look for the key data in the record */
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_LAT, &lat));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_LON, &lon));
	DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqi(rec, DBA_KEY_MOBILE, &mobile));
	if (mobile)
	{
		DBA_RUN_OR_GOTO(cleanup, dba_record_key_enqc(rec, DBA_KEY_IDENT, &ident));
	} else {
		ident = NULL;
	}

	/* Bind key parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &lat, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &lon, 0, 0);
	/* Casting to char* because ODBC is unaware of const */
	if (mobile)
	{
		ident_ind = SQL_NTS;
		SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)ident, 0, &ident_ind);
		query_sel = query_sel_mobile;
	}
	else
	{
		ident_ind = SQL_NULL_DATA;
		SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)"", 0, &ident_ind);
		query_sel = query_sel_fixed;
	}

	/* Check for an existing pseudoana with these data */
	if (*id == -1)
	{
		/* Bind variable and indicator for SELECT results */
		SQLBindCol(stm, 1, SQL_C_SLONG, &old_id, sizeof(old_id), &old_id_ind);
		
		/* Casting to char* because ODBC is unaware of const */
		res = SQLExecDirect(stm, (unsigned char*)query_sel, SQL_NTS);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm, "looking for existing pseudoana");
			goto cleanup;
		}

		/* Get the result */
		if (SQLFetch(stm) != SQL_NO_DATA)
		{
			if (old_id_ind != sizeof(old_id))
			{
				err = dba_error_consistency("checking that the size of the ID coming from the database is correct");
				goto cleanup;
			}

			*id = old_id;
			has_data = 1;
			DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_ANA_ID, *id));

			/* If we don't have to update the data, we're finished */
			if (! rewrite)
			{
				SQLFreeHandle(SQL_HANDLE_STMT, stm);
				return dba_error_ok();
			}
		}

		res = SQLCloseCursor(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			if (has_data)
				err = dba_error_odbc(SQL_HANDLE_STMT, stm, "preparing for replacing into pseudoana");
			else
				err = dba_error_odbc(SQL_HANDLE_STMT, stm, "preparing for inserting into pseudoana");
			goto cleanup;
		}
	}

	/* Insert or update the values in the database */

	if ((val = dba_record_key_peek_value(rec, DBA_KEY_HEIGHT)) != NULL)
	{
		height_ind = 0;
		height = strtol(val, 0, 10);
	} else
		height_ind = SQL_NULL_DATA;
	SQLBindParameter(stm, 4, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &height, 0, &height_ind);
	
	if ((val = dba_record_key_peek_value(rec, DBA_KEY_HEIGHTBARO)) != NULL)
	{
		heightbaro_ind = 0;
		heightbaro = strtol(val, 0, 10);
	} else
		heightbaro_ind = SQL_NULL_DATA;
	SQLBindParameter(stm, 5, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &heightbaro, 0, &heightbaro_ind);
	
	if ((val = dba_record_key_peek_value(rec, DBA_KEY_BLOCK)) != NULL)
	{
		block_ind = 0;
		block = strtol(val, 0, 10);
	} else
		block_ind = SQL_NULL_DATA;
	SQLBindParameter(stm, 6, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &block, 0, &block_ind);
	
	if ((val = dba_record_key_peek_value(rec, DBA_KEY_STATION)) != NULL)
	{
		station_ind = 0;
		station = strtol(val, 0, 10);
	} else
		station_ind = SQL_NULL_DATA;
	SQLBindParameter(stm, 7, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &station, 0, &station_ind);
	
	if ((name = dba_record_key_peek_value(rec, DBA_KEY_NAME)) != NULL)
		name_ind = SQL_NTS;
	else
	{
		name = "";
		name_ind = SQL_NULL_DATA;
	}
	/* Casting to char* because ODBC is unaware of const */
	SQLBindParameter(stm, 8, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)name, 0, &name_ind);

	if (*id != -1)
	{
		SQLBindParameter(stm, 9, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, id, 0, 0);
		/* Casting to char* because ODBC is unaware of const */
		res = SQLExecDirect(stm, (unsigned char*)query_repl, SQL_NTS);

		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm, "replacing old data in pseudoana");
			goto cleanup;
		}
	} else {
		/* Casting to char* because ODBC is unaware of const */
		res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);

		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into pseudoana");
			goto cleanup;
		}

		/* Get the ID of the last inserted pseudoana */
		DBA_RUN_OR_GOTO(cleanup, dba_last_insert_id(db->od_conn, id));
		
		DBA_RUN_OR_GOTO(cleanup, dba_record_key_seti(rec, DBA_KEY_ANA_ID, *id));
	}
	
	SQLFreeHandle(SQL_HANDLE_STMT, stm);

cleanup:
	return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err dba_insert_context(dba db, dba_record rec, int* id)
{
	const char* query_sel =
		"SELECT id FROM context WHERE id_ana=? AND id_report=? AND datetime=?"
		" AND ltype=? AND l1=? AND l2=?"
		" AND ptype=? AND p1=? AND p2=?";
	const char* query =
		"INSERT INTO context VALUES (NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
	int id_ana;
	int id_report;
	char datebuf[25];
	SQLINTEGER datebuf_ind;
	int ltype;
	int l1;
	int l2;
	int ptype;
	int p1;
	int p2;
	int old_id;
	SQLINTEGER old_id_ind;
	SQLHSTMT stm;
	int res;
	dba_err err;

	assert(db);

	/* Retrieve data */
	DBA_RUN_OR_GOTO(fail, dba_record_key_enqi(rec, DBA_KEY_ANA_ID, &id_ana));
	DBA_RUN_OR_GOTO(fail, dba_get_rep_cod(db, rec, &id_report));
	{
		const char *year, *month, *day, *hour, *min, *sec;
		/* Also input the seconds, defaulting to 0 if not found */
		sec = dba_record_key_peek_value(rec, DBA_KEY_SEC);
		/* Datetime needs to be computed */
		if ((year  = dba_record_key_peek_value(rec, DBA_KEY_YEAR)) != NULL &&
			(month = dba_record_key_peek_value(rec, DBA_KEY_MONTH)) != NULL &&
			(day   = dba_record_key_peek_value(rec, DBA_KEY_DAY)) != NULL &&
			(hour  = dba_record_key_peek_value(rec, DBA_KEY_HOUR)) != NULL &&
			(min   = dba_record_key_peek_value(rec, DBA_KEY_MIN)) != NULL)
			datebuf_ind = snprintf(datebuf, 30,
					"%04ld-%02ld-%02ld %02ld:%02ld:%02ld",
						strtol(year, 0, 10),
						strtol(month, 0, 10),
						strtol(day, 0, 10),
						strtol(hour, 0, 10),
						strtol(min, 0, 10),
						sec != NULL ? strtol(sec, 0, 10) : 0);
		else
		{
			err = dba_error_notfound("looking for datetime informations");
			goto fail;
		}
	}
	DBA_RUN_OR_GOTO(fail, dba_record_key_enqi(rec, DBA_KEY_LEVELTYPE, &ltype));
	DBA_RUN_OR_GOTO(fail, dba_record_key_enqi(rec, DBA_KEY_L1, &l1));
	DBA_RUN_OR_GOTO(fail, dba_record_key_enqi(rec, DBA_KEY_L2, &l2));
	DBA_RUN_OR_GOTO(fail, dba_record_key_enqi(rec, DBA_KEY_PINDICATOR, &ptype));
	DBA_RUN_OR_GOTO(fail, dba_record_key_enqi(rec, DBA_KEY_P1, &p1));
	DBA_RUN_OR_GOTO(fail, dba_record_key_enqi(rec, DBA_KEY_P2, &p2));

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

	/* Bind parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_ana, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_report, 0, 0);
	SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, &datebuf, 0, &datebuf_ind);
	SQLBindParameter(stm, 4, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &ltype, 0, 0);
	SQLBindParameter(stm, 5, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &l1, 0, 0);
	SQLBindParameter(stm, 6, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &l2, 0, 0);
	SQLBindParameter(stm, 7, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &ptype, 0, 0);
	SQLBindParameter(stm, 8, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &p1, 0, 0);
	SQLBindParameter(stm, 9, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &p2, 0, 0);

	/* Check for an existing context with these data */

	/* Bind variable and indicator for SELECT results */
	SQLBindCol(stm, 1, SQL_C_SLONG, &old_id, sizeof(old_id), &old_id_ind);
	
	/* Casting to char* because ODBC is unaware of const */
	res = SQLExecDirect(stm, (unsigned char*)query_sel, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "looking for existing context");
		goto fail;
	}

	/* If there is an existing record, use its ID and don't do an INSERT */
	if (SQLFetch(stm) != SQL_NO_DATA)
	{
		if (old_id_ind != sizeof(old_id))
		{
			err = dba_error_consistency("checking that the size of the ID coming from the database is correct");
			goto fail;
		}

		*id = old_id;

		SQLFreeHandle(SQL_HANDLE_STMT, stm);
		return dba_error_ok();
	}

	res = SQLCloseCursor(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "preparing for inserting into context");
		goto fail;
	}

	/* Casting to char* because ODBC is unaware of const */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into context");
		goto fail;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, stm);

	/* Get the ID of the last inserted levellayer */
	return dba_last_insert_id(db->od_conn, id);

fail:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err;
}

#ifdef DBA_USE_TRANSACTIONS
static dba_err dba_run_one_shot_query(SQLHDBC od_conn, const char* query)
{
	SQLHSTMT stm;
	int res;

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

	/* Casting to char* because ODBC is unaware of const */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		dba_err err = dba_error_odbc(SQL_HANDLE_STMT, stm, "Beginning a transaction");
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
		return err;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return dba_error_ok();
}

static dba_err dba_begin(SQLHDBC od_conn) { return dba_run_one_shot_query(od_conn, "BEGIN"); }
static dba_err dba_commit(SQLHDBC od_conn) { return dba_run_one_shot_query(od_conn, "COMMIT"); }
/* Run unchecked to avoid altering the error status */
static void dba_rollback(SQLHDBC od_conn)
{
	SQLHSTMT stm;
	int res;

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return;

	res = SQLExecDirect(stm, (unsigned char*)"ROLLBACK", 8);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return;

	SQLFreeHandle(SQL_HANDLE_STMT, stm);
}
#else
/* TODO: lock and unlock tables instead */
static dba_err dba_begin(SQLHDBC od_conn) { return dba_error_ok(); }
static dba_err dba_commit(SQLHDBC od_conn) { return dba_error_ok(); }
static void dba_rollback(SQLHDBC od_conn) {}
#endif

/*
 * If can_replace, then existing data can be rewritten, else it can only add new data
 *
 * If update_pseudoana, then the pseudoana informations are overwritten using
 * information from `rec'; else data from `rec' is written into pseudoana only
 * if there is no suitable anagraphical data for it.
 */
dba_err dba_insert_or_replace(dba db, dba_record rec, int can_replace, int update_pseudoana)
{
	/*
	 * FIXME: REPLACE will change the ID of the replaced rows, breaking the
	 * connection with the attributes.
	 * If on MySQL 4.1, we can use INSERT ON DUPLICATE KEY UPDATE value=?
	 * If on MySQL 4.1.1, we can use INSERT ON DUPLICATE KEY UPDATE value=VALUES(value)
	 * Else, we need to do a select first to get the ID.
	 */
	const char* insert_query =
		"INSERT INTO data (id_context, id_var, value)"
		" VALUES(?, ?, ?)";
	const char* replace_query =
		"REPLACE INTO data (id_context, id_var, value)"
		" VALUES(?, ?, ?)";
	dba_err err;
	dba_record_cursor item;
	int id_pseudoana;
	int id_context;
	dba_varcode id_var;
	char value[255];
	SQLINTEGER value_ind;
	SQLHSTMT stm;
	int res;
	
	assert(db);

	/* Check for the existance of non-context data, otherwise it's all useless */
	if (dba_record_iterate_first(rec) == NULL)
		return dba_error_consistency("looking for data to insert");

	/* Begin the transaction */
	DBA_RUN_OR_RETURN(dba_begin(db->od_conn));

	/* Insert the pseudoana data, and get the ID */
	DBA_RUN_OR_GOTO(failed1, dba_insert_pseudoana(db, rec, &id_pseudoana, update_pseudoana));

	/* Insert the context data, and get the ID */
	DBA_RUN_OR_GOTO(failed1, dba_insert_context(db, rec, &id_context));

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
		goto failed1;
	}

	/* Compile the SQL query */
	/* Casting to char* because ODBC is unaware of const */
	res = SQLPrepare(stm, (unsigned char*)(can_replace ? replace_query : insert_query), SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "compiling query to insert into 'data'");
		goto failed;
	}

	/* Bind parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_context, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &id_var, 0, 0);
	SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, value, 0, &value_ind);

	/* Insert all found variables */
	for (item = dba_record_iterate_first(rec); item != NULL;
			item = dba_record_iterate_next(rec, item))
	{
		/* Datum to be inserted, linked to id_pseudoana and all the other IDs */
		dba_var var = dba_record_cursor_variable(item);
		const char* cur_value;

		DBA_RUN_OR_GOTO(failed, dba_var_enqc(var, &cur_value));

		/* Variable ID */
		id_var = dba_var_code(var);

		/* Variable value */
		if ((value_ind = strlen(cur_value)) > 256)
			value_ind = 255;
		strncpy(value, cur_value, value_ind);
		value[value_ind] = 0;

		/*
		fprintf(stderr, "Inserting %d %s[%d] %s[%d] %d %d %d %d\n",
				id_var, value, value_ind, datebuf, datebuf_ind, id_report, id_pseudoana, id_levellayer, id_timerange);
		*/

		res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into 'data'");
			goto failed;
		}

		{
			int id;
			DBA_RUN_OR_GOTO(failed, dba_last_insert_id(db->od_conn, &id));
			dba_record_cursor_set_id(item, id);
		}
	}
			
	SQLFreeHandle(SQL_HANDLE_STMT, stm);

	DBA_RUN_OR_GOTO(failed1, dba_commit(db->od_conn));

	return dba_error_ok();

	/* Exits with cleanup after error */
failed:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
failed1:
	dba_rollback(db->od_conn);
	return err;
}

dba_err dba_insert(dba db, dba_record rec)
{
	return dba_insert_or_replace(db, rec, 1, 1);
}

dba_err dba_insert_new(dba db, dba_record rec)
{
	return dba_insert_or_replace(db, rec, 0, 0);
}

#if 0
dba_err dba_ana_count(dba db, int* count)
{
	SQLHSTMT stm;
	SQLINTEGER id_ind;
	int res;
	dba_err err;

	assert(db);

	/* Allocate statement handle for select */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement");

	/* Bind variable and indicator */
	SQLBindCol(stm, 1, SQL_C_SLONG, count, sizeof(*count), &id_ind);
	
	res = SQLExecDirect(stm, "SELECT COUNT(*) FROM pseudoana", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "querying number of entries in table 'pseudoana'");
		goto dba_ana_count_failed;
	}

	if (SQLFetch(stm) == SQL_NO_DATA)
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "retrieving number of entries in table 'pseudoana'");
		goto dba_ana_count_failed;
	}

	if (id_ind != sizeof(*count))
	{
		err = dba_error_consistency("checking that the size of the count coming from the database is correct");
		goto dba_ana_count_failed;
	}

	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return dba_error_ok();

dba_ana_count_failed:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err;
}
#endif

static dba_err dba_cursor_new(dba db, dba_cursor* cur)
{
	assert(db);
	*cur = (dba_cursor)calloc(1, sizeof(struct _dba_cursor));
	if (!*cur)
		return dba_error_alloc("trying to allocate a new dba_cursor object");
	(*cur)->db = db;
	return dba_error_ok();
}

dba_err dba_ana_query(dba db, dba_cursor* cur, int* count)
{
	const char* query =
		"SELECT pa.id, pa.lat, pa.lon, pa.ident, pa.height, pa.heightbaro,"
		"       pa.block, pa.station, pa.name"
		"  FROM pseudoana AS pa"
		" ORDER BY pa.id";
	dba_err err;
	SQLHSTMT stm;
	int res;

	assert(db);

	/* Allocate a new cursor */
	DBA_RUN_OR_RETURN(dba_cursor_new(db, cur));

	/* Setup the new cursor */
	(*cur)->type = ANA;

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		free(*cur);
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
	}

	(*cur)->stm = stm;
	/* (*cur)->out_rep_id = -1; */

	/* Bind output fields */
#define DBA_QUERY_BIND(num, type, name) \
	SQLBindCol(stm, num, type, &(*cur)->out_##name, sizeof((*cur)->out_##name), &(*cur)->out_##name##_ind);
	DBA_QUERY_BIND(1, SQL_C_SLONG, ana_id);
	DBA_QUERY_BIND(2, SQL_C_SLONG, lat);
	DBA_QUERY_BIND(3, SQL_C_SLONG, lon);
	DBA_QUERY_BIND(4, SQL_C_CHAR, ident);
	DBA_QUERY_BIND(5, SQL_C_SLONG, height);
	DBA_QUERY_BIND(6, SQL_C_SLONG, heightbaro);
	DBA_QUERY_BIND(7, SQL_C_SLONG, block);
	DBA_QUERY_BIND(8, SQL_C_SLONG, station);
	DBA_QUERY_BIND(9, SQL_C_CHAR, name);
#undef DBA_QUERY_BIND

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE ANA query \"%s\"", query);
		goto dba_ana_query_failed;
	}

	/* Get the number of affected rows */
	{
		SQLINTEGER ana_count;
		res = SQLRowCount(stm, &ana_count);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm, "getting row count");
			goto dba_ana_query_failed;
		}
		(*cur)->count = ana_count;
		*count = ana_count;
		/*fprintf(stderr, "COUNT: %d\n", count);*/
	}

	/* Retrieve results will happen in dba_cursor_next() */

	/* Done.  No need to deallocate the statement, it will be done by
	 * dba_cursor_delete */
	return dba_error_ok();

	/* Exit point with cleanup after error */
dba_ana_query_failed:
	dba_cursor_delete(*cur);
	*cur = 0;
	return err;
}

#define CHECKED_STORE(settype, var, key) \
	do{ \
		if (cur->out_##var##_ind != SQL_NULL_DATA) {\
			/*fprintf(stderr, "SETTING %s to %d\n", #var, cur->out_##var);*/ \
			DBA_RUN_OR_RETURN(dba_record_key_##settype(rec, key, cur->out_##var)); \
		} \
	} while (0)

static dba_err dba_ana_cursor_to_rec(dba_cursor cur, dba_record rec)
{
	assert(cur);
	assert(cur->db);

	/* Copy the resulting data into `rec' */
	CHECKED_STORE(seti, ana_id, DBA_KEY_ANA_ID);
	CHECKED_STORE(seti, lat, DBA_KEY_LAT);
	CHECKED_STORE(seti, lon, DBA_KEY_LON);
	if (cur->out_ident_ind != SQL_NULL_DATA && cur->out_ident[0] == 0)
	{
		CHECKED_STORE(setc, ident, DBA_KEY_IDENT);
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MOBILE, 1));
	}
	else
	{
		dba_record_key_unset(rec, DBA_KEY_IDENT);
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MOBILE, 0));
	}
	CHECKED_STORE(seti, height, DBA_KEY_HEIGHT);
	CHECKED_STORE(seti, heightbaro, DBA_KEY_HEIGHTBARO);
	CHECKED_STORE(seti, block, DBA_KEY_BLOCK);
	CHECKED_STORE(seti, station, DBA_KEY_STATION);
	CHECKED_STORE(setc, name, DBA_KEY_NAME);
	return dba_error_ok();
}

dba_err dba_ana_cursor_next(dba_cursor cur, dba_record rec, int* is_last)
{
	assert(cur);
	assert(cur->db);
	assert(cur->type == ANA);

	/* Fetch new data */
	if (SQLFetch(cur->stm) == SQL_NO_DATA)
	{
		*is_last = 1;
		return dba_error_notfound("retrieving a SQL query result (probably there are no more results to retrieve)");
	}

	/* Check if this is the last value */
	*is_last = --cur->count == 0;

	/* Empty the record from old data */
	dba_record_clear(rec);

	/* Store the data into the record */
	DBA_RUN_OR_RETURN(dba_ana_cursor_to_rec(cur, rec));

	return dba_error_ok();
}

static dba_err dba_prepare_select(dba db, dba_record rec, SQLHSTMT stm)
{
	int parm_num = 1;
	const char* val;

	/* Bind select fields */

#define PARM_INT(field, key, sql) do {\
	if ((val = dba_record_key_peek_value(rec, key)) != NULL) { \
		db->sel_##field = strtol(val, 0, 10); \
		TRACE("found " #field ": adding " sql ". val is %d\n", db->sel_##field); \
		strcat(db->querybuf, sql); \
		SQLBindParameter(stm, parm_num++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &db->sel_##field, 0, 0); \
	} } while (0)

	/* Set the time extremes */
	{
		int minvalues[6], maxvalues[6];
		DBA_RUN_OR_RETURN(dba_record_parse_date_extremes(rec, minvalues, maxvalues));
		
		if (minvalues[0] != -1)
		{
			/* Add constraint on the minimum date interval */
			snprintf(db->sel_dtmin, 25, "%04d-%02d-%02d %02d:%02d:%02d",
					minvalues[0], minvalues[1], minvalues[2],
					minvalues[3], minvalues[4], minvalues[5]);
			strcat(db->querybuf, " AND c.datetime >= ?");
			TRACE("found min time interval: adding AND c.datetime >= ?.  val is %s\n", db->sel_dtmin);
			SQLBindParameter(stm, parm_num++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)db->sel_dtmin, 0, 0);
		}
		
		if (maxvalues[0] != -1)
		{
			snprintf(db->sel_dtmax, 25, "%04d-%02d-%02d %02d:%02d:%02d",
					maxvalues[0], maxvalues[1], maxvalues[2],
					maxvalues[3], maxvalues[4], maxvalues[5]);
			strcat(db->querybuf, " AND c.datetime <= ?");
			TRACE("found max time interval: adding AND c.datetime <= ?.  val is %s\n", db->sel_dtmax);
			SQLBindParameter(stm, parm_num++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)db->sel_dtmax, 0, 0);
		}
	}

	PARM_INT(data_id, DBA_KEY_DATA_ID, " AND d.id = ?");
	PARM_INT(ana_id, DBA_KEY_ANA_ID, " AND pa.id = ?");
	PARM_INT(latmin, DBA_KEY_LATMIN, " AND pa.lat > ?");
	PARM_INT(latmax, DBA_KEY_LATMAX, " AND pa.lat < ?");
	PARM_INT(lonmin, DBA_KEY_LONMIN, " AND pa.lon > ?");
	PARM_INT(lonmax, DBA_KEY_LONMAX, " AND pa.lon < ?");

	{
		const char* mobile_sel = dba_record_key_peek_value(rec, DBA_KEY_MOBILE);

		if (mobile_sel != NULL)
		{
			if (mobile_sel[0] == '0')
			{
				strcat(db->querybuf, " AND pa.ident IS NULL");
				TRACE("found fixed/mobile: adding AND pa.ident IS NULL.\n");
			} else {
				strcat(db->querybuf, " AND NOT pa.ident IS NULL");
				TRACE("found fixed/mobile: adding AND NOT pa.ident IS NULL\n");
			}
		}
	}

	if ((db->sel_ident = dba_record_key_peek_value(rec, DBA_KEY_IDENT)) != NULL)
	{
		strcat(db->querybuf, " AND pa.ident = ?");
		TRACE("found ident: adding AND pa.ident = ?.  val is %s\n", db->sel_ident);
		SQLBindParameter(stm, parm_num++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)db->sel_ident, 0, 0);
	}

	PARM_INT(pindicator, DBA_KEY_PINDICATOR, " AND c.ptype = ?");
	PARM_INT(p1, DBA_KEY_P1, " AND c.p1 = ?");
	PARM_INT(p2, DBA_KEY_P2, " AND c.p2 = ?");
	PARM_INT(leveltype, DBA_KEY_LEVELTYPE, " AND c.ltype = ?");
	PARM_INT(l1, DBA_KEY_L1, " AND c.l1 = ?");
	PARM_INT(l2, DBA_KEY_L2, " AND c.l2 = ?");

	if ((val = dba_record_key_peek_value(rec, DBA_KEY_VAR)) != NULL)
	{
		db->sel_b = dba_descriptor_code(val);
		TRACE("found b: adding AND d.id_var = ?. val is %d %s\n", db->sel_b, val);
		strcat(db->querybuf, " AND d.id_var = ?");
		SQLBindParameter(stm, parm_num++, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &db->sel_b, 0, 0);
	}
	if ((val = dba_record_key_peek_value(rec, DBA_KEY_VARLIST)) != NULL)
	{
		size_t pos;
		size_t len;
		TRACE("found blist: adding AND d.id_var IN (%s)\n", val);
		strcat(db->querybuf, " AND d.id_var IN (");
		for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
		{
			dba_varcode code = DBA_STRING_TO_VAR(val + pos + 1);
			if (pos == 0)
				snprintf(db->querybuf + strlen(db->querybuf), 5, "%d", code);
			else
				snprintf(db->querybuf + strlen(db->querybuf), 6, ",%d", code);
		}
		strcat(db->querybuf, ")");
		/*
		strcat(db->querybuf, " AND d.id_var IN (?)");
		SQLBindParameter(stm, parm_num++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)db->sel_blist, 0, 0);
		*/
	}

	PARM_INT(rep_cod, DBA_KEY_REP_COD, " AND ri.id = ?");

	if ((db->sel_rep_memo = dba_record_key_peek_value(rec, DBA_KEY_REP_MEMO)) != NULL)
	{
		strcat(db->querybuf, " AND ri.memo = ?");
		TRACE("found rep_memo: adding AND ri.memo = ?.  val is %s\n", db->sel_rep_memo);
		SQLBindParameter(stm, parm_num++, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, (char*)db->sel_rep_memo, 0, 0);
	}

	PARM_INT(priority, DBA_KEY_PRIORITY, " AND ri.prio = ?");
	PARM_INT(priomin, DBA_KEY_PRIOMIN, " AND ri.prio >= ?");
	PARM_INT(priomax, DBA_KEY_PRIOMAX, " AND ri.prio <= ?");
	PARM_INT(block, DBA_KEY_BLOCK, " AND pa.block = ?");
	PARM_INT(station, DBA_KEY_STATION, " AND pa.station = ?");

	return dba_error_ok();

#undef PARM_INT
}


dba_err dba_query(dba db, dba_record rec, dba_cursor* cur, int* count)
{
	const char* query =
		"SELECT pa.id, pa.lat, pa.lon, pa.ident, pa.height, pa.heightbaro,"
		"       pa.block, pa.station, pa.name,"
		"       c.ltype, c.l1, c.l2,"
		"       c.ptype, c.p1, c.p2,"
		"       d.id_var, c.datetime, d.value, c.id_report, ri.memo, ri.prio, d.id"
		"  FROM pseudoana AS pa, context AS c, data AS d, repinfo AS ri"
		" WHERE d.id_context = c.id AND c.id_ana = pa.id AND c.id_report = ri.id";
	dba_err err;
	SQLHSTMT stm;
	int res;

	assert(db);

	/* Allocate a new cursor */
	DBA_RUN_OR_RETURN(dba_cursor_new(db, cur));

	/* Setup the new cursor */
	(*cur)->type = DATA;

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		free(cur);
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
	}

	(*cur)->stm = stm;

	/* Write the SQL query */

	/* Initial query */
	strcpy(db->querybuf, query);

	/* Bind output fields */
#define DBA_QUERY_BIND(num, type, name) \
	SQLBindCol(stm, num, type, &(*cur)->out_##name, sizeof((*cur)->out_##name), &(*cur)->out_##name##_ind);
	DBA_QUERY_BIND( 1, SQL_C_SLONG, ana_id);
	DBA_QUERY_BIND( 2, SQL_C_SLONG, lat);
	DBA_QUERY_BIND( 3, SQL_C_SLONG, lon);
	DBA_QUERY_BIND( 4, SQL_C_CHAR, ident);
	DBA_QUERY_BIND( 5, SQL_C_SLONG, height);
	DBA_QUERY_BIND( 6, SQL_C_SLONG, heightbaro);
	DBA_QUERY_BIND( 7, SQL_C_SLONG, block);
	DBA_QUERY_BIND( 8, SQL_C_SLONG, station);
	DBA_QUERY_BIND( 9, SQL_C_CHAR, name);
	DBA_QUERY_BIND(10, SQL_C_SLONG, leveltype);
	DBA_QUERY_BIND(11, SQL_C_SLONG, l1);
	DBA_QUERY_BIND(12, SQL_C_SLONG, l2);
	DBA_QUERY_BIND(13, SQL_C_SLONG, pindicator);
	DBA_QUERY_BIND(14, SQL_C_SLONG, p1);
	DBA_QUERY_BIND(15, SQL_C_SLONG, p2);
	DBA_QUERY_BIND(16, SQL_C_SLONG, idvar);
	DBA_QUERY_BIND(17, SQL_C_CHAR, datetime);
	DBA_QUERY_BIND(18, SQL_C_CHAR, value);
	/* DBA_QUERY_BIND(21, SQL_C_SLONG, rep_id); */
	DBA_QUERY_BIND(19, SQL_C_SLONG, rep_cod);
	DBA_QUERY_BIND(20, SQL_C_CHAR, rep_memo);
	DBA_QUERY_BIND(21, SQL_C_SLONG, priority);
	DBA_QUERY_BIND(22, SQL_C_SLONG, data_id);
#undef DBA_QUERY_BIND
	
	/* Add the select part */
	DBA_RUN_OR_GOTO(failed, dba_prepare_select(db, rec, stm));

	if (dba_record_key_peek_value(rec, DBA_KEY_QUERYBEST) != NULL)
		strcat(db->querybuf,
			" GROUP BY d.id_var, c.id_ana, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2, c.datetime"
			" HAVING ri.prio=MAX(ri.prio) ORDER BY c.id_ana, c.datetime, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2");
	else
		strcat(db->querybuf, " ORDER BY c.id_ana, c.datetime, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2, ri.prio");

/* 	strcat(db->querybuf, " ORDER BY ri.prio, d.id_report"); */
/* fprintf(stderr, "QUERY: %s\n", db->querybuf); */

	TRACE("Performing query: %s\n", db->querybuf);

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)db->querybuf, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", db->querybuf);
		goto failed;
	}

	/* Get the number of affected rows */
	{
		SQLINTEGER rowcount;
		res = SQLRowCount(stm, &rowcount);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm, "getting row count");
			goto failed;
		}
		(*cur)->count = *count = rowcount;
	}

	/* Retrieve results will happen in dba_cursor_next() */

	/* Done.  No need to deallocate the statement, it will be done by
	 * dba_cursor_delete */
	return dba_error_ok();

	/* Exit point with cleanup after error */
failed:
	dba_cursor_delete(*cur);
	*cur = 0;
	return err;
}


static dba_err dba_cursor_var_to_rec(dba_cursor cur, dba_record rec)
{
	assert(cur);
	assert(cur->db);
	/*assert(cur->out_rep_id != -1); */
	{
		char bname[7];
		snprintf(bname, 7, "B%02d%03d",
					DBA_VAR_X(cur->out_idvar),
					DBA_VAR_Y(cur->out_idvar));
		DBA_RUN_OR_RETURN(dba_record_key_setc(rec, DBA_KEY_VAR, bname));
		DBA_RUN_OR_RETURN(dba_record_var_setc(rec, cur->out_idvar, cur->out_value));
		DBA_RUN_OR_RETURN(dba_record_var_setid(rec, cur->out_idvar, cur->out_data_id));
	}
	return dba_error_ok();
}
static dba_err dba_cursor_to_rec(dba_cursor cur, dba_record rec)
{
	assert(cur);
	assert(cur->db);
	/* assert(cur->out_rep_id != -1); */

	/* Copy the resulting data into `rec' */
	DBA_RUN_OR_RETURN(dba_ana_cursor_to_rec(cur, rec));

	CHECKED_STORE(seti, leveltype, DBA_KEY_LEVELTYPE);
	CHECKED_STORE(seti, l1, DBA_KEY_L1);
	CHECKED_STORE(seti, l2, DBA_KEY_L2);
	CHECKED_STORE(seti, pindicator, DBA_KEY_PINDICATOR);
	CHECKED_STORE(seti, p1, DBA_KEY_P1);
	CHECKED_STORE(seti, p2, DBA_KEY_P2);
	if (cur->out_datetime_ind != SQL_NULL_DATA)
	{
		/*fprintf(stderr, "SETTING %s to %d\n", #var,  _db_cursor[cur].out_##var); */
		int year, mon, day, hour, min, sec;
		if (sscanf(cur->out_datetime,
					"%04d-%02d-%02d %02d:%02d:%02d", &year, &mon, &day, &hour, &min, &sec) != 6)
			return dba_error_consistency("parsing datetime string \"%s\"", cur->out_datetime);

		DBA_RUN_OR_RETURN(dba_record_key_setc(rec, DBA_KEY_DATETIME, cur->out_datetime));
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_YEAR, year));
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MONTH, mon));
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_DAY, day));
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_HOUR, hour));
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_MIN, min));
		DBA_RUN_OR_RETURN(dba_record_key_seti(rec, DBA_KEY_SEC, sec));
	} 
	/* CHECKED_STORE(seti, rep_id); */
	CHECKED_STORE(seti, rep_cod, DBA_KEY_REP_COD);
	CHECKED_STORE(setc, rep_memo, DBA_KEY_REP_MEMO);
	CHECKED_STORE(seti, priority, DBA_KEY_PRIORITY);
	CHECKED_STORE(seti, data_id, DBA_KEY_DATA_ID);
	DBA_RUN_OR_RETURN(dba_cursor_var_to_rec(cur, rec));
	return dba_error_ok();
}

dba_err dba_cursor_next(dba_cursor cur, dba_record rec, dba_varcode* var, int* is_last)
{
	assert(cur);
	assert(cur->db);

	assert(cur->type == DATA);

	/* Fetch the first row */
	if (SQLFetch(cur->stm) == SQL_NO_DATA)
	{
		*is_last = 1;
		return dba_error_notfound("retrieving a SQL query result (probably there are no more results to retrieve)");
	}

	/* Empty the record from old data */
	dba_record_clear(rec);

	/* Store the data into the record */
	DBA_RUN_OR_RETURN(dba_cursor_to_rec(cur, rec));

	/* Store the database ID together with the value */
	DBA_RUN_OR_RETURN(dba_record_var_setid(rec, cur->out_idvar, cur->out_data_id));

	*is_last = --cur->count == 0;

	/* Store the variable ID */
	*var = cur->out_idvar;

	return dba_error_ok();
}
#undef CHECKED_STORE

void dba_cursor_delete(dba_cursor cur)
{
	assert(cur);
	assert(cur->db);

	SQLFreeHandle(SQL_HANDLE_STMT, cur->stm);
	free(cur);
}

#ifdef DBA_USE_DELETE_USING
dba_err dba_delete(dba db, dba_record rec)
{
	const char* query =
		"DELETE FROM d, a"
		" USING pseudoana AS pa, context AS c, repinfo AS ri, data AS d"
		"  LEFT JOIN attr AS a ON a.id_data = d.id"
		" WHERE d.id_context = c.id AND c.id_ana = pa.id AND c.id_report = ri.id";
	dba_err err;
	SQLHSTMT stm;
	int res;

	assert(db);

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
	}

	/* Write the SQL query */

	/* Initial query */
	strcpy(db->querybuf, query);

	/* Bind select fields */
	if ((err = dba_prepare_select(db, rec, stm)) != DBA_OK)
		goto dba_delete_failed;

	/*fprintf(stderr, "QUERY: %s\n", db->querybuf);*/

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)db->querybuf, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", db->querybuf);
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
dba_err dba_delete(dba db, dba_record rec)
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
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
	}
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm1);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
		return dba_error_odbc(SQL_HANDLE_STMT, stm1, "Allocating new statement handle");
	}
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm2);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		SQLFreeHandle(SQL_HANDLE_STMT, stm);
		SQLFreeHandle(SQL_HANDLE_STMT, stm1);
		return dba_error_odbc(SQL_HANDLE_STMT, stm2, "Allocating new statement handle");
	}

	/* Write the SQL query */

	/* Initial query */
	strcpy(db->querybuf, query);

	/* Bind select fields */
	DBA_RUN_OR_GOTO(cleanup, dba_prepare_select(db, rec, stm));

	/* Bind output field */
	SQLBindCol(stm, 1, SQL_C_SLONG, &id, sizeof(id), NULL);

	/*fprintf(stderr, "QUERY: %s\n", db->querybuf);*/

	/* Perform the query */
	TRACE("Performing query %s\n", db->querybuf);
	res = SQLExecDirect(stm, (unsigned char*)db->querybuf, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", db->querybuf);
		goto cleanup;
	}

	/* Compile the DELETE query for the data */
	res = SQLPrepare(stm1, (unsigned char*)"DELETE FROM data WHERE id=?", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm1, "compiling query to delete data entries");
		goto cleanup;
	}
	/* Bind parameters */
	SQLBindParameter(stm1, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id, 0, 0);

	/* Compile the DELETE query for the associated QC */
	res = SQLPrepare(stm2, (unsigned char*)"DELETE FROM attr WHERE id_data=?", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm2, "compiling query to delete entries related to QC data");
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
			err = dba_error_odbc(SQL_HANDLE_STMT, stm1, "deleting entry %d from the 'data' table", id);
			goto cleanup;
		}
		res = SQLExecute(stm2);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm2, "deleting QC data related to 'data' entry %d", id);
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

dba_err dba_qc_query(dba db, int id_data, dba_varcode* qcs, int qcs_size, dba_record qc, int* count)
{
	char query[100 + 100*6];
	SQLHSTMT stm;
	dba_err err;
	int res;
	int out_type;
	const char out_value[255];

	assert(db);

	/* Create the query */
	if (qcs == NULL)
		/* If qcs is null, query all QC data */
		strcpy(query,
				"SELECT a.type, a.value"
				"  FROM attr AS a, data AS d"
				" WHERE d.id = ? AND a.id_data = d.id");
	else {
		int i, qs;
		char* q;
		if (qcs_size > 100)
			return dba_error_consistency("checking bound of 100 QC values to retrieve per query");
		strcpy(query,
				"SELECT a.type, a.value"
				"  FROM attr AS a, data AS d"
				" WHERE d.id = ? AND a.id_data = d.id AND a.type IN (");
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
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

	/* Bind input parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_data, 0, 0);

	/* Bind output fields */
	SQLBindCol(stm, 1, SQL_C_SLONG, &out_type, sizeof(out_type), 0);
	SQLBindCol(stm, 2, SQL_C_CHAR, &out_value, sizeof(out_value), 0);
	
	TRACE("QC read query: %s with id_data %d\n", query, id_data);

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", query);
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

dba_err dba_qc_insert_or_replace(dba db, int id_data, /*dba_record rec, dba_varcode var,*/ dba_record qc, int can_replace)
{
	const char* insert_query =
		"INSERT INTO attr (id_data, type, value)"
		" VALUES(?, ?, ?)";
	const char* replace_query =
		"REPLACE INTO attr (id_data, type, value)"
		" VALUES(?, ?, ?)";
	dba_err err;
	dba_record_cursor item;
#if 0
	int id_data;
#endif
	dba_varcode type;
	char value[255];
	SQLINTEGER value_ind;
	SQLHSTMT stm;
	int res;
	
	assert(db);

	/* Begin the transaction */
	DBA_RUN_OR_RETURN(dba_begin(db->od_conn));

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		dba_rollback(db->od_conn);
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
	}

	/* Compile the INSERT/UPDATE SQL query */
	/* Casting to char* because ODBC is unaware of const */
	res = SQLPrepare(stm, (unsigned char*)(can_replace ? replace_query : insert_query), SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "compiling query to insert into 'attr'");
		goto dba_qc_insert_failed;
	}

	/* Bind parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_data, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &type, 0, 0);
	SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, value, 0, &value_ind);
	
	/* Insert all found variables */
	for (item = dba_record_iterate_first(qc); item != NULL;
			item = dba_record_iterate_next(qc, item))
	{
		dba_var variable = dba_record_cursor_variable(item);
		const char* cur_value = dba_var_value(variable);

		/* Variable ID */
		type = dba_var_code(variable);

		/* Variable value */
		if ((value_ind = strlen(cur_value)) > 256)
			value_ind = 255;
		strncpy(value, cur_value, value_ind);
		value[value_ind] = 0;

		res = SQLExecute(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into 'attr'");
			goto dba_qc_insert_failed;
		}
	}
			
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	if ((err = dba_commit(db->od_conn)))
	{
		dba_rollback(db->od_conn);
		return err;
	}
	return dba_error_ok();

	/* Exits with cleanup after error */
dba_qc_insert_failed:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	dba_rollback(db->od_conn);
	return err;
}

dba_err dba_qc_insert(dba db, int id_data, /*dba_record rec, dba_varcode var,*/ dba_record qc)
{
	return dba_qc_insert_or_replace(db, id_data, /*rec, var,*/ qc, 1);
}

dba_err dba_qc_insert_new(dba db, int id_data, /*dba_record rec, dba_varcode var,*/ dba_record qc)
{
	return dba_qc_insert_or_replace(db, id_data, /*rec, var,*/ qc, 0);
}

dba_err dba_qc_delete(dba db, int id_data, dba_varcode* qcs, int qcs_size)
{
	char query[60 + 100*6];
	SQLHSTMT stm;
	dba_err err;
	int res;

	assert(db);

	// Create the query
	if (qcs == NULL)
		strcpy(query, "DELETE FROM attr WHERE id_data = ?");
	else {
		int i, qs;
		char* q;
		if (qcs_size > 100)
			return dba_error_consistency("checking bound of 100 QC values to delete per query");
		strcpy(query, "DELETE FROM attr WHERE id_data = ? AND type IN (");
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
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

	/* Bind parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_data, 0, 0);

	TRACE("Performing query %s for id %d\n", query, id_data);
	
	/* Execute the DELETE SQL query */
	/* Casting to char* because ODBC is unaware of const */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_error_odbc(SQL_HANDLE_STMT, stm, "deleting data from 'attr'");
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
