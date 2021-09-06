Parameters used when setting up a query
=======================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

.. _parms_query:

query_stations
--------------

================= ========= ======================================= =========================================================================
Name              Type      Description                             Comment
================= ========= ======================================= =========================================================================
``priority``      Integer   Priority of the type of report          Every type of report has an associated priority that controls which data
                                                                    are first returned when there is more than one in the same physical space.
                                                                    It can be changed by editing /etc/dballe/repinfo.csv
``priomax``       Integer   Maximum priority of reports queed
``priomin``       Integer   Minimum priority of reports queed
``report``        String    Type of report                          Alias for ``rep_memo``
``rep_memo``      String    Type of report                          Alias for ``report``
``ana_id``        Integer   Station database ID                     internal DB-ALLe ID referring to an existing station, usable as a shortcut reference
                                                                    instead of specifying the full data
``mobile``        Integer   Station is mobile                       Set to 1 to query mobile station, such as a ship or a flight; to 0 to query only
                                                                    fixed stations, such as synop or metar
``ident``         String    Station identifier
``lat``           Float     Latitude                                Setting as integer requires the value * 10^5; equivalent to setting latmin and latmax
``lon``           Float     Longitude                               Setting as integer requires the value * 10^5; equivalent to setting lonmin and lonmax
``latmax``        Float     Maximum latitude queried                Setting as integer requires the value * 10^5
``latmin``        Float     Minimum latitude queried                Setting as integer requires the value * 10^5
``lonmax``        Float     Maximum longitude queried               Setting as integer requires the value * 10^5
``lonmin``        Float     Minimum longitude queried               Setting as integer requires the value * 10^5
``var``           String    Station measures this variable          When setting var, varlist is cleared
``varlist``       String    Station measures one of these variables Comma-separated list of variable codes to query; when setting varlist, var is cleared
``query``         String    Query behaviour modifier                Comma-separated list of query modifiers.  See :ref:`parms_query_modifiers`.
``ana_filter``    String    Filter on station values                Restricts the results to only those stations which have a pseudoana value that matches
                                                                    the filter. See :ref:`parms_filters`.
``limit``         Integer   Maximum number of results to return
``block``         Integer   WMO block number of the station
``station``       Integer   WMO station number of the station
================= ========= ======================================= =========================================================================


query_station_data, remove_station_data
---------------------------------------

================= ========= =================================== =========================================================================
Name              Type      Description                         Comment
================= ========= =================================== =========================================================================
``priority``      Integer   Priority of the type of report      Every type of report has an associated priority that controls which data
                                                                are first returned when there is more than one in the same physical space.
                                                                It can be changed by editing /etc/dballe/repinfo.csv
``priomax``       Integer   Maximum priority of reports queed
``priomin``       Integer   Minimum priority of reports queed
``report``        String    Type of report                      Alias for ``rep_memo``
``rep_memo``      String    Type of report                      Alias for ``report``
``ana_id``        Integer   Station database ID                 internal DB-ALLe ID referring to an existing station, usable as a shortcut reference instead of specifying the full data
``mobile``        Integer   Station is mobile                   Set to 1 to query mobile station, such as a ship or a flight; to 0 to query only fixed stations, such as synop or metar
``ident``         String    Station identifier
``lat``           Float     Latitude                            Setting as integer requires the value * 10^5; equivalent to setting latmin and latmax
``lon``           Float     Longitude                           Setting as integer requires the value * 10^5; equivalent to setting lonmin and lonmax
``latmax``        Float     Maximum latitude queried            Setting as integer requires the value * 10^5
``latmin``        Float     Minimum latitude queried            Setting as integer requires the value * 10^5
``lonmax``        Float     Maximum longitude queried           Setting as integer requires the value * 10^5
``lonmin``        Float     Minimum longitude queried           Setting as integer requires the value * 10^5
``var``           String    Variable queried                    When setting var, varlist is cleared
``varlist``       String    List of variables to query          Comma-separated list of variable codes to query; when setting varlist, var is cleared
``query``         String    Query behaviour modifier            Comma-separated list of query modifiers.  See :ref:`parms_query_modifiers`.
``ana_filter``    String    Filter on station values            Restricts the results to only those stations which have a pseudoana value that matches the filter. See :ref:`parms_filters`.
``data_filter``   String    Filter on data                      Restricts the results to only the variables of the given type, which have a value that matches the filter. See :ref:`parms_filters`.
``attr_filter``   String    Filter on data attributes           Restricts the results to only those data which have an attribute that matches the filter. See :ref:`parms_filters`.
``limit``         Integer   Maximum number of results to return
``block``         Integer   WMO block number of the station
``station``       Integer   WMO station number of the station
================= ========= =================================== =========================================================================


query_data, remove_data, query_summary, query_messages
------------------------------------------------------

