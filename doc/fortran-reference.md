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

#### idba_error_code()
<a name='idba_error_code'></a>

Parameters:


Return the error code for the last function that was called.

See dba_error_code()

The error code. Please see the documentation of ::dba_err_code for the
possible values.


#### idba_error_message(message)
<a name='idba_error_message'></a>

Parameters:

* `message`: The string holding the error messag. If the string is not
  long enough, it will be truncated.

Return the error message for the last function that was called.

The error message is just a description of the error code. To see more
details of the specific condition that caused the error, use
fdba_error_context() and fdba_error_details()

See dba_error_message()


#### idba_error_context(message)
<a name='idba_error_context'></a>

Parameters:

* `message`: The string holding the error context. If the string is
  not long enough, it will be truncated.

Return a string describing the error context description for the last
function that was called.

This string describes what the code that failed was trying to do.

See dba_error_context()


#### idba_error_details(message)
<a name='idba_error_details'></a>

Parameters:

* `message`: The string holding the error details. If the string is
  not long enough, it will be truncated.

Return a string with additional details about the error for the last
function that was called.

This string contains additional details about the error in case the
code was able to get extra informations about it, for example by
querying the error functions of an underlying module.

See dba_error_details()


#### idba_error_set_callback(code, func, data, handle)
<a name='idba_error_set_callback'></a>

Parameters:

* `code`: The error code (See ::dba_err_code) of the error that
  triggers this callback. If DBA_ERR_NONE is used, then the callback
  is invoked on all errors.
* `handle`: A handle that can be used to remove the callback

Set a callback to be invoked when an error of a specific kind happens.


#### idba_error_remove_callback(handle)
<a name='idba_error_remove_callback'></a>

Parameters:

* `handle`: The handle previously returned by idba_error_set_callback

Remove a previously set callback.


#### idba_default_error_handler(debug)
<a name='idba_default_error_handler'></a>

Parameters:


Predefined error callback that prints a message and exits.

The message is printed only if a non-zero value is supplied as user
data


#### idba_error_handle_tolerating_overflows(debug)
<a name='idba_error_handle_tolerating_overflows'></a>

Parameters:


Predefined error callback that prints a message and exists, except in
case of overflow errors.

In case of overflows it prints a warning and continues execution


### Session routines

#### idba_presentati(dbahandle, dsn, user, password)
<a name='idba_presentati'></a>

Parameters:

* `dsn`: The ODBC DSN of the database to use
* `dbahandle`: The database handle that can be passed to
  idba_preparati to work with the database.

Connect to the database.

This function can be called more than once once to connect to
different databases at the same time.


#### idba_arrivederci(dbahandle)
<a name='idba_arrivederci'></a>

Parameters:

* `dbahandle`: The database handle to close.

Disconnect from the database.


#### idba_preparati(dbahandle, handle, anaflag, dataflag, attrflag)
<a name='idba_preparati'></a>

Parameters:

* `dbahandle`: The main DB-ALLe connection handle
* `handle`: The session handle returned by the function
* `anaflag`: Controls access to pseudoana records and can have these
  values: "read" pseudoana records cannot be inserted. "write" it is
  possible to insert and delete pseudoana records.

Open a new session.

You can call idba_preparati() many times and get more handles. This
allows to perform many operations on the database at the same time.

idba_preparati() has three extra parameters that can be used to limit
write operations on the database, as a limited protection against
programming errors.

Note that some combinations of parameters are illegal, such as
anaflag=read and dataflag=add (when adding a new data, it's sometimes
necessary to insert new pseudoana records), or dataflag=rewrite and
qcflag=read (when deleting data, their attributes are deleted as
well).


#### idba_messaggi(handle, filename, mode, type)
<a name='idba_messaggi'></a>

Parameters:

* `handle`: The session handle returned by the function
* `filename`: Name of the file to open

Start working with a message file.


#### idba_fatto(handle)
<a name='idba_fatto'></a>

Parameters:

* `handle`: Handle to the session to be closed.

Close a session.


### Input/output routines

#### idba_seti(handle, parameter, value)
<a name='idba_seti'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Set an integer value in input.


#### idba_setb(handle, parameter)
<a name='idba_setb'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Set a byte value in input.


#### idba_setr(handle, parameter, value)
<a name='idba_setr'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Set a real value in input.


#### idba_setd(handle, parameter, value)
<a name='idba_setd'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Set a real*8 value in input.


#### idba_setc(handle, parameter, value)
<a name='idba_setc'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Set a character value in input.


#### idba_enqi(handle, parameter, value)
<a name='idba_enqi'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Read an integer value from the output.


#### idba_enqb(handle, parameter)
<a name='idba_enqb'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Read a byte value from the output.


#### idba_enqr(handle, parameter, value)
<a name='idba_enqr'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Read a real value from the output.


#### idba_enqd(handle, parameter, value)
<a name='idba_enqd'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Read a real*8 value from the output.


#### idba_enqc(handle, parameter, value)
<a name='idba_enqc'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Read a character value from the output.


#### idba_unset(handle, parameter)
<a name='idba_unset'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Remove one value from the input.


#### idba_unsetb(handle)
<a name='idba_unsetb'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Remove all Bxxyyy values from the input.


#### idba_unsetall(handle)
<a name='idba_unsetall'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Completely clear the input, removing all values.


#### idba_setcontextana(handle)
<a name='idba_setcontextana'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Signal that the input values that are set are related to station
values instead of normal variables.


### Input/output shortcuts

#### idba_setlevel(handle, ltype1, l1, ltype2, l2)
<a name='idba_setlevel'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Set all level information.


#### idba_settimerange(handle, ptype, p1, p2)
<a name='idba_settimerange'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Set all time range information.


