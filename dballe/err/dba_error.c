#define _GNU_SOURCE
#include "dba_error.h"

#include <config.h>

#include <stdio.h>	/* vasprintf */
#include <stdlib.h>	/* free */
#include <string.h>	/* strerror */
#include <stdarg.h> /* va_start, va_end */
#include <errno.h>
#include <assert.h>

#include <execinfo.h>

static const char* dba_err_desc[] = {
/*  0 */	"no error",
/*  1 */	"item not found",
/*  2 */	"wrong variable type",
/*  3 */	"cannot allocate memory",
/*  4 */	"ODBC error",
/*  5 */	"handle management error",
/*  6 */	"buffer is too short to fit data",
/*  7 */	"error reported by the system",
/*  8 */	"consistency check failed",
/*  9 */	"parse error",
/* 10 */	"write error",
/* 11 */	"feature not implemented"
};

struct _dba_error_info
{
	dba_err_code code;
	char* context;
	char* details;
	char* backtrace;
};

static dba_err_code lasterr = DBA_ERR_NONE;
static char* context = NULL;
static char* details = NULL;
static char* err_backtrace = NULL;
static int should_dealloc = 0;

typedef struct _dba_err_callbacks {
	dba_err_code trigger;
	dba_err_callback cb;
	void* data;
	struct _dba_err_callbacks* next;
}* dba_err_callbacks;

static dba_err_callbacks callbacks = NULL;

static void reset_error(dba_err_code code)
{
	if (context != NULL && should_dealloc)
		free(context);
	context = NULL;
	should_dealloc = 1;

	if (details != NULL)
		free(details);
	details = NULL;

	if (err_backtrace != NULL)
		free(err_backtrace);
	err_backtrace = NULL;

	lasterr = code;
}

static void add_backtrace()
{
	const int trace_size = 50;
	void *addrs[trace_size];
	size_t size = backtrace (addrs, trace_size);
	char **strings = backtrace_symbols (addrs, size);
	int tot_size = 0;
	int i, j;

	for (i = 0; i < size; i++)
		tot_size += strlen(strings[i]) + 2;

	err_backtrace = (char*)malloc(tot_size);

	for (i = 0, j = 0; i < size && j < tot_size; i++)
		j += snprintf(err_backtrace + j, tot_size - j, "%s\n", strings[i]);

	free (strings);
}

static void dba_error_set_callback_rec(dba_err_callbacks* chain, dba_err_code code, dba_err_callback cb, void* data)
{
	if (*chain == NULL)
	{
		*chain = (dba_err_callbacks)malloc(sizeof(struct _dba_err_callbacks));
		(*chain)->trigger = code;
		(*chain)->cb = cb;
		(*chain)->data = data;
		(*chain)->next = NULL;
	}
	else if ((*chain)->trigger == code && (*chain)->cb == cb && (*chain)->data == data)
		return;
	else
		dba_error_set_callback_rec(&((*chain)->next), code, cb, data);
}

static void dba_error_remove_callback_rec(dba_err_callbacks* chain, dba_err_code code, dba_err_callback cb, void* data)
{
	if (*chain == NULL)
		return;
	
	if ((*chain)->trigger == code && (*chain)->cb == cb && (*chain)->data == data)
	{
		dba_err_callbacks cur = (*chain);
		*chain = (*chain)->next;
		free(cur);
	}
	else
		dba_error_remove_callback_rec(&((*chain)->next), code, cb, data);
}

void dba_error_set_callback(dba_err_code code, dba_err_callback cb, void* data)
{
	dba_error_set_callback_rec(&callbacks, code, cb, data);
}

void dba_error_remove_callback(dba_err_code code, dba_err_callback cb, void* data)
{
	dba_error_remove_callback_rec(&callbacks, code, cb, data);
}

static void dba_error_check_callbacks(dba_err_callbacks chain)
{
	if (chain == NULL)
		return;

	if (chain->trigger == DBA_ERR_NONE || chain->trigger == lasterr)
		chain->cb(chain->data);

	dba_error_check_callbacks(chain->next);
}

dba_err_code dba_error_get_code()
{
	return lasterr;
}

const char* dba_error_get_message()
{
	return dba_err_desc[lasterr];
}

const char* dba_error_get_context()
{
	return context == NULL ? "(no error context available)" : context;
}

const char* dba_error_get_details()
{
	return details;
}

const char* dba_error_get_backtrace()
{
	return err_backtrace;
}


dba_err dba_error_ok()
{
	reset_error(DBA_ERR_NONE);
	return DBA_OK;
}

dba_err dba_error_alloc(const char* message)
{
	reset_error(DBA_ERR_ALLOC);
	/* We cannot allocate memory for the message here, as allocation would
	 * probably fail */
	context = (char*)message;
	should_dealloc = 0;
	dba_error_check_callbacks(callbacks);
	return DBA_ERROR;
}

dba_err dba_error_system(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	reset_error(DBA_ERR_SYSTEM);
	vasprintf(&context, fmt, ap);
	va_end(ap);
	details = strdup(strerror(errno));
	add_backtrace();
	dba_error_check_callbacks(callbacks);
	return DBA_ERROR;
}

dba_err dba_error_parse(const char* file, int line, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	reset_error(DBA_ERR_PARSE);
	vasprintf(&details, fmt, ap);
	va_end(ap);

	asprintf(&context, "parsing %s:%d", file, line);
	
	add_backtrace();
	dba_error_check_callbacks(callbacks);
	return DBA_ERROR;
}

dba_err dba_error_generic0(dba_err_code code, char* _context, char* _details)
{
	reset_error(code);
	context = _context;
	details = _details;
	add_backtrace();
	dba_error_check_callbacks(callbacks);
	return DBA_ERROR;
}

dba_err dba_error_generic1(dba_err_code code, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	reset_error(code);
	vasprintf(&context, fmt, ap);
	va_end(ap);
	add_backtrace();
	dba_error_check_callbacks(callbacks);
	return DBA_ERROR;
}

void dba_error_state_get(dba_error_info* info)
{
	/* Allocate if needed */
	if (*info == NULL)
	{
		*info = (dba_error_info)calloc(1, sizeof(struct _dba_error_info));
		if (*info == NULL)
			return;
	}

	/* Clear the old values if needed */
	if ((*info)->context != NULL)
		free((*info)->context);
	(*info)->context = NULL;

	if ((*info)->details != NULL)
		free((*info)->details);
	(*info)->details = NULL;

	/* Copy the error informations */
	(*info)->code = lasterr;
	if (context != NULL)
		(*info)->context = strdup(context);
	if (details != NULL)
		(*info)->details = strdup(details);
	if (err_backtrace != NULL)
		(*info)->backtrace = strdup(err_backtrace);
}

void dba_error_state_set(dba_error_info info)
{
	assert(info != NULL);

	reset_error(info->code);

	if (info->context)
		context = strdup(info->context);
	if (info->details)
		details = strdup(info->details);
	if (info->backtrace)
		err_backtrace = strdup(info->backtrace);
}

void dba_error_state_delete(dba_error_info* info)
{
	assert(info != NULL);
	assert(*info != NULL);

	if ((*info)->context != NULL)
		free((*info)->context);

	if ((*info)->details != NULL)
		free((*info)->details);

	if ((*info)->backtrace != NULL)
		free((*info)->backtrace);

	free(*info);
	*info = 0;
}

/* vim:set ts=4 sw=4: */
