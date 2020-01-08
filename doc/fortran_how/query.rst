Querying the database
=====================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran

.. _querying:

Querying the database
---------------------

Queries are made by giving one or more extremes of space, time, level or time
range.  See :ref:`parms_query` for a list of all available query parameters.


Querying the station values
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example code to query all the stations in a given area::

    ierr = idba_setd(handle, "latmin", 30.D0)
    ierr = idba_setd(handle, "latmax", 50.D0)
    ierr = idba_setd(handle, "lonmin", 10.D0)
    ierr = idba_setd(handle, "lonmax", 20.D0)
    ierr = idba_query_stations(handle, count)
    do while (count.gt.0)
      ierr = idba_next_station(handle)
      ierr = idba_enqi(handle, "ana_id", id)
      ! Pseudoana values can be read as well:
      ierr = idba_enqc(handle, "name", cname)
      ierr = idba_enqd(handle, "B07001", height)
      ! ...query more data and work with it...
      count = count - 1
    enddo

This code introduces two new functions:

* :c:func:`idba_query_stations`: performs the query and returns the number of stations it
  finds.
* :c:func:`idba_next_station`: gets a station out of the results of :c:func:`idba_query_stations`.
  If there are no more stations, the function fails.

After :c:func:`idba_next_station`, the output record will also contain all the pseudoana
values available for the station.  If `rep_cod` or `rep_memo` are specified as
query parameters, the pseudoana values of that network will be used.  Else,
:c:func:`idba_next_station` will use all available pseudoana values, choosing the one in
the network with the highest priority in case the same pseudoana value is
available on more than one network.


Querying the measured values
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example code to query all the values in a given area and time::

    ierr = idba_seti(handle, "latmin", 30)
    ierr = idba_seti(handle, "latmax", 50)
    ierr = idba_seti(handle, "lonmin", 10)
    ierr = idba_seti(handle, "lonmax", 20)
    ierr = idba_seti(handle, "yearmin", 2004)
    ierr = idba_seti(handle, "yearmax", 2004)
    ierr = idba_query_data(handle, count)
    do while (count.gt.0)
      ierr = idba_next_data(handle, param)
      ! get the value of this variable
      ierr = idba_enqc(handle, param, cvalue)
      ierr = idba_enqd(handle, "lat", dlat)
      ierr = idba_enqd(handle, "lon", dlon)
      ! query more data and work with it
      count = count - 1
    enddo

This code introduces two new functions:

* :c:func:`idba_query_data`: performs the query and returns the number of values it
  finds.
* :c:func:`idba_next_data`: gets a value out of the result of :c:func:`idba_query_data`.  If
  there are no more stations, the function fails.


Modifiers for queries
---------------------

DB-All.e allows to set special query behaviours using the ``"query"``
parameter.  The available options are:

* ``best``: When measures from different kinds of reports exist in the same
  physical space, do not return them all, but only return the one of the record
  type with the highest priority.


Shortcuts to stations and data
------------------------------

DB-All.e offers two shortcuts to represent pseudoana entries and data in the
database: the ``ana_id`` and the ``data_id`` keys, that are set in the
output of every :c:func:`idba_next_data`.

``ana_id`` represents a pseudoana entry.  Every time one needs to specify a
set of latitude, longitude, fixed/mobile, one could use the corresponding
``ana_id`` value, if known, and get a faster search.

``data_id`` represents a data entry.  Every time one needs to identify some
data setting latitude, longitude, level layer, time range and so on, one can
just provide the value of ``data_id``, and also get a faster search.


Code examples
-------------

Query data, then query station data
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

::

    ierr = idba_begin(dbhandle, handle, "read", "read", "read")
    ierr = idba_begin(dbhandle, handleana, "read", "read", "read")
    
    ! Prepare a query
    ierr = idba_setd (handle, "latmin", 10)
    ...
    ierr = idba_setd (handle, "lonmax", 60)
    
    ! Make the query
    ierr = idba_query_data (handle, N)
    
    ! Iterate the results
    do i=1,N
      ierr = idba_next_data (handle, varname)
    
      ! Read data about the variable we just had
      ierr = idba_enqlevel (handle, ltype, l1, l2)
    
      ! Read pseudoana data about the variable we just had
      ! Setup a query for the station with 'query_stations'
      ierr = idba_enqi (handle, "ana_id", anaid)
      ierr = idba_seti (handleana, "ana_id", anaid)
    
      ! Query.  Nstaz should always be 1 because we query a specific station
      ierr = idba_query_stations (handleana, Nstaz)
    
      ! Fetch the data
      ierr = idba_next_station (handleana)
    
      ! Read the data about the station
      ! All the data inserted with set_station_context is available here
      ierr = idba_enqi (handleana, "height", height)
    enddo
