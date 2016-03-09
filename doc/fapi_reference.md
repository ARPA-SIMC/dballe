# Fortran API reference

## Summary of routines

### Error management routines

<table class="table">
<thead>
<tr>
    <th>Name</th>
    <th>Description</th>
</th>
</thead>
<tbody>
<tr><td><code><a href='#idba_error_code'>idba_error_code()</a></code></td><td>Return the error code for the last function that was called.</td></tr>
<tr><td><code><a href='#idba_error_message'>idba_error_message(message)</a></code></td><td>Return the error message for the last function that was called.</td></tr>
<tr><td><code><a href='#idba_error_context'>idba_error_context(message)</a></code></td><td>Return a string describing the error context description for the last function that was called.</td></tr>
<tr><td><code><a href='#idba_error_details'>idba_error_details(message)</a></code></td><td>Return a string with additional details about the error for the last function that was called.</td></tr>
<tr><td><code><a href='#idba_error_set_callback'>idba_error_set_callback(code, func, data, handle)</a></code></td><td>Set a callback to be invoked when an error of a specific kind happens.</td></tr>
<tr><td><code><a href='#idba_error_remove_callback'>idba_error_remove_callback(handle)</a></code></td><td>Remove a previously set callback.</td></tr>
<tr><td><code><a href='#idba_default_error_handler'>idba_default_error_handler(debug)</a></code></td><td>Predefined error callback that prints a message and exits.</td></tr>
<tr><td><code><a href='#idba_error_handle_tolerating_overflows'>idba_error_handle_tolerating_overflows(debug)</a></code></td><td>Predefined error callback that prints a message and exists, except in case of overflow errors.</td></tr>
</tbody>
</table>

### Session routines

These routines are used to begin and end working sessions with
DB-All.e.
<table class="table">
<thead>
<tr>
    <th>Name</th>
    <th>Description</th>
</th>
</thead>
<tbody>
<tr><td><code><a href='#idba_presentati'>idba_presentati(dbahandle, dsn, user, password)</a></code></td><td>Connect to the database.</td></tr>
<tr><td><code><a href='#idba_arrivederci'>idba_arrivederci(dbahandle)</a></code></td><td>Disconnect from the database.</td></tr>
<tr><td><code><a href='#idba_preparati'>idba_preparati(dbahandle, handle, anaflag, dataflag, attrflag)</a></code></td><td>Open a new session.</td></tr>
<tr><td><code><a href='#idba_messaggi'>idba_messaggi(handle, filename, mode, type)</a></code></td><td>Start working with a message file.</td></tr>
<tr><td><code><a href='#idba_fatto'>idba_fatto(handle)</a></code></td><td>Close a session.</td></tr>
</tbody>
</table>

### Input/output routines

These routines are used to set the input and read the output of action
routines.
<table class="table">
<thead>
<tr>
    <th>Name</th>
    <th>Description</th>
</th>
</thead>
<tbody>
<tr><td><code><a href='#idba_seti'>idba_seti(handle, parameter, value)</a></code></td><td>Set an integer value in input.</td></tr>
<tr><td><code><a href='#idba_setb'>idba_setb(handle, parameter)</a></code></td><td>Set a byte value in input.</td></tr>
<tr><td><code><a href='#idba_setr'>idba_setr(handle, parameter, value)</a></code></td><td>Set a real value in input.</td></tr>
<tr><td><code><a href='#idba_setd'>idba_setd(handle, parameter, value)</a></code></td><td>Set a real*8 value in input.</td></tr>
<tr><td><code><a href='#idba_setc'>idba_setc(handle, parameter, value)</a></code></td><td>Set a character value in input.</td></tr>
<tr><td><code><a href='#idba_enqi'>idba_enqi(handle, parameter, value)</a></code></td><td>Read an integer value from the output.</td></tr>
<tr><td><code><a href='#idba_enqb'>idba_enqb(handle, parameter)</a></code></td><td>Read a byte value from the output.</td></tr>
<tr><td><code><a href='#idba_enqr'>idba_enqr(handle, parameter, value)</a></code></td><td>Read a real value from the output.</td></tr>
<tr><td><code><a href='#idba_enqd'>idba_enqd(handle, parameter, value)</a></code></td><td>Read a real*8 value from the output.</td></tr>
<tr><td><code><a href='#idba_enqc'>idba_enqc(handle, parameter, value)</a></code></td><td>Read a character value from the output.</td></tr>
<tr><td><code><a href='#idba_unset'>idba_unset(handle, parameter)</a></code></td><td>Remove one value from the input.</td></tr>
<tr><td><code><a href='#idba_unsetb'>idba_unsetb(handle)</a></code></td><td>Remove all Bxxyyy values from the input.</td></tr>
<tr><td><code><a href='#idba_unsetall'>idba_unsetall(handle)</a></code></td><td>Completely clear the input, removing all values.</td></tr>
<tr><td><code><a href='#idba_setcontextana'>idba_setcontextana(handle)</a></code></td><td>Signal that the input values that are set are related to station values instead of normal variables.</td></tr>
</tbody>
</table>

