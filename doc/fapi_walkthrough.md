# A walk through the Fortran API

<a name="include"></a>
## Including the DB-All.e interface file

If you work using Fortran 90, DB-All.e comes with an interface file that you
can use to enable type checking on all the DB-All.e API.

To make use of the interface file, just include it at the beginning of your
program:

```fortran
      include "dballe/dballeff.h"
```

The Fortran 90 interface also allows to replace all the functions [idba_enqi][],
[idba_enqr][], [idba_enqd][] and [idba_enqc][] with [idba_enq][] and to replace all
the functions [idba_seti][], [idba_setr][], [idba_setd][] and [idba_setc][] with
[idba_set][].


<a name="errors"></a>
## Error management

* All errors are reported as function return values
* All results are reported as output parameters
* All functions `idba_set*` set the input of action routines
* All functions `idba_enq*` get the output of action routines

Errors can be handled by checking the return value of every function:

```fortran
      ! Example error handling
      ierr = idba_presentati(dbhandle, "dsn", "user", "password")
      if (ierr.ne.0) then
           ! handle the error...
      end if
```

Or they can be handled by installing a callback function that is automatically
called in case of error:

```fortran
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
```

The previous code will setup DB-ALLe to call `error_handler` after any error,
passing it the integer value 42.  The callback can be removed at any time by
calling [idba_error_remove_callback][]:

```fortran
      ! How to remove a callback
      ierr = idba_error_remove_callback(cb_handle)
```

This is a useful error handling function:

```fortran
      ! The error handler needs to be declared 'external'
      external error_handler

      ! Compute the length of a string
      ! (this is an utility function that is used by the error handler
      !  to print nicer error messages)
      integer function istrlen(string)
      character string*(*)
        istrlen = len(string)
        do while ((string(istrlen:istrlen).eq." " .or.
     $       string(istrlen:istrlen).eq."").and.
     $       istrlen.gt.0)
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
```

This code introduces three new functions:

* [idba_error_message][]:
  returns a string describing what type of error has happened.
* [idba_error_context][]:
  returns a string describing what DB-All.e was trying to do when the error
  happened.
* [idba_error_details][]:
  returns a detailed description of the error, when available.  If no detailed
  description is available, it returns an empty string.

A similar error handling behaviour can be obtained by using the predefined
convenience function [idba_default_error_handler][]:

<a name="fun_error_handler"></a>

```fortran
      ! Declare the external function (not necessary if you include dballeff.h)
      external idba_default_error_handler

      ! Use it as the error handling callback
      ierr = idba_error_set_callback(0, idba_default_error_handler, 1, cb_handle)
```

An alternative error handler called [idba_error_handler_tolerating_overflows][]
is available: it exists on all errors instead of value overflows, in what case
it prints a warning to standard error and allows the program to continue.  The
overflow error can then be catched, if needed, by inspecting the error code
returned by the DB-All.e function that causes the error.

This is how to use it:

```fortran
      ! Declare the external function (not necessary if you include dballeff.h)
      external idba_error_handler_tolerating_overflows

      ! Use it as the error handling callback
      ierr = idba_error_set_callback(0, idba_error_handler_tolerating_overflows, 1, cb_handle)
```

<a name="init_db"></a>
## Starting the work

Before any action routine, you need to connect to the database.  Connecting to
the database will give you a *handle* that you can use to open sessions.

This code will open a connection with DB-All.e, then it will start a session:

```fortran
      ! Connect to the database and get a handle to work with it
      ierr = idba_presentati(dbhandle, "url", "ignored", "ignored")
      ierr = idba_preparati(dbhandle, handle, "read", "read", "read")

      ! ...do your work...

      ! End of the work
      ierr = idba_fatto(handle)
      ierr = idba_arrivederci(dbhandle)
```

You call [idba_presentati][] to connect to the databse. The parameters are
the [database connection URL](fapi_connect.md), and two parameters that were
used in the past and are now ignored.

You can call [idba_preparati][] many times and get more handles.  This allows
to perform many operations on the database at the same time.

[idba_preparati][] has three extra parameters that can be used to limit
write operations on the database, as a limited protection against programming
errors.

