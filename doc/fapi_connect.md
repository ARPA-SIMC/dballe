# Connecting to DB-All.e

DB-All.e can work with several storage backends, identified via a URL.

This is how to connect via URL using various ways of working with DB-All.e.
`dbtype://info` is a placeholder for the connetion URL, that is documented
below.

Specifying the URL directly:

Fortran:
```fortran
ierr = idba_presentati(dbhandle, "dbtype://info", DBA_MVC, DBA_MVC)
```

dbadb:
```sh
dbadb --dsn="dbtype://info" import …
```

provami:
```sh
provami "dbtype://info"
```

python:
```py
db = dballe.DB.connect_from_url("dbtype://info")
```


Specifying the URL via an environment variable:

```sh
# Export the environment variable
export DBA_DB="dbtype://info"
```

Fortran:
```fortran
ierr = idba_presentati(dbhandle, DBA_MVC, DBA_MVC, DBA_MVC)
```

dbadb:
```sh
dbadb import …
```

provami:
```sh
provami
```

```py
db = dballe.DB.connect_from_url(os.environ["DBA_DB"])
```


## URL syntax

### For SQLite

SQLite URLs need only specify the `.sqlite` file name.

They can be either in the form `sqlite:file.sqlite` or `sqlite://file.sqlite`.

If the environment variable `DBA_INSECURE_SQLITE` is set, then SQLite access
will be faster but data consistency will not be guaranteed.


### For PostgreSQL

DB-All.e uses standard PostgreSQL connection URIs. For example: `postgresql://user@host/db`

See [the PostgreSQL documentation](http://www.postgresql.org/docs/9.4/static/libpq-connect.html#LIBPQ-CONNSTRING)
for the complete documentation.


### For MySQL

DB-All.e uses a MySQL connection URL with a syntax similar to [the one used by
the JDBC connector](http://dev.mysql.com/doc/connector-j/en/connector-j-reference-configuration-properties.html):

    mysql://[host][:port]/[database][?propertyName1][=propertyValue1][&propertyName2][=propertyValue2]...

The only property names currently used by DB-All.e are `user` and `password`;
the rest are ignored.

For example: `mysql://host/db?user=username&password=secret`


### For memdb

memdb is an internal memory-only storage that can be used to process small
amounts of data without requiring a database server or a file.

Its url is just `mem:`.
