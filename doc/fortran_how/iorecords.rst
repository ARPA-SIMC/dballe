Input/output records
====================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran


Setting input and reading output
--------------------------------

Input to action routines is done using the functions ``idba_set*``, and output
is read with the functions ``idba_enq*`` (see :ref:`routines`)::

    ! Set the extremes of an area and retrieve all the stations in it
    ierr = idba_setd(handle, "latmin", 30.D0)
    ierr = idba_setd(handle, "latmax", 50.D0)
    ierr = idba_setd(handle, "lonmin", 10.D0)
    ierr = idba_setd(handle, "lonmax", 20.D0)
    ierr = idba_query_stations(handle, count)

    ! Get the informations about a station
    do while (count.gt.0)
      ierr = idba_next_station(handle)
      ierr = idba_enqc(handle, "name", cname)
      ierr = idba_enqi(handle, "ana_id", id)
      ierr = idba_enqd(handle, "lat", lat)
      ierr = idba_enqd(handle, "lon", lon)
      ! ....
      count = count - 1
    enddo

Note that, when one uses :ref:`idba_setc`, :ref:`idba_seti`, :ref:`idba_enqc`,
:ref:`idba_enqi` with parameters that have some decimal digits, DB-All.e will
work with values as if they did not have a decimal point.  That is, if latitude
`10.124323` is read with :ref:`idba_enqi`, then the result will be `10124323`.

The following example shows what happens::

    ! Set the latitude to 30.0 degrees
    ierr = idba_setr(handle, "lat", 30.0)
    ! Set the latitude to 30.0 degrees
    ierr = idba_setd(handle, "lat", 30.0D0)

    ! Set the latitude to 0.00030 degrees
    ierr = idba_seti(handle, "lat", 30)
    ! Set !the latitude to 30.0 degrees
    ierr = idba_seti(handle, "lat", 3000000)

    ! Set the latitude to 0.00030 degrees
    ierr = idba_setc(handle, "lat", "30")
    ! Set the latitude to 30.0 degrees
    ierr = idba_setc(handle, "lat", "3000000")


Input/output shortcuts
----------------------

There are a few functions that are shortcuts to other input and output
functions:

:ref:`idba_enqdate` is a shortcut to::

    idba_enqi(handle, "year", year)
    idba_enqi(handle, "month", month)
    idba_enqi(handle, "day", day)
    idba_enqi(handle, "hour", hour)
    idba_enqi(handle, "min", minute)
    idba_enqi(handle, "sec", second)

:ref:`idba_setdate` is a shortcut to::

    idba_seti(handle, "year", year)
    idba_seti(handle, "month", month)
    idba_seti(handle, "day", day)
    idba_seti(handle, "hour", hour)
    idba_seti(handle, "min", minute)
    idba_seti(handle, "sec", second)

:ref:`idba_enqlevel` is a shortcut to::

    idba_enqi(handle, "leveltype1", type1)
    idba_enqi(handle, "l1", l1)
    idba_enqi(handle, "leveltype2", type2)
    idba_enqi(handle, "l2", l2)


:ref:`idba_setlevel` is a shortcut to::

    idba_seti(handle, "leveltype1", type1)
    idba_seti(handle, "l1", l1)
    idba_seti(handle, "leveltype2", type2)
    idba_seti(handle, "l2", l2)

:ref:`idba_enqtimerange` is a shortcut to::

    idba_enqi(handle, "pindicator", type)
    idba_enqi(handle, "p1", p1)
    idba_enqi(handle, "p2", p2)

:ref:`idba_settimerange` is a shortcut to::

    idba_seti(handle, "pindicator", type)
    idba_seti(handle, "p1", p1)
    idba_seti(handle, "p2", p2)


Parameter names
---------------

There are three different kinds of parameter names one can use:

* DB-All.e parameters (see :ref:`parms`), that have a special meaning to
  DB-All.e: for example they can be part of the coordinate system, or
  space/time extremes to use to query the database.  They are indicated simply
  with their name (for example, `"lat"` or `"yearmin"`).
* :ref:`btable`, represent all possible sorts of
  observed data, and are indicated in the form `Bxxyyy`, where `xxyyy` are the
  X and Y values from the WMO table B.
* :ref:`aliases` that are short, easy to remember names which
  can be used instead of frequently used WMO B variables.


Queries and observed data
-------------------------

The ``idba_set*`` and ``idba_enq*`` functions can also be used to set and get
observation data.  To do so, use as parameter the string ``"Bxxyyy"``, where
``xx`` and ``yyy`` are the X and Y values of the BUFR/CREX table B describing
the observed data.

For example::

    ! Set the speed of the wind (very useful in summer)
    ierr = idba_setr(handle, "B11002", 1.8)
    ! Also set the temperature
    ierr = idba_setr(handle, "B12001", 21.8)
    ierr = idba_insert_data(handle)


Attributes
----------

The ``idba_set*`` and ``idba_enq`` groups of functions can also be used to
set and get attributes on data.  To do so, use as parameter the string
``"*Bxxyyy"``, where ``xx`` and ``yyy`` are the X and Y values of the
BUFR/CREX table B describing the attribute.

For example::

    ! Set the confidence of the wind speed value we inserted
    ! in the last 'idba_insert_data'
    ierr = idba_setr(handle, "*B33007", 75.0)
    ierr = idba_setc(handle, "*var_related", "B11002")
    ierr = idba_insert_attributes(handle)


