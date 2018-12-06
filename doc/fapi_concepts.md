# Fortran API concepts

<a name="sessions"></a>
### Connections, sessions and handles

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
session with [idba_commit][], then all the changes made will be preserved. If
you do not call [idba_commit][], such as if your program unexpectedly
terminates, then all the changes made will be undone.

<a name="report"></a>
### Report

A homogeneous group of stations with the same kind of measures and management
(for example: `synop`, `metar`, a specific kind of forecast...).

The type of report implicitly defines a priority of the measured value over
other equivalent values.  This can be used to select a "best value" from a
specific set of coordinates, where for example, values measured by synoptic
stations are preferred over values measured by a satellite, which in turn is
preferred over a value computed by a forecast model.

Priorities associated to the report type can be customized when creating the
database (see [[idba_reinit_db][]](fapi_reference.md#idba_reinit_db)).

<a name="station"></a>
### Station

Stations are uniquely identified by:

* *latitude*.
* *longitude*.
* *station identifier* (if the station is mobile).
* *rep_memo* (currently only in `memdb:` databases; in the future, on all databases)

<a name="station_value"></a>
### Station values

Each station entry can have zero or more values associated to it. They are just
like measured variables, but lack date, time, level, and time range
information. They can be used to store station attributes, like the height
above sea level, or the center operating it.

The interface to work with station values is the same as the interface to work
with normal values, except that date, time, level, and timerange information
are not set when inserting or querying data, and
[[idba_setcontextana][]](fapi_reference.md#idba_setcontextana) is called instead, to signal the
intention of working with station values.

See section \ref{sec-querying} for examples on how to work with values.

<a name="level"></a>
### Level or layer

The vertical coordinate of a value in DB-All.e is described using a level
description or a layer description.

A level is defined univocally by a code table (`leveltype1`) and a
numerical value (`l1`).

A layer is defined univocally by its two bounding levels (`leveltype1`,
`l1`, `leveltype2`, `l2`). 

See [the levels table](fapi_levels.md) for a description of the level types and
associated level values.

<a name="trange"></a>
### Time range

Another characteristic of a value in DB-All.e is the time period to which the
datum refers: it can be, for example, a value measured in a specific instant,
or a cumulated or averaged value over an interval of time.

The time range is defined by a table code (`pindicator`) and two
numerical values (`p1` and `p2`). For their meaning, see [the Time ranges
table](fapi_tranges.md).

<a name="varcode"></a>
### Variable code (varcode)

A code from a local variation of BUFR/CREX Table B that describes the nature of
the variable, such as what it measures and the units used for measuring. It is
identified by a [*local B table descriptor*](fapi_btable.md).

<a name="value"></a>
### Value

A measured value is a datum that can be expressed as a floating point (single
or double precision), integer, character or byte, depending on what value it
measures.

Every datum is univocally defined by a system of coordinates with 7 dimensions:

1. A [report](#report) type.
2. The [variable code](#varcode).
3. The *date and time* of validity of the datum, be it observed, forecast or
   estimated.
4. *station* information about what generated the value (also defining its x
   and y coordinates).
5. The vertical coordinates in space, as a description of the *level*.
6. The *time range* of the measurement.

See section [Querying](#sec_querying) for examples on how to work with values.

<a name="attributes"></a>
### Attributes

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

See section [Attributes](#sec_attrs) for examples on how to handle attributes.

<a name="routines"></a>
### Input, output, actions

Work with DB-All.e happens using *action routines*.  An action routine
typically reads some input, performs an action and produces some output.
Example of action routines are [idba_query_data][] to query data from a
database and [idba_prendilo][] to write data into the database.

The input and the output of action routines are collections of parameters which
have a name and a value.  A list of parameters can be found [in the
appendix](#parmtable).

You can set the input parameters using the `idba_set*` functions:
<a name="fun_idba_set"></a>

* `idba_seti(handle, "param", intvalue)`: Set the input parameter to the
  integer value `intvalue`.
* `idba_setc(handle, "param", charvalue)`:
  Set the input parameter to the character value `charvalue`
* `idba_setr(handle, "param", realvalue)`:
  Set the input parameter to the real value `realvalue`
* `idba_setd(handle, "param", doublevalue)`:
  Set the input parameter to the `real*8` value `doublevalue`
* `idba_setcontextana(handle)`:
  <a name="fun_setcontextana"></a>
  Sets the date, time, level and time range in the input record to some magic
  values, to signal that we are going to work with station attributes instead
  of normal values.

You can read the output parameters using the `idba_enq*` functions:
<a name="fun_idba_enq"></a>

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
code:

```fortran
      ! A possible misunderstanding
      ierr = idba_seti(handle, "height", 1)
      ierr = idba_enqi(handle, "height", val)
```

the value of `val` after the [idba_enqi][] will not probably be 1, and
it could be either a value indicating ``missing value'' (in case no
`height` parameter is set in the output parameters) or a `height` value
previously retrieved by an action routine.

To reset one input parameter you can use [idba_unset][]:
<a name="fun_idba_unset"></a>

```fortran
      ! We do not want to limit results by latitude this time
      ierr = idba_unset(handle, "latmin")
      ierr = idba_unset(handle, "latmax")
      ierr = idba_query_data(handle, count)
```

Alternatively, you can reset an input parameter by setting it to one of the
special ``missing value'' values listed below.

To reset all input parameters you can use [idba_unsetall][]:
<a name="fun_idba_unsetall"></a>

```fortran
      ! Restart the query from scratch
      ierr = idba_unsetall(handle)
      ierr = idba_setd(handle, "latmin", 10.D0)
```

To reset only Bxxyyy variables you can use [idba_unsetb][]:
<a name="fun_idba_unsetb"></a>

```fortran
      ! Insert a new variable with the old station, level and so on
      ierr = idba_unsetb(handle)
      ierr = idba_setd(handle, "B12101", 21.5)
```

There is no way to reset output parameters: it is not needed since all action
routines will get rid of old output values before producing new ones.

In case one of the `idba_enq*` functions is called on a parameter which
does not exist, it will return a special value that indicates "missing
value".  This is a list of such special values:

<table class="table">
<thead>
<tr><th>Data Type</th><th>Missing value indicator</th></tr>
</thead>
<tbody>
<tr><td>String </td><td> ""             </tr>
<tr><td>Int    </td><td> 0x7fffffff     </tr>
<tr><td>Real   </td><td> -1.1754944E-38 </tr>
<tr><td>Double </td><td> -2.22507E-308  </tr>
</tbody>
</table>


[idba_error_code]: fapi_reference.md#idba_error_code
[idba_error_message]: fapi_reference.md#idba_error_message
[idba_error_context]: fapi_reference.md#idba_error_context
[idba_error_details]: fapi_reference.md#idba_error_details
[idba_error_set_callback]: fapi_reference.md#idba_error_set_callback
[idba_error_remove_callback]: fapi_reference.md#idba_error_remove_callback
[idba_default_error_handler]: fapi_reference.md#idba_default_error_handler
[idba_error_handle_tolerating_overflows]: fapi_reference.md#idba_error_handle_tolerating_overflows
[idba_connect]: fapi_reference.md#idba_connect
[idba_disconnect]: fapi_reference.md#idba_disconnect
[idba_begin]: fapi_reference.md#idba_begin
[idba_begin_messages]: fapi_reference.md#idba_begin_messages
[idba_commit]: fapi_reference.md#idba_commit
[idba_seti]: fapi_reference.md#idba_seti
[idba_setb]: fapi_reference.md#idba_setb
[idba_setr]: fapi_reference.md#idba_setr
[idba_setd]: fapi_reference.md#idba_setd
[idba_setc]: fapi_reference.md#idba_setc
[idba_enqi]: fapi_reference.md#idba_enqi
[idba_enqb]: fapi_reference.md#idba_enqb
[idba_enqr]: fapi_reference.md#idba_enqr
[idba_enqd]: fapi_reference.md#idba_enqd
[idba_enqc]: fapi_reference.md#idba_enqc
[idba_unset]: fapi_reference.md#idba_unset
[idba_unsetb]: fapi_reference.md#idba_unsetb
[idba_unsetall]: fapi_reference.md#idba_unsetall
[idba_setcontextana]: fapi_reference.md#idba_setcontextana
[idba_setlevel]: fapi_reference.md#idba_setlevel
[idba_settimerange]: fapi_reference.md#idba_settimerange
[idba_setdate]: fapi_reference.md#idba_setdate
[idba_setdatemin]: fapi_reference.md#idba_setdatemin
[idba_setdatemax]: fapi_reference.md#idba_setdatemax
[idba_enqlevel]: fapi_reference.md#idba_enqlevel
[idba_enqtimerange]: fapi_reference.md#idba_enqtimerange
[idba_enqdate]: fapi_reference.md#idba_enqdate
[idba_reinit_db]: fapi_reference.md#idba_reinit_db
[idba_query_stations]: fapi_reference.md#idba_query_stations
[idba_next_station]: fapi_reference.md#idba_next_station
[idba_query_data]: fapi_reference.md#idba_query_data
[idba_next_data]: fapi_reference.md#idba_next_data
[idba_prendilo]: fapi_reference.md#idba_prendilo
[idba_dimenticami]: fapi_reference.md#idba_dimenticami
[idba_remove_all]: fapi_reference.md#idba_remove_all
[idba_voglioancora]: fapi_reference.md#idba_voglioancora
[idba_ancora]: fapi_reference.md#idba_ancora
[idba_critica]: fapi_reference.md#idba_critica
[idba_scusa]: fapi_reference.md#idba_scusa
[idba_messages_open_input]: fapi_reference.md#idba_messages_open_input
[idba_messages_open_output]: fapi_reference.md#idba_messages_open_output
[idba_messages_read_next]: fapi_reference.md#idba_messages_read_next
[idba_messages_write_next]: fapi_reference.md#idba_messages_write_next
[idba_spiegal]: fapi_reference.md#idba_spiegal
[idba_spiegat]: fapi_reference.md#idba_spiegat
[idba_spiegab]: fapi_reference.md#idba_spiegab
