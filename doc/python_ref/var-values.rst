Measured variables
------------------

.. toctree::
    :maxdepth: 2

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