### Input/output shortcuts

The following routines are shortcuts for common combinations of
Input/Output routines.
<table class="table">
<thead>
<tr>
    <th>Name</th>
    <th>Description</th>
</th>
</thead>
<tbody>
<tr><td><code><a href='#idba_setlevel'>idba_setlevel(handle, ltype1, l1, ltype2, l2)</a></code></td><td>Set all level information.</td></tr>
<tr><td><code><a href='#idba_settimerange'>idba_settimerange(handle, ptype, p1, p2)</a></code></td><td>Set all time range information.</td></tr>
<tr><td><code><a href='#idba_setdate'>idba_setdate(handle, year, month, day, hour, min, sec)</a></code></td><td>Set all date information.</td></tr>
<tr><td><code><a href='#idba_setdatemin'>idba_setdatemin(handle, year, month, day, hour, min, sec)</a></code></td><td>Set the minimum date for a query.</td></tr>
<tr><td><code><a href='#idba_setdatemax'>idba_setdatemax(handle, year, month, day, hour, min, sec)</a></code></td><td>Set the maximum date for a query.</td></tr>
<tr><td><code><a href='#idba_enqlevel'>idba_enqlevel(handle, ltype1, l1, ltype2, l2)</a></code></td><td>Read all level information.</td></tr>
<tr><td><code><a href='#idba_enqtimerange'>idba_enqtimerange(handle, ptype, p1, p2)</a></code></td><td>Read all time range information.</td></tr>
<tr><td><code><a href='#idba_enqdate'>idba_enqdate(handle, year, month, day, hour, min, sec)</a></code></td><td>Read all date information.</td></tr>
</tbody>
</table>

### Action routines

<table class="table">
<thead>
<tr>
    <th>Name</th>
    <th>Description</th>
</th>
</thead>
<tbody>
<tr><td><code><a href='#idba_scopa'>idba_scopa(handle, repinfofile)</a></code></td><td>Reinitialize the database, removing all data and loading report information.</td></tr>
<tr><td><code><a href='#idba_quantesono'>idba_quantesono(handle, count)</a></code></td><td>Query the stations in the database.</td></tr>
<tr><td><code><a href='#idba_elencamele'>idba_elencamele(handle)</a></code></td><td>Retrieve the data about one station.</td></tr>
<tr><td><code><a href='#idba_voglioquesto'>idba_voglioquesto(handle, count)</a></code></td><td>Query the data in the database.</td></tr>
<tr><td><code><a href='#idba_dammelo'>idba_dammelo(handle, parameter)</a></code></td><td>Retrieve the data about one value.</td></tr>
<tr><td><code><a href='#idba_prendilo'>idba_prendilo(handle)</a></code></td><td>Insert a new value in the database.</td></tr>
<tr><td><code><a href='#idba_dimenticami'>idba_dimenticami(handle)</a></code></td><td>Remove from the database all values that match the query.</td></tr>
<tr><td><code><a href='#idba_remove_all'>idba_remove_all(handle)</a></code></td><td>Remove all values from the database.</td></tr>
<tr><td><code><a href='#idba_voglioancora'>idba_voglioancora(handle, count)</a></code></td><td>Query attributes about a variable.</td></tr>
<tr><td><code><a href='#idba_ancora'>idba_ancora(handle, parameter)</a></code></td><td>Retrieve one attribute from the result of idba_voglioancora().</td></tr>
<tr><td><code><a href='#idba_critica'>idba_critica(handle)</a></code></td><td>Insert new attributes for a variable.</td></tr>
<tr><td><code><a href='#idba_scusa'>idba_scusa(handle)</a></code></td><td>Remove attribute information for a variable.</td></tr>
</tbody>
</table>

