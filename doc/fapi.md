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

## Fortran API documentation index

* [Fortran API concepts](fapi_concepts.md), explains terms used through the
  documentation and concepts specific to DB-All.e.
* [Walk through the API](fapi_walkthrough.md), introduces most aspects of the
  API.
* [API function reference](fapi_reference.md), the complete reference to every
  function.
* [Input/output/query parameters](fapi_parms.md), a table with all input,
  output and query parameters that are not B table entries.
* [Varcode aliases](fapi_aliases.md), aliases that can be used in input
  routines instead of B table codes.
* [Connection methods](fapi_connect.md), documents the various ways that can be
  used to connect to a DB-All.e database.
* [Level types](fapi_ltypes.md), description of values that define vertical
  levels and layers.
* [Time ranges](fapi_tranges.md), descriptions of values that define time
  ranges.
* [Local B table](fapi_btable.md), contents of the local B table.
* [Environment variable](dballe_env.md), environment variables that influence the
  behaviour of DB-All.e.
