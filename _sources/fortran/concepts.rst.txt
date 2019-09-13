Fortran API concepts
====================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran

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
session with :c:func:`idba_commit`, then all the changes made will be preserved. If
you do not call :c:func:`idba_commit`, such as if your program unexpectedly
terminates, then all the changes made will be undone.

.. _report:

Report
------

A homogeneous group of stations with the same kind of measures and management
(for example: ``synop``, ``metar``, a specific kind of forecast...).

The type of report implicitly defines a priority of the measured value over
other equivalent values.  This can be used to select a "best value" from a
specific set of coordinates, where for example, values measured by synoptic
stations are preferred over values measured by a satellite, which in turn is
preferred over a value computed by a forecast model.

Priorities associated to the report type can be customized when creating the
database (see :c:func:`idba_reinit_db`).

.. _station:

Station
-------

Stations are uniquely identified by:

* *latitude*.
* *longitude*.
* *station identifier* (if the station is mobile).
* *rep_memo*


Station values
--------------

Each station entry can have zero or more values associated to it. They are just
like measured variables, but lack date, time, level, and time range
information. They can be used to store station attributes, like the height
above sea level, or the center operating it.

The interface to work with station values is the same as the interface to work
with normal values, except that date, time, level, and timerange information
are not set when inserting or querying data, and
:c:func:`idba_set_station_context` is called instead, to signal the
intention of working with station values.

See :ref:`querying` for examples on how to work with values.

Level or layer
--------------

The vertical coordinate of a value in DB-All.e is described using a level
description or a layer description.

A level is defined univocally by a code table (``leveltype1``) and a
numerical value (``l1``).

A layer is defined univocally by its two bounding levels (``leveltype1``,
``l1``, ``leveltype2``, ``l2``). 

See :ref:`levels` for a description of the level types and
associated level values.

Time range
----------

Another characteristic of a value in DB-All.e is the time period to which the
datum refers: it can be, for example, a value measured in a specific instant,
or a cumulated or averaged value over an interval of time.

The time range is defined by a table code (``pindicator``) and two
numerical values (``p1`` and ``p2``). For their meaning, see :ref:`tranges`.

.. _varcode:

Variable code (varcode)
-----------------------

A code from a local variation of BUFR/CREX Table B that describes the nature of
the variable, such as what it measures and the units used for measuring. It is
identified by a *local B table descriptor* (see :ref:`btable`).

Value
-----

A measured value is a datum that can be expressed as a floating point (single
or double precision), integer, character or byte, depending on what value it
measures.

Every datum is univocally defined by a system of coordinates with 7 dimensions:

1. A report type (see :ref:`report`).
2. The variable code (see :ref:`varcode`).
3. The *date and time* of validity of the datum, be it observed, forecast or
   estimated.
4. *station* information about what generated the value (also defining its x
   and y coordinates).
5. The vertical coordinates in space, as a description of the *level*.
6. The *time range* of the measurement.

Attributes
----------

Values in DB-All.e are also associated to zero or more *attributes*.  An
attribute is a value that describes an aspect of a variable, like for example a
confidence interval.

Attributes in DB-All.e consist on a value and a local B table descriptor,
which describes the nature of the variable, such as what it represents and the
units in which it is represented.

Attributes are uniquely identified by the variable they refer to and the B
table descriptor of the attribute.  As a consequence, you can only have one
attribute per value with a specific B table descriptor.  This means that, for
example, a variable can have at most one confidence interval attribute.

.. _routines:

Input, output, actions
----------------------

Work with DB-All.e happens using *action routines*.  An action routine
typically reads some input, performs an action and produces some output.
Example of action routines are [idba_query_data][] to query data from a
database and [idba_insert_data][] to write data into the database.

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

Note that all [idba_set][] functions set input parameters, while all `idb_enq*`
functions read output parameters.  You cannot read input parameters or set
output parameters: that is the job of action routines.

In other words, input and output parameters are different things.  In this
code::

    ! A possible misunderstanding
    ierr = idba_seti(handle, "height", 1)
    ierr = idba_enqi(handle, "height", val)

the value of `val` after the :c:func:`idba_enqi` will not probably be 1, and
it could be either a value indicating ``missing value`` (in case no
``height`` parameter is set in the output parameters) or a ``height`` value
previously retrieved by an action routine.

To reset one input parameter you can use :c:func:`idba_unset`::

    ! We do not want to limit results by latitude this time
    ierr = idba_unset(handle, "latmin")
    ierr = idba_unset(handle, "latmax")
    ierr = idba_query_data(handle, count)

Alternatively, you can reset an input parameter by setting it to one of the
special ``missing value`` values listed below.

To reset all input parameters you can use :c:func:`idba_unsetall`::

    ! Restart the query from scratch
    ierr = idba_unsetall(handle)
    ierr = idba_setd(handle, "latmin", 10.D0)

To reset only Bxxyyy variables you can use :c:func:`idba_unsetb`::

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
