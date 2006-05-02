#include <dballe/db/dba_db.h>
#include <dballe/db/internals.h>
#include <dballe/db/pseudoana.h>
#include <dballe/db/context.h>
#include <dballe/db/data.h>
#include <dballe/db/attr.h>
#include <dballe/core/dba_record.h>
#include <dballe/core/dba_var.h>
#include <dballe/core/csv.h>
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

struct _dba_db_cursor {
	dba_db db;
	enum { ANA, DATA, QC } type;
	SQLHSTMT stm;

	int count;

	/* Bound variables */
#define DB_CUR_VAR(type, name) type out_##name; SQLINTEGER out_##name##_ind
#define DB_CUR_CHARVAR(name, len) char out_##name[len]; SQLINTEGER out_##name##_ind
	DB_CUR_VAR(int, lat);
	DB_CUR_VAR(int, lon);
	DB_CUR_CHARVAR(ident, 64);
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
	"   UNIQUE INDEX(lat, lon, ident(8))"
	") " TABLETYPE,
	"CREATE TABLE data ("
	"   id_context	INTEGER NOT NULL,"
	"	id_var		SMALLINT NOT NULL,"
	"	value		VARCHAR(255) NOT NULL,"
	"	INDEX (id_context),"
	"   UNIQUE INDEX(id_var, id_context)"
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
	") " TABLETYPE,
	"CREATE TABLE attr ("
	"   id_context	INTEGER NOT NULL,"
	"	id_var		SMALLINT NOT NULL,"
	"   type		SMALLINT NOT NULL,"
	"   value		VARCHAR(255) NOT NULL,"
	"   INDEX (id_context, id_var),"
	"   UNIQUE INDEX (id_context, id_var, type)"
	") " TABLETYPE,
};


dba_err dba_db_init()
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

	return dba_error_ok();
}

void dba_db_shutdown()
{
	SQLFreeHandle(SQL_HANDLE_ENV, dba_od_env);
}

dba_err dba_db_create(const char* dsn, const char* user, const char* password, dba_db* db)
{
	dba_err err = DBA_OK;
	int sqlres;

	/* Allocate a new handle */
	if ((*db = (dba_db)calloc(1, sizeof(struct _dba_db))) == NULL)
		return dba_error_alloc("trying to allocate a new dba_db object");

	/*
	 * This is very conservative:
	 * The query size plus 30 possible select values, maximum of 30 characters each
	 * plus 400 characters for the various combinations of the two min and max datetimes,
	 * plus 255 for the blist
	 */
	DBA_RUN_OR_GOTO(fail, dba_querybuf_create(170 + 30*30 + 400 + 255, &((*db)->querybuf)));

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
	sqlres = SQLPrepare((*db)->stm_last_insert_id, (unsigned char*)"SELECT LAST_INSERT_ID()", SQL_NTS);
	if ((sqlres != SQL_SUCCESS) && (sqlres != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, (*db)->stm_last_insert_id, "compiling query for querying the last insert id");
		goto fail;
	}

	DBA_RUN_OR_GOTO(fail, dba_db_pseudoana_create(*db, &((*db)->pseudoana)));
	DBA_RUN_OR_GOTO(fail, dba_db_context_create(*db, &((*db)->context)));
	DBA_RUN_OR_GOTO(fail, dba_db_data_create(*db, &((*db)->data)));
	DBA_RUN_OR_GOTO(fail, dba_db_attr_create(*db, &((*db)->attr)));

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
	if (db->querybuf != NULL)
		dba_querybuf_delete(db->querybuf);
	if (db->od_conn != NULL)
	{
		if (db->connected)
			SQLDisconnect(db->od_conn);
		SQLFreeHandle(SQL_HANDLE_DBC, db->od_conn);
	}
	free(db);
}

dba_err dba_db_reset(dba_db db, const char* deffile)
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
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
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
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm,
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
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm,
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
				err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into 'data'");
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

