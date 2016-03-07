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


# Appendix

## Input and output parameters

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
    <td>Value of the attribute</td>
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
    <td>present, indicates the name of the last attribute returned</td>
    <td> </td>
  </tr>
  <tr>
    <td><code>*varlist</code></td>
    <td>Character</td>
    <td>255 chars</td>
    <td>List of attributes to query</td>
    <td>ignored</td>
    <td>optional</td>
    <td>absent</td>
    <td>Comma-separated list of attribute B codes wanted on output</td>
  </tr>
  <tr>
    <td><code>*var_related</code></td>
    <td>Character</td>
    <td>6 chars</td>
    <td>Variable related to the attribute to query</td>
    <td>required</td>
    <td>required</td>
    <td>absent</td>
    <td>It is automatically set by <code>idba_dammelo</code> and <code>idba_prendilo</code> (when <code>idba_prendilo</code> inserts a single variable)</td>
  </tr>
  <tr>
    <td><code>*context_id</code></td>
    <td>Numeric</td>
    <td>10 digits</td>
    <td>Context ID of the variable related to the attribute to query</td>
    <td>required</td>
    <td>required</td>
    <td>absent</td>
    <td>It is automatically set by <code>idba_dammelo</code> and <code>idba_prendilo</code></td>
   </tr>
</tbody>
</table>
