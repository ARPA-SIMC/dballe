.. _concepts:

General concepts
================

.. toctree::
   :maxdepth: 2


.. _report:

Report
------

A homogeneous group of stations with the same kind of measures and management
(for example: ``synop``, ``metar``, a specific kind of forecast...).

The type of report implicitly defines a priority of the measured value over
other equivalent values.  This can be used to select a "best value" from a
specific set of coordinates, where for example, values measured by synoptic
stations are preferred over values measured by a satellite, which in turn is
preferred over a value computed by a forecast model (see :ref:`parms_query_modifiers`).

Priorities associated to the report type can be customized when creating the
database.

Report names are always lowercased before use.


.. _station:

Station
-------

Stations are uniquely identified by:

* *latitude*.
* *longitude*.
* *station identifier* (if the station is mobile).
* *report*


.. _station_values:

Station values
--------------

Each station entry can have zero or more values associated to it. They are just
like measured variables, but lack date, time, level, and time range
information. They can be used to store station attributes, like the height
above sea level, or the center operating it.


.. _level:

Level or layer
--------------

The vertical coordinate of a value in DB-All.e is described using a level
description or a layer description.

A level is defined univocally by a code table (``leveltype1``) and a
numerical value (``l1``).

A layer is defined univocally by its two bounding levels (``leveltype1``,
``l1``, ``leveltype2``, ``l2``). 

See :ref:`levels` for a description of the level types and
associated level values.


.. _timerange:

Time range
----------

A value in DB-All.e is also defined by the time period to which the datum
refers: it can be, for example, a value measured in a specific instant, or a
cumulated or averaged value over an interval of time.

The time range is defined by a table code (``pindicator``) and two
numerical values (``p1`` and ``p2``). For their meaning, see :ref:`tranges`.


.. _varcode:

Variable code (varcode)
-----------------------

A code from a local variation of BUFR/CREX Table B that describes the nature of
the variable, such as what it measures and the units used for measuring. It is
identified by a *local B table descriptor* (see :ref:`btable`).


Value
-----

A measured value is a datum that can be expressed as a floating point (single
or double precision), integer, character or byte, depending on what value it
measures.

Every datum is univocally defined by a system of coordinates with 7 dimensions:

1. A report type (see :ref:`report`).
2. The variable code (see :ref:`varcode`).
3. The *date and time* of validity of the datum, be it observed, forecast or
   estimated.
4. *station* information about what generated the value (also defining its x
   and y coordinates).
5. The vertical coordinates in space, as a description of the *level*.
6. The *time range* of the measurement.


.. _attributes:

Attributes
----------

Values in DB-All.e are also associated to zero or more *attributes*.  An
attribute is a value that describes an aspect of a variable, like for example a
confidence interval.

Attributes in DB-All.e consist on a value and a local B table descriptor,
which describes the nature of the variable, such as what it represents and the
units in which it is represented.

Attributes are uniquely identified by the variable they refer to and the B
table descriptor of the attribute.  As a consequence, you can only have one
attribute per value with a specific B table descriptor.  This means that, for
example, a variable can have at most one confidence interval attribute.