The first extra parameter controls access to station values and can have
these values:

* `"read"`: station values cannot be modified.
* `"write"`: station values can be added and removed.

The second extra parameter control access to observed data and can have
these values:

* `"read"`: data cannot be modified in any way.
* `"add"`: data can be added to the database, but existing data cannot be
  modified.  Deletions are disabled.  This is used to insert new data in the
  database while preserving the data that was already present in it.
* `"write"`: data can freely be added, overwritten and deleted.

The third extra parameter controls access to data attributes and can have
these values:

* `"read"`: attributes cannot be modified in any way.
* `"write"`: attributes can freely be added, overwritten and deleted.

Note that some combinations are illegal. For example, you cannot have "read" on
station values and "add" on data, because  when adding a new data, it is
sometimes necessary to insert new station records). You also cannot have
"rewrite" on data and "read` on attributes, because when deleting data, their
attributes are deleted as well.

<a name="init_message"></a>
## Starting the work on a message

Instead of connecting to a database, you can use the DB-All.e API to read and
write message reports in BUFR, CREX format, and read, but not write, messages
in AOF format.

To do that, use [idba_messaggi][] instead of both [idba_presentati][] and
[idba_preparati][].  To write a message, your code will look like:

```fortran
      ! Connect to the database and get a handle to work with it
      ierr = idba_messaggi(handle, "file.bufr", "r", "auto")

      ! ...do your work...

      ! End of the work
      ierr = idba_fatto(handle)
```

[idba_messaggi][] has three parameters:

1. the name of the file to open
2. the open mode ("r" for read, "w" for write or create, "a" for append).
   See the documentation of libc's `fopen` for more details.
3. the file format.  It can be "BUFR", "CREX", "AOF" or "AUTO".  "AUTO" tells
   DB-All.e to autodetect the file format, but it only works when reading
   files, not when writing new one.

You can call [idba_messaggi][] many times and read or write many files.  You
can even call [idba_messaggi][] many time on the same file as long as you
open it read only.

Once you open a file, you can use the other DB-All.e functions on it.  There
are slight differences between working on a database and working on a file, and
they are explained later in the section [Working with files](#db_file_differences).


## Setting input and reading output

Input to action routines is done using the functions `idba_set*`, and output
is read with the functions `idba_enq*` (see [the introduction](fapi_concepts.md#routines)):

```fortran
      ! Set the extremes of an area and retrieve all the stations in it
      ierr = idba_setd(handle, "latmin", 30.D0)
      ierr = idba_setd(handle, "latmax", 50.D0)
      ierr = idba_setd(handle, "lonmin", 10.D0)
      ierr = idba_setd(handle, "lonmax", 20.D0)
      ierr = idba_quantesono(handle, count)

      ! Get the informations about a station
      do while (count.gt.0)
        ierr = idba_elencamele(handle)
        ierr = idba_enqc(handle, "name", cname)
        ierr = idba_enqi(handle, "ana_id", id)
        ierr = idba_enqd(handle, "lat", lat)
        ierr = idba_enqd(handle, "lon", lon)
        ! ....
        count = count - 1
      enddo
```

Note that, when one uses [idba_setc][], [idba_seti][], [idba_enqc][],
[idba_enqi][] with parameters that have some decimal digits, DB-All.e will
work with values as if they did not have a decimal point.  That is, if latitude
`10.124323` is read with [idba_enqi][], then the result will be `10124323`.

The following example shows what happens:

```fortran
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
```

## Input/output shortcuts

There are a few functions that are shortcuts to other input and output
functions:

`idba_enqdate(handle, year, month, day, hour, minute, second)` is a shortcut
to:

```fortran
  idba_enqi(handle, "year", year)
  idba_enqi(handle, "month", month)
  idba_enqi(handle, "day", day)
  idba_enqi(handle, "hour", hour)
  idba_enqi(handle, "min", minute)
  idba_enqi(handle, "sec", second)
```

`idba_setdate(handle, year, month, day, hour, minute, second)` is a shortcut
to:

```fortran
  idba_seti(handle, "year", year)
  idba_seti(handle, "month", month)
  idba_seti(handle, "day", day)
  idba_seti(handle, "hour", hour)
  idba_seti(handle, "min", minute)
  idba_seti(handle, "sec", second)
```

`idba_enqlevel(handle, type1, l1, type2, l2)` is a shortcut to:

```fortran
  idba_enqi(handle, "leveltype1", type1)
  idba_enqi(handle, "l1", l1)
  idba_enqi(handle, "leveltype2", type2)
  idba_enqi(handle, "l2", l2)
```

`idba_setlevel(handle, type1, l1, type2, l2)` is a shortcut to:

```fortran
  idba_seti(handle, "leveltype1", type1)
  idba_seti(handle, "l1", l1)
  idba_seti(handle, "leveltype2", type2)
  idba_seti(handle, "l2", l2)
```

`idba_enqtimerange(handle, type, p1, p2)` is a shortcut to:

```fortran
  idba_enqi(handle, "pindicator", type)
  idba_enqi(handle, "p1", p1)
  idba_enqi(handle, "p2", p2)
```

`idba_settimerange(handle, type, p1, p2)` is a shortcut to:

```fortran
  idba_seti(handle, "pindicator", type)
  idba_seti(handle, "p1", p1)
  idba_seti(handle, "p2", p2)
```


## Parameter names

There are three different kinds of parameter names one can use:

* [DB-All.e parameters](fapi_parms.md), that have a special meaning to
  DB-All.e: for example they can be part of the coordinate system, or
  space/time extremes to use to query the database.  They are indicated simply
  with their name (for example, `"lat"` or `"yearmin"`).
* [WMO table B variables](fapi_btable.md), represent all possible sorts of
  observed data, and are indicated in the form `Bxxyyy`, where `xxyyy` are the
  X and Y values from the WMO table B.
* [Variable aliases](fapi_aliases.md) that are short, easy to remember names which
  can be used instead of frequently used WMO B variables.

## Queries and observed data

The `idba_set*` and `idba_enq*` functions can also be used to set and get
observation data.  To do so, use as parameter the string `"Bxxyyy"`, where
`xx` and `yyy` are the X and Y values of the BUFR/CREX table B describing
the observed data.

For example:

```fortran
      ! Set the speed of the wind (very useful in summer)
      ierr = idba_setr(handle, "B11002", 1.8)
      ! Also set the temperature
      ierr = idba_setr(handle, "B12001", 21.8)
      ierr = idba_prendilo(handle)
```

## Attributes

The [idba_set][] and [idba_enq][] groups of functions can also be used to
set and get attributes on data.  To do so, use as parameter the string
`"*Bxxyyy"`, where `xx` and `yyy` are the X and Y values of the
BUFR/CREX table B describing the attribute.

For example:

```fortran
      ! Set the confidence of the wind speed value we inserted
      ! in the last 'idba_prendilo'
      ierr = idba_setr(handle, "*B33007", 75.0)
      ierr = idba_setc(handle, "*var_related", "B11002")
      ierr = idba_critica(handle)
```

## Querying the database

Queries are made by giving one or more extremes of space, time, level or time
range.  See [the parameter table](fapi_parms.md) for a list of all available query
parameters, in the column "On query input".

## Querying the station values

Example code to query all the stations in a given area:

```fortran
      ierr = idba_setd(handle, "latmin", 30.D0)
      ierr = idba_setd(handle, "latmax", 50.D0)
      ierr = idba_setd(handle, "lonmin", 10.D0)
      ierr = idba_setd(handle, "lonmax", 20.D0)
      ierr = idba_quantesono(handle, count)
      do while (count.gt.0)
        ierr = idba_elencamele(handle)
        ierr = idba_enqi(handle, "ana_id", id)
        ! Pseudoana values can be read as well:
        ierr = idba_enqc(handle, "name", cname)
        ierr = idba_enqd(handle, "B07001", height)
        ! ...query more data and work with it...
        count = count - 1
      enddo
```

This code introduces two new functions:

* [idba_quantesono][]: performs the query and returns the number of stations it
  finds.
* [idba_elencamele][]: gets a station out of the results of [idba_quantesono][].
  If there are no more stations, the function fails.

After [idba_elencamele][], the output record will also contain all the pseudoana
values available for the station.  If `rep_cod` or `rep_memo` are specified as
query parameters, the pseudoana values of that network will be used.  Else,
[idba_elencamele][] will use all available pseudoana values, choosing the one in
the network with the highest priority in case the same pseudoana value is
available on more than one network.

## Querying the values

Example code to query all the values in a given area and time:

```fortran
      ierr = idba_seti(handle, "latmin", 30)
      ierr = idba_seti(handle, "latmax", 50)
      ierr = idba_seti(handle, "lonmin", 10)
      ierr = idba_seti(handle, "lonmax", 20)
      ierr = idba_seti(handle, "yearmin", 2004)
      ierr = idba_seti(handle, "yearmax", 2004)
      ierr = idba_voglioquesto(handle, count)
      do while (count.gt.0)
        ierr = idba_dammelo(handle, param)
        ! get the value of this variable
        ierr = idba_enqc(handle, param, cvalue)
        ierr = idba_enqd(handle, "lat", dlat)
        ierr = idba_enqd(handle, "lon", dlon)
        ! query more data and work with it
        count = count - 1
      enddo
```

This code introduces two new functions:

* [idba_voglioquesto][]: performs the query and returns the number of values it
  finds.
* [idba_dammelo][]: gets a value out of the result of [idba_voglioquesto][].  If
  there are no more stations, the function fails.

## Clearing the database

You can initialise or reinitialise the database using [idba_scopa][]:

```fortran
      ! Start the work with a clean database
      ierr = idba_scopa(handle, "repinfo.csv")
```

[idba_scopa][] clears the database if it exists, then recreates all the
needed tables.  Finally, it populates the informations about the reports (such
as the available report types, their mnemonics and their priority) using the
data in the file given as argument.

The file is in CSV format, with 6 columns:

1. Report code (corresponding to parameter `rep_cod`)
2. Mnemonic name (corresponding to parameter `rep_memo`)
3. Report description
4. Report priority (corresponding to parameter `priority`)
5. Ignored
6. Ignored

If `""` is given instead of the file name, [idba_scopa][] will read the
data from `/etc/repinfo.csv`.

This is an example of the contents of the file:

```csv
01,synop,report synottico,100,oss,0
02,metar,metar,80,oss,0
03,temp,radiosondaggio,100,oss,2
04,ana_lm,valori analizzati LM,-1,ana,255
05,ana,analisi,-10,pre,255
06,pre_cleps_box1.5maxel001,previsti cosmo leps box 1.5 gradi valore max elemento 1,-1,pre,255
07,pre_lmn_box1.5med,previzione Lokal Model nudging box 1.5 gradi valore medio,-1,pre,255
08,pre_lmp_spnp0,previsione Lkal Model prognostica interpolato punto piu' vicino,-1,pre,255
09,boe,dati omdametrici,100,oss,31
```

[idba_scopa][] will not work unless `rewrite` has been enabled for the
data when opening the database.


## Inserting data

Data is inserted using [idba_prendilo][]:

```fortran
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
      ierr = idba_prendilo(handle)
```

This code introduces a new function:

* [idba_prendilo][]:
  inserts a new value in the database.  All the information about the parameter
  to insert is taken from the input previously set by `idba_set*` functions.

  When data of the same kind and with the same characteristics already exists,
  the behaviour of [idba_prendilo][] is defined by the parameter passed to
  [idba_preparati][] when creating the handle.  See [Starting the
  work](#ch_work_start) for more informations.

[idba_prendilo][] will work in different ways according to the data opening
mode of the database:

* `read`: causes an error, because the data cannot be read.
* `add`: new data can be inserted, but causes an error when trying to insert a
  value that already exists.
* `rewrite`: new data can be inserted, and existing data is overwritten.

Also, behaviour changes according to the station values opening mode:

* `"reuse"`: when inserting data, if an existing pseudoana record for the data
  is found, it will be reused.
* `"rewrite"`: when inserting data, if an existing pseudoana record for the
  data is found, it will be completely overwritten with the parameters in
  input.

Note that the database cannot be opened in pseudoana `read` mode when data
is `add` or `rewrite`.

## Deleting data

Data is deleted using [idba_dimenticami][]:

```fortran
      ! Delete all data from the station with id 4 in year 2002
      ierr = idba_seti(handle, "ana_id", 4)
      ierr = idba_seti(handle, "year", 2002)
      ierr = idba_dimenticami(handle)
```

This code introduces a new function:

* [idba_dimenticami][]: deletes all the data found in the extremes specified in
  input.

[idba_dimenticami][] will not work unless `rewrite` has been enabled for
the data when opening the database.

## Reading attributes

Attributes are read using [idba_ancora][]:

```fortran
      ! ...setup a query...
      idba_voglioquesto(handle, count)
      do while (count.gt.0)
        ierr = idba_dammelo(handle, param)

        ! Read QC informations about the last value read
        ierr = idba_voglioancora(handle, qc_count)
        do while (qc_count.gt.0)
            ierr = idba_ancora(handle, param) 
            ierr = idba_enqc(handle, param, value)
            ! ...process the value...
            qc_count = qc_count - 1
        enddo

        count = count - 1
      enddo
```

This code introduces two new functions:

* [idba_voglioancora][]:
  Performs a query to retrieve attributes for the last variable read by
  [idba_dammelo][].  It returns the number of attributes available.
* [idba_ancora][]:
  Retrieves one by one the values queried by [idba_voglioancora][] if
  there are no more items available, the function will fail.

  The parameter `param` will be set to the name (in the form `*Bxxyyy`) of
  the attribute just read.

It is possible to read attributes at a later time giving a context ID and a B
table value:

```fortran
      ! Read the context ID after a prendilo or a dammelo
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
      ierr = idba_voglioancora(handle, qc_count)
      do while (qc_count.gt.0)
          ierr = idba_ancora(handle, param) 
          ierr = idba_enqc(handle, param, value)
          ! ...process the value...
          qc_count = qc_count - 1
      enddo
```

## Writing attributes

Attributes are written using [idba_critica][], which can be used after an
[idba_dammelo][], after an [idba_prendilo][] or at any time using a stored data
id.  These three case differ on how to communicate to [idba_critica][] what is
the data about which to write attributes.

When used after [idba_dammelo][], [idba_critica][] can refer directly to the
last data retrieved:

```fortran
      ! ...setup a query...
      ierr = idba_voglioquesto(handle, count)
      do while (count.gt.0)
        ierr = idba_dammelo(handle, param)
        ! ...process data...

        ! Set the attributes
        ierr = idba_seti(handle, "*B33007", 75)
        ierr = idba_seti(handle, "*B33006", 42)
        ierr = idba_critica(handle)

        count = count - 1
      enddo
```

After an [idba_prendilo][] instead, since [idba_prendilo][] can write more than
one data at a time, we need to tell [idba_critica][] which of them we are
referring to:

```fortran
      ! Insert wind speed and temperature
      ierr = idba_setr(handle, "B11002", 1.8)
      ierr = idba_setr(handle, "B12001", 22)
      ierr = idba_prendilo(handle)

      ! Set the attributes
      ierr = idba_seti(handle, "*B33007", 75)

      ! Use "*var_related" to indicate which of the two variables we are annotating
      ierr = idba_setc(handle, "*var_related", "B11002")

      ierr = idba_critica(handle)
```

[idba_critica][] can also be called at any time using a previously stored data it:

```fortran
      ! ...perform a query with idba_voglioquesto...
      do while (count.gt.0)
        ierr = idba_dammelo(handle, param)
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
      ierr = idba_critica(handle)
```

This code introduces a new function:

* [idba_critica][]
  Set one or more attributes about a variable.
  
  The variable can be identified directly by using `idba_seti(handle,
  "*context_id", id)` and `idba_seti(handle, "*var_related", name)`.
  These parameters are automatically set by the [idba_dammelo][] and
  [idba_prendilo][] action routines.

  The attributes and values are set as input to [idba_critica][] using the
  `idba_set*` functions with an asterisk in front of the variable name.

[idba_critica][] will work in different ways according to the attributes
opening mode of the database:

* `"read"`: attributes cannot be modified in any way.
* `"rewrite"`: attributes can be added, and existing attributes can be
  overwritten.


## Deleting attributes

Attributes are deleted using [[idba_scusa][]](fapi_reference.md#idba_scusa):

```fortran
      ! Delete the confidence interval from the wind speed

      ! The referring variable is identified in the same way as with
      ! idba_critica:
      ierr = idba_seti(handle, "*context_id", saved_id)
      ierr = idba_seti(handle, "*var_related", "B11002")

      ! The attributes to delete are selected by setting "*varlist":
      ierr = idba_setc(handle, "*varlist", "*B33007")
      ierr = idba_scusa(handle)
```

This code introduces a new function:

* [idba_scusa][]:
  Delete attributes from a variable identified in the same way as with

[idba_scusa][] will not work unless the database has been opened in
attribute `rewrite` mode.


## Ending the work

When you are finished working with a handle, you release it with
[idba_fatto][]:

```fortran
      ! We are finished with this handle
      ierr = idba_fatto(handle)
```

When you are finished working with DB-ALLe, you use [idba_arrivederci][] to
close all connections and release all resources:

```fortran
      ! We do not need to work with dballe anymore
      ierr = idba_arrivederci(dbh)
```


## Shortcuts to stations and data

DB-All.e offers two shortcuts to represent pseudoana entries and data in the
database: the `ana_id` and the `data_id` keys, that are set in the
output of every [idba_dammelo][].

`ana_id` represents a pseudoana entry.  Every time one needs to specify a
set of latitude, longitude, fixed/mobile, one could use the corresponding
`ana_id` value, if known, and get a faster search.

`data_id` represents a data entry.  Every time one needs to identify some
data setting latitude, longitude, level layer, time range and so on, one can
just provide the value of `data_id`, and also get a faster search.

## Helpers for pretty printing

There are a number of functions in DB-All.e, the `idba_spiega\*` group of
functions, that are not needed for normal processing but can be useful to
improve the presentation of data to users.

All these function take a number of parameters and a string, and they store a
proper description of the values into the string.

The functions are:

* `idba_spiegal(handle,ltype1,l1,ltype2,l2,string)`:
  Describes a level.  For example, `idba_spiegal(handle,106,10,106,20,string)`
  will store in `string` something like: *"Layer between 10hm and
  20hm above ground"*.
* `idba_spiegat(handle,ptype,p1,p2,string)`:
  Describes a time range.  For example, `idba_spiegat(handle,3,0,600,string)`
  will store in `string` something like: *"Average between reference
  time+0s to reference time+600s"*.
* `idba_spiegab(handle,varcode,value,string)`:
  Describe a value.  For example, `idba_spiegab(handle,"B12001","280",string)`
  will store in `string` something like: *"280 (K) TEMPERATURE/DRY-BULB
  TEMPERATURE"*.

## Modifiers for queries

DB-All.e allows to set special query behaviours using the `"query"`
parameter.  The available options are:

* `best`: When measures from different kinds of reports exist in the same
  physical space, do not return them all, but only return the one of the record
  type with the highest priority.

## Working with files

This is a list of the differences between working with files and working with
databases:

* You do not need to call [idba_presentati][] and [idba_arrivederci][]: the work
  session starts at [idba_messaggi][] and ends at [idba_fatto][]
* When reading, performing [idba_quantesono][] or [idba_voglioquesto][] a second
  time advances to the next message in the file.
* Query parameters set before an [idba_voglioquesto][] have no effect: filtering
  data is not implemented for files. Since it may be implemented in the future,
  it is suggested to avoid setting query parameters before an
  [idba_voglioquesto][] to avoid unexpected changes of behaviour with future
  versions of DB-All.e.
* When reading, you will see that there are no more messages because
  [idba_quantesono][] or [idba_voglioquesto][] will return 0.
* When writing, you can use the `query` input parameter to [idba_prendilo][] to
  control when a new message is started.  If you set it to `subset`, then the
  data will be inserted in a new BUFR or CREX subset.  If you set it to
  `message`, you will start a new message.

  After `"message"` you can specify a template for the message, using one of
  the names listed by `dbadb export -t list`, for example: `"message generic"`.
  If you do not specify a template name, an appropriate template will
  automatically be chosen for you.
* Setting `rep_memo` you can influence the output template of messages: if you
  set it to a synop report code, you will create a synop message.


<a name="examples"></a>
## Code examples

### Insert station data, then insert data

```fortran
ierr = idba_preparati(dbhandle, handle, "write", "add", "write")

! Insert data about a station
ierr = idba_setr (handle, "lat", 11.345)
ierr = idba_setr (handle, "lon", 44.678)
ierr = idba_setr (handle, "height", 23)
ierr = idba_prendilo (handle)

! Read the station ID for the station we just inserted
! Use *ana_id instead of ana_id after an idba_prendilo
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
ierr = idba_prendilo (handle)
```


### Query data, then query station data

```fortran
ierr = idba_preparati(dbhandle, handle, "read", "read", "read")
ierr = idba_preparati(dbhandle, handleana, "read", "read", "read")

! Prepare a query
ierr = idba_setd (handle, "latmin", 10)
...
ierr = idba_setd (handle, "lonmax", 60)

! Make the query
ierr = idba_voglioquesto (handle, N)

! Iterate the results
do i=1,N
  ierr = idba_dammelo (handle, varname)

  ! Read data about the variable we just had
  ierr = idba_enqlevel (handle, ltype, l1, l2)

  ! Read pseudoana data about the variable we just had
  ! Setup a query for the station with 'quantesono'
  ierr = idba_enqi (handle, "ana_id", anaid)
  ierr = idba_seti (handleana, "ana_id", anaid)

  ! Query.  Nstaz should always be 1 because we query a specific station
  ierr = idba_quantesono (handleana, Nstaz)

  ! Fetch the data
  ierr = idba_elencamele (handleana)

  ! Read the data about the station
  ! All the data inserted with setcontextana is available here
  ierr = idba_enqi (handleana, "height", height)
enddo
```

<a name="ch_trouble"></a>
# FAQ and Troubleshooting

## How do I access the station values?

There are two ways:

If you know in advances what variables you want to read, you can find them
among the results of [idba_elencamele][]:

```fortran
      ! Query station data
      ierr = idba_quantesono(handle, count)

      ! Get the informations about a station
      do i=1,count
        ierr = idba_elencamele(handle)
        ierr = idba_enqc(handle, "name", cname)
        ierr = idba_enqi(handle, "B02001", type)
        ! ....
      enddo
```

If you want to see all the extra station data available, you can make an
explicit query for the extra station data using [idba_voglioquesto][] and
[idba_dammelo][]:

```fortran
      ierr = idba_seti("ana_id", id)
      ierr = idba_voglioquesto(handle, count)
      do i=1,count
        ierr = idba_dammelo(handle, param)
        ! get the value of this variable
        ierr = idba_enqc(handle, param, cvalue)
        print*,param,": ",cvalue
      enddo
```



[idba_error_code]: fapi_reference.md#idba_error_code
[idba_error_message]: fapi_reference.md#idba_error_message
[idba_error_context]: fapi_reference.md#idba_error_context
[idba_error_details]: fapi_reference.md#idba_error_details
[idba_error_set_callback]: fapi_reference.md#idba_error_set_callback
[idba_error_remove_callback]: fapi_reference.md#idba_error_remove_callback
[idba_default_error_handler]: fapi_reference.md#idba_default_error_handler
[idba_error_handle_tolerating_overflows]: fapi_reference.md#idba_error_handle_tolerating_overflows
[idba_presentati]: fapi_reference.md#idba_presentati
[idba_arrivederci]: fapi_reference.md#idba_arrivederci
[idba_preparati]: fapi_reference.md#idba_preparati
[idba_messaggi]: fapi_reference.md#idba_messaggi
[idba_fatto]: fapi_reference.md#idba_fatto
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
[idba_scopa]: fapi_reference.md#idba_scopa
[idba_quantesono]: fapi_reference.md#idba_quantesono
[idba_elencamele]: fapi_reference.md#idba_elencamele
[idba_voglioquesto]: fapi_reference.md#idba_voglioquesto
[idba_dammelo]: fapi_reference.md#idba_dammelo
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
