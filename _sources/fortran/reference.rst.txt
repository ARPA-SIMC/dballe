Fortran dballe functions
========================

.. toctree::
    :maxdepth: 2

.. highlight:: fortran

Summary of routines
-------------------

Error management routines
^^^^^^^^^^^^^^^^^^^^^^^^^

================================================ ============================================================
Name                                             Description
================================================ ============================================================
:c:func:`idba_error_code`                        Return the error code for the last function that was called.
:c:func:`idba_error_message`                     Return the error message for the last function that was called.
:c:func:`idba_error_context`                     Return a string describing the error context description for the last function that was called.
:c:func:`idba_error_details`                     Return a string with additional details about the error for the last function that was called.
:c:func:`idba_error_set_callback`                Set a callback to be invoked when an error of a specific kind happens.
:c:func:`idba_error_remove_callback`             Remove a previously set callback.
:c:func:`idba_default_error_handler`             Predefined error callback that prints a message and exits.
:c:func:`idba_error_handle_tolerating_overflows` Predefined error callback that prints a message and exists, except in case of overflow errors.
================================================ ============================================================


Session routines
^^^^^^^^^^^^^^^^

These routines are used to begin and end working sessions with DB-All.e.

================================================ ============================================================
Name                                             Description
================================================ ============================================================
:c:func:`idba_connect`                           Connect to the database.
:c:func:`idba_disconnect`                        Disconnect from the database.
:c:func:`idba_begin`                             Open a new session.
:c:func:`idba_begin_messages`                    Start working with a message file.
:c:func:`idba_commit`                            Close a session.
================================================ ============================================================


Input/output routines
^^^^^^^^^^^^^^^^^^^^^

These routines are used to set the input and read the output of action routines.

================================================ ============================================================
Name                                             Description
================================================ ============================================================
:c:func:`idba_seti`                              Set an integer value in input.
:c:func:`idba_setb`                              Set a byte value in input.
:c:func:`idba_setr`                              Set a real value in input.
:c:func:`idba_setd`                              Set a real*8 value in input.
:c:func:`idba_setc`                              Set a character value in input.
:c:func:`idba_enqi`                              Read an integer value from the output.
:c:func:`idba_enqb`                              Read a byte value from the output.
:c:func:`idba_enqr`                              Read a real value from the output.
:c:func:`idba_enqd`                              Read a real*8 value from the output.
:c:func:`idba_enqc`                              Read a character value from the output.
:c:func:`idba_unset`                             Remove one value from the input.
:c:func:`idba_unsetb`                            Remove all Bxxyyy values from the input.
:c:func:`idba_unsetall`                          Completely clear the input, removing all values.
:c:func:`idba_set_station_context`               Signal that the input values that are set are related to station values instead of normal variables.
================================================ ============================================================


Input/output shortcuts
^^^^^^^^^^^^^^^^^^^^^^

The following routines are shortcuts for common combinations of
Input/Output routines.

================================================ ============================================================
Name                                             Description
================================================ ============================================================
:c:func:`idba_setlevel`                          Set all level information.
:c:func:`idba_settimerange`                      Set all time range information.
:c:func:`idba_setdate`                           Set all date information.
:c:func:`idba_setdatemin`                        Set the minimum date for a query.
:c:func:`idba_setdatemax`                        Set the maximum date for a query.
:c:func:`idba_enqlevel`                          Read all level information.
:c:func:`idba_enqtimerange`                      Read all time range information.
:c:func:`idba_enqdate`                           Read all date information.
================================================ ============================================================

Action routines
^^^^^^^^^^^^^^^