/*
 * Insert or replace data in pseudoana taking the values from rec.
 * If rec did not contain ana_id, it will be set by this function.
 */
static dba_err dba_insert_pseudoana(dba_db db, dba_record rec, int* id, int rewrite)
{
	dba_err err = DBA_OK;
	int mobile;
	const char* val;
	dba_db_pseudoana a;

	assert(db);
	a = db->pseudoana;

	/* Look for an existing ID if provided */
	if ((val = dba_record_key_peek_value(rec, DBA_KEY_ANA_ID)) != NULL)
		*id = strtol(val, 0, 10);
	else
		*id = -1;

	/* If we don't need to rewrite, we are done */
	if (*id != -1 && !rewrite)
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

		/* If we don't have to update the data, we're done */
		if (*id != -1 && !rewrite)
			goto cleanup;
	}

	/* Insert or update the values in the database */

	if (*id != -1)
	{
		a->id = *id;
		DBA_RUN_OR_GOTO(cleanup, dba_db_pseudoana_update(a));
	} else {
		DBA_RUN_OR_GOTO(cleanup, dba_db_pseudoana_insert(a, id));
	}

	/* DBA_RUN_OR_GOTO(cleanup, update_pseudoana_extra_info(db, rec, *id)); */
	
cleanup:
	return err == DBA_OK ? dba_error_ok() : err;
}

static dba_err dba_insert_context(dba_db db, dba_record rec, int id_ana, int* id)
{
	const char *year, *month, *day, *hour, *min, *sec;
	dba_db_context c;
	assert(db);
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
dba_err dba_db_insert_or_replace(dba_db db, dba_record rec, int can_replace, int update_pseudoana, int* ana_id, int* context_id)
{
	dba_err err = DBA_OK;
	dba_db_data d;
	dba_record_cursor item;
	int id_pseudoana;
	
	assert(db);
	d = db->data;

	/* Check for the existance of non-context data, otherwise it's all useless */
	if (dba_record_iterate_first(rec) == NULL)
		return dba_error_consistency("looking for data to insert");

	/* Begin the transaction */
	DBA_RUN_OR_RETURN(dba_db_begin(db));

	/* Insert the pseudoana data, and get the ID */
	DBA_RUN_OR_GOTO(fail, dba_insert_pseudoana(db, rec, &id_pseudoana, update_pseudoana));

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

dba_err dba_db_insert(dba_db db, dba_record rec)
{
	return dba_db_insert_or_replace(db, rec, 1, 1, NULL, NULL);
}

dba_err dba_db_insert_new(dba_db db, dba_record rec)
{
	return dba_db_insert_or_replace(db, rec, 0, 0, NULL, NULL);
}

static dba_err dba_db_cursor_new(dba_db db, dba_db_cursor* cur)
{
	assert(db);
	*cur = (dba_db_cursor)calloc(1, sizeof(struct _dba_db_cursor));
	if (!*cur)
		return dba_error_alloc("trying to allocate a new dba_db_cursor object");
	(*cur)->db = db;
	return dba_error_ok();
}

dba_err dba_db_ana_query(dba_db db, dba_db_cursor* cur, int* count)
{
	const char* query =
		"SELECT pa.id, pa.lat, pa.lon, pa.ident"
		"  FROM pseudoana AS pa"
		" ORDER BY pa.id";
	dba_err err;
	SQLHSTMT stm;
	int res;

	assert(db);

	/* Allocate a new cursor */
	DBA_RUN_OR_RETURN(dba_db_cursor_new(db, cur));

	/* Setup the new cursor */
	(*cur)->type = ANA;

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		free(*cur);
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
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
#undef DBA_QUERY_BIND

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE ANA query \"%s\"", query);
		goto dba_ana_query_failed;
	}

	/* Get the number of affected rows */
	{
		SQLINTEGER ana_count;
		res = SQLRowCount(stm, &ana_count);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "getting row count");
			goto dba_ana_query_failed;
		}
		(*cur)->count = ana_count;
		*count = ana_count;
		/*fprintf(stderr, "COUNT: %d\n", count);*/
	}

	/* Retrieve results will happen in dba_db_cursor_next() */

	/* Done.  No need to deallocate the statement, it will be done by
	 * dba_db_cursor_delete */
	return dba_error_ok();

	/* Exit point with cleanup after error */
dba_ana_query_failed:
	dba_db_cursor_delete(*cur);
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

static dba_err dba_ana_cursor_to_rec(dba_db_cursor cur, dba_record rec)
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
	return dba_error_ok();
}

static dba_err dba_ana_add_extra(dba_db_cursor cur, dba_record rec)
{
	/* Extra variables to add:
	 *
	 * HEIGHT,      B07001  1793
	 * HEIGHT_BARO, B07031  1823
	 * ST_NAME,     B01019   275
	 * BLOCK,       B01001   257
	 * STATION,     B01002   258
	*/
	const char* query =
		"SELECT d.id_var, d.value"
		"  FROM context AS c, data AS d"
		" WHERE c.id = d.id_context AND c.id_ana = ?"
		"   AND c.datetime = '1000-01-01 00:00:00'"
		"   AND c.id_report = 254"
		"   AND c.ltype = 257 AND c.l1 = 0 AND c.l2 = 0"
		"   AND c.ptype = 0 AND c.p1 = 0 AND c.p2 = 0"
		"   AND d.id_var IN (257, 258, 275, 1793, 1823)";

	dba_err err = DBA_OK;
	SQLHSTMT stm;
	int res;
	dba_db db = cur->db;
	dba_varcode out_code;
	char out_val[256];
	SQLINTEGER out_val_ind;

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

	/* Bind input fields */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(cur->out_ana_id), 0, 0);

	/* Bind output fields */
	SQLBindCol(stm, 1, SQL_C_USHORT, &out_code, sizeof(out_code), 0);
	SQLBindCol(stm, 2, SQL_C_CHAR, &out_val, sizeof(out_val), &out_val_ind);

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)query, SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE ANA extra query \"%s\"", query);
		goto cleanup;
	}

	/* Get the results and save them in the record */
	while (SQLFetch(stm) != SQL_NO_DATA)
	{
		switch (out_code)
		{
			case DBA_VAR(0, 1,  1): // BLOCK
				DBA_RUN_OR_GOTO(cleanup, dba_record_key_setc(rec, DBA_KEY_BLOCK, out_val));
				break;
			case DBA_VAR(0, 1,  2): // STATION
				DBA_RUN_OR_GOTO(cleanup, dba_record_key_setc(rec, DBA_KEY_STATION, out_val));
				break;
			case DBA_VAR(0, 1, 19): // NAME
				DBA_RUN_OR_GOTO(cleanup, dba_record_key_setc(rec, DBA_KEY_NAME, out_val));
				break;
			case DBA_VAR(0, 7,  1): // HEIGHT
				DBA_RUN_OR_GOTO(cleanup, dba_record_key_setc(rec, DBA_KEY_HEIGHT, out_val));
				break;
			case DBA_VAR(0, 7, 31): // HEIGHTBARO
				DBA_RUN_OR_GOTO(cleanup, dba_record_key_setc(rec, DBA_KEY_HEIGHTBARO, out_val));
				break;
		}
	}

cleanup:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_ana_cursor_next(dba_db_cursor cur, dba_record rec, int* is_last)
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

	/* Add the extra ana info */
	DBA_RUN_OR_RETURN(dba_ana_add_extra(cur, rec));

	return dba_error_ok();
}