### Message routines

<table class="table">
<thead>
<tr>
    <th>Name</th>
    <th>Description</th>
</th>
</thead>
<tbody>
<tr><td><code><a href='#idba_messages_open_input'>idba_messages_open_input(handle, filename, mode, format, simplified)</a></code></td><td>Open a BUFR, CREX, or AOF file for reading.</td></tr>
<tr><td><code><a href='#idba_messages_open_output'>idba_messages_open_output(handle, filename, mode, format)</a></code></td><td>Open a BUFR, CREX, or AOF file for writing.</td></tr>
<tr><td><code><a href='#idba_messages_read_next'>idba_messages_read_next(handle, found)</a></code></td><td>Read the next message and import it in the database.</td></tr>
<tr><td><code><a href='#idba_messages_write_next'>idba_messages_write_next(handle, template_name)</a></code></td><td>Export the data from the database that match the current query and add them to the current message.</td></tr>
</tbody>
</table>

### Pretty-printing routines

<table class="table">
<thead>
<tr>
    <th>Name</th>
    <th>Description</th>
</th>
</thead>
<tbody>
<tr><td><code><a href='#idba_spiegal'>idba_spiegal(handle, ltype1, l1, ltype2, l2, result)</a></code></td><td>Format the description of a level given its value.</td></tr>
<tr><td><code><a href='#idba_spiegat'>idba_spiegat(handle, ptype, p1, p2, result)</a></code></td><td>Format the description of a time range given its value.</td></tr>
<tr><td><code><a href='#idba_spiegab'>idba_spiegab(handle, varcode, value, result)</a></code></td><td>Format the description of a variable given its varcode and its value.</td></tr>
</tbody>
</table>

## Reference of routines

### Error management routines

<a name='idba_error_code'></a>
#### idba_error_code()

Return value:

The error code.
Return the error code for the last function that was called.

This is a list of known codes:

* 0: No error
* 1: Item not found
* 2: Wrong variable type
* 3: Cannot allocate memory
* 4: ODBC error
* 5: Handle management error
* 6: Buffer is too short to fit data
* 7: Error reported by the system
* 8: Consistency check failed
* 9: Parse error
* 10: Write error
* 11: Regular expression error
* 12: Feature not implemented
* 13: Value outside acceptable domain

<a name='idba_error_message'></a>
#### idba_error_message(message)

Parameters:

* `message`: The string holding the error message. If the string is
  not long enough, it will be truncated.

Return the error message for the last function that was called.