================================================ ============================================================
Name                                             Description
================================================ ============================================================
:c:func:`idba_reinit_db`                         Reinitialize the database, removing all data and loading report information.
:c:func:`idba_query_stations`                    Query the stations in the database.
:c:func:`idba_next_station`                      Retrieve the data about one station.
:c:func:`idba_query_data`                        Query the data in the database.
:c:func:`idba_next_data`                         Retrieve the data about one value.
:c:func:`idba_insert_data`                       Insert a new value in the database.
:c:func:`idba_remove_data`                       Remove from the database all values that match the query.
:c:func:`idba_remove_all`                        Remove all values from the database.
:c:func:`idba_query_attributes`                  Query attributes about a variable.
:c:func:`idba_next_attribute`                    Retrieve one attribute from the result of idba_query_attributes().
:c:func:`idba_insert_attributes`                 Insert new attributes for a variable.
:c:func:`idba_remove_attributes`                 Remove attribute information for a variable.
================================================ ============================================================

Message routines
^^^^^^^^^^^^^^^^

================================================ ============================================================
Name                                             Description
================================================ ============================================================
:c:func:`idba_messages_open_input`               Open a BUFR, or CREX file for reading.
:c:func:`idba_messages_open_output`              Open a BUFR, or CREX file for writing.
:c:func:`idba_messages_read_next`                Read the next message and import it in the database.
:c:func:`idba_messages_write_next`               Export the data from the database that match the current query and add them to the current message.
================================================ ============================================================

Pretty-printing routines
^^^^^^^^^^^^^^^^^^^^^^^^

================================================ ============================================================
Name                                             Description
================================================ ============================================================
:c:func:`idba_describe_level`                    Format the description of a level given its value.
:c:func:`idba_describe_timerange`                Format the description of a time range given its value.
:c:func:`idba_describe_var`                      Format the description of a variable given its varcode and its value.
================================================ ============================================================

Deprecated aliases
^^^^^^^^^^^^^^^^^^

The following routines are deprecated compatibility aliases for other
API functions.

================================================ ============================================================
Old name                                         New name
================================================ ============================================================
``idba_presentati``                              :c:func:`idba_connect`
``idba_arrivederci``                             :c:func:`idba_disconnect`
``idba_preparati``                               :c:func:`idba_begin`
``idba_messaggi``                                :c:func:`idba_begin_messages`
``idba_fatto``                                   :c:func:`idba_commit`
``idba_setcontextana``                           :c:func:`idba_set_station_context`
``idba_scopa``                                   :c:func:`idba_reinit_db`
``idba_quantesono``                              :c:func:`idba_query_stations`
``idba_elencamele``                              :c:func:`idba_next_station`
``idba_voglioquesto``                            :c:func:`idba_query_data`
``idba_dammelo``                                 :c:func:`idba_next_data`
``idba_prendilo``                                :c:func:`idba_insert_data`
``idba_dimenticami``                             :c:func:`idba_remove_data`
``idba_voglioancora``                            :c:func:`idba_query_attributes`
``idba_ancora``                                  :c:func:`idba_next_attribute`
``idba_critica``                                 :c:func:`idba_insert_attributes`
``idba_scusa``                                   :c:func:`idba_remove_attributes`
``idba_spiegal``                                 :c:func:`idba_describe_level`
``idba_spiegat``                                 :c:func:`idba_describe_timerange`
``idba_spiegab``                                 :c:func:`idba_describe_var`
================================================ ============================================================


Reference of routines
---------------------

Error management routines
^^^^^^^^^^^^^^^^^^^^^^^^^

.. c:function:: idba_error_code()

   Return the error code for the last function that was called.

   :return: The error code.
   :rtype: int

   This is a list of known codes:

   * 0: No error
   * 1: Item not found
   * 2: Wrong variable type
   * 3: Cannot allocate memory
   * 4: Database error
   * 5: Handle management error
   * 6: Buffer is too short to fit data
   * 7: Error reported by the system
   * 8: Consistency check failed
   * 9: Parse error
   * 10: Write error
   * 11: Regular expression error
   * 12: Feature not implemented
   * 13: Value outside acceptable domain