dba_err dba_db_query(dba_db db, dba_record rec, dba_db_cursor* cur, int* count)
{
	const char* query =
		"SELECT pa.id, pa.lat, pa.lon, pa.ident,"
		"       c.ltype, c.l1, c.l2,"
		"       c.ptype, c.p1, c.p2,"
		"       d.id_var, c.datetime, d.value, ri.id, ri.memo, ri.prio"
		"  FROM context AS c"
		"  JOIN pseudoana AS pa ON c.id_ana = pa.id"
		"  JOIN data AS d ON d.id_context = c.id"
		"  JOIN repinfo AS ri ON c.id_report = ri.id";
		/*
SELECT  count(*)
 WHERE c.datetime = '2005-08-03 06:00:00'
   AND c.ptype = 4 AND c.p1 = 64800 AND c.p2 = 151200
   AND d.id_var = 3339
   AND ri.memo = 'clepsspnpoel001'
 ORDER BY c.id_ana, c.datetime, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2, ri.prio
 		*/
	
	dba_err err;
	SQLHSTMT stm;
	int res;
	int pseq = 1;
	const char* val;
	int has_bs = 0;

	assert(db);

	/* Allocate a new cursor */
	DBA_RUN_OR_RETURN(dba_db_cursor_new(db, cur));

	/* Setup the new cursor */
	(*cur)->type = DATA;

	/* Allocate statement handle */
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		free(cur);
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
	}

	(*cur)->stm = stm;

	/* Write the SQL query */

	/* Initial query */
	dba_querybuf_reset(db->querybuf);
	DBA_RUN_OR_RETURN(dba_querybuf_append(db->querybuf, query));

	/* Extend the JOIN part to look for given block and station */
	if ((has_bs = (dba_record_key_peek_value(rec, DBA_KEY_BLOCK) != NULL)
		  || (dba_record_key_peek_value(rec, DBA_KEY_STATION) != NULL)))
	{
		DBA_RUN_OR_GOTO(failed, dba_querybuf_append(db->querybuf, 
					" JOIN context AS cbs ON "
					"     c.id_ana = cbs.id_ana"
					" AND cbs.id_report = 254"
					" AND cbs.datetime = '1000-01-01 00:00:00'"
					" AND cbs.ltype = 257 AND cbs.l1 = 0 AND cbs.l2 = 0"
					" AND cbs.ptype = 0 AND cbs.p1 = 0 AND cbs.p2 = 0"));
		if ((val = dba_record_key_peek_value(rec, DBA_KEY_BLOCK)) != NULL)
		{
			DBA_RUN_OR_GOTO(failed, dba_querybuf_append(db->querybuf,
					" JOIN data AS dblo ON dblo.id_context = cbs.id AND dblo.id_var = 257 AND dblo.value = "));
			DBA_RUN_OR_GOTO(failed, dba_querybuf_append(db->querybuf, val));
		}
		if ((val = dba_record_key_peek_value(rec, DBA_KEY_STATION)) != NULL)
		{
			DBA_RUN_OR_GOTO(failed, dba_querybuf_append(db->querybuf,
					" JOIN data AS dsta ON dsta.id_context = cbs.id AND dsta.id_var = 258 AND dsta.value = "));
			DBA_RUN_OR_GOTO(failed, dba_querybuf_append(db->querybuf, val));
		}
	}

	/* Add WHERE part */
	DBA_RUN_OR_GOTO(failed, dba_querybuf_append(db->querybuf,
		" WHERE true "));

	/* Bind output fields */
