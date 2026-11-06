Variable metadata
-----------------

.. toctree::
    :maxdepth: 2

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
