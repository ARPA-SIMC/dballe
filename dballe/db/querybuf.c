#include <dballe/db/querybuf.h>

#include <stdlib.h>
#include <stdarg.h>

struct _dba_querybuf
{
	char* buf;
	int maxsize;
	int size;
};

dba_err dba_querybuf_create(int maxsize, dba_querybuf* buf)
{
	if (maxsize < 1)
		return dba_error_consistency("checking that the querybuf max size is more than 0");
	
	if (((*buf) = (dba_querybuf)calloc(1, sizeof(struct _dba_querybuf))) == NULL)
		return dba_error_alloc("Allocating a new dba_querybuf");

	if (((*buf)->buf = (char*)malloc(maxsize)) == NULL)
	{
		free(*buf);
		*buf = NULL;
		return dba_error_alloc("Allocating the string buffer for a dba_querybuf");
	}

	(*buf)->maxsize = maxsize;
	(*buf)->buf[0] = 0;
	(*buf)->size = 0;

	return dba_error_ok();
}

void dba_querybuf_delete(dba_querybuf buf)
{
	free(buf->buf);
	free(buf);
}

void dba_querybuf_reset(dba_querybuf buf)
{
	buf->buf[0] = 0;
	buf->size = 0;
}

const char* dba_querybuf_get(dba_querybuf buf)
{
	return buf->buf;
}

int dba_querybuf_size(dba_querybuf buf)
{
	return buf->size;
}

dba_err dba_querybuf_append(dba_querybuf buf, const char* str)
{
	const char* s;
	for (s = str; *s; s++)
	{
		if (buf->size >= buf->maxsize - 1)
		{
			/* Leave the string properly null-terminated also in case of error */
			buf->buf[buf->maxsize - 1] = 0;
			return dba_error_consistency("checking that the string to append fits in the querybuf");
		}
		buf->buf[buf->size++] = *s;
	}
	buf->buf[buf->size] = 0;
	return dba_error_ok();
}

dba_err dba_querybuf_appendf(dba_querybuf buf, const char* fmt, ...)
{
	int size;
	va_list ap;
	va_start(ap, fmt);
	size = vsnprintf(buf->buf + buf->size, buf->maxsize - buf->size, fmt, ap);
	if (size >= buf->maxsize - buf->size)
	{
		buf->buf[buf->maxsize - 1] = 0;
		return dba_error_consistency("checking that the formatted string to append fits in the querybuf");
	}
	buf->size += size;
	va_end(ap);

	return dba_error_ok();
}

/* vim:set ts=4 sw=4: */