.. c:function:: idba_error_message(message, message_len)

   :arg message: The string where the error message will be written. If the string is not long enough, it will be truncated.
   :arg message_len: The size of the string buffer passed as message
   :return: The error message for the last function that was called.

   The error message is just a description of the error code. To see more
   details of the specific condition that caused the error, use
   :c:func:`idba_error_context` and :c:func:`idba_error_details`.


.. c:function:: idba_error_context(message, message_len)

   :arg message: The string where the error context will be written. If the string is not long enough, it will be truncated.
   :arg message_len: The size of the string buffer passed as message
   :return: A string describing the error context description for the last function that was called.

   This string describes what the code that failed was trying to do.

.. c:function:: idba_error_details(message, message_len)

   :arg message: The string where the error details will be written. If the string is not long enough, it will be truncated.
   :arg message_len: The size of the string buffer passed as message
   :return: The string holding the error details. If the string is not long enough, it will be truncated.

   Return a string with additional details about the error for the last
   function that was called.

   This string contains additional details about the error in case the
   code was able to get extra informations about it, for example by
   querying the error functions of an underlying module.

.. c:function:: idba_error_set_callback(code, func, data, handle)

   Set a callback to be invoked when an error of a specific kind happens.

   :arg code: The error code of the error that triggers this callback. If
              DBA_ERR_NONE is used, then the callback is invoked on all errors.
   :arg func: The function to be called.
   :arg data: An arbitrary integer data that is passed verbatim to the
              callback function when invoked.
   :arg handle: A handle that can be used to remove the callback
   :return: The error indicator for the function
   :rtype: int


.. c:function:: idba_error_remove_callback(handle)

   Remove a previously set callback.

   :arg handle: The handle previously returned by :c:func:`idba_error_set_callback`
   :return: The error indicator for the function
   :rtype: int

.. c:function:: idba_default_error_handler(debug)

   Predefined error callback that prints a message and exits.

   The message is printed only if a non-zero value is supplied as user data

.. c:function:: idba_error_handle_tolerating_overflows(debug)

   Predefined error callback that prints a message and exists, except in
   case of overflow errors.

   In case of overflows it prints a warning and continues execution


Session routines
^^^^^^^^^^^^^^^^

.. c:function:: idba_connect(dbahandle, url)

   Connect to the database.

   :arg url: The URL of the database to use
   :arg user: Used in the past, now it is ignored.
   :arg password: Used in the past, now it is ignored.
   :arg dbahandle: The database handle that can be passed to :c:func:`idba_begin` to work with the database.
   :return: The error indicator for the function

   This function can be called more than once to connect to different
   databases at the same time.

   The function expects to find a properly initialised DB-All.e database.
   Append ``&wipe=true`` to the end of the url to wipe any existing
   DB-All.e information from the database if it exists, then recreate it
   from scratch.


.. c:function:: idba_disconnect(dbahandle)

   Disconnect from the database.

   :arg dbahandle: The database handle to close.


.. c:function:: idba_begin(dbahandle, handle, anaflag, dataflag, attrflag)

   Open a new session.

   :arg dbahandle: The main DB-ALLe connection handle
   :arg handle: The session handle created by the function
   :arg anaflag: station values access level
   :arg dataflag: data values access level
   :arg attrflag: attribute access level
   :return: The error indicator for the function

   You can call :c:func:`idba_begin` many times and get more
   handles. This allows to perform many operations on the database at the
   same time.

   :c:func:`idba_begin()` has three extra parameters that can be
   used to limit write operations on the database, as a limited
   protection against programming errors:

   ``anaflag`` controls access to station value records and can have these
   values:

   * ``"read"`` station records cannot be inserted.
   * ``"write"`` it is possible to insert and delete pseudoana records.

   ``dataflag`` controls access to observed data and can have these values:

   * ``"read"`` data cannot be modified in any way.
   * ``"add"`` data can be added to the database, but existing data cannot
     be modified. Deletions are disabled. This is used to insert new data
     in the database while preserving the data that was already present
     in it.
   * ``"write"`` data can freely be added, overwritten and deleted.

   ``attrflag`` controls access to data attributes and can have these values:

   * ``"read"`` attributes cannot be modified in any way.
   * ``"write"`` attributes can freely be added, overwritten and deleted.

   Note that some combinations of parameters are illegal, such as ``anaflag=read``
   and ``dataflag=add`` (when adding a new data, it's sometimes necessary to insert
   new pseudoana records), or ``dataflag=rewrite`` and ``attrflag=read`` (when deleting
   data, their attributes are deleted as well).


