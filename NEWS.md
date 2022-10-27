# New in version 9.3

* Implemented debian packaging (#273, #274)

# New in version 9.2

* Added entries for mobile telephony links
* Fix bug in `dballe_txt_add_entry` when precision is not a power of 10
* `dballe_txt_add_entry` finds max code among the selected category (#271)
* Added B13207 "Water skin velocity module"

# New in version 9.1

* Implemented new query modifier `query=last` (#80)
* Fixed a DeprecationWarning when querying latitude/longitude with Decimal values (#264)
* Open/create a Xapian explorer only when needed, fixing locking issues on
  concurrent access (#262)
* Added `ilat` e `ilon` (integer lat/lon) for `Station` objects (#247)
* Added examples for messages import/export (#255)
* Removed dependency on nose (#268)
* Added Turbidity and ORP variables (#267)

# New in version 9.0

* C++ API changed: `CursorMessage::get_message` has been unified with
  `CursorMessage::detach_message` and now returns a `std::shared_ptr<Message>`.
  `CursorMessage::detach_message` has been removed. (#171)
* C++ API changed: it now uses `std::shared_ptr` instead of `std::unique_ptr`
  to return and refer to cursors. (#171)
* C++ API changed: dballe::Message now inherits `std::shared_from_this` and
  should always be used as `std::shared_ptr` (#171)
* C++ API changed: dballe::Cursor now inherits `std::shared_from_this` and
  should always be used as `std::shared_ptr` (#171)
* Now a cursor for a dballe transaction is invalidated when the transaction
  commits/rolls back. This avoids the need for a context manager to handle the
  cursor lifetime in the Python API (#171)

# New in version 8.20

* Added UV index variable

# New in version 8.19

* Fixed B13211 length (#239)

# New in version 8.18

* Added variables 025194 011211 011212 011213 011214 011215 011216

# New in version 8.16

* Added `dbadb import --domain-errors={unset|clamp|tag}`. `tag` is only
  available when compiling with wreport from version 3.29. (#241)

# New in version 8.15

* Added Lifted, virtual T and Skin Temperature variables
* Added support for python 3.9 in nose2 tests
* Dropped support for nose1

# New in version 8.14

* Added `dbadb import --domain-errors={unset|clamp}`. `clamp` is only available
  when compiling with wreport from version 3.28. (#241)
* Fixed querying by datetime extremes in explorer (#242)
* Added alternative meson build system

# New in version 8.13

* Always ignore stations without contexts (#235)

# New in version 8.12

* Improved documentation
* Use BUFR unit in documentation (#222)
* `dballe.Message.query_data()` error when the BUFR has only station data (#213)
* Fixed bugs in Explorer (#217, #218, #228)
* `attr_filter` supports != operator (#224)
* Fixed JSON datetime parser (#230)
* Fixed segmentation fault querying min and max datetimes in
* dballe.CursorSummarySummary (#232)

# New in version 8.11

* Fixed compilation error for gcc 10 (#211)
* Fix errors after failed starting of transactions and raise clear errors if using a db connection in a forked process (#210)

# New in version 8.10

* Fixed explorer query (#209)

# New in version 8.9

* Worked around an internal compiler error on Centos7
* Made Xapian support really optional

# New in version 8.8

* Added `dballe.Explorer` examples to python HOWTO (#181)
* Creating an Explorer with a pathname makes it load/save from that file, which
  can be a JSON file if it ends with `.json` or no Xapian support is compiled
  in, otherwise Explorer will persist using an indexed Xapian database.
* Undo breaking changes to stable API on importers/exporters

# New in version 8.7

* Fixed the command line documentation of possible input types (#202)
* Restructured and tested documentation (#204, #205, #206)
* JSON is now supported for encoding/decoding wherever BUFR and CREX are (#202)

# New in version 8.6

* Improved python API documentation (#187, #189, #191)
* Improved documentation of dballe types (#199)
* Turned a segfault into a proper exception (#197)
* Parse again '-' as missing (#200)

# New in version 8.5

* Fixed passing strings as datetime values from python (#174)

# New in version 8.4

* Redesigned and unified all the documentation
* B table updates
* Allow querying from python without specifying all the datetime min/max values
* Fixed querying longitude ranges from python
* Fixed reading unset query results from python

# New in version 8.3

* Python API: allow other C python extensions to access dballe python objects
* B table updates

# New in version 8.2

* Python API: fixed `Message.query_station_data` method (#160)
* Add level 161 Depth below water surface (#161)

# New in version 8.1

* Python API: bindings for core::Data (#158)
* Python API: add doc for keywords in cursor classes (#155)
* Python API: enabled cursor-based iteration (#154)
* Implemented `dbadb --varlist` (#149)

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

# New in version 7.33

* dballe V7 is now the default (#97) and V6 databases are now unsupported
* fixed various bugs (#82, #85, #86, #87, #91, #93, #98, #103, #105, #107)
* added new variables to dballe.txt
* various performance optimizations (#112, #116)
* `DBA_FORTRAN_TRANSACTION` is now the default (#67)
* improved handling of rejected messages and consistency errors
* fixed PostgreSQL support when pkg-config is missing
* dependency on newer sqlite is now conditional

# New in version 7.26

* dballe V7 is now the default (#97)
* closed #82, #85, #87, #91, #93, #98
* minor fixes for copr and travis automation
* added new variables to dballe.txt

# New in version 4.0.4

* Store `rep_cod` information in generic messages
* Allow to create messages without having to set leveltype2 and l2 if not needed
* Implemented working with messages with the Fortran API
* Refactored the code for the Fortran API
* Fixed `TOT_PREC12` time range values
* Implemented `bufrex_msg_reset_sections`
* Implemented Bufrex::resetSections and same in Python
* Test against NaN on setr and setd
* Use clean program name in manpages
* Store height and pressure as data and not as ana, for flights

# New in version 4.0.2

* Fix a compiler error with newer GCC

# New in version 4.0.1

* Fix repinfo update
* Ported rconvert to new rpy module
* Show the currently selected station details in provami, even if it has no data
* Decode BUFR edition 2
* Added tables to decode BUFR edition 2 radar reports
* Fixed encoding of BUFR flags
* Fixed the problem in provami that fails when the only restriction used is the ana filter
* Drafted support for pollution data
* Use standard floating point huge values to exchange missing values with Fortran
* Fortran API: `idba_elencamele` now properly cleans old values from the output record
* Fixed handling of varlist when querying attributes from fortran
* Documentation improvements for fapi.tex
* Hack a way to sync the values of missing values between Fortran and C
* Fixed bufr decoder to work on ARM
* Clean records before reading in new data in fortran `idba_dammelo`

# New in version 3.5

* Fixed transactions in cursor.c: now queries don't all happen in a single, huge transaction

# Changes from version 2.5 to version 2.6

* dbadb: dbadb export failed to export various kinds of station data
* dbadb: implemented dbadb cleanup
* dbadb: implemented dbadb stations
* dbadb manpage: refer to report code and report memo instead of just
  'type'
* dbadb manpage: mention what are the query parameters that can be used
* `dba_var_set*` and `idba_set*` operations now check for overflows and report
  `DBA_ERR_TOOLONG` if it happens
* fortran API: `idba_spiegab` now formats the values with the right amount of
  decimal digits
* fortran API: added another predefined callback
  (`idba_error_handle_tolerating_overflows`) that only prints a warning in case
  of overflow
* fapi.tex: documented that setting a variable to a missing value is
  equivalent to an `idba_unset`
* fapi.tex: documented the use of the dballef.h Fortran 90 interface file
* README: rewritten to only provide quick start information