#### idba_setdate(handle, year, month, day, hour, min, sec)
<a name='idba_setdate'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Set all date information.


#### idba_setdatemin(handle, year, month, day, hour, min, sec)
<a name='idba_setdatemin'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Set the minimum date for a query.


#### idba_setdatemax(handle, year, month, day, hour, min, sec)
<a name='idba_setdatemax'></a>

Parameters:

* `year`: Maximum year to set in the query

Set the maximum date for a query.


#### idba_enqlevel(handle, ltype1, l1, ltype2, l2)
<a name='idba_enqlevel'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `ltype`: Level type from the output record

Read all level information.


#### idba_enqtimerange(handle, ptype, p1, p2)
<a name='idba_enqtimerange'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `ptype`: P indicator from the output record

Read all time range information.


#### idba_enqdate(handle, year, month, day, hour, min, sec)
<a name='idba_enqdate'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `year`: Year from the output record

Read all date information.


### Action routines

#### idba_scopa(handle, repinfofile)
<a name='idba_scopa'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Reinitialize the database, removing all data and loading report
information.

It requires the database to be opened in rewrite mode.


#### idba_quantesono(handle, count)
<a name='idba_quantesono'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Query the stations in the database.

Results are retrieved using idba_elencamele().


#### idba_elencamele(handle)
<a name='idba_elencamele'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Retrieve the data about one station.

After invocation, the output record is filled with information about
the station and its station values.

If there are no more stations to read, the function will fail with
DBA_ERR_NOTFOUND.


#### idba_voglioquesto(handle, count)
<a name='idba_voglioquesto'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `count`: Number of values returned by the function

Query the data in the database.

Results are retrieved using idba_dammelo().


#### idba_dammelo(handle, parameter)
<a name='idba_dammelo'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Contains the ID of the parameter retrieved by this
  fetch

Retrieve the data about one value.

After invocation, the output record is filled with information about
the value.

If there are no more values to read, the function will fail with
DBA_ERR_NOTFOUND.


#### idba_prendilo(handle)
<a name='idba_prendilo'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Insert a new value in the database.

This function will fail if the database is open in data readonly mode,
and it will refuse to overwrite existing values if the database is
open in data add mode.

If the database is open in station reuse mode, the station values
provided on input will be used to create a station record if it is
missing, but will be ignored if it is already present. If it is open
in station rewrite mode instead, the station values on input will be
used to replace all the existing station values.


#### idba_dimenticami(handle)
<a name='idba_dimenticami'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Remove from the database all values that match the query.

This function will fail unless the database is open in data rewrite
mode.


#### idba_remove_all(handle)
<a name='idba_remove_all'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Remove all values from the database.

The difference with idba_scopa() is that it preserves the existing
report information.


#### idba_voglioancora(handle, count)
<a name='idba_voglioancora'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `count`: Number of values returned by the function

Query attributes about a variable.

The variable queried is either:

the last variable returned by idba_dammelo()the last variable inserted
by idba_prendilo()the variable selected by settings *context_id and
*var_related. Results are retrieved using idba_ancora().


#### idba_ancora(handle, parameter)
<a name='idba_ancora'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `parameter`: Contains the ID of the parameter retrieved by this
  fetch

Retrieve one attribute from the result of idba_voglioancora().


#### idba_critica(handle)
<a name='idba_critica'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Insert new attributes for a variable.

The variable is either:

the last variable returned by idba_dammelo()the last variable inserted
by idba_prendilo()the variable selected by settings *context_id and
*var_related. The attributes that will be inserted are all those set
by the functions idba_seti(), idba_setc(), idba_setr(), idba_setd(),
using an asterisk in front of the variable name.

Contrarily to idba_prendilo(), this function resets all the attribute
information (and only attribute information) previously set in input,
so the values to be inserted need to be explicitly set every time.

This function will fail if the database is open in attribute readonly
mode, and it will refuse to overwrite existing values if the database
is open in attribute add mode.


#### idba_scusa(handle)
<a name='idba_scusa'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Remove attribute information for a variable.

The variable is either:

the last variable returned by idba_dammelo()the last variable inserted
by idba_prendilo()the variable selected by settings *context_id and
*var_related. The attribute informations to be removed are selected
with: idba_setc(handle,"*varlist","*B33021,*B33003");


### Message routines

#### idba_messages_open_input(handle, filename, mode, format, simplified)
<a name='idba_messages_open_input'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Open a BUFR, CREX, or AOF file for reading.

Each session can only have one open input file: if one was previously
open, it is closed before opening the new one.


#### idba_messages_open_output(handle, filename, mode, format)
<a name='idba_messages_open_output'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Open a BUFR, CREX, or AOF file for writing.

Each session can only have one open input file: if one was previously
open, it is closed before opening the new one.


#### idba_messages_read_next(handle, found)
<a name='idba_messages_read_next'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `found`: True if a message has been imported, false if we are at the
  end of the input file.

Read the next message and import it in the database.

The access mode of the session controls how data is imported:


#### idba_messages_write_next(handle, template_name)
<a name='idba_messages_write_next'></a>

Parameters:

* `handle`: Handle to a DB-All.e session

Export the data from the database that match the current query and add
them to the current message.


### Pretty-printing routines

#### idba_spiegal(handle, ltype1, l1, ltype2, l2, result)
<a name='idba_spiegal'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `result`: The string with the description of the level.

Format the description of a level given its value.


#### idba_spiegat(handle, ptype, p1, p2, result)
<a name='idba_spiegat'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `result`: The string with the description of the time range.

Format the description of a time range given its value.


#### idba_spiegab(handle, varcode, value, result)
<a name='idba_spiegab'></a>

Parameters:

* `handle`: Handle to a DB-All.e session
* `result`: The string with the description of the time range.

Format the description of a variable given its varcode and its value.

