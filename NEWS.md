# Work in progress

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
