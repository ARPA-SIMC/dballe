Fortran dballe functions
========================

DB-All.e is a fast, temporary, on-disk database where meteorological data can
be stored, searched, retrieved and updated.  To make computation easier, data
is stored as physical data, that is, as measures of a variable in a specific
point of space and time, rather than as a sequence of reports.

This is a quick introduction to DB-All.e.  It is intended as a quick read to
get up to speed, and as a quick reference for everyday use.

.. toctree::
   :maxdepth: 2

   concepts
   walkthrough
   connect
   transactions
   reference
   faq

..
  * [Varcode aliases](fapi_aliases.md), aliases that can be used in input
    routines instead of B table codes.
  * [Level types](fapi_ltypes.md), description of values that define vertical
    levels and layers.
  * [Time ranges](fapi_tranges.md), descriptions of values that define time
    ranges.
  * [Environment variable](dballe_env.md), environment variables that influence the
    behaviour of DB-All.e.
