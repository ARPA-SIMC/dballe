Input parameters for attribute-related action routines
======================================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:


.. _parms_attrs:

.. list-table::
   :header-rows: 1

   * - Name
     - Unit
     - Format
     - Description
     - On insert
     - On query
     - On results
     - Comment
   * - ``*Bxxyyy``
     - depends
     - depends
     - Value of the attribute
     - required
     - ignored
     - present
     -
   * - ``*var``
     - Character
     - 7 chars
     - Attribute queried
     - ignored
     - optional
     - present
     - indicates the name of the last attribute returned
   * - ``*varlist``
     - Character
     - 255 chars
     - List of attributes to query
     - ignored
     - optional
     - absent
     - Comma-separated list of attribute B codes wanted on output
   * - ``*var_related``
     - Character
     - 6 chars
     - Variable related to the attribute to query
     - required
     - required
     - absent
     - It is automatically set by ``idba_next_data`` and ``idba_insert_data`` (when ``idba_insert_data`` inserts a single variable)
   * - ``*context_id``
     - Numeric
     - 10 digits
     - Context ID of the variable related to the attribute to query
     - required
     - required
     - absent
     - It is automatically set by ``idba_next_data`` and ``idba_insert_data``
