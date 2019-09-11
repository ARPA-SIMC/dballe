Python dballe.volnd package
===========================

.. toctree::
    :maxdepth: 2


The volnd API
-------------

.. automodule:: dballe.volnd
    :members:
    :undoc-members:


Dimensions
~~~~~~~~~~

This is the list of dimensions supported by dballe.volnd:

.. autoclass:: dballe.volnd.AnaIndex
    :members:
    :undoc-members:

The data object used by :class:`AnaIndex` is:

.. autoclass:: dballe.volnd.AnaIndexEntry
    :members:
    :undoc-members:


.. autoclass:: dballe.volnd.NetworkIndex
    :members:
    :undoc-members:

.. autoclass:: dballe.volnd.LevelIndex
    :members:
    :undoc-members:

.. autoclass:: dballe.volnd.TimeRangeIndex
    :members:
    :undoc-members:

.. autoclass:: dballe.volnd.DateTimeIndex
    :members:
    :undoc-members:

.. autoclass:: dballe.volnd.IntervalIndex
    :members:
    :undoc-members:


Extracting data
~~~~~~~~~~~~~~~

The extraction is done using the :func:`dballe.volnd.read` function:


.. autofunction:: dballe.volnd.read

The result of :func:`dballe.volnd.read` is a dict mapping output variable names
to a :class:`dballe.volnd.Data` object with the results.  All the Data objects
share their indexes unless the *xxx*-Index definitions have been created with
``shared=False``.

This is the dballe.volnd.Data class documentation:

.. autoclass:: dballe.volnd.Data
    :members:
    :undoc-members:
