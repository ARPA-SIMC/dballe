Database
========

:cpp:class:`dballe::DB` is the class implementing access to a DB-All.e
database. Operations are performed via a :cpp:class:`dballe::Transaction`,
although :cpp:class:`dballe::DB` provides shortcut methods that transparently
create a transaction, perform the operation and commit it.

Data is queried using a :cpp:class:`dballe::Query`, and inserted using a
:cpp:class:`dballe::Data`.


.. doxygenclass:: dballe::DB
   :members:

.. doxygenclass:: dballe::Transaction
   :members:

.. doxygenclass:: dballe::Query
   :members:

.. doxygenclass:: dballe::Data
   :members:

.. doxygenclass:: dballe::DBConnectOptions
   :members:

.. doxygenclass:: dballe::DBImportOptions
   :members:

.. doxygenclass:: dballe::DBInsertOptions
   :members:
