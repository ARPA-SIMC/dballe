# DB-ALLe Guide to the Fortran API

This document is a quick introduction to the Fortran API of DB-ALLe.  It
proceeds step by step to introducing different parts of the library using
commented example code.

## Introduction

DB-All.e is a fast, temporary, on-disk database where meteorological data can
be stored, searched, retrieved and updated.  To make computation easier, data
is stored as physical data, that is, as measures of a variable in a specific
point of space and time, rather than as a sequence of reports.

This is a quick introduction to DB-All.e.  It is intended as a quick read to
get up to speed, and as a quick reference for everyday use.

*Editorial notes*

This guide contains examples for all routines, plus examples for all known
common usage patterns of dballe.  Also, commented Fortran code is preferred
to long explanations whenever it makes sense.

## Fortran API documentation index

* [Fortran API concepts](fapi_concepts.md), explains terms used through the
  documentation and concepts specific to DB-All.e.
* [Walk through the API](fapi_walkthrough.md), introduces most aspects of the
  API.
* [API function reference](fapi_reference.md), the complete reference to every
  function.

# Appendix

## Input and output parameters

### For data-related action routines

<a name="parmtable"></a>

\input{../dballe/core/record_keyword.tex}

### For attribute-related action routines

<table class="table">
<thead>
  <tr>
    <th>Name</th>
    <th>Unit</th>
    <th>Format</th>
    <th>Description</th>
    <th>On insert input</th>
    <th>On query input</th>
    <th>On output</th>
    <th>Comment</th>
  </tr>
</thead>
<tbody>
  <tr>
    <td><code>*Bxxyyy</code></td>
    <td>depends</td>
    <td>depends</td>
    <td>Value of the acoderibute</td>
    <td>required</td>
    <td>ignored</td>
    <td>present</td>
    <td> </td>
  </tr>
  <tr>
    <td><code>*var</code></td>
    <td>Character</td>
    <td>7 chars</td>
    <td>Acoderibute queried</td>
    <td>ignored</td>
    <td>optional</td>
    <td>present, indicates the name of the last acoderibute returned</td>
    <td> </td>
  </tr>
  <tr>
    <td><code>*varlist</code></td>
    <td>Character</td>
    <td>255 chars</td>
    <td>List of acodeributes to query</td>
    <td>ignored</td>
    <td>optional</td>
    <td>absent</td>
    <td>Comma-separated list of acoderibute B codes wanted on output</td>
  </tr>
  <tr>
    <td><code>*var_related</code></td>
    <td>Character</td>
    <td>6 chars</td>
    <td>Variable related to the acoderibute to query</td>
    <td>required</td>
    <td>required</td>
    <td>absent</td>
    <td>It is automatically set by <code>idba_dammelo</code> and <code>idba_prendilo</code> (when <code>idba_prendilo</code> inserts a single variable)</td>
  </tr>
  <tr>
    <td><code>*context_id</code></td>
    <td>Numeric</td>
    <td>10 digits</td>
    <td>Context ID of the variable related to the acoderibute to query</td>
    <td>required</td>
    <td>required</td>
    <td>absent</td>
    <td>It is automatically set by <code>idba_dammelo</code> and <code>idba_prendilo</code></td>
   </tr>
</tbody>
</table>


## Values for level/layer and time range

### Level/layer
<a name="levels"></a>

\input{../dballe/msg/ltypes.tex}

### Time range
<a name="tranges"></a>

\input{../dballe/msg/tranges.tex}

# Contents of the local B table
<a name="btable"></a>

\input{../tables/dballe.tex}

# Variable aliases
<a name="aliastable"></a>

\input{../dballe/core/aliases.tex}

# Database connection methods
<a name="dburls"></a>

Instead of the usual DSN, user and password triplet it is possible to connect
to the database using an URL-like syntax.

## Connecting via ODBC

### Specifying DSN, user and password explicitly

Fortran:
```fortran
ierr = idba_presentati(dbhandle, "dsn", "username", "password") 
```

dbadb:
```sh
dbadb --dsn=dsn --user=username --pass=password
```

### Specifying DSN, user and password via URL

Fortran:
```fortran
ierr = idba_presentati(dbhandle, "odbc://username:password@dsn", DBA_MVC, DBA_MVC)
```

dbadb:
```sh
dbadb --dsn="odbc://username:password@dsn" …
```

provami:
```sh
provami "odbc://username:password@dsn"
```

### Specifying DSN, user and password via an environment variable

```sh
# Export the environment variable
export DBA_DB="odbc://username:password@dsn"
```

Fortran:
```fortran
ierr = idba_presentati(dbhandle, DBA_MVC, DBA_MVC, DBA_MVC)
```

dbadb:
```sh
dbadb …
```

provami:
```sh
provami
```

## Using a SQLite file directly

```sh
export DBA_DB="sqlite:temp.sqlite"
dbadb --wipe-first import data.bufr
# ...run something here that processes the data...
dbadb export > processed-data.bufr
rm temp.sqlite
```

## Via URL

Fortran:
```fortran
ierr = idba_presentati(dbhandle, "sqlite:file.sqlite", DBA_MVC, DBA_MVC)
```

dbadb:
```sh
dbadb --dsn="sqlite:file.sqlite" dots
```

provami:
```
provami --dsn="sqlite:file.sqlite"
```

# Summary of routines

This appendix contains some reference tables for the functions in the Fortran
API.