.. c:function:: idba_begin_messages(handle, filename, mode, type)

   Start working with a message file.

   :arg handle: The session handle returned by the function
   :arg filename: Name of the file to open
   :arg mode: File open mode. It can be ``"r"`` for read, ``"w"`` for write
              (the old file is deleted), ``"a"`` for append
   :arg type: Format of the data in the file. It can be: ``"BUFR"``,
              ``"CREX"``, ``"AUTO"`` (autodetect, read only)
   :return: The error indicator for the function


.. c:function:: idba_commit(handle)

   Close a session.
    
   :arg handle: Handle to the session to be closed.


Input/output routines
^^^^^^^^^^^^^^^^^^^^^

.. c:function:: idba_seti(handle, parameter, value)

   Set an integer value in input.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to set. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: The value to assign to the parameter
   :return: The error indicator for the function


.. c:function:: idba_setb(handle, parameter, value)

   Set a byte value in input.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to set. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: The value to assign to the parameter
   :return: The error indicator for the function


.. c:function:: idba_setr(handle, parameter, value)

   Set a real value in input.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to set. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: The value to assign to the parameter
   :return: The error indicator for the function


.. c:function:: idba_setd(handle, parameter, value)

   Set a ``real*8`` value in input.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to set. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: The value to assign to the parameter
   :return: The error indicator for the function


.. c:function:: idba_setc(handle, parameter, value)

   Set a character value in input.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to set. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: The value to assign to the parameter
   :return: The error indicator for the function


.. c:function:: idba_enqi(handle, parameter, value)

   Read an integer value from the output.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to query. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: Where the value will be returned
   :return: The error indicator for the function


.. c:function:: idba_enqb(handle, parameter, value)

   Read a byte value from the output.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to query. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: Where the value will be returned
   :return: The error indicator for the function


.. c:function:: idba_enqr(handle, parameter, value)

   Read a real value from the output.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to query. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: Where the value will be returned
   :return: The error indicator for the function


.. c:function:: idba_enqd(handle, parameter, value)

   Read a ``real*8`` value from the output.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to query. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: Where the value will be returned
   :return: The error indicator for the function


.. c:function:: idba_enqc(handle, parameter, value, value_len)

   Read a character value from the output.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to query. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :arg value: Where the value will be returned
   :return: The error indicator for the function


.. c:function:: idba_unset(handle, parameter)

   Remove one value from the input.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Parameter to remove. It can be the code of a WMO variable
                   prefixed by ``"B"`` (such as ``"B01023"``); the code of a QC value
                   prefixed by ``"*B"`` (such as ``"*B01023"``) or a keyword among the ones
                   defined in :ref:`parms`.
   :return: The error indicator for the function


.. c:function:: idba_unsetb(handle)

   Remove all ``Bxxyyy`` values from the input.

   :arg handle: Handle to a DB-All.e session


.. c:function:: idba_unsetall(handle)

   Completely clear the input, removing all values.

   :arg handle: Handle to a DB-All.e session


.. c:function:: idba_set_station_context(handle)

   Signal that the input values that are set are related to station
   values instead of normal variables.

   :arg handle: Handle to a DB-All.e session
   :return: The error indicator for the function


Input/output shortcuts
^^^^^^^^^^^^^^^^^^^^^^

