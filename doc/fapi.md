# DB-ALLe Guide to the Fortran API

DB-All.e is a fast, temporary, on-disk database where meteorological data can
be stored, searched, retrieved and updated.  To make computation easier, data
is stored as physical data, that is, as measures of a variable in a specific
point of space and time, rather than as a sequence of reports.

The Fortran documentation [can be found here](https://arpa-simc.github.io/dballe/fortran/dballe.html):

* [Fortran API concepts](https://arpa-simc.github.io/dballe/fortran/concepts.html),
  explains terms used through the documentation and concepts specific to DB-All.e.
* [Walk through the API](https://arpa-simc.github.io/dballe/fortran/walkthrough.html),
  introduces most aspects of the API.
* [Connection methods](https://arpa-simc.github.io/dballe/connect.html),
  documents the various ways that can be used to connect to a DB-All.e database.
* [API function reference](https://arpa-simc.github.io/dballe/fortran/reference.html),
  the complete reference to every function.
* [Transactional behaviour](https://arpa-simc.github.io/dballe/fortran/transactions.html),
  documents if and when updates to the database take effect, and what happens
  when multiple programs work on the same DB-All.e database at the same time.
* [Input/output/query parameters](https://arpa-simc.github.io/dballe/parms.html),
  a table with all input, output and query parameters that are not B table entries.
* [Local B table](https://arpa-simc.github.io/dballe/btable.html),
  contents of the local B table.
* [Level types](https://arpa-simc.github.io/dballe/ltypes.html),
  description of values that define vertical levels and layers.
* [Time ranges](https://arpa-simc.github.io/dballe/tranges.html),
  descriptions of values that define time ranges.
* [Varcode aliases](https://arpa-simc.github.io/dballe/aliases.html),
  aliases that can be used in input routines instead of B table codes.
* [Environment variable](https://arpa-simc.github.io/dballe/env.html),
  environment variables that influence the behaviour of DB-All.e.
