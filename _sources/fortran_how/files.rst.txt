.. _working_with_files:

Working with files
==================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran

This is a list of the differences between working with files and working with
databases:

* You do not need to call :c:func:`idba_connect` and :c:func:`idba_disconnect`: the work
  session starts at :c:func:`idba_begin_messages` and ends at :c:func:`idba_commit`
* When reading, performing :c:func:`idba_query_stations` or :c:func:`idba_query_data` a second
  time advances to the next message in the file.
* Query parameters set before an :c:func:`idba_query_data` have no effect: filtering
  data is not implemented for files. Since it may be implemented in the future,
  it is suggested to avoid setting query parameters before an
  :c:func:`idba_query_data` to avoid unexpected changes of behaviour with future
  versions of DB-All.e.
* When reading, you will see that there are no more messages because
  :c:func:`idba_query_stations` or :c:func:`idba_query_data` will return 0.
* When writing, you can use the `query` input parameter to :c:func:`idba_insert_data` to
  control when a new message is started.  If you set it to `subset`, then the
  data will be inserted in a new BUFR or CREX subset.  If you set it to
  `message`, you will start a new message.

  After `"message"` you can specify a template for the message, using one of
  the names listed by `dbadb export -t list`, for example: `"message generic"`.
  If you do not specify a template name, an appropriate template will
  automatically be chosen for you.
* Setting `rep_memo` you can influence the output template of messages: if you
  set it to a synop report code, you will create a synop message.
