#define _GNU_SOURCE
#include <dballe/db/repinfo.h>
#include <dballe/db/internals.h>
#include <dballe/core/verbose.h>
#include <dballe/core/csv.h>

#include <config.h>

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

static void clear_memo_index(dba_db_repinfo ri)
{
	free(ri->memo_idx);
	ri->memo_idx = NULL;
}

static dba_err cache_append(dba_db_repinfo ri, int id, const char* memo, const char* desc, int prio, const char* descriptor, int tablea)
{
	/* Ensure that we are adding things in order */
	if (ri->cache_size > 0 && ri->cache[ri->cache_size - 1].id >= id)
		return dba_error_consistency(
				"checking that value to append to repinfo cache (%d) "
				"is greather than the last value in che cache (%d)", id, ri->cache[ri->cache_size - 1].id);

	clear_memo_index(ri);

	/* Enlarge buffer if needed */
	if (ri->cache_size == ri->cache_alloc_size)
	{
		dba_db_repinfo_cache new = (dba_db_repinfo_cache)realloc(
				ri->cache, ri->cache_alloc_size * 2 * sizeof(struct _dba_db_repinfo_cache));
		if (new == NULL)
			return dba_error_alloc("enlarging the dba_db_repinfo-cache");
		ri->cache = new;
		ri->cache_alloc_size *= 2;
	}

	ri->cache[ri->cache_size].id = id;
	ri->cache[ri->cache_size].memo = strdup(memo);
	ri->cache[ri->cache_size].desc = strdup(desc);
	ri->cache[ri->cache_size].prio = prio;
	ri->cache[ri->cache_size].descriptor = strdup(descriptor);
	ri->cache[ri->cache_size].tablea = tablea;
	ri->cache[ri->cache_size].new_memo = NULL;
	ri->cache[ri->cache_size].new_desc = NULL;
	ri->cache[ri->cache_size].new_prio = 0;
	ri->cache[ri->cache_size].new_descriptor = NULL;
	ri->cache[ri->cache_size].new_tablea = 0;
	++ri->cache_size;

	return dba_error_ok();
}

static void cache_clear(dba_db_repinfo ri)
{
	int i;
	for (i = 0; i < ri->cache_size; ++i)
	{
		if (ri->cache[i].memo != NULL)
			free(ri->cache[i].memo);
		if (ri->cache[i].desc != NULL)
			free(ri->cache[i].desc);
		if (ri->cache[i].descriptor != NULL)
			free(ri->cache[i].descriptor);

		if (ri->cache[i].new_memo != NULL)
			free(ri->cache[i].new_memo);
		if (ri->cache[i].new_desc != NULL)
			free(ri->cache[i].new_desc);
		if (ri->cache[i].new_descriptor != NULL)
			free(ri->cache[i].new_descriptor);
	}
	clear_memo_index(ri);
	ri->cache_size = 0;
}

static int cache_find_by_id(dba_db_repinfo ri, int id)
{
	/* Binary search the ID */
	int begin, end;

	begin = -1, end = ri->cache_size;
	while (end - begin > 1)
	{
		int cur = (end + begin) / 2;
		if (ri->cache[cur].id > id)
			end = cur;
		else
			begin = cur;
	}
	if (begin == -1 || ri->cache[begin].id != id)
		return -1;
	else
		return begin;
}

static int cache_find_by_memo(dba_db_repinfo ri, const char* memo)
{
	/* Binary search the memo index */
	int begin, end;

	begin = -1, end = ri->cache_size;
	while (end - begin > 1)
	{
		int cur = (end + begin) / 2;
		if (strcmp(ri->memo_idx[cur].memo, memo) > 0)
			end = cur;
		else
			begin = cur;
	}
	if (begin == -1 || strcmp(ri->memo_idx[begin].memo, memo) != 0)
		return -1;
	else
		return begin;
}

static int memoidx_cmp(const void* a, const void* b)
{
	return strcmp(((const dba_db_repinfo_memoidx)a)->memo, ((const dba_db_repinfo_memoidx)b)->memo);
}

