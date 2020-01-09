dballe types
============

.. toctree::
    :maxdepth: 2


Measured variables
------------------

A measured variable is represented by a :class:`dballe.Var` object, which
contains a value (int, float, or str) annotated with information in a
:class:`dballe.Varinfo` object.

:class:`dballe.Varinfo` objects are catalogued in :class:`dballe.Vartable`
tables, which associate WMO B variable codes (like ``B12101``) to name,
descriptions, measurement units, significant digits, and so on.

:class:`dballe.Var`, :class:`dballe.Varinfo`, and :class:`dballe.Vartable` come
via `wreport`__, and `dballe.txt` contents (see :ref:`btable`) can be accessed
with the shortcut functions :func:`dballe.varinfo` and :func:`dballe.var`.

__ https://github.com/ARPA-SIMC/wreport


.. autoclass:: dballe.Var
    :members:
    :undoc-members:

.. autoclass:: dballe.Varinfo
    :members:
    :undoc-members:

.. autoclass:: dballe.Vartable
    :members:
    :undoc-members:

.. autofunction:: dballe.varinfo

.. autofunction:: dballe.var


Variable metadata
-----------------

:class:`dballe.Level`, :class:`dballe.Trange`, :class:`dballe.Station`, and
:class:`dballe.DBStation`, together with python's ``datetime.datetime`` class,
can be used to describe the DB-All.e metadata for measured variables, as
shortcuts for groups of closely related values.

The difference between :class:`dballe.Station` and :class:`dballe.DBStation` is
that the ``DB`` version also contains the database ID as ``ana_id``.

DB-All.e also provides :func:`dballe.describe_level` and :func:`dballe.describe_trange`
to turn level and timerange information into a string describing them.

.. autoclass:: dballe.Level
    :members:
    :undoc-members:

.. autoclass:: dballe.Trange
    :members:
    :undoc-members:

.. autoclass:: dballe.Station
    :members:
    :undoc-members:

.. autoclass:: dballe.DBStation
    :members:
    :undoc-members:

.. autofunction:: dballe.describe_level

.. autofunction:: dballe.describe_trange


Variables and their metadata
----------------------------

:class:`dballe.Data` finally provides a way to group together one or more
variables and their metadata.

.. autoclass:: dballe.Data
    :members:
    :undoc-members:
