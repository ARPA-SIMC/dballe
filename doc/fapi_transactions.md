# Transactional behaviour

> Transaction processing is information processing in computer science that is
> divided into individual, indivisible operations called transactions. Each
> transaction must succeed or fail as a complete unit; it can never be only
> partially complete.
>
> (from [wikipedia](https://en.wikipedia.org/wiki/Transaction_processing)

DB-All.e operations are grouped into *transactions*, which succeed or fail as a
complete unit, and can never be only partially complete.

In Fortran, a transaction begins with [idba_preparati][] and ends with
[idba_fatto][]. If [idba_fatto][] is not called, all modifications done using
that session handle will be discarded as if they had never happened. When
[idba_fatto][] is called, all modifications done using that session handle are
saved and will be available for others to read.

For example, this code will print `0` and then `1`:

```fortran
idba_presentati(dbhandle, "dbtype://info?wipe=true")

! Write some data
idba_preparati(dbhandle, handle_write, "write", "write", "write")
…
idba_prendilo(handle_write)

! Read it before calling idba_fatto: the modifications are not yet visible
! outside the session that is writing them
idba_preparati(dbhandle, handle_read, "read", "read", "read")
idba_voglioquesto(handle_read, count)
print*, count
idba_fatto(handle_read)
idba_fatto(handle_write)

! Read if after calling idba_fatto: the modifications are visible now
idba_preparati(dbhandle, handle_read, "read", "read", "read")
idba_voglioquesto(handle_read, count)
print*, count
idba_fatto(handle_read)
```

## SQLite specific limitations

SQLite does not support writing data while another session is reading it. This
means that `idba_fatto` will exit with an error if there is a read session
open. For example, this code will fail:

```fortran
idba_preparati(dbhandle, handle_write, "write", "write", "write")
idba_preparati(dbhandle, handle_read, "read", "read", "read")
idba_fatto(handle_write) ! fails: there is a read session open
idba_fatto(handle_read)
```

and this code will work:

```fortran
idba_preparati(dbhandle, handle_write, "write", "write", "write")
idba_preparati(dbhandle, handle_read, "read", "read", "read")
idba_fatto(handle_read)
idba_fatto(handle_write) ! succeeds: no read session is open
```

[idba_preparati]: fapi_reference.md#idba_preparati
[idba_fatto]: fapi_reference.md#idba_fatto