static dba_err rebuild_memo_idx(dba_db_repinfo ri)
{
	int i;
	clear_memo_index(ri);
	if ((ri->memo_idx = (dba_db_repinfo_memoidx)malloc(ri->cache_size * sizeof(struct _dba_db_repinfo_memoidx))) == NULL)
		return dba_error_alloc("allocating new repinfo memo index");
	for (i = 0; i < ri->cache_size; ++i)
	{
		strncpy(ri->memo_idx[i].memo, ri->cache[i].memo, 30);
		ri->memo_idx[i].memo[29] = 0;
		ri->memo_idx[i].id = ri->cache[i].id;
	}
	qsort(ri->memo_idx, ri->cache_size, sizeof(struct _dba_db_repinfo_memoidx), memoidx_cmp);
	return dba_error_ok();
}

static dba_err dba_db_repinfo_read_cache(dba_db_repinfo ri)
{
	dba_err err = DBA_OK;
	SQLHSTMT stm;

	int id;
	char memo[30];
	SQLINTEGER memo_ind;
	char description[255];
	SQLINTEGER description_ind;
	int prio;
	SQLINTEGER prio_ind;
	char descriptor[6];
	SQLINTEGER descriptor_ind;
	int tablea;
	SQLINTEGER tablea_ind;

	int res;

	cache_clear(ri);

	DBA_RUN_OR_RETURN(dba_db_statement_create(ri->db, &stm));

	SQLBindCol(stm, 1, SQL_C_ULONG, &id, sizeof(id), 0);
	SQLBindCol(stm, 2, SQL_C_CHAR, &memo, sizeof(memo), &memo_ind);
	SQLBindCol(stm, 3, SQL_C_CHAR, &description, sizeof(description), &description_ind);
	SQLBindCol(stm, 4, SQL_C_ULONG, &prio, sizeof(prio), &prio_ind);
	SQLBindCol(stm, 5, SQL_C_CHAR, &descriptor, sizeof(descriptor), &descriptor_ind);
	SQLBindCol(stm, 6, SQL_C_ULONG, &tablea, sizeof(tablea), &tablea_ind);

	res = SQLExecDirect(stm, (unsigned char*)
			"SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "compiling query to read data from 'repinfo'");
		goto cleanup;
	}

	/* Get the results and save them in the record */
	while (SQLFetch(stm) != SQL_NO_DATA)
		DBA_RUN_OR_GOTO(cleanup, cache_append(ri, id, memo, description, prio, descriptor, tablea));

	/* Rebuild the memo index as well */
	DBA_RUN_OR_GOTO(cleanup, rebuild_memo_idx(ri));

cleanup:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_repinfo_create(dba_db db, dba_db_repinfo* ins)
{
	dba_err err = DBA_OK;
	dba_db_repinfo res = NULL;

	if ((res = (dba_db_repinfo)malloc(sizeof(struct _dba_db_repinfo))) == NULL)
		return dba_error_alloc("creating a new dba_db_repinfo");
	res->db = db;
	res->memo_idx = NULL;

	/* Create and read the repinfo cache */
	if ((res->cache = (dba_db_repinfo_cache)malloc(16 * sizeof(struct _dba_db_repinfo_cache))) == NULL)
		return dba_error_alloc("creating 16 elements of dba_db_repinfo_cache");
	res->cache_size = 0;
	res->cache_alloc_size = 16;
	DBA_RUN_OR_GOTO(cleanup, dba_db_repinfo_read_cache(res));

#if 0
	const char* select_fixed_query =
		"SELECT id FROM repinfo WHERE lat=? AND lon=? AND ident IS NULL";
	const char* select_mobile_query =
		"SELECT id FROM repinfo WHERE lat=? AND lon=? AND ident=?";
	const char* insert_query =
		"INSERT INTO repinfo (lat, lon, ident)"
		" VALUES (?, ?, ?);";
	const char* update_query =
		"UPDATE repinfo SET lat=?, lon=?, ident=? WHERE id=?";

	res->sfstm = NULL;
	res->smstm = NULL;
	res->istm = NULL;
	res->ustm = NULL;

	/* Create the statement for select fixed */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->sfstm)));
	SQLBindParameter(res->sfstm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lat), 0, 0);
	SQLBindParameter(res->sfstm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lon), 0, 0);
	SQLBindCol(res->sfstm, 1, SQL_C_SLONG, &(res->id), sizeof(res->id), 0);
	r = SQLPrepare(res->sfstm, (unsigned char*)select_fixed_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->sfstm, "compiling query to look for fixed repinfo IDs");
		goto cleanup;
	}

	/* Create the statement for select mobile */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->smstm)));
	SQLBindParameter(res->smstm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lat), 0, 0);
	SQLBindParameter(res->smstm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lon), 0, 0);
	SQLBindParameter(res->smstm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, res->ident, 0, &(res->ident_ind));
	SQLBindCol(res->smstm, 1, SQL_C_SLONG, &(res->id), sizeof(res->id), 0);
	r = SQLPrepare(res->smstm, (unsigned char*)select_mobile_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->smstm, "compiling query to look for mobile repinfo IDs");
		goto cleanup;
	}

	/* Create the statement for insert */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->istm)));
	SQLBindParameter(res->istm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lat), 0, 0);
	SQLBindParameter(res->istm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lon), 0, 0);
	SQLBindParameter(res->istm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, res->ident, 0, &(res->ident_ind));
	r = SQLPrepare(res->istm, (unsigned char*)insert_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->istm, "compiling query to insert into 'repinfo'");
		goto cleanup;
	}

	/* Create the statement for update */
	DBA_RUN_OR_GOTO(cleanup, dba_db_statement_create(db, &(res->ustm)));
	SQLBindParameter(res->ustm, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lat), 0, 0);
	SQLBindParameter(res->ustm, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->lon), 0, 0);
	SQLBindParameter(res->ustm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, res->ident, 0, &(res->ident_ind));
	SQLBindParameter(res->ustm, 4, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &(res->id), 0, 0);
	r = SQLPrepare(res->ustm, (unsigned char*)update_query, SQL_NTS);
	if ((r != SQL_SUCCESS) && (r != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, res->ustm, "compiling query to update repinfo");
		goto cleanup;
	}

#endif

	*ins = res;
	res = NULL;
	
cleanup:
	if (res != NULL)
		dba_db_repinfo_delete(res);
	return err == DBA_OK ? dba_error_ok() : err;
};

void dba_db_repinfo_delete(dba_db_repinfo ins)
{
	if (ins->cache != NULL)
	{
		cache_clear(ins);
		free(ins->cache);
	}
#if 0
	if (ins->sfstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->sfstm);
	if (ins->smstm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->smstm);
	if (ins->istm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->istm);
	if (ins->ustm != NULL)
		SQLFreeHandle(SQL_HANDLE_STMT, ins->ustm);
#endif
	free(ins);
}

dba_err dba_db_repinfo_get_id(dba_db_repinfo ri, const char* memo, int* id)
{
	if (ri->memo_idx == NULL)
		DBA_RUN_OR_RETURN(rebuild_memo_idx(ri));
	int pos = cache_find_by_memo(ri, memo);
	if (pos == -1)
		return dba_error_notfound("looking for repinfo corresponding to '%s'", memo);
	*id = ri->cache[pos].id;
	return dba_error_ok();
}

dba_err dba_db_repinfo_has_id(dba_db_repinfo ri, int id, int* exists)
{
	*exists = (cache_find_by_id(ri, id) != -1);
	return dba_error_ok();
}

struct _newitem
{
	struct _dba_db_repinfo_cache item;
	struct _newitem* next;
};
typedef struct _newitem* newitem;

