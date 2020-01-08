.. _connect:

Connect URLs
============

DB-All.e can work with several storage backends, identified via a URL.

This is how to connect via URL using various ways of working with DB-All.e.
``dbtype://info`` is a placeholder for the connetion URL, that is documented
below.

Specifying the URL directly:

.. highlight:: fortran

Fortran::

    ierr = idba_connect(dbhandle, "dbtype://info")

.. highlight:: sh

dbadb::

    dbadb --dsn="dbtype://info" import …

provami::

    provami "dbtype://info"

.. highlight:: python

python::

    db = dballe.DB.connect_from_url("dbtype://info")


.. highlight:: sh

Specifying the URL via an environment variable::

    # Export the environment variable
    export DBA_DB="dbtype://info"

.. highlight:: fortran

Fortran::

    ierr = idba_connect(dbhandle, DBA_MVC)

.. highlight:: sh

dbadb::

    dbadb import …

provami::

    provami

.. highlight:: python

python::

    db = dballe.DB.connect_from_url(os.environ["DBA_DB"])


URL syntax
----------

.. highlight:: none

For SQLite
^^^^^^^^^^

SQLite URLs need only specify the ``.sqlite`` file name.

They can be either in the form ``sqlite:file.sqlite`` or ``sqlite://file.sqlite``.

If the environment variable ``DBA_INSECURE_SQLITE`` is set, then SQLite access
will be faster but data consistency will not be guaranteed.


For PostgreSQL
^^^^^^^^^^^^^^

DB-All.e uses standard PostgreSQL connection URIs. For example: ``postgresql://user@host/db``

See `the PostgreSQL documentation`__ for the complete documentation.

__ http://www.postgresql.org/docs/9.4/static/libpq-connect.html#LIBPQ-CONNSTRING


For MySQL
^^^^^^^^^

DB-All.e uses a MySQL connection URL with a syntax similar to `the one used by
the JDBC connector`__::

    mysql://[host][:port]/[database][?propertyName1][=propertyValue1][&propertyName2][=propertyValue2]...

The only property names currently used by DB-All.e are ``user`` and ``password``;
the rest are ignored.

For example: ``mysql://host/db?user=username&password=secret``

__ http://dev.mysql.com/doc/connector-j/en/connector-j-reference-configuration-properties.html

URL actions
-----------

``?wipe=yes/true/1``
^^^^^^^^^^^^^^^^^^^^

You can add a ``wipe`` query string argument to the connection URL to have
DB-All.e create or recreate the database on connection.

You can also use ``?wipe`` without argument. Note that ``?wipe=`` with an
empty argument also triggers a wipe.

