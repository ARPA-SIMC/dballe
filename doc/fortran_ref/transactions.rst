Transactional behaviour
=======================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran

DB-All.e operations are grouped into *transactions*, which succeed or fail as a
complete unit, and can never be only partially complete.

    Transaction processing is information processing in computer science that is
    divided into individual, indivisible operations called transactions. Each
    transaction must succeed or fail as a complete unit; it can never be only
    partially complete.
    
    (from https://en.wikipedia.org/wiki/Transaction_processing)

In Fortran, a transaction begins with :ref:`idba_begin` and ends with
:ref:`idba_commit`. If :ref:`idba_commit` is not called, all modifications done using
that session handle will be discarded as if they had never happened. When
:ref:`idba_commit` is called, all modifications done using that session handle are
saved and will be available for others to read.

For example, this code will print ``0`` and then ``1``::

    idba_connect(dbhandle, "dbtype://info?wipe=true")
    
    ! Write some data
    idba_begin(dbhandle, handle_write, "write", "write", "write")
    ! …
    idba_insert_data(handle_write)
    
    ! Read it before calling idba_commit: the modifications are not yet visible
    ! outside the session that is writing them
    idba_begin(dbhandle, handle_read, "read", "read", "read")
    idba_query_data(handle_read, count)
    print*, count
    idba_commit(handle_read)
    idba_commit(handle_write)
    
    ! Read if after calling idba_commit: the modifications are visible now
    idba_begin(dbhandle, handle_read, "read", "read", "read")
    idba_query_data(handle_read, count)
    print*, count
    idba_commit(handle_read)


SQLite specific limitations
---------------------------

SQLite does not support writing data while another session is reading it. This
means that :ref:`idba_commit` will exit with an error if there is a read session
open. For example, this code will fail::

    idba_begin(dbhandle, handle_write, "write", "write", "write")
    idba_begin(dbhandle, handle_read, "read", "read", "read")
    idba_commit(handle_write) ! fails: there is a read session open
    idba_commit(handle_read)

and this code will work::

    idba_begin(dbhandle, handle_write, "write", "write", "write")
    idba_begin(dbhandle, handle_read, "read", "read", "read")
    idba_commit(handle_read)
    idba_commit(handle_write) ! succeeds: no read session is open
