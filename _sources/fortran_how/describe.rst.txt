Helpers for pretty-printing
===========================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran

There are a number of functions in DB-All.e, the ``idba_describe_\*`` group of
functions, that are not needed for normal processing but can be useful to
improve the presentation of data to users.

All these function take a number of parameters and a string, and they store a
proper description of the values into the string.

The functions are:

* :c:func:`idba_describe_level`:
  Describes a level.  For example, ``idba_describe_level(handle,106,10,106,20,string)``
  will store in ``string`` something like: *"Layer between 10hm and
  20hm above ground"*.
* :c:func:`idba_describe_timerange`:
  Describes a time range.  For example, ``idba_describe_timerange(handle,3,0,600,string)``
  will store in ``string`` something like: *"Average between reference
  time+0s to reference time+600s"*.
* :c:func:`idba_describe_var(handle,varcode,value,string)`:
  Describe a value.  For example, ``idba_describe_var(handle,"B12001","280",string)``
  will store in ``string`` something like: *"280 (K) TEMPERATURE/DRY-BULB
  TEMPERATURE"*.