.. c:function:: idba_setlevel(handle, ltype1, l1, ltype2, l2)

   Set all level information.

   :arg handle: Handle to a DB-All.e session
   :arg ltype1: Level type to set in the input record
   :arg l1: L1 to set in the input record
   :arg ltype2: Level type to set in the input record
   :arg l2: L2 to set in the input record
   :return: The error indicator for the function


.. c:function:: idba_settimerange(handle, ptype, p1, p2)

   Set all time range information.

   :arg handle: Handle to a DB-All.e session
   :arg ptype: P indicator to set in the input record
   :arg p1: P1 to set in the input record
   :arg p2: P2 to set in the input record
   :return: The error indicator for the function


.. c:function:: idba_setdate(handle, year, month, day, hour, min, sec)

   Set all date information.

   :arg handle: Handle to a DB-All.e session
   :arg year: Year to set in the input record
   :arg month: Month to set in the input
   :arg day: Day to set in the input
   :arg hour: Hour to set in the input
   :arg min: Minute to set in the input
   :arg sec: Second to set in the input
   :return: The error indicator for the function


.. c:function:: idba_setdatemin(handle, year, month, day, hour, min, sec)

   Set the minimum date for a query.

   :arg handle: Handle to a DB-All.e session
   :arg year: Minimum year to set in the query
   :arg month: Minimum month to set in the query
   :arg day: Minimum day to set in the query
   :arg hour: Minimum hour to set in the query
   :arg min: Minimum minute to set in the query
   :arg sec: Minimum second to set in the query
   :return: The error indicator for the function


.. c:function:: idba_setdatemax(handle, year, month, day, hour, min, sec)

   Set the maximum date for a query.

   :arg handle: Handle to a DB-All.e session
   :arg year: Maximum year to set in the query
   :arg month: Maximum month to set in the query
   :arg day: Maximum day to set in the query
   :arg hour: Maximum hour to set in the query
   :arg min: Maximum minute to set in the query
   :arg sec: Maximum second to set in the query
   :return: The error indicator for the function


.. c:function:: idba_enqlevel(handle, ltype1, l1, ltype2, l2)

   Read all level information.

   :arg handle: Handle to a DB-All.e session
   :arg ltype1: Type of the first level from the output record
   :arg l1: L1 from the output record
   :arg ltype2: Type of the second level from the output record
   :arg l2: L2 from the output record
   :return: The error indicator for the function


.. c:function:: idba_enqtimerange(handle, ptype, p1, p2)

   Read all time range information.

   :arg handle: Handle to a DB-All.e session
   :arg ptype: P indicator from the output record
   :arg p1: P1 from the output record
   :arg p2: P2 from the output record
   :return: The error indicator for the function


.. c:function:: idba_enqdate(handle, year, month, day, hour, min, sec)

   Read all date information.

   :arg handle: Handle to a DB-All.e session
   :arg year: Year from the output record
   :arg month: Month from the output record
   :arg day: Day from the output record
   :arg hour: Hour from the output record
   :arg min: Minute from the output record
   :arg sec: Second from the output record
   :return: The error indicator for the function


Action routines
^^^^^^^^^^^^^^^

.. c:function:: idba_reinit_db(handle, repinfofile)

   Reinitialize the database, removing all data and loading report information.

   :arg handle: Handle to a DB-All.e session
   :arg repinfofile: CSV file with the default report informations. See :c:func:`idba_reset` documentation for the format of the file.
   :return: The error indicator for the function

   It requires the database to be opened in rewrite mode.


.. c:function:: idba_query_stations(handle, count)

   Query the stations in the database.

   :arg handle: Handle to a DB-All.e session
   :arg count: The count of elements
   :return: The error indicator for the function

   Results are retrieved using :c:func:`idba_next_station`.

   There is no guarantee on the ordering of results of :c:func:`idba_query_stations`/:c:func:`idba_next_station`.


