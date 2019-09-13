A walk through the Fortran API
==============================

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
:c:func:`idba_enqi`, :c:func:`idba_enqr`, :c:func:`idba_enqd` and
:c:func:`idba_enqc` with ``idba_enq`` and to replace all the functions
:c:func:`idba_seti`, :c:func:`idba_setr`, :c:func:`idba_setd` and
:c:func:`idba_setc` with ``idba_set``.


Error management
----------------

* All errors are reported as function return values
* All results are reported as output parameters
* All functions ``idba_set*`` set the input of action routines
* All functions ``idba_enq*`` get the output of action routines

Errors can be handled by checking the return value of every function::

    ! Example error handling
    ierr = idba_connect(dbhandle, "dburl")
    if (ierr.ne.0) then
         ! handle the error...
    end if

Or they can be handled by installing a callback function that is automatically
called in case of error::

    !     How to set a callback
    !       * the first parameter is the error code that triggers the callback (0
    !         means 'trigger on all errors')
    !       * the second parameter is the routine to call when the error happens
    !         (remember to declare the function as 'external'
    !       * the third parameter is a convenience arbitrary integer that will be
    !         passed to the function
    !       * the fourth parameter is used to return a handle that can be used to
    !         remove the callback
    ierr = idba_error_set_callback(0, error_handler, 42, cb_handle)

The previous code will setup DB-ALLe to call ``error_handler`` after any error,
passing it the integer value 42.  The callback can be removed at any time by
calling :c:func:`idba_error_remove_callback`::

    ! How to remove a callback
    ierr = idba_error_remove_callback(cb_handle)

This is an example of a useful error handling function::

    ! The error handler needs to be declared 'external'
    external error_handler

    ! Compute the length of a string
    ! (this is an utility function that is used by the error handler
    !  to print nicer error messages)
    integer function istrlen(string)
    character string*(*)
      istrlen = len(string)
      do while ((string(istrlen:istrlen).eq." " .or. string(istrlen:istrlen).eq."").and. istrlen.gt.0)
         istrlen = istrlen - 1
      enddo
      return
    end

    ! Generic error handler: print all available information
    ! about the error, then exit
    subroutine error_handler(val)
    integer val
    character buf*1000
      print *,ier," testcb in ",val
      call idba_error_message(buf)
      print *,buf(:istrlen(buf))
      call idba_error_context(buf)
      print *,buf(:istrlen(buf))
      call idba_error_details(buf)
      print *,buf(:istrlen(buf))
      call exit (1)
      return
    end

This code introduces three new functions:

* :c:func:`idba_error_message`:
  returns a string describing what type of error has happened.
* :c:func:`idba_error_context`:
  returns a string describing what DB-All.e was trying to do when the error
  happened.
* :c:func:`idba_error_details`:
  returns a detailed description of the error, when available.  If no detailed
  description is available, it returns an empty string.

A similar error handling behaviour can be obtained by using the predefined
convenience function :c:func:`idba_default_error_handler`::

    ! Declare the external function (not necessary if you include dballeff.h)
    external idba_default_error_handler

    ! Use it as the error handling callback
    ierr = idba_error_set_callback(0, idba_default_error_handler, 1, cb_handle)

An alternative error handler called :c:func:`idba_error_handle_tolerating_overflows`
is available: it exists on all errors instead of value overflows, in what case
it prints a warning to standard error and allows the program to continue.  The
overflow error can then be catched, if needed, by inspecting the error code
returned by the DB-All.e function that causes the error.

This is how to use it::

    ! Declare the external function (not necessary if you include dballeff.h)
    external idba_error_handler_tolerating_overflows

    ! Use it as the error handling callback
    ierr = idba_error_set_callback(0, idba_error_handler_tolerating_overflows, 1, cb_handle)

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

You call :c:func:`idba_connect` to connect to the databse. The parameters are
the database connection URL (see :ref:`connect`), and two parameters that were
used in the past and are now ignored.

You can call :c:func:`idba_begin` many times and get more handles.  This allows
to perform many operations on the database at the same time.

:c:func:`idba_begin` has three extra parameters that can be used to limit
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

To do that, use :c:func:`idba_begin_messages` instead of both :c:func:`idba_connect` and
:c:func:`idba_begin`.  To write a message, your code will look like::

    ! Connect to the database and get a handle to work with it
    ierr = idba_begin_messages(handle, "file.bufr", "r", "auto")

    ! ...do your work...

    ! End of the work
    ierr = idba_commit(handle)

:c:func:`idba_begin_messages` has three parameters:

1. the name of the file to open
2. the open mode (``"r"`` for read, ``"w"`` for write or create, ``"a"`` for append).
   See the documentation of libc's ``fopen`` for more details.
3. the file format.  It can be ``"BUFR"``, ``"CREX"`, or ``"AUTO"``.  ``"AUTO"`` tells
   DB-All.e to autodetect the file format, but it only works when reading
   files, not when writing new one.

You can call :c:func:`idba_begin_messages` many times and read or write many files.  You
can even call :c:func:`idba_begin_messages` many time on the same file as long as you
open it read only.

Once you open a file, you can use the other DB-All.e functions on it.  There
are slight differences between working on a database and working on a file, and
they are explained later in the section :ref:`working_with_files`.


Setting input and reading output
--------------------------------

Input to action routines is done using the functions ``idba_set*``, and output
is read with the functions ``idba_enq*`` (see :ref:`routines`)::

    ! Set the extremes of an area and retrieve all the stations in it
    ierr = idba_setd(handle, "latmin", 30.D0)
    ierr = idba_setd(handle, "latmax", 50.D0)
    ierr = idba_setd(handle, "lonmin", 10.D0)
    ierr = idba_setd(handle, "lonmax", 20.D0)
    ierr = idba_query_stations(handle, count)

    ! Get the informations about a station
    do while (count.gt.0)
      ierr = idba_next_station(handle)
      ierr = idba_enqc(handle, "name", cname)
      ierr = idba_enqi(handle, "ana_id", id)
      ierr = idba_enqd(handle, "lat", lat)
      ierr = idba_enqd(handle, "lon", lon)
      ! ....
      count = count - 1
    enddo

Note that, when one uses :c:func:`idba_setc`, :c:func:`idba_seti`, :c:func:`idba_enqc`,
:c:func:`idba_enqi` with parameters that have some decimal digits, DB-All.e will
work with values as if they did not have a decimal point.  That is, if latitude
`10.124323` is read with :c:func:`idba_enqi`, then the result will be `10124323`.

The following example shows what happens::

    ! Set the latitude to 30.0 degrees
    ierr = idba_setr(handle, "lat", 30.0)
    ! Set the latitude to 30.0 degrees
    ierr = idba_setd(handle, "lat", 30.0D0)

    ! Set the latitude to 0.00030 degrees
    ierr = idba_seti(handle, "lat", 30)
    ! Set !the latitude to 30.0 degrees
    ierr = idba_seti(handle, "lat", 3000000)

    ! Set the latitude to 0.00030 degrees
    ierr = idba_setc(handle, "lat", "30")
    ! Set the latitude to 30.0 degrees
    ierr = idba_setc(handle, "lat", "3000000")


Input/output shortcuts
----------------------

There are a few functions that are shortcuts to other input and output
functions:

:c:func:`idba_enqdate` is a shortcut to::

    idba_enqi(handle, "year", year)
    idba_enqi(handle, "month", month)
    idba_enqi(handle, "day", day)
    idba_enqi(handle, "hour", hour)
    idba_enqi(handle, "min", minute)
    idba_enqi(handle, "sec", second)

:c:func:`idba_setdate` is a shortcut to::

    idba_seti(handle, "year", year)
    idba_seti(handle, "month", month)
    idba_seti(handle, "day", day)
    idba_seti(handle, "hour", hour)
    idba_seti(handle, "min", minute)
    idba_seti(handle, "sec", second)

:c:func:`idba_enqlevel` is a shortcut to::

    idba_enqi(handle, "leveltype1", type1)
    idba_enqi(handle, "l1", l1)
    idba_enqi(handle, "leveltype2", type2)
    idba_enqi(handle, "l2", l2)


:c:func:`idba_setlevel` is a shortcut to::

    idba_seti(handle, "leveltype1", type1)
    idba_seti(handle, "l1", l1)
    idba_seti(handle, "leveltype2", type2)
    idba_seti(handle, "l2", l2)

:c:func:`idba_enqtimerange` is a shortcut to::

    idba_enqi(handle, "pindicator", type)
    idba_enqi(handle, "p1", p1)
    idba_enqi(handle, "p2", p2)

:c:func:`idba_settimerange` is a shortcut to::

    idba_seti(handle, "pindicator", type)
    idba_seti(handle, "p1", p1)
    idba_seti(handle, "p2", p2)


Parameter names
---------------

There are three different kinds of parameter names one can use:

* DB-All.e parameters (see :ref:`parms`), that have a special meaning to
  DB-All.e: for example they can be part of the coordinate system, or
  space/time extremes to use to query the database.  They are indicated simply
  with their name (for example, `"lat"` or `"yearmin"`).
* :ref:`btable`, represent all possible sorts of
  observed data, and are indicated in the form `Bxxyyy`, where `xxyyy` are the
  X and Y values from the WMO table B.
* :ref:`aliases` that are short, easy to remember names which
  can be used instead of frequently used WMO B variables.


Queries and observed data
-------------------------

The ``idba_set*`` and ``idba_enq*`` functions can also be used to set and get
observation data.  To do so, use as parameter the string ``"Bxxyyy"``, where
``xx`` and ``yyy`` are the X and Y values of the BUFR/CREX table B describing
the observed data.

For example::

    ! Set the speed of the wind (very useful in summer)
    ierr = idba_setr(handle, "B11002", 1.8)
    ! Also set the temperature
    ierr = idba_setr(handle, "B12001", 21.8)
    ierr = idba_insert_data(handle)


Attributes
----------

The ``idba_set*`` and ``idba_enq`` groups of functions can also be used to
set and get attributes on data.  To do so, use as parameter the string
``"*Bxxyyy"``, where ``xx`` and ``yyy`` are the X and Y values of the
BUFR/CREX table B describing the attribute.

For example::

    ! Set the confidence of the wind speed value we inserted
    ! in the last 'idba_insert_data'
    ierr = idba_setr(handle, "*B33007", 75.0)
    ierr = idba_setc(handle, "*var_related", "B11002")
    ierr = idba_insert_attributes(handle)


.. _querying:

Querying the database
---------------------

Queries are made by giving one or more extremes of space, time, level or time
range.  See :ref:`parms_query` for a list of all available query parameters.


Querying the station values
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example code to query all the stations in a given area::

    ierr = idba_setd(handle, "latmin", 30.D0)
    ierr = idba_setd(handle, "latmax", 50.D0)
    ierr = idba_setd(handle, "lonmin", 10.D0)
    ierr = idba_setd(handle, "lonmax", 20.D0)
    ierr = idba_query_stations(handle, count)
    do while (count.gt.0)
      ierr = idba_next_station(handle)
      ierr = idba_enqi(handle, "ana_id", id)
      ! Pseudoana values can be read as well:
      ierr = idba_enqc(handle, "name", cname)
      ierr = idba_enqd(handle, "B07001", height)
      ! ...query more data and work with it...
      count = count - 1
    enddo

This code introduces two new functions:

* :c:func:`idba_query_stations`: performs the query and returns the number of stations it
  finds.
* :c:func:`idba_next_station`: gets a station out of the results of :c:func:`idba_query_stations`.
  If there are no more stations, the function fails.

After :c:func:`idba_next_station`, the output record will also contain all the pseudoana
values available for the station.  If `rep_cod` or `rep_memo` are specified as
query parameters, the pseudoana values of that network will be used.  Else,
:c:func:`idba_next_station` will use all available pseudoana values, choosing the one in
the network with the highest priority in case the same pseudoana value is
available on more than one network.


Querying the measured values
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example code to query all the values in a given area and time::

    ierr = idba_seti(handle, "latmin", 30)
    ierr = idba_seti(handle, "latmax", 50)
    ierr = idba_seti(handle, "lonmin", 10)
    ierr = idba_seti(handle, "lonmax", 20)
    ierr = idba_seti(handle, "yearmin", 2004)
    ierr = idba_seti(handle, "yearmax", 2004)
    ierr = idba_query_data(handle, count)
    do while (count.gt.0)
      ierr = idba_next_data(handle, param)
      ! get the value of this variable
      ierr = idba_enqc(handle, param, cvalue)
      ierr = idba_enqd(handle, "lat", dlat)
      ierr = idba_enqd(handle, "lon", dlon)
      ! query more data and work with it
      count = count - 1
    enddo

This code introduces two new functions:

* :c:func:`idba_query_data`: performs the query and returns the number of values it
  finds.
* :c:func:`idba_next_data`: gets a value out of the result of :c:func:`idba_query_data`.  If
  there are no more stations, the function fails.


Clearing the database
---------------------

You can initialise or reinitialise the database using :c:func:`idba_reinit_db`::

   ! Start the work with a clean database
   ierr = idba_reinit_db(handle, "repinfo.csv")

:c:func:`idba_reinit_db` clears the database if it exists, then recreates all the
needed tables.  Finally, it populates the informations about the reports (such
as the available report types, their mnemonics and their priority) using the
data in the file given as argument.

The file is in CSV format, with 6 columns:

1. Report code (corresponding to parameter ``rep_cod``)
2. Mnemonic name (corresponding to parameter ``rep_memo``)
3. Report description
4. Report priority (corresponding to parameter ``priority``)
5. Ignored
6. Ignored

If ``""`` is given instead of the file name, :c:func:`idba_reinit_db` will read the
data from ``/etc/repinfo.csv``.

.. highlight:: csv

This is an example of the contents of the file::

    01,synop,report synottico,100,oss,0
    02,metar,metar,80,oss,0
    03,temp,radiosondaggio,100,oss,2
    04,ana_lm,valori analizzati LM,-1,ana,255
    05,ana,analisi,-10,pre,255
    06,pre_cleps_box1.5maxel001,previsti cosmo leps box 1.5 gradi valore max elemento 1,-1,pre,255
    07,pre_lmn_box1.5med,previzione Lokal Model nudging box 1.5 gradi valore medio,-1,pre,255
    08,pre_lmp_spnp0,previsione Lkal Model prognostica interpolato punto piu' vicino,-1,pre,255
    09,boe,dati omdametrici,100,oss,31

.. highlight:: fortran

:c:func:`idba_reinit_db` will not work unless ``rewrite`` has been enabled for the
data when opening the database.


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


Deleting data
-------------

Data is deleted using :c:func:`idba_remove_data`::

    ! Delete all data from the station with id 4 in year 2002
    ierr = idba_seti(handle, "ana_id", 4)
    ierr = idba_seti(handle, "year", 2002)
    ierr = idba_remove_data(handle)

This code introduces a new function:

* :c:func:`idba_remove_data`: deletes all the data found in the extremes specified in input.

:c:func:`idba_remove_data` will not work unless ``rewrite`` has been enabled for
the data when opening the database.


Reading attributes
------------------

Attributes are read using :c:func:`idba_next_attribute`::

    ! ...setup a query...
    idba_query_data(handle, count)
    do while (count.gt.0)
      ierr = idba_next_data(handle, param)

      ! Read QC informations about the last value read
      ierr = idba_query_attributes(handle, qc_count)
      do while (qc_count.gt.0)
          ierr = idba_next_attribute(handle, param) 
          ierr = idba_enqc(handle, param, value)
          ! ...process the value...
          qc_count = qc_count - 1
      enddo

      count = count - 1
    enddo

This code introduces two new functions:

* :c:func:`idba_query_attributes`:
  Performs a query to retrieve attributes for the last variable read by
  :c:func:`idba_next_data`.  It returns the number of attributes available.
* :c:func:`idba_next_attribute`:
  Retrieves one by one the values queried by :c:func:`idba_query_attributes` if
  there are no more items available, the function will fail.

  The parameter ``param`` will be set to the name (in the form ``*Bxxyyy``) of
  the attribute just read.

It is possible to read attributes at a later time giving a context ID and a B
table value::

    ! Read the context ID after a insert_data or a next_data
    idba_enqi(handle, "context_id", id)

    ! ...a while later...

    ! Query the attributes of the variable with the given
    ! context ID and B table value
    idba_seti(handle, "*context_id", id)
    idba_seti(handle, "*var_related", "B12001")

    ! These are ways one could choose specific attributes:
    ! one attribute: idba_setc(handle, "*var", "B33007")
    ! some attributes: idba_setc(handle, "*varlist", "B33007,B33036")
    ! by default, all attributes are returned

    ! Read QC informations about the last value read
    ierr = idba_query_attributes(handle, qc_count)
    do while (qc_count.gt.0)
        ierr = idba_next_attribute(handle, param) 
        ierr = idba_enqc(handle, param, value)
        ! ...process the value...
        qc_count = qc_count - 1
    enddo


Writing attributes
------------------

Attributes are written using :c:func:`idba_insert_attributes`, which can be used after an
:c:func:`idba_next_data`, after an :c:func:`idba_insert_data` or at any time using a stored data
id.  These three case differ on how to communicate to :c:func:`idba_insert_attributes` what is
the data about which to write attributes.

When used after :c:func:`idba_next_data`, :c:func:`idba_insert_attributes` can refer directly to the
last data retrieved::

    ! ...setup a query...
    ierr = idba_query_data(handle, count)
    do while (count.gt.0)
      ierr = idba_next_data(handle, param)
      ! ...process data...

      ! Set the attributes
      ierr = idba_seti(handle, "*B33007", 75)
      ierr = idba_seti(handle, "*B33006", 42)
      ierr = idba_insert_attributes(handle)

      count = count - 1
    enddo

After an :c:func:`idba_insert_data` instead, since :c:func:`idba_insert_data` can write more than
one data at a time, we need to tell :c:func:`idba_insert_attributes` which of them we are
referring to::

    ! Insert wind speed and temperature
    ierr = idba_setr(handle, "B11002", 1.8)
    ierr = idba_setr(handle, "B12001", 22)
    ierr = idba_insert_data(handle)

    ! Set the attributes
    ierr = idba_seti(handle, "*B33007", 75)

    ! Use "*var_related" to indicate which of the two variables we are annotating
    ierr = idba_setc(handle, "*var_related", "B11002")

    ierr = idba_insert_attributes(handle)

:c:func:`idba_insert_attributes` can also be called at any time using a previously stored data it::

    ! ...perform a query with idba_query_data...
    do while (count.gt.0)
      ierr = idba_next_data(handle, param)
      ! ...process data...

      ! This variable is interesting: save the context ID
      ! to refer to it later
      ierr = idba_enqi(handle, "context_id", saved_id)

      count = count - 1
    enddo

    ! ...some time later...

    ! Insert attributes about that interesting variable
    ierr = idba_seti(handle, "*B33007", 75)
    ierr = idba_seti(handle, "*B33006", 42)

    ! Select the variable using its context id
    ! and variable code
    ierr = idba_seti(handle, "*context_id", saved_id)
    ierr = idba_seti(handle, "*var_related", "B11001")
    ierr = idba_insert_attributes(handle)

This code introduces a new function:

* :c:func:`idba_insert_attributes`
  Set one or more attributes about a variable.
  
  The variable can be identified directly by using ``idba_seti(handle, "*context_id", id)`` and ``idba_seti(handle, "*var_related", name)``.
  These parameters are automatically set by the :c:func:`idba_next_data` and
  :c:func:`idba_insert_data` action routines.

  The attributes and values are set as input to :c:func:`idba_insert_attributes` using the
  `idba_set*` functions with an asterisk in front of the variable name.

:c:func:`idba_insert_attributes` will work in different ways according to the attributes
opening mode of the database:

* ``"read"``: attributes cannot be modified in any way.
* ``"rewrite"``: attributes can be added, and existing attributes can be
  overwritten.


Deleting attributes
-------------------

Attributes are deleted using :c:func:`idba_remove_attributes`::

    ! Delete the confidence interval from the wind speed

    ! The referring variable is identified in the same way as with
    ! idba_insert_attributes:
    ierr = idba_seti(handle, "*context_id", saved_id)
    ierr = idba_seti(handle, "*var_related", "B11002")

    ! The attributes to delete are selected by setting "*varlist":
    ierr = idba_setc(handle, "*varlist", "*B33007")
    ierr = idba_remove_attributes(handle)

This code introduces a new function:

* :c:func:`idba_remove_attributes`:
  Delete attributes from a variable identified in the same way as with

:c:func:`idba_remove_attributes` will not work unless the database has been opened in
attribute `rewrite` mode.


Ending the work
---------------

When you are finished working with a handle, you release it with
:c:func:`idba_commit`::

    ! We are finished with this handle
    ierr = idba_commit(handle)

When you are finished working with DB-ALLe, you use :c:func:`idba_disconnect` to
close all connections and release all resources::

    ! We do not need to work with dballe anymore
    ierr = idba_disconnect(dbh)


Shortcuts to stations and data
------------------------------

DB-All.e offers two shortcuts to represent pseudoana entries and data in the
database: the ``ana_id`` and the ``data_id`` keys, that are set in the
output of every :c:func:`idba_next_data`.

``ana_id`` represents a pseudoana entry.  Every time one needs to specify a
set of latitude, longitude, fixed/mobile, one could use the corresponding
``ana_id`` value, if known, and get a faster search.

``data_id`` represents a data entry.  Every time one needs to identify some
data setting latitude, longitude, level layer, time range and so on, one can
just provide the value of ``data_id``, and also get a faster search.


Helpers for pretty printing
---------------------------

There are a number of functions in DB-All.e, the ``idba_spiega\*`` group of
functions, that are not needed for normal processing but can be useful to
improve the presentation of data to users.

All these function take a number of parameters and a string, and they store a
proper description of the values into the string.

The functions are:

* :c:func:`idba_describe_level`:
  Describes a level.  For example, ``idba_describe_level(handle,106,10,106,20,string)``
  will store in ``string`` something like: *"Layer between 10hm and
  20hm above ground"*.
* :c:func:`idba_describe_timerange`:
  Describes a time range.  For example, ``idba_describe_timerange(handle,3,0,600,string)``
  will store in ``string`` something like: *"Average between reference
  time+0s to reference time+600s"*.
* :c:func:`idba_describe_var(handle,varcode,value,string)`:
  Describe a value.  For example, ``idba_describe_var(handle,"B12001","280",string)``
  will store in ``string`` something like: *"280 (K) TEMPERATURE/DRY-BULB
  TEMPERATURE"*.


Modifiers for queries
---------------------

DB-All.e allows to set special query behaviours using the ``"query"``
parameter.  The available options are:

* ``best``: When measures from different kinds of reports exist in the same
  physical space, do not return them all, but only return the one of the record
  type with the highest priority.


.. _working_with_files:

Working with files
------------------

This is a list of the differences between working with files and working with
databases:

* You do not need to call :c:func:`idba_connect` and :c:func:`idba_disconnect`: the work
  session starts at :c:func:`idba_begin_messages` and ends at :c:func:`idba_commit`
* When reading, performing :c:func:`idba_query_stations` or :c:func:`idba_query_data` a second
  time advances to the next message in the file.
* Query parameters set before an :c:func:`idba_query_data` have no effect: filtering
  data is not implemented for files. Since it may be implemented in the future,
  it is suggested to avoid setting query parameters before an
  :c:func:`idba_query_data` to avoid unexpected changes of behaviour with future
  versions of DB-All.e.
* When reading, you will see that there are no more messages because
  :c:func:`idba_query_stations` or :c:func:`idba_query_data` will return 0.
* When writing, you can use the `query` input parameter to :c:func:`idba_insert_data` to
  control when a new message is started.  If you set it to `subset`, then the
  data will be inserted in a new BUFR or CREX subset.  If you set it to
  `message`, you will start a new message.

  After `"message"` you can specify a template for the message, using one of
  the names listed by `dbadb export -t list`, for example: `"message generic"`.
  If you do not specify a template name, an appropriate template will
  automatically be chosen for you.
* Setting `rep_memo` you can influence the output template of messages: if you
  set it to a synop report code, you will create a synop message.


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


Query data, then query station data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    ierr = idba_begin(dbhandle, handle, "read", "read", "read")
    ierr = idba_begin(dbhandle, handleana, "read", "read", "read")
    
    ! Prepare a query
    ierr = idba_setd (handle, "latmin", 10)
    ...
    ierr = idba_setd (handle, "lonmax", 60)
    
    ! Make the query
    ierr = idba_query_data (handle, N)
    
    ! Iterate the results
    do i=1,N
      ierr = idba_next_data (handle, varname)
    
      ! Read data about the variable we just had
      ierr = idba_enqlevel (handle, ltype, l1, l2)
    
      ! Read pseudoana data about the variable we just had
      ! Setup a query for the station with 'query_stations'
      ierr = idba_enqi (handle, "ana_id", anaid)
      ierr = idba_seti (handleana, "ana_id", anaid)
    
      ! Query.  Nstaz should always be 1 because we query a specific station
      ierr = idba_query_stations (handleana, Nstaz)
    
      ! Fetch the data
      ierr = idba_next_station (handleana)
    
      ! Read the data about the station
      ! All the data inserted with set_station_context is available here
      ierr = idba_enqi (handleana, "height", height)
    enddo
