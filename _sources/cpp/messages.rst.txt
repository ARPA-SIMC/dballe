Messages
========

DB-All.e can read weather bulletins via `wreport
<https://github.com/ARPA-SIMC/wreport/>`_ and scan their contents to given them
a physical interpretation.

Interpreted messages are represented by the :cpp:class:`dballe::Message`.
Binary messages are represented by the :cpp:class:`dballe::BinaryMessage`, and
are read via :cpp:class:`dballe::File`.

You can use :cpp:class:`dballe::Importer` to go from a
:cpp:class:`dballe::BinaryMessage` to a :cpp:class:`dballe::Message`.

You can use :cpp:class:`dballe::Exporter` to go from a
:cpp:class:`dballe::Message` to a :cpp:class:`dballe::BinaryMessage`.


.. doxygenclass:: dballe::Message
   :members:

.. doxygenclass:: dballe::BinaryMessage
   :members:

.. doxygenclass:: dballe::File
   :members:

.. doxygenclass:: dballe::ImporterOptions
   :members:

.. doxygenclass:: dballe::Importer
   :members:

.. doxygenclass:: dballe::ExporterOptions
   :members:

.. doxygenclass:: dballe::Exporter
   :members:

.. doxygenenum:: dballe::MessageType

.. doxygenfunction:: dballe::format_message_type
