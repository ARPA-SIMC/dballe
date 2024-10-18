Starting the work
=================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran

Including the DB-All.e interface file
-------------------------------------

If you work using Fortran 90, DB-All.e comes with an interface file that you
can use to enable type checking on all the DB-All.e API.

To make use of the interface file, just include it at the beginning of your
program::

    include "dballe/dballeff.h"

The Fortran 90 interface also allows to replace all the functions
:ref:`idba_enqi`, :ref:`idba_enqr`, :ref:`idba_enqd` and
:ref:`idba_enqc` with ``idba_enq`` and to replace all the functions
:ref:`idba_seti`, :ref:`idba_setr`, :ref:`idba_setd` and
:ref:`idba_setc` with ``idba_set``.


.. _starting_the_work:

Starting the work
-----------------

Before any action routine, you need to connect to the database.  Connecting to
the database will give you a *handle* that you can use to open sessions.

This code will open a connection with DB-All.e, then it will start a session::

    ! Connect to the database and get a handle to work with it
    ierr = idba_connect(dbhandle, "url")
    ierr = idba_begin(dbhandle, handle, "read", "read", "read")

    ! ...do your work...

    ! End of the work
    ierr = idba_commit(handle)
    ierr = idba_disconnect(dbhandle)

You call :ref:`idba_connect` to connect to the databse. The parameters are
the database connection URL (see :ref:`connect`), and two parameters that were
used in the past and are now ignored.

You can call :ref:`idba_begin` many times and get more handles.  This allows
to perform many operations on the database at the same time.

:ref:`idba_begin` has three extra parameters that can be used to limit
write operations on the database, as a limited protection against programming
errors.

The first extra parameter controls access to station values and can have
these values:

* ``"read"``: station values cannot be modified.
* ``"write"``: station values can be added and removed.

The second extra parameter control access to observed data and can have
these values:

* ``"read"``: data cannot be modified in any way.
* ``"add"``: data can be added to the database, but existing data cannot be
  modified.  Deletions are disabled.  This is used to insert new data in the
  database while preserving the data that was already present in it.
* ``"write"``: data can freely be added, overwritten and deleted.

The third extra parameter controls access to data attributes and can have
these values:

* ``"read"``: attributes cannot be modified in any way.
* ``"write"``: attributes can freely be added, overwritten and deleted.

Note that some combinations are illegal. For example, you cannot have "read" on
station values and "add" on data, because  when adding a new data, it is
sometimes necessary to insert new station records). You also cannot have
"rewrite" on data and "read` on attributes, because when deleting data, their
attributes are deleted as well.


Starting the work on a message
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Instead of connecting to a database, you can use the DB-All.e API to read and
write message reports in BUFR and CREX format.

To do that, use :ref:`idba_begin_messages` instead of both :ref:`idba_connect` and
:ref:`idba_begin`.  To write a message, your code will look like::

    ! Connect to the database and get a handle to work with it
    ierr = idba_begin_messages(handle, "file.bufr", "r", "auto")

    ! ...do your work...

    ! End of the work
    ierr = idba_commit(handle)

:ref:`idba_begin_messages` has three parameters:

1. the name of the file to open
2. the open mode (``"r"`` for read, ``"w"`` for write or create, ``"a"`` for append).
   See the documentation of libc's ``fopen`` for more details.
3. the file format.  It can be ``"BUFR"``, ``"CREX"`, or ``"AUTO"``.  ``"AUTO"`` tells
   DB-All.e to autodetect the file format, but it only works when reading
   files, not when writing new one.

You can call :ref:`idba_begin_messages` many times and read or write many files.  You
can even call :ref:`idba_begin_messages` many time on the same file as long as you
open it read only.

Once you open a file, you can use the other DB-All.e functions on it.  There
are slight differences between working on a database and working on a file, and
they are explained later in the section :ref:`working_with_files`.


Ending the work
---------------

When you are finished working with a handle, you release it with
:ref:`idba_commit`::

    ! We are finished with this handle
    ierr = idba_commit(handle)

When you are finished working with DB-ALLe, you use :ref:`idba_disconnect` to
close all connections and release all resources::

    ! We do not need to work with dballe anymore
    ierr = idba_disconnect(dbh)