.. c:function:: idba_next_station(handle)

   Retrieve the data about one station.

   :arg handle: Handle to a DB-All.e session
   :return: The error indicator for the function

   After invocation, the output record is filled with information about the station and its station values.

   If there are no more stations to read, the function will fail with ``DBA_ERR_NOTFOUND``.


.. c:function:: idba_query_data(handle, count)

   Query the data in the database.

   :arg handle: Handle to a DB-All.e session
   :arg count: Number of values returned by the function
   :return: The error indicator for the function

   Results are retrieved using :c:func:`idba_next_data`.

   Results are sorted by (in order): ``ana_id``, ``datetime``, ``level``, ``time range``, ``varcode``.
   The ``ana_id`` changes slowest, and the ``varcode`` changes fastest.

   Ordering by ``ana_id`` effectively does grouping by station rather than ordering.

   Sort order can change in the future, with the invariant that the slowest to
   change remains ``ana_id``, followed by ``datetime``, and the fastest to change
   remains the ``varcode``.


.. c:function:: idba_next_data(handle, parameter, parameter_len)

   Retrieve the data about one value.

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Contains the variable code of the parameter retrieved by this fetch
   :return: The error indicator for the function

   After invocation, the output record is filled with information about the value.

   If there are no more values to read, the function will fail with ``DBA_ERR_NOTFOUND``.


.. c:function:: idba_insert_data(handle)

   Insert a new value in the database.

   :arg handle: Handle to a DB-All.e session
   :return: The error indicator for the function

   This function will fail if the database is open in data readonly mode, and
   it will refuse to overwrite existing values if the database is open in data
   add mode.

   If the database is open in station reuse mode, the station values provided
   on input will be used to create a station record if it is missing, but will
   be ignored if it is already present. If it is open in station rewrite mode
   instead, the station values on input will be used to replace all the
   existing station values.


.. c:function:: idba_remove_data(handle)

   Remove from the database all values that match the query.

   :arg handle: Handle to a DB-All.e session
   :return: The error indicator for the function

   This function will fail unless the database is open in data rewrite mode.


.. c:function:: idba_remove_all(handle)

   Remove all values from the database.

   :arg handle: Handle to a DB-All.e session
   :return: The error indicator for the function

   The difference with :c:func:`idba_reinit_db` is that it preserves the
   existing report information.


.. c:function:: idba_query_attributes(handle, count)

   Query attributes about a variable.

   :arg handle: Handle to a DB-All.e session
   :arg count: Number of values returned by the function
   :return: The error indicator for the function

   The variable queried is either:
   
   * the last variable returned by :c:func:`idba_next_data`
   * the last variable inserted by :c:func:`idba_insert_data`
   * the variable selected by settings ``*context_id`` and ``*var_related``.
   
   Results are retrieved using :c:func:`idba_next_attribute`.


.. c:function:: idba_next_attribute(handle, parameter, parameter_len)

   Retrieve one attribute from the result of idba_query_attributes().

   :arg handle: Handle to a DB-All.e session
   :arg parameter: Contains the ID of the parameter retrieved by this fetch
   :return: The error indicator for the function


.. c:function:: idba_insert_attributes(handle)

   Insert new attributes for a variable.

   :arg handle: Handle to a DB-All.e session
   :return: The error indicator for the function

   The variable is either:
   
   * the last variable returned by :c:func:`idba_next_data`
   * the last variable inserted by :c:func:`idba_insert_data`
   * the variable selected by settings ``*context_id`` and ``*var_related``.
   
   The attributes that will be inserted are all those set by the
   functions :c:func:`idba_seti`, :c:func:`idba_setc`, :c:func:`idba_setr`,
   :c:func:`idba_setd`, using an asterisk in front of the variable name.

   Contrarily to :c:func:`idba_insert_data`, this function resets all the
   attribute information (and only attribute information) previously set in
   input, so the values to be inserted need to be explicitly set every time.

   This function will fail if the database is open in attribute readonly
   mode, and it will refuse to overwrite existing values if the database
   is open in attribute add mode.

