Fortran API concepts
====================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran

This is a list of basic concepts used by the Fortran API.

See :ref:`concepts` for general concepts.

.. _sessions:

Connections, sessions and handles
---------------------------------

DB-All.e stores meteorological values in a database.  This database can be
optionally shared or accessed using the network.

You contact DB-All.e by creating a *connection*, and you work in DB-All.e using
one or more *sessions*.  A connection is an established link between your
program and DB-All.e, and you usually open it at the beginning of your work and
close it at the end.

Within a connection you can create many working sessions.  This is used to do
different things at the same time, like reading a set of values while writing
computed results.

You can also set some safety features on sessions: for example, when you create
a session for reading values you can set it to disable all writes, which helps
you catch some programming mistakes.

You refer to the connection and the sessions using *handles*.  A handle is an
integer value that you use when you need to refer to something that is not
otherwise representable in any Fortran data type.

When you create a connection or a session, DB-All.e will give you the integer
handle that you will later use to refer to it.

Sessions run with their own database transaction, which means that the changes
they perform on data are handled in an all-or-nothing fashion. If you close a
session with :ref:`idba_commit`, then all the changes made will be preserved. If
you do not call :ref:`idba_commit`, such as if your program unexpectedly
terminates, then all the changes made will be undone.


Station values
--------------

The interface to work with station values is the same as the interface to work
with normal values, except that date, time, level, and timerange information
are not set when inserting or querying data, and
:ref:`idba_set_station_context` is called instead, to signal the
intention of working with station values.

See :ref:`querying` for examples on how to work with values.


.. _routines:

Input, output, actions
----------------------

Work with DB-All.e happens using *action routines*.  An action routine
typically reads some input, performs an action and produces some output.
Example of action routines are :ref:`idba_query_data` to query data from a
database and :ref:`idba_insert_data` to write data into the database.

The input and the output of action routines are collections of parameters which
have a name and a value.  A list of parameters can be found in :ref:`parms`.

You can set the input parameters using the `idba_set*` functions:

* `idba_seti(handle, "param", intvalue)`: Set the input parameter to the
  integer value `intvalue`.
* `idba_setc(handle, "param", charvalue)`:
  Set the input parameter to the character value `charvalue`
* `idba_setr(handle, "param", realvalue)`:
  Set the input parameter to the real value `realvalue`
* `idba_setd(handle, "param", doublevalue)`:
  Set the input parameter to the `real*8` value `doublevalue`
* `idba_set_station_context(handle)`:
  Sets the date, time, level and time range in the input record to some magic
  values, to signal that we are going to work with station attributes instead
  of normal values.

You can read the output parameters using the `idba_enq*` functions:

* `idba_enqi(handle, "param", intvalue)`:
  Read the output parameter into the integer value `intvalue`
* `idba_enqc(handle, "param", charvalue)`:
  Read the output parameter into the character value `charvalue`
* `idba_enqr(handle, "param", realvalue)`:
  Read the output parameter into the real value `realvalue`
* `idba_enqd(handle, "param", doublevalue)`:
  Read the output parameter into the `real*8` value `doublevalue`

Note that all ``idba_set*`` functions set input parameters, while all ``idb_enq*``
functions read output parameters.  You cannot read input parameters or set
output parameters: that is the job of action routines.

In other words, input and output parameters are different things.  In this
code::

    ! A possible misunderstanding
    ierr = idba_seti(handle, "height", 1)
    ierr = idba_enqi(handle, "height", val)

the value of `val` after the :ref:`idba_enqi` will not probably be 1, and
it could be either a value indicating ``missing value`` (in case no
``height`` parameter is set in the output parameters) or a ``height`` value
previously retrieved by an action routine.

To reset one input parameter you can use :ref:`idba_unset`::

    ! We do not want to limit results by latitude this time
    ierr = idba_unset(handle, "latmin")
    ierr = idba_unset(handle, "latmax")
    ierr = idba_query_data(handle, count)

Alternatively, you can reset an input parameter by setting it to one of the
special ``missing value`` values listed below.

To reset all input parameters you can use :ref:`idba_unsetall`::

    ! Restart the query from scratch
    ierr = idba_unsetall(handle)
    ierr = idba_setd(handle, "latmin", 10.D0)

To reset only Bxxyyy variables you can use :ref:`idba_unsetb`::

    ! Insert a new variable with the old station, level and so on
    ierr = idba_unsetb(handle)
    ierr = idba_setd(handle, "B12101", 21.5)

There is no way to reset output parameters: it is not needed since all action
routines will get rid of old output values before producing new ones.

In case one of the ``idba_enq*`` functions is called on a parameter which
does not exist, it will return a special value that indicates "missing
value".  This is a list of such special values:

=========== =======================
Data Type   Missing value indicator
=========== =======================
String      ``""``
Int         ``0x7fffffff``
Real        ``-1.1754944E-38``
Double      ``-2.22507E-308``
=========== =======================
