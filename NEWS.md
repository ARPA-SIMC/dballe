# New in version 8.0

* Added `NEWS.md` file
* Rationalised keyword usage in Fortran API. See [Input/Output query parameters
  documentation](doc/fapi_parms.md) for the updated reference.
* Added Message, Importer, Exporter, File, DB, Transaction, Cursor\*, Data,
  Values to the C++ stable API
* Dropped support for Python 2
* Dropped support for unused AOF format
* Renamed fortran API functions with self-explanatory English names. The old
  names are kept as aliases for compatibility with existing/old Fortran code
* Python API changes:
   * Added dballe.Message
   * Added dballe.Importer
   * Added dballe.Exporter
   * Added dballe.File
   * Added dballe.Explorer
   * Removed dballe.Record in favour of using dict() for input, and using
     `Cursor*.__getitem__` for output.
   * Renamed `Cursor*.attr_query` to `Cursor*.query_attrs`
   * Added `Cursor*.insert_attrs`
   * Added `Cursor*.remove_attrs`
   * Added `Cursor*.remove()`
   * Added `DB.query_messages`
   * DB or Transaction: `.query_*`: query argument is now optional, omit it to
     query everything
   * Added `Cursor.query` and `Cursor.data`
   * Allow to pass a `Cursor` instead of a `dict` to `insert_data` and
     `insert_station_data`, to efficiently copy data between databases.
   * Deprecated `DB.insert_station_data`, `DB.insert_data`,
     `DB.remove_station_data`, `DB.remove_data`, `DB.query_stations`,
     `DB.query_station_data`, `DB.query_data`, `DB.query_summary`,
     `DB.query_messages`, `DB.attr_query_station`, `DB.attr_query_data`,
     `DB.attr_insert_station`, `DB.attr_insert_data`, `DB.attr_remove_station`,
     `DB.attr_remove_data`.
     Use the same methods in `dballe.Transaction` instead.
 * The use of `?wipe` is now [documented](doc/fapi_connect.mdx) and supported
   in C++, Python, Fortran, and command line. The value of `wipe` is now parsed
   to prevent unexpectedly clearing a database when using something like
   `?wipe=no`.