.. c:function:: idba_remove_attributes(handle)

   Remove attribute information for a variable.

   :arg handle: Handle to a DB-All.e session
   :return: The error indicator for the function

   The variable is either:
   
   * the last variable returned by :c:func:`idba_next_data`
   * the last variable inserted by :c:func:`idba_insert_data`
   * the variable selected by settings ``*context_id`` and ``*var_related``.

   The attribute informations to be removed are selected with::

       idba_setc(handle, "*varlist", "*B33021,*B33003");


Message routines
^^^^^^^^^^^^^^^^

.. c:function:: idba_messages_open_input(handle, filename, mode, format, simplified)

   Open a BUFR, or CREX, file for reading.

   :arg handle: Handle to a DB-All.e session
   :arg filename: The file name
   :arg mode: The opening mode. See the mode parameter of libc's fopen() call for details.
   :arg format: The file format (``"BUFR"``, or ``"CREX"``)
   :arg simplified: true if the file is imported in simplified mode, false if
                    it is imported in precise mode. This controls approximating
                    levels and time ranges to standard values.
   :return: The error indicator for the function

   Each session can only have one open input file: if one was previously open,
   it is closed before opening the new one.


.. c:function:: idba_messages_open_output(handle, filename, mode, format)

   Open a BUFR, or CREX file for writing.

   :arg handle: Handle to a DB-All.e session
   :arg filename: The file name
   :arg mode: The opening mode. See the mode parameter of libc's fopen() call for details.
   :arg format: The file format (``"BUFR"``, or ``"CREX"``)
   :return: The error indicator for the function

   Each session can only have one open output file: if one was previously open,
   it is closed before opening the new one.


.. c:function:: idba_messages_read_next(handle, found)

   Read the next message and import it in the database.

   :arg handle: Handle to a DB-All.e session
   :arg found: True if a message has been imported, false if we are at the end of the input file.
   :return: The error indicator for the function

   The access mode of the session controls how data is imported:
   
   * station and data mode cannot be ``"read"``.
   * if data mode is ``"add"``, existing data will not be overwritten.
   * if attribute mode is ``"read"``, attributes will not be imported.


.. c:function:: idba_messages_write_next(handle, template_name)

   Export the data from the database that match the current query and add
   them to the current message.

   :arg handle: Handle to a DB-All.e session
   :arg template_name: The template name used to decide the layout of variables
                       in the messages that are exported.
   :return: The error indicator for the function


Pretty-printing routines
^^^^^^^^^^^^^^^^^^^^^^^^

.. c:function:: idba_describe_level(handle, ltype1, l1, ltype2, l2, result, result_len)

   Format the description of a level given its value.

   :arg handle: Handle to a DB-All.e session
   :arg ltype1: Level type to set in the input record
   :arg l1: L1 to set in the input record
   :arg ltype2: Level type to set in the input record
   :arg l2: L2 to set in the input record
   :arg result: The string with the description of the level.
   :return: The error indicator for the function


.. c:function:: idba_describe_timerange(handle, ptype, p1, p2, result, result_len)

   Format the description of a time range given its value.

   :arg handle: Handle to a DB-All.e session
   :arg ptype: P indicator to set in the input record
   :arg p1: P1 to set in the input record
   :arg p2: P2 to set in the input record
   :arg result: The string with the description of the time range.
   :return: The error indicator for the function


.. c:function:: idba_describe_var(handle, varcode, value, result, result_len)

   Format the description of a variable given its varcode and its value.

   :arg handle: Handle to a DB-All.e session
   :arg varcode: B table code of the variable (`"Bxxyyy"`)
   :arg value: Value of the variable, as read with :c:func:`idba_enqc`
   :arg result: The string with the description of the time range.
   :return: The error indicator for the function
