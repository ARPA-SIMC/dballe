Cursors
=======

Classes that contain multiple data, like :cpp:class:`dballe::Message` and
:cpp:class:`dballe::DB`, can be iterated via a :cpp:class:`Cursor` subclass.

All cursors can be iterated in the same way, and each :cpp:class:`Cursor`
subclass provides access to the specific type of information being queried.


.. doxygenclass:: dballe::Cursor
   :members:

.. doxygenclass:: dballe::CursorStation
   :members:

.. doxygenclass:: dballe::CursorStationData
   :members:

.. doxygenclass:: dballe::CursorData
   :members:

.. doxygenclass:: dballe::CursorSummary
   :members:

.. doxygenclass:: dballe::CursorMessage
   :members:
