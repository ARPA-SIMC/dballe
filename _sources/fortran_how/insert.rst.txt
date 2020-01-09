Inserting data
==============

.. toctree::
   :maxdepth: 2

.. highlight:: fortran

Inserting data
--------------

Data is inserted using :c:func:`idba_insert_data`:

    ! Insert a new data in the database
    ierr = idba_setr(handle, "ana_id", 4)
    ierr = idba_setr(handle, "rep_memo", "synop")
    ierr = idba_setd(handle, "lat", 44.500D0)
    ierr = idba_setd(handle, "lon", 11.328D0)
    ierr = idba_setr(handle, "year", 2005)
    ierr = idba_setr(handle, "month", 7)
    ierr = idba_setr(handle, "day", 26)
    ...
    ierr = idba_setr(handle, "B11002", 1.8)
    ierr = idba_insert_data(handle)

This code introduces a new function:

* :c:func:`idba_insert_data`:
  inserts a new value in the database.  All the information about the parameter
  to insert is taken from the input previously set by ``idba_set*`` functions.

  When data of the same kind and with the same characteristics already exists,
  the behaviour of :c:func:`idba_insert_data` is defined by the parameter passed to
  :c:func:`idba_begin` when creating the handle.  See :ref:`starting_the_work` for more informations.

:c:func:`idba_insert_data` will work in different ways according to the data opening
mode of the database:

* ``read``: causes an error, because the data cannot be read.
* ``add``: new data can be inserted, but causes an error when trying to insert a
  value that already exists.
* ``rewrite``: new data can be inserted, and existing data is overwritten.

Also, behaviour changes according to the station values opening mode:

* ``"reuse"``: when inserting data, if an existing pseudoana record for the data
  is found, it will be reused.
* ``"rewrite"``: when inserting data, if an existing pseudoana record for the
  data is found, it will be completely overwritten with the parameters in
  input.

Note that the database cannot be opened in pseudoana ``read`` mode when data
is ``add`` or ``rewrite``.


Code examples
-------------

Insert station data, then insert data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    ierr = idba_begin(dbhandle, handle, "write", "add", "write")
    
    ! Insert data about a station
    ierr = idba_setr (handle, "lat", 11.345)
    ierr = idba_setr (handle, "lon", 44.678)
    ierr = idba_setr (handle, "height", 23)
    ierr = idba_insert_data (handle)
    
    ! Read the station ID for the station we just inserted
    ! Use *ana_id instead of ana_id after an idba_insert_data
    ierr = idba_enqi (handle, "*ana_id", anaid)
    
    ! Reset the input data
    ierr = idba_unsetall (handle)
    
    ! Add data to the station we just inserted
    ierr = idba_seti (handle, "ana_id", anaid)
    ierr = idba_setlevel (handle, 100, 1, 0, 0)
    ierr = idba_settimerange (handle, 0, 0, 0)
    ierr = idba_setdate (handle, 2006, 06, 20, 19, 30, 0)
    ierr = idba_seti (handle, "t", 21)
    ierr = idba_setc (handle, "B12345", "ciao")
    ierr = idba_insert_data (handle)


