Parameters used when inserting values
=====================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

.. _parms_insert:

============== ======================== ========================== ======================================================
Name           Unit                     Description                Comment
============== ======================== ========================== ======================================================
``rep_memo``   String                   Type of report             Alias for ``report``
``report``     String                   Type of report             Alias for ``rep_memo``
``ana_id``     Integer                  Station database ID        Can optionally be specified instead of ``lat``, ``lon``, ``ident``, ``rep_memo``: internal DB-ALLe ID referring to an existing station, usable as a shortcut
``ident``      String                   Station identifier         Optional: if missing, the station is fixed; if present, it is mobile
``lat``        Float                    Latitude
``lon``        Float                    Longitude
``year``       Integer                  Year
``month``      Integer                  Month                      Optional: when missing, the minimum valid value is used
``day``        Integer                  Day                        Optional: when missing, the minimum valid value is used
``hour``       Integer                  Hour                       Optional: when missing, the minimum valid value is used
``min``        Integer                  Minutes                    Optional: when missing, the minimum valid value is used
``sec``        Integer                  Seconds                    Optional: when missing, the minimum valid value is used
``leveltype1`` Integer                  Type of first level
``l1``         Integer                  Level layer L1
``leveltype2`` Integer                  Type of second level
``l2``         Integer                  Level layer L2
``pindicator`` Integer                  P indicator for time range
``p1``         Integer                  Time range P1
``p2``         Integer                  Time range P2
``datetime``   datetime                 Datetime                   Only available in Python
``level``      :class:`dballe.Level`    Level                      Only available in Python
``trange``     :class:`dballe.Trange`   Time range                 Only available in Python
============== ======================== ========================== ======================================================

Any variable code (as `Bxxyyy` or as a DB-All.e alias) can also be set to insert a variable.

In the Fortran API, after a data is inserted in the database, one can query
`ana_id` to get the database ID of the station used for that data.