The error message is just a description of the error code. To see more
details of the specific condition that caused the error, use
[idba_error_context()](#idba_error_context) and
[idba_error_details()](#idba_error_details)
<a name='idba_error_context'></a>
#### idba_error_context(message)

Parameters:

* `message`: The string holding the error context. If the string is
  not long enough, it will be truncated.

Return a string describing the error context description for the last
function that was called.

This string describes what the code that failed was trying to do.
<a name='idba_error_details'></a>
#### idba_error_details(message)

Parameters:

* `message`: The string holding the error details. If the string is
  not long enough, it will be truncated.

Return a string with additional details about the error for the last
function that was called.

This string contains additional details about the error in case the
code was able to get extra informations about it, for example by
querying the error functions of an underlying module.
<a name='idba_error_set_callback'></a>
#### idba_error_set_callback(code, func, data, handle)

Parameters:

* `code`: The error code of the error that triggers this callback. If
  DBA_ERR_NONE is used, then the callback is invoked on all errors.
* `func`: The function to be called.
* `data`: An arbitrary integer data that is passed verbatim to the
  callback function when invoked.
* `handle`: A handle that can be used to remove the callback

Return value:

The error indicator for the function
Set a callback to be invoked when an error of a specific kind happens.

<a name='idba_error_remove_callback'></a>
#### idba_error_remove_callback(handle)

Parameters:

* `handle`: The handle previously returned by
  [idba_error_set_callback()](#idba_error_set_callback)

Return value:

The error indicator for the function
Remove a previously set callback.

<a name='idba_default_error_handler'></a>
#### idba_default_error_handler(debug)

Predefined error callback that prints a message and exits.

The message is printed only if a non-zero value is supplied as user
data
<a name='idba_error_handle_tolerating_overflows'></a>
#### idba_error_handle_tolerating_overflows(debug)

Predefined error callback that prints a message and exists, except in
case of overflow errors.

In case of overflows it prints a warning and continues execution
### Session routines

<a name='idba_presentati'></a>
#### idba_presentati(dbahandle, dsn, user, password)

Parameters:

* `dsn`: The ODBC DSN of the database to use
* `user`: The username used to connect to the database
* `password`: The username used to connect to the database
* `dbahandle`: The database handle that can be passed to
  [idba_preparati()](#idba_preparati) to work with the database.

Return value:

The error indication for the function.
Connect to the database.

This function can be called more than once once to connect to
different databases at the same time.
<a name='idba_arrivederci'></a>
#### idba_arrivederci(dbahandle)

Parameters:

* `dbahandle`: The database handle to close.

Disconnect from the database.

<a name='idba_preparati'></a>
#### idba_preparati(dbahandle, handle, anaflag, dataflag, attrflag)

Parameters:

* `dbahandle`: The main DB-ALLe connection handle
* `handle`: The session handle created by the function
* `anaflag`: station values access level
* `dataflag`: data values access level
* `attrflag`: attribute access level

Return value:

The error indication for the function.
Open a new session.

You can call [idba_preparati()](#idba_preparati) many times and get
more handles. This allows to perform many operations on the database
at the same time.
[idba_preparati()](#idba_preparati) has three extra parameters that
can be used to limit write operations on the database, as a limited
protection against programming errors:
`anaflag` controls access to station value records and can have these
values:

* `"read"` pseudoana records cannot be inserted.
* `"write"` it is possible to insert and delete pseudoana records.

`dataflag` controls access to observed data and can have these values:

* `"read"` data cannot be modified in any way.
* `"add"` data can be added to the database, but existing data cannot
  be modified. Deletions are disabled. This is used to insert new data
  in the database while preserving the data that was already present
  in it.
* `"write"` data can freely be added, overwritten and deleted.

`attrflag` controls access to data attributes and can have these
values:

* `"read"` attributes cannot be modified in any way.
* `"write"` attributes can freely be added, overwritten and deleted.

Note that some combinations of parameters are illegal, such as
anaflag=read and dataflag=add (when adding a new data, it's sometimes
necessary to insert new pseudoana records), or dataflag=rewrite and
attrflag=read (when deleting data, their attributes are deleted as
well).
<a name='idba_messaggi'></a>
#### idba_messaggi(handle, filename, mode, type)

Parameters:

* `handle`: The session handle returned by the function
* `filename`: Name of the file to open
* `mode`: File open mode. It can be `"r"` for read, `"w"` for write
  (the old file is deleted), `"a"` for append
* `type`: Format of the data in the file. It can be: `"BUFR"`,
  `"CREX"`, `"AOF"` (read only), `"AUTO"` (autodetect, read only)
* `force_report`: if 0, nothing happens; otherwise, choose the output
  message template using this report type instead of the one in the
  message

Return value:

The error indication for the function.
Start working with a message file.

<a name='idba_fatto'></a>
#### idba_fatto(handle)

Parameters:

* `handle`: Handle to the session to be closed.

Close a session.

### Input/output routines

<a name='idba_seti'></a>
#### idba_seti(handle, parameter, value)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to set. It can be the code of a WMO variable
  prefixed by `"B"` (such as `"B01023"`); the code of a QC value
  prefixed by `"*B"` (such as `"*B01023"`) or a keyword among the ones
  defined in [fapi_parms.md](fapi_parms.md)
* `value`: The value to assign to the parameter

Return value:

The error indicator for the function
Set an integer value in input.

<a name='idba_setb'></a>
#### idba_setb(handle, parameter)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to set. It can be the code of a WMO variable
  prefixed by `"B"` (such as `"B01023"`); the code of a QC value
  prefixed by `"*B"` (such as `"*B01023"`) or a keyword among the ones
  defined in [fapi_parms.md](fapi_parms.md)
* `value`: The value to assign to the parameter

Return value:

The error indicator for the function
Set a byte value in input.

<a name='idba_setr'></a>
#### idba_setr(handle, parameter, value)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to set. It can be the code of a WMO variable
  prefixed by `"B"` (such as `"B01023"`); the code of a QC value
  prefixed by `"*B"` (such as `"*B01023"`) or a keyword among the ones
  defined in [fapi_parms.md](fapi_parms.md)
* `value`: The value to assign to the parameter

Return value:

The error indicator for the function
Set a real value in input.

<a name='idba_setd'></a>
#### idba_setd(handle, parameter, value)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to set. It can be the code of a WMO variable
  prefixed by `"B"` (such as `"B01023"`); the code of a QC value
  prefixed by `"*B"` (such as `"*B01023"`) or a keyword among the ones
  defined in [fapi_parms.md](fapi_parms.md)
* `value`: The value to assign to the parameter

Return value:

The error indicator for the function
Set a real*8 value in input.

<a name='idba_setc'></a>
#### idba_setc(handle, parameter, value)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to set. It can be the code of a WMO variable
  prefixed by `"B"` (such as `"B01023"`); the code of a QC value
  prefixed by `"*B"` (such as `"*B01023"`) or a keyword among the ones
  defined in [fapi_parms.md](fapi_parms.md)
* `value`: The value to assign to the parameter

Return value:

The error indicator for the function
Set a character value in input.

<a name='idba_enqi'></a>
#### idba_enqi(handle, parameter, value)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to query. It can be the code of a WMO
  variable prefixed by `"B"` (such as `"B01023"`); the code of a QC
  value prefixed by `"*B"` (such as `"*B01023"`) or a keyword among
  the ones defined in [fapi_parms.md](fapi_parms.md)
* `value`: Where the value will be returned

Return value:

The error indicator for the function
Read an integer value from the output.

<a name='idba_enqb'></a>
#### idba_enqb(handle, parameter)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to query. It can be the code of a WMO
  variable prefixed by `"B"` (such as `"B01023"`); the code of a QC
  value prefixed by `"*B"` (such as `"*B01023"`) or a keyword among
  the ones defined in [fapi_parms.md](fapi_parms.md)
* `value`: Where the value will be returned

Return value:

The error indicator for the function
Read a byte value from the output.

<a name='idba_enqr'></a>
#### idba_enqr(handle, parameter, value)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to query. It can be the code of a WMO
  variable prefixed by `"B"` (such as `"B01023"`); the code of a QC
  value prefixed by `"*B"` (such as `"*B01023"`) or a keyword among
  the ones defined in [fapi_parms.md](fapi_parms.md)
* `value`: Where the value will be returned

Return value:

The error indicator for the function
Read a real value from the output.

<a name='idba_enqd'></a>
#### idba_enqd(handle, parameter, value)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to query. It can be the code of a WMO
  variable prefixed by `"B"` (such as `"B01023"`); the code of a QC
  value prefixed by `"*B"` (such as `"*B01023"`) or a keyword among
  the ones defined in [fapi_parms.md](fapi_parms.md)
* `value`: Where the value will be returned

Return value:

The error indicator for the function
Read a real*8 value from the output.

<a name='idba_enqc'></a>
#### idba_enqc(handle, parameter, value)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to query. It can be the code of a WMO
  variable prefixed by `"B"` (such as `"B01023"`); the code of an
  attribute prefixed by `"*B"` (such as `"*B01023"`) or a keyword
  among the ones defined in [fapi_parms.md](fapi_parms.md)
* `value`: Where the value will be returned

Return value:

The error indicator for the function
Read a character value from the output.

<a name='idba_unset'></a>
#### idba_unset(handle, parameter)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Parameter to remove. It can be the code of a WMO
  variable prefixed by `"B"` (such as `"B01023"`); the code of a QC
  value prefixed by `"*B"` (such as `"*B01023"`) or a keyword among
  the ones defined in [fapi_parms.md](fapi_parms.md)

Return value:

The error indicator for the function
Remove one value from the input.

<a name='idba_unsetb'></a>
#### idba_unsetb(handle)

Parameters:

* `handle`: Handle to a DB-All.e session

Remove all Bxxyyy values from the input.

<a name='idba_unsetall'></a>
#### idba_unsetall(handle)

Parameters:

* `handle`: Handle to a DB-All.e session

Completely clear the input, removing all values.

<a name='idba_setcontextana'></a>
#### idba_setcontextana(handle)

Parameters:

* `handle`: Handle to a DB-All.e session

Return value:

The error indicator for the function
Signal that the input values that are set are related to station
values instead of normal variables.

### Input/output shortcuts

<a name='idba_setlevel'></a>
#### idba_setlevel(handle, ltype1, l1, ltype2, l2)

Parameters:

* `handle`: Handle to a DB-All.e session
* `ltype1`: Level type to set in the input record
* `l1`: L1 to set in the input record
* `ltype2`: Level type to set in the input record
* `l2`: L2 to set in the input record

Return value:

The error indicator for the function
Set all level information.

<a name='idba_settimerange'></a>
#### idba_settimerange(handle, ptype, p1, p2)

Parameters:

* `handle`: Handle to a DB-All.e session
* `ptype`: P indicator to set in the input record
* `p1`: P1 to set in the input record
* `p2`: P2 to set in the input record

Return value:

The error indicator for the function
Set all time range information.

<a name='idba_setdate'></a>
#### idba_setdate(handle, year, month, day, hour, min, sec)

Parameters:

* `handle`: Handle to a DB-All.e session
* `year`: Year to set in the input record
* `month`: Month to set in the input
* `day`: Day to set in the input
* `hour`: Hour to set in the input
* `min`: Minute to set in the input
* `sec`: Second to set in the input

Return value:

The error indicator for the function
Set all date information.

<a name='idba_setdatemin'></a>
#### idba_setdatemin(handle, year, month, day, hour, min, sec)

Parameters:

* `handle`: Handle to a DB-All.e session
* `year`: Minimum year to set in the query
* `month`: Minimum month to set in the query
* `day`: Minimum day to set in the query
* `hour`: Minimum hour to set in the query
* `min`: Minimum minute to set in the query
* `sec`: Minimum second to set in the query

Return value:

The error indicator for the function
Set the minimum date for a query.

<a name='idba_setdatemax'></a>
#### idba_setdatemax(handle, year, month, day, hour, min, sec)

Parameters:

* `year`: Maximum year to set in the query
* `month`: Maximum month to set in the query
* `day`: Maximum day to set in the query
* `hour`: Maximum hour to set in the query
* `min`: Maximum minute to set in the query
* `sec`: Maximum second to set in the query

Return value:

The error indicator for the function
Set the maximum date for a query.

Handle to a DB-All.e session
<a name='idba_enqlevel'></a>
#### idba_enqlevel(handle, ltype1, l1, ltype2, l2)

Parameters:

* `handle`: Handle to a DB-All.e session
* `ltype`: Level type from the output record
* `l1`: L1 from the output record
* `l2`: L2 from the output record

Return value:

The error indicator for the function
Read all level information.

<a name='idba_enqtimerange'></a>
#### idba_enqtimerange(handle, ptype, p1, p2)

Parameters:

* `handle`: Handle to a DB-All.e session
* `ptype`: P indicator from the output record
* `p1`: P1 from the output record
* `p2`: P2 from the output record

Return value:

The error indicator for the function
Read all time range information.

<a name='idba_enqdate'></a>
#### idba_enqdate(handle, year, month, day, hour, min, sec)

Parameters:

* `handle`: Handle to a DB-All.e session
* `year`: Year from the output record
* `month`: Month the output record
* `day`: Day the output record
* `hour`: Hour the output record
* `min`: Minute the output record
* `sec`: Second the output record

Return value:

The error indicator for the function
Read all date information.

### Action routines

<a name='idba_scopa'></a>
#### idba_scopa(handle, repinfofile)

Parameters:

* `handle`: Handle to a DB-All.e session
* `repinfofile`: CSV file with the default report informations. See
  dba_reset() documentation for the format of the file.

Return value:

The error indicator for the function
Reinitialize the database, removing all data and loading report
information.

It requires the database to be opened in rewrite mode.
<a name='idba_quantesono'></a>
#### idba_quantesono(handle, count)

Parameters:

* `handle`: Handle to a DB-All.e session
* `count`: The count of elements

Return value:

The error indicator for the function
Query the stations in the database.

Results are retrieved using [idba_elencamele()](#idba_elencamele).
<a name='idba_elencamele'></a>
#### idba_elencamele(handle)

Parameters:

* `handle`: Handle to a DB-All.e session

Return value:

The error indicator for the function
Retrieve the data about one station.

After invocation, the output record is filled with information about
the station and its station values.
If there are no more stations to read, the function will fail with
DBA_ERR_NOTFOUND.
<a name='idba_voglioquesto'></a>
#### idba_voglioquesto(handle, count)

Parameters:

* `handle`: Handle to a DB-All.e session
* `count`: Number of values returned by the function

Return value:

The error indicator for the function
Query the data in the database.

Results are retrieved using [idba_dammelo()](#idba_dammelo).
<a name='idba_dammelo'></a>
#### idba_dammelo(handle, parameter)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Contains the ID of the parameter retrieved by this
  fetch

Return value:

The error indicator for the function
Retrieve the data about one value.

After invocation, the output record is filled with information about
the value.
If there are no more values to read, the function will fail with
DBA_ERR_NOTFOUND.
<a name='idba_prendilo'></a>
#### idba_prendilo(handle)

Parameters:

* `handle`: Handle to a DB-All.e session

Return value:

The error indicator for the function
Insert a new value in the database.

This function will fail if the database is open in data readonly mode,
and it will refuse to overwrite existing values if the database is
open in data add mode.
If the database is open in station reuse mode, the station values
provided on input will be used to create a station record if it is
missing, but will be ignored if it is already present. If it is open
in station rewrite mode instead, the station values on input will be
used to replace all the existing station values.
<a name='idba_dimenticami'></a>
#### idba_dimenticami(handle)

Parameters:

* `handle`: Handle to a DB-All.e session

Return value:

The error indicator for the function
Remove from the database all values that match the query.

This function will fail unless the database is open in data rewrite
mode.
<a name='idba_remove_all'></a>
#### idba_remove_all(handle)

Parameters:

* `handle`: Handle to a DB-All.e session

Return value:

The error indicator for the function
Remove all values from the database.

The difference with [idba_scopa()](#idba_scopa) is that it preserves
the existing report information.
<a name='idba_voglioancora'></a>
#### idba_voglioancora(handle, count)

Parameters:

* `handle`: Handle to a DB-All.e session
* `count`: Number of values returned by the function

Return value:

The error indicator for the function
Query attributes about a variable.

The variable queried is either:

* the last variable returned by `None`
* the last variable inserted by `None`
* the variable selected by settings `*context_id` and `*var_related`.

Results are retrieved using [idba_ancora()](#idba_ancora).
<a name='idba_ancora'></a>
#### idba_ancora(handle, parameter)

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Contains the ID of the parameter retrieved by this
  fetch

Return value:

The error indicator for the function
Retrieve one attribute from the result of idba_voglioancora().

<a name='idba_critica'></a>
#### idba_critica(handle)

Parameters:

* `handle`: Handle to a DB-All.e session

Return value:

The error indicator for the function
Insert new attributes for a variable.

The variable is either:

* the last variable returned by `None`
* the last variable inserted by `None`
* the variable selected by settings `*context_id` and `*var_related`.

The attributes that will be inserted are all those set by the
functions [idba_seti()](#idba_seti), [idba_setc()](#idba_setc),
[idba_setr()](#idba_setr), [idba_setd()](#idba_setd), using an
asterisk in front of the variable name.
Contrarily to [idba_prendilo()](#idba_prendilo), this function resets
all the attribute information (and only attribute information)
previously set in input, so the values to be inserted need to be
explicitly set every time.
This function will fail if the database is open in attribute readonly
mode, and it will refuse to overwrite existing values if the database
is open in attribute add mode.
<a name='idba_scusa'></a>
#### idba_scusa(handle)

Parameters:

* `handle`: Handle to a DB-All.e session

Return value:

The error indicator for the function
Remove attribute information for a variable.

The variable is either:

* the last variable returned by `None`
* the last variable inserted by `None`
* the variable selected by settings `*context_id` and `*var_related`.

The attribute informations to be removed are selected with:


```fortran
idba_setc(handle, "*varlist", "*B33021,*B33003");
```


### Message routines

<a name='idba_messages_open_input'></a>
#### idba_messages_open_input(handle, filename, mode, format, simplified)

Parameters:

* `handle`: Handle to a DB-All.e session
* `filename`: The file name
* `mode`: The opening mode. See the mode parameter of libc's fopen()
  call for details.
* `format`: The file format ("BUFR", "CREX", or "AOF")
* `simplified`: true if the file is imported in simplified mode, false
  if it is imported in precise mode. This controls approximating
  levels and time ranges to standard values.

Return value:

The error indication for the function.
Open a BUFR, CREX, or AOF file for reading.

Each session can only have one open input file: if one was previously
open, it is closed before opening the new one.
<a name='idba_messages_open_output'></a>
#### idba_messages_open_output(handle, filename, mode, format)

Parameters:

* `handle`: Handle to a DB-All.e session
* `filename`: The file name
* `mode`: The opening mode. See the mode parameter of libc's fopen()
  call for details.
* `format`: The file format ("BUFR", "CREX", or "AOF")

Return value:

The error indication for the function.
Open a BUFR, CREX, or AOF file for writing.

Each session can only have one open input file: if one was previously
open, it is closed before opening the new one.
<a name='idba_messages_read_next'></a>
#### idba_messages_read_next(handle, found)

Parameters:

* `handle`: Handle to a DB-All.e session
* `found`: True if a message has been imported, false if we are at the
  end of the input file.

Return value:

The error indication for the function.
Read the next message and import it in the database.

The access mode of the session controls how data is imported:

* station and data mode cannot be "read".
* if data mode is "add", existing data will not be overwritten.
* if attribute mode is "read", attributes will not be imported.

<a name='idba_messages_write_next'></a>
#### idba_messages_write_next(handle, template_name)

Parameters:

* `handle`: Handle to a DB-All.e session
* `template_name`: The template name used to decide the layout of
  variables in the messages that are exported.

Return value:

The error indication for the function.
Export the data from the database that match the current query and add
them to the current message.

### Pretty-printing routines

<a name='idba_spiegal'></a>
#### idba_spiegal(handle, ltype1, l1, ltype2, l2, result)

Parameters:

* `handle`: Handle to a DB-All.e session
* `ltype1`: Level type to set in the input record
* `l1`: L1 to set in the input record
* `ltype2`: Level type to set in the input record
* `l2`: L2 to set in the input record
* `result`: The string with the description of the level.

Return value:

The error indication for the function.
Format the description of a level given its value.

<a name='idba_spiegat'></a>
#### idba_spiegat(handle, ptype, p1, p2, result)

Parameters:

* `handle`: Handle to a DB-All.e session
* `ptype`: P indicator to set in the input record
* `p1`: P1 to set in the input record
* `p2`: P2 to set in the input record
* `result`: The string with the description of the time range.

Return value:

The error indication for the function.
Format the description of a time range given its value.

<a name='idba_spiegab'></a>
#### idba_spiegab(handle, varcode, value, result)

Parameters:

* `handle`: Handle to a DB-All.e session
* `varcode`: B table code of the variable (`"Bxxyyy"`)
* `value`: Value of the variable, as read with
  [idba_enqc()](#idba_enqc)
* `result`: The string with the description of the time range.

Return value:

The error indication for the function.
Format the description of a variable given its varcode and its value.

