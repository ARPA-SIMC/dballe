Parameters used when reading query results
==========================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

.. _parms_read_station:

Parameters used when reading station query results
--------------------------------------------------

============== ======================= =============================== ====================================================== 
Name           Unit                    Description                     Comment
============== ======================= =============================== ====================================================== 
``priority``   Integer                 Priority of this type of report Every type of report has an associated priority that controls
                                                                       which data are first returned when there is more than one in the
                                                                       same physical space.  It can be changed by editing ``/etc/dballe/repinfo.csv``
``rep_memo``   String                  Type of report                  Alias for ``report``
``report``     String                  Type of report                  Alias for ``rep_memo``
``ana_id``     Integer                 Station database ID             Internal DB-ALLe ID referring to an existing station, usable as a shortcut
``mobile``     Integer                 Station is mobile               Set to ``1`` if the station is mobile, such as a ship or a flight; else ``0``
``ident``      String                  Station identifier              present if ``mobile=1``
``lat``        Float                   Latitude
``lon``        Float                   Longitude
``coords``     Coords                  Station coordinates             Only available in Python
``station``    :class:`dballe.Station` Station                         Only available in Python
============== ======================= =============================== ====================================================== 


.. _parms_read_stationdata:

Parameters used when reading query results for station values
-------------------------------------------------------------

============== ======================= =============================== ====================================================== 
Name           Unit                    Description                     Comment
============== ======================= =============================== ====================================================== 
``priority``   Integer                 Priority of this type of report Every type of report has an associated priority that controls
                                                                       which data are first returned when there is more than one in the
                                                                       same physical space.  It can be changed by editing ``/etc/dballe/repinfo.csv``
``rep_memo``   String                  Type of report                  Alias for ``report``
``report``     String                  Type of report                  Alias for ``rep_memo``
``ana_id``     Integer                 Station database ID             Internal DB-ALLe ID referring to an existing station, usable as a shortcut
``mobile``     Integer                 Station is mobile               Set to ``1`` if the station is mobile, such as a ship or a flight; else ``0``
``ident``      String                  Station identifier              present if ``mobile=1``
``lat``        Float                   Latitude
``lon``        Float                   Longitude
``coords``     :class:`dballe.Coords`  Station coordinates             Only available in Python
``station``    :class:`dballe.Station` Station                         Only available in Python
``var``        String                  Variable code                   Set to the current variable code when iterating results
``variable``   :class:`dballe.Var`     Variable                        Only available in Python
``attrs``      List                    Attributes                      Set to the current variable code when iterating results
``context_id`` Integer                 ID of the variable              ID identifying a variable in the database, can be used as a shortcut to access its attributes
============== ======================= =============================== ====================================================== 

The variable value can be queried using the code in ``var``.


.. _parms_read_data:

Parameters used when reading query results for data values
----------------------------------------------------------

============== ======================== =============================== ====================================================== 
Name           Unit                     Description                     Comment
============== ======================== =============================== ====================================================== 
``priority``   Integer                  Priority of this type of report Every type of report has an associated priority that controls
                                                                        which data are first returned when there is more than one in the
                                                                        same physical space.  It can be changed by editing ``/etc/dballe/repinfo.csv``