================= ====================== =================================== =========================================================================
Name              Type                   Description                         Comment
================= ====================== =================================== =========================================================================
``priority``      Integer                Priority of the type of report      Every type of report has an associated priority that controls which data
                                                                             are first returned when there is more than one in the same physical space.
                                                                             It can be changed by editing /etc/dballe/repinfo.csv
``priomax``       Integer                Maximum priority of reports queed
``priomin``       Integer                Minimum priority of reports queed
``report``        String                 Type of report                      Alias for ``rep_memo``
``rep_memo``      String                 Type of report                      Alias for ``report``
``ana_id``        Integer                Station database ID                 internal DB-ALLe ID referring to an existing station, usable as a shortcut reference instead of specifying the full data
``mobile``        Integer                Station is mobile                   Set to 1 to query mobile station, such as a ship or a flight; to 0 to query only fixed stations, such as synop or metar
``ident``         String                 Station identifier
``lat``           Float                  Latitude                            Setting as integer requires the value * 10^5; equivalent to setting latmin and latmax
``lon``           Float                  Longitude                           Setting as integer requires the value * 10^5; equivalent to setting lonmin and lonmax
``latmax``        Float                  Maximum latitude queried            Setting as integer requires the value * 10^5
``latmin``        Float                  Minimum latitude queried            Setting as integer requires the value * 10^5
``lonmax``        Float                  Maximum longitude queried           Setting as integer requires the value * 10^5
``lonmin``        Float                  Minimum longitude queried           Setting as integer requires the value * 10^5
``year``          Integer                Year                                Equivalent to setting yearmin and yearmax
``month``         Integer                Month                               Equivalent to setting monthmin and monthmax
``day``           Integer                Day                                 Equivalent to setting daymin and daymax
``hour``          Integer                Hour                                Equivalent to setting hourmin and hourmax
``min``           Integer                Minutes                             Equivalent to setting minumin and minumax
``sec``           Integer                Seconds                             Equivalent to setting secmin and secmax
``yearmax``       Integer                Maximum year queried
``yearmin``       Integer                Year or minimum year queried
``monthmax``      Integer                Maximum month queried
``monthmin``      Integer                Minimum month queried
``daymax``        Integer                Maximum day queried
``daymin``        Integer                Minimum day queried
``hourmax``       Integer                Maximum hour queried
``hourmin``       Integer                Minumum hour queried
``minumax``       Integer                Maxminum minutes queried
``minumin``       Integer                Minimum minutes queried
``secmax``        Integer                Maxminum seconds queried
``secmin``        Integer                Minimum seconds queried
``datetime``      datetime               Sets ``year``…``sec`` values        Only available in Python
``datetimemin``   datetime               Sets ``yearmin``…``secmin`` values  Only available in Python
``datetimemax``   datetime               Sets ``yearmax``…``secmax`` values  Only available in Python
``level``         :class:`dballe.Level`  Sets ``leveltype1``…``l2`` values   Only available in Python
``trange``        :class:`dballe.Trange` Sets ``pindicator``…``p2`` values   Only available in Python
``leveltype1``    Integer                Type of first level
``l1``            Integer                Level layer L1
``leveltype2``    Integer                Type of second level
``l2``            Integer                Level layer L2
``pindicator``    Integer                P indicator for time range
``p1``            Integer                Time range P1
``p2``            Integer                Time range P2
``var``           String                 Variable queried                    When setting var, varlist is cleared
``varlist``       String                 List of variables to query          Comma-separated list of variable codes to query; when setting varlist, var is cleared
``query``         String                 Query behaviour modifier            Comma-separated list of query modifiers.  See :ref:`parms_query_modifiers`.
``ana_filter``    String                 Filter on station values            Restricts the results to only those stations which have a pseudoana value that matches the filter. See :ref:`parms_filters`.
``data_filter``   String                 Filter on data                      Restricts the results to only the variables of the given type, which have a value that matches the filter. See :ref:`parms_filters`.
``attr_filter``   String                 Filter on data attributes           Restricts the results to only those data which have an attribute that matches the filter. See :ref:`parms_filters`.
``limit``         Integer                Maximum number of results to return
``block``         Integer                WMO block number of the station
``station``       Integer                WMO station number of the station
================= ====================== =================================== =========================================================================


.. _parms_query_modifiers:

Possible values for query modifiers
-----------------------------------

When setting ``query=…`` to alter behaviour of a query, one can use a
comma-separated list of these values:

=========== =======================================================================================
Name        Description
=========== =======================================================================================
``best``    When the same datum exists in multiple networks, return only the one with the highest priority.
``last``    When the same datum exists at different times, return only the most recent one.
``attrs``   Optimize for when data attributes will be read on the query result. See `issue114`_.
``bigana``  Not used anymore.
``nosort``  Run the query faster, but give no guarantees on the ordering of the results.
``stream``  Not used anymore.
``details`` Populate ``count`` and minimum/maximum datetime information in summary query results. See: :ref:`parms_read_summary`.
=========== =======================================================================================

.. _issue114: https://github.com/ARPA-SIMC/dballe/issues/114