static dba_err read_repinfo_file(dba_db_repinfo ri, const char* deffile, newitem* newitems)
{
	dba_err err = DBA_OK;
	FILE* in;

	if (deffile == 0)
	{
		deffile = getenv("DBA_REPINFO");
		if (deffile == 0 || deffile[0] == 0)
			deffile = CONF_DIR "/repinfo.csv";
	}

	/* Open the input CSV file */
	in = fopen(deffile, "r");
	if (in == NULL)
		return dba_error_system("opening file %s", deffile);

	/* Read the CSV file */
	{
		char* columns[7];
		int line;
		int i;

		for (line = 0; (i = dba_csv_read_next(in, columns, 7)) != 0; line++)
		{
			int id, pos;

			if (i != 6)
			{
				err = dba_error_parse(deffile, line, "Expected 6 columns, got %d", i);
				goto cleanup;
			}

			id = strtol(columns[0], 0, 10);
			pos = cache_find_by_id(ri, id);
			if (pos == -1)
			{
				/* New entry */
				newitem new = (newitem)calloc(1, sizeof(struct _newitem));
				if (new == NULL)
				{
					err = dba_error_alloc("allocating memory to store a new repinfo item");
					goto cleanup;
				}
				new->item.new_memo = columns[1];
				new->item.new_desc = columns[2];
				new->item.new_prio = strtol(columns[3], 0, 10);
				new->item.new_descriptor = columns[4];
				new->item.new_tablea = strtol(columns[5], 0, 10);
				new->next = *newitems;
				*newitems = new;
			} else {
				/* Possible update on an existing entry */
				ri->cache[pos].new_memo = columns[1];
				ri->cache[pos].new_desc = columns[2];
				ri->cache[pos].new_prio = strtol(columns[3], 0, 10);
				ri->cache[pos].new_descriptor = columns[4];
				ri->cache[pos].new_tablea = strtol(columns[5], 0, 10);
			}

			free(columns[0]);
			free(columns[3]);
			free(columns[5]);
		}
	}

cleanup:
	fclose(in);
	return err == DBA_OK ? dba_error_ok() : err;
}

dba_err dba_db_repinfo_update(dba_db_repinfo ri, const char* deffile)
{
	dba_err err = DBA_OK;
	newitem newitems = NULL;
	SQLHSTMT stm;
	int res;
	int id;
	int i;

	/* Read the new repinfo data from file */
	DBA_RUN_OR_RETURN(read_repinfo_file(ri, deffile, &newitems));

	/* Verify that the update is possible */
	DBA_RUN_OR_RETURN(dba_db_statement_create(ri->db, &stm));
	SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &id, 0, 0);
	res = SQLPrepare(stm, (unsigned char*)"SELECT id FROM context WHERE id_report = ? LIMIT 1", SQL_NTS);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
	{
		err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "compiling query to check if a repinfo item is in use");
		goto cleanup;
	}

	for (i = 0; i < ri->cache_size; ++i)
	{
		/* Ensure that we are not deleting a repinfo entry that is already in use */
		if (ri->cache[i].memo != NULL && ri->cache[i].new_memo == NULL)
		{
			id = ri->cache[i].id;

			res = SQLExecute(stm);
			if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			{
				err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "checking if a repinfo entry is in use");
				goto cleanup;
			}

			if (SQLFetch(stm) != SQL_NO_DATA)
			{
				err = dba_error_consistency(
						"trying to delete repinfo entry %d,%s which is currently in use",
						ri->cache[i].id, ri->cache[i].memo);
				goto cleanup;
			}
		}
	}

	res = SQLCloseCursor(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_update_repinfo read cursor");

	/* Perform the changes */

	/* Insert the new items */
	if (newitems != NULL)
	{
		newitem cur;
		char memo[30];
		char description[255];
		int prio;
		char descriptor[6];
		int tablea;

		res = SQLPrepare(stm, (unsigned char*)
				"INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)"
				"     VALUES (?, ?, ?, ?, ?, ?)", SQL_NTS);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "compiling query to insert into 'repinfo'");
			goto cleanup;
		}

		SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &id, 0, 0);
		SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, memo, 0, 0);
		SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, description, 0, 0);
		SQLBindParameter(stm, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &prio, 0, 0);
		SQLBindParameter(stm, 5, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, descriptor, 0, 0);
		SQLBindParameter(stm, 6, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &tablea, 0, 0);

		for (cur = newitems; cur != NULL; cur = cur->next)
		{
			id = cur->item.id;
			strncpy(memo, cur->item.new_memo, 30); memo[29] = 0;
			strncpy(description, cur->item.new_desc, 255); description[254] = 0;
			prio = cur->item.new_prio;
			strncpy(descriptor, cur->item.new_descriptor, 6); descriptor[5] = 0;
			tablea = cur->item.new_tablea;

			res = SQLExecute(stm);
			if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			{
				err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "inserting new data into 'data'");
				goto cleanup;
			}
		}

		res = SQLCloseCursor(stm);
		if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		{
			err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_update_repinfo insert cursor");
			goto cleanup;
		}
	}

	/* Update the items that were modified */
	for (i = 0; i < ri->cache_size; ++i)
	{
		if (ri->cache[i].memo != NULL && ri->cache[i].new_memo != NULL)
		{
			SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, ri->cache[i].new_memo, 0, 0);
			SQLBindParameter(stm, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, ri->cache[i].new_desc, 0, 0);
			SQLBindParameter(stm, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &(ri->cache[i].new_prio), 0, 0);
			SQLBindParameter(stm, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 0, 0, ri->cache[i].new_descriptor, 0, 0);
			SQLBindParameter(stm, 5, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(ri->cache[i].new_tablea), 0, 0);
			SQLBindParameter(stm, 6, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(ri->cache[i].id), 0, 0);

			res = SQLExecDirect(stm, (unsigned char*)
					"UPDATE repinfo set memo=?, description=?, prio=?, descriptor=? tablea=?"
					"  WHERE id=?", SQL_NTS);
			if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			{
				err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "updating data in 'repinfo'");
				goto cleanup;
			}

			res = SQLCloseCursor(stm);
			if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			{
				err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_update_repinfo update cursor");
				goto cleanup;
			}
		}
		else if (ri->cache[i].memo != NULL && ri->cache[i].new_memo == NULL)
		{
			SQLBindParameter(stm, 1, SQL_PARAM_INPUT, SQL_C_ULONG, SQL_INTEGER, 0, 0, &(ri->cache[i].id), 0, 0);

			res = SQLExecDirect(stm, (unsigned char*)"DELETE FROM repinfo WHERE id=?", SQL_NTS);
			if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			{
				err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "deleting item %d from 'repinfo'");
				goto cleanup;
			}

			res = SQLCloseCursor(stm);
			if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
			{
				err = dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_update_repinfo delete cursor");
				goto cleanup;
			}
		}
	}