#define DBA_QUERY_BIND(num, type, name) \
	SQLBindCol(stm, num, type, &(*cur)->out_##name, sizeof((*cur)->out_##name), &(*cur)->out_##name##_ind);
	DBA_QUERY_BIND( 1, SQL_C_SLONG, ana_id);
	DBA_QUERY_BIND( 2, SQL_C_SLONG, lat);
	DBA_QUERY_BIND( 3, SQL_C_SLONG, lon);
	DBA_QUERY_BIND( 4, SQL_C_CHAR, ident);
	DBA_QUERY_BIND( 5, SQL_C_SLONG, leveltype);
	DBA_QUERY_BIND( 6, SQL_C_SLONG, l1);
	DBA_QUERY_BIND( 7, SQL_C_SLONG, l2);
	DBA_QUERY_BIND( 8, SQL_C_SLONG, pindicator);
	DBA_QUERY_BIND( 9, SQL_C_SLONG, p1);
	DBA_QUERY_BIND(10, SQL_C_SLONG, p2);
	DBA_QUERY_BIND(11, SQL_C_SLONG, idvar);
	DBA_QUERY_BIND(12, SQL_C_CHAR, datetime);
	DBA_QUERY_BIND(13, SQL_C_CHAR, value);
	/* DBA_QUERY_BIND(21, SQL_C_SLONG, rep_id); */
	DBA_QUERY_BIND(14, SQL_C_SLONG, rep_cod);
	DBA_QUERY_BIND(15, SQL_C_CHAR, rep_memo);
	DBA_QUERY_BIND(16, SQL_C_SLONG, priority);
#undef DBA_QUERY_BIND
	
	/* Add the select part */
	DBA_RUN_OR_GOTO(failed, dba_db_prepare_select(db, rec, stm, &pseq));

	if (dba_record_key_peek_value(rec, DBA_KEY_QUERYBEST) != NULL)
		DBA_RUN_OR_GOTO(failed, dba_querybuf_append(db->querybuf,
			" GROUP BY d.id_var, c.id_ana, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2, c.datetime"
			" HAVING ri.prio=MAX(ri.prio)"
			" ORDER BY c.id_ana, c.datetime, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2"));
	else
		DBA_RUN_OR_GOTO(failed, dba_querybuf_append(db->querybuf,
			" ORDER BY c.id_ana, c.datetime, c.ltype, c.l1, c.l2, c.ptype, c.p1, c.p2, ri.prio"));

/* 	strcat(db->querybuf, " ORDER BY ri.prio, d.id_report"); */
/* fprintf(stderr, "QUERY: %s\n", db->querybuf); */

	TRACE("Performing query: %s\n", dba_querybuf_get(db->querybuf));

	/* Perform the query */
	res = SQLExecDirect(stm, (unsigned char*)dba_querybuf_get(db->querybuf), dba_querybuf_size(db->querybuf));
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "performing DBALLE query \"%s\"", dba_querybuf_get(db->querybuf));
		goto failed;
	}

	/* Get the number of affected rows */
	{
		SQLINTEGER rowcount;
		res = SQLRowCount(stm, &rowcount);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "getting row count");
			goto failed;
		}
		(*cur)->count = *count = rowcount;
	}

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

static dba_err dba_db_cursor_var_to_rec(dba_db_cursor cur, dba_record rec)
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
	}
	return dba_error_ok();
}
static dba_err dba_db_cursor_to_rec(dba_db_cursor cur, dba_record rec)
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
	DBA_RUN_OR_RETURN(dba_db_cursor_var_to_rec(cur, rec));
	return dba_error_ok();
}

dba_err dba_db_cursor_next(dba_db_cursor cur, dba_record rec, dba_varcode* var, int* context_id, int* is_last)
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
	DBA_RUN_OR_RETURN(dba_db_cursor_to_rec(cur, rec));

	/* Add the extra ana info */
	/* DBA_RUN_OR_RETURN(dba_ana_add_extra(cur, rec)); */

	/* Return the context ID to refer to the current context */
	*context_id = cur->out_data_id;

	*is_last = --cur->count == 0;

	/* Store the variable ID */
	*var = cur->out_idvar;

	return dba_error_ok();
}
#undef CHECKED_STORE

void dba_db_cursor_delete(dba_db_cursor cur)
{
	assert(cur);
	assert(cur->db);

	SQLFreeHandle(SQL_HANDLE_STMT, cur->stm);
	free(cur);
}

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
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
	}

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
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");
	}
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
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

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
	dba_db_attr a = db->attr;
	
	assert(db);

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
	res = SQLAllocHandle(SQL_HANDLE_STMT, db->od_conn, &stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "Allocating new statement handle");

	/* Bind parameters */
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &id_context, 0, 0);
	SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_USHORT, SQL_INTEGER, 0, 0, &id_var, 0, 0);

	TRACE("Performing query %s for id %d\n", query, id_data);
	
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
