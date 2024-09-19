Working with attributes
=======================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran


Reading attributes
------------------

Attributes are read using :ref:`idba_next_attribute`::

    ! ...setup a query...
    idba_query_data(handle, count)
    do while (count.gt.0)
      ierr = idba_next_data(handle, param)

      ! Read QC informations about the last value read
      ierr = idba_query_attributes(handle, qc_count)
      do while (qc_count.gt.0)
          ierr = idba_next_attribute(handle, param) 
          ierr = idba_enqc(handle, param, value)
          ! ...process the value...
          qc_count = qc_count - 1
      enddo

      count = count - 1
    enddo

This code introduces two new functions:

* :ref:`idba_query_attributes`:
  Performs a query to retrieve attributes for the last variable read by
  :ref:`idba_next_data`.  It returns the number of attributes available.
* :ref:`idba_next_attribute`:
  Retrieves one by one the values queried by :ref:`idba_query_attributes` if
  there are no more items available, the function will fail.

  The parameter ``param`` will be set to the name (in the form ``*Bxxyyy``) of
  the attribute just read.

It is possible to read attributes at a later time giving a context ID and a B
table value::

    ! Read the context ID after a insert_data or a next_data
    idba_enqi(handle, "context_id", id)

    ! ...a while later...

    ! Query the attributes of the variable with the given
    ! context ID and B table value
    idba_seti(handle, "*context_id", id)
    idba_seti(handle, "*var_related", "B12001")

    ! These are ways one could choose specific attributes:
    ! one attribute: idba_setc(handle, "*var", "B33007")
    ! some attributes: idba_setc(handle, "*varlist", "B33007,B33036")
    ! by default, all attributes are returned

    ! Read QC informations about the last value read
    ierr = idba_query_attributes(handle, qc_count)
    do while (qc_count.gt.0)
        ierr = idba_next_attribute(handle, param) 
        ierr = idba_enqc(handle, param, value)
        ! ...process the value...
        qc_count = qc_count - 1
    enddo


Writing attributes
------------------

Attributes are written using :ref:`idba_insert_attributes`, which can be used after an
:ref:`idba_next_data`, after an :ref:`idba_insert_data` or at any time using a stored data
id.  These three case differ on how to communicate to :ref:`idba_insert_attributes` what is
the data about which to write attributes.

When used after :ref:`idba_next_data`, :ref:`idba_insert_attributes` can refer directly to the
last data retrieved::

    ! ...setup a query...
    ierr = idba_query_data(handle, count)
    do while (count.gt.0)
      ierr = idba_next_data(handle, param)
      ! ...process data...

      ! Set the attributes
      ierr = idba_seti(handle, "*B33007", 75)
      ierr = idba_seti(handle, "*B33006", 42)
      ierr = idba_insert_attributes(handle)

      count = count - 1
    enddo

After an :ref:`idba_insert_data` instead, since :ref:`idba_insert_data` can write more than
one data at a time, we need to tell :ref:`idba_insert_attributes` which of them we are
referring to::

    ! Insert wind speed and temperature
    ierr = idba_setr(handle, "B11002", 1.8)
    ierr = idba_setr(handle, "B12001", 22)
    ierr = idba_insert_data(handle)

    ! Set the attributes
    ierr = idba_seti(handle, "*B33007", 75)

    ! Use "*var_related" to indicate which of the two variables we are annotating
    ierr = idba_setc(handle, "*var_related", "B11002")

    ierr = idba_insert_attributes(handle)

:ref:`idba_insert_attributes` can also be called at any time using a previously stored data it::

    ! ...perform a query with idba_query_data...
    do while (count.gt.0)
      ierr = idba_next_data(handle, param)
      ! ...process data...

      ! This variable is interesting: save the context ID
      ! to refer to it later
      ierr = idba_enqi(handle, "context_id", saved_id)

      count = count - 1
    enddo

    ! ...some time later...

    ! Insert attributes about that interesting variable
    ierr = idba_seti(handle, "*B33007", 75)
    ierr = idba_seti(handle, "*B33006", 42)

    ! Select the variable using its context id
    ! and variable code
    ierr = idba_seti(handle, "*context_id", saved_id)
    ierr = idba_seti(handle, "*var_related", "B11001")
    ierr = idba_insert_attributes(handle)

This code introduces a new function:

* :ref:`idba_insert_attributes`
  Set one or more attributes about a variable.
  
  The variable can be identified directly by using ``idba_seti(handle, "*context_id", id)`` and ``idba_seti(handle, "*var_related", name)``.
  These parameters are automatically set by the :ref:`idba_next_data` and
  :ref:`idba_insert_data` action routines.

  The attributes and values are set as input to :ref:`idba_insert_attributes` using the
  `idba_set*` functions with an asterisk in front of the variable name.

:ref:`idba_insert_attributes` will work in different ways according to the attributes
opening mode of the database:

* ``"read"``: attributes cannot be modified in any way.
* ``"rewrite"``: attributes can be added, and existing attributes can be
  overwritten.


Deleting attributes
-------------------

Attributes are deleted using :ref:`idba_remove_attributes`::

    ! Delete the confidence interval from the wind speed

    ! The referring variable is identified in the same way as with
    ! idba_insert_attributes:
    ierr = idba_seti(handle, "*context_id", saved_id)
    ierr = idba_seti(handle, "*var_related", "B11002")

    ! The attributes to delete are selected by setting "*varlist":
    ierr = idba_setc(handle, "*varlist", "*B33007")
    ierr = idba_remove_attributes(handle)

This code introduces a new function:

* :ref:`idba_remove_attributes`:
  Delete attributes from a variable identified in the same way as with

:ref:`idba_remove_attributes` will not work unless the database has been opened in
attribute `rewrite` mode.