``rep_memo``   String                   Type of report                  Alias for ``report``
``report``     String                   Type of report                  Alias for ``rep_memo``
``ana_id``     Integer                  Station database ID             Internal DB-ALLe ID referring to an existing station, usable as a shortcut
``mobile``     Integer                  Station is mobile               Set to ``1`` if the station is mobile, such as a ship or a flight; else ``0``
``ident``      String                   Station identifier              present if ``mobile=1``
``lat``        Float                    Latitude
``lon``        Float                    Longitude
``coords``     :class:`dballe.Coords`   Station coordinates             Only available in Python
``station``    :class:`dballe.Station`  Station                         Only available in Python
``datetime``   datetime                 Datetime                        Only available in Python
``year``       Integer                  Year
``month``      Integer                  Month
``day``        Integer                  Day
``hour``       Integer                  Hour
``min``        Integer                  Minutes
``sec``        Integer                  Seconds
``level``      :class:`dballe.Level`    Level                           Only available in Python
``leveltype1`` Integer                  Type of first level
``l1``         Integer                  Level layer L1
``leveltype2`` Integer                  Type of second level
``l2``         Integer                  Level layer L2
``trange``     :class:`dballe.Trange`   Time range                      Only available in Python
``pindicator`` Integer                  P indicator for time range
``p1``         Integer                  Time range P1
``p2``         Integer                  Time range P2
``var``        String                   Variable code                   Set to the current variable code when iterating results
``variable``   :class:`dballe.Var`      Variable                        Only available in Python
``attrs``      List                     Attributes                      Set to the current variable code when iterating results
``context_id`` Integer                  ID of the variable              ID identifying a variable in the database, can be used as a shortcut to access its attributes
============== ======================== =============================== ====================================================== 

The variable value can be queried using the code in ``var``.


.. _parms_read_summary:

Parameters used when reading query results for summary values
-------------------------------------------------------------

=============== ======================== ================================== ====================================================== 
Name            Unit                     Description                        Comment
=============== ======================== ================================== ====================================================== 
``priority``    Integer                  Priority of this type of report    Every type of report has an associated priority that controls
                                                                            which data are first returned when there is more than one in the
                                                                            same physical space.  It can be changed by editing ``/etc/dballe/repinfo.csv``
``rep_memo``    String                   Type of report                     Alias for ``report``
``report``      String                   Type of report                     Alias for ``rep_memo``
``ana_id``      Integer                  Station database ID                Internal DB-ALLe ID referring to an existing station, usable as a shortcut
``mobile``      Integer                  Station is mobile                  Set to ``1`` if the station is mobile, such as a ship or a flight; else ``0``
``ident``       String                   Station identifier                 present if ``mobile=1``
``lat``         Float                    Latitude
``lon``         Float                    Longitude
``coords``      :class:`dballe.Coords`   Station coordinates                Only available in Python
``station``     :class:`dballe.Station`  Station                            Only available in Python
``level``       :class:`dballe.Level`    Level                              Only available in Python
``leveltype1``  Integer                  Type of first level
``l1``          Integer                  Level layer L1
``leveltype2``  Integer                  Type of second level
``l2``          Integer                  Level layer L2
``trange``      :class:`dballe.Trange`   Time range                         Only available in Python
``pindicator``  Integer                  P indicator for time range
``p1``          Integer                  Time range P1
``p2``          Integer                  Time range P2
``datetimemax`` datetime                 Maximum datetime for this variable Only available with ``query=details``. Only available in Python
``datetimemin`` datetime                 Minimum datetime for this variable Only available with ``query=details``. Only available in Python
``yearmax``     Integer                  Maximum year for this variable     Only available with ``query=details``.
``yearmin``     Integer                  Minimum year for this variable     Only available with ``query=details``.
``monthmax``    Integer                  Maximum month for this variable    Only available with ``query=details``.
``monthmin``    Integer                  Minimum month for this variable    Only available with ``query=details``.
``daymax``      Integer                  Maximum day for this variable      Only available with ``query=details``.
``daymin``      Integer                  Minimum day for this variable      Only available with ``query=details``.
``hourmax``     Integer                  Maximum hour for this variable     Only available with ``query=details``.
``hourmin``     Integer                  Minumum hour for this variable     Only available with ``query=details``.
``minumax``     Integer                  Maximum minutes for this variable  Only available with ``query=details``.
``minumin``     Integer                  Minimum minutes for this variable  Only available with ``query=details``.
``secmax``      Integer                  Maximum seconds for this variable  Only available with ``query=details``.
``secmin``      Integer                  Minimum seconds for this variable  Only available with ``query=details``.
``var``         String                   Variable code
``count``       Integer                  Number of values for this variable Only available with ``query=details``.
``context_id``  Integer                  Number of values for this variable Only available with ``query=details``. Deprecated: use ``count``
=============== ======================== ================================== ====================================================== 