cleanup:
	SQLFreeHandle(SQL_HANDLE_STMT, stm);
	while (newitems != NULL)
	{
		newitem next = newitems->next;
		free(newitems->item.new_memo);
		free(newitems->item.new_desc);
		free(newitems->item.new_descriptor);
		free(newitems);
		newitems = next;
	}
	return err == DBA_OK ? dba_error_ok() : err;
}
#if 0
void dba_db_repinfo_set_ident(dba_db_repinfo ins, const char* ident)
{
	int len = strlen(ident);
	if (len > 64) len = 64;
	memcpy(ins->ident, ident, len);
	ins->ident[len] = 0;
	ins->ident_ind = len; 
}

dba_err dba_db_repinfo_get_id(dba_db_repinfo ins, int *id)
{
	SQLHSTMT stm = ins->ident_ind == SQL_NULL_DATA ? ins->sfstm : ins->smstm;

	int res = SQLExecute(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "looking for repinfo IDs");

	/* Get the result */
	if (SQLFetch(stm) != SQL_NO_DATA)
		*id = ins->id;
	else
		*id = -1;

	res = SQLCloseCursor(stm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, stm, "closing dba_db_repinfo_get_id cursor");

	return dba_error_ok();
}

dba_err dba_db_repinfo_insert(dba_db_repinfo ins, int *id)
{
	int res = SQLExecute(ins->istm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->istm, "inserting new data into repinfo");

	return dba_db_last_insert_id(ins->db, id);
}

dba_err dba_db_repinfo_update(dba_db_repinfo ins)
{
	int res = SQLExecute(ins->ustm);
	if ((res != SQL_SUCCESS) && (res != SQL_SUCCESS_WITH_INFO))
		return dba_db_error_odbc(SQL_HANDLE_STMT, ins->ustm, "updating repinfo");

	return dba_error_ok();
}
#endif

/* vim:set ts=4 sw=4: */
