.. _format_json:

JSON import/export format
=========================

.. toctree::
   :maxdepth: 2

DB-All.e can import and export streams of messages encoded in JSON format.
Since such streams of messages could be potentially very long, and benefit from
being processed as they are streamed, streams are composed of
`one self-contained JSON per line`__.

__ https://jsonlines.org/

Each JSON line contains an *Object* with these keywords:

* ``version`` (String): version of the JSON structure used for messages. For now
  this is always ``"0.1"``.
* ``network`` (String): report name (see :ref:`report`)
* ``ident`` (String or null): station identifier (see :ref:`station`) for mobile
  stations, or ``null`` for fixed stations
* ``lon`` (Number): station longitude
* ``lat`` (Number): station latitude
* ``date`` (String): date and time, always in UTC, in the format
  ``YYYY-MM-DDTHH:MM:SSZ``. Because of some flexibility in the date parser, a
  space can be accepted instead of ``T``, and the trailing ``Z`` can be
  omitted. When generating JSON for DB-All.e, please always use the format
  ``YYYY-MM-DDTHH:MM:SSZ``.
* ``data`` (Array of Objects): list of variables in the message.
  
Each data variable is an *Object* with these keywords:

* ``level`` (Array of 4 x (Number or null)): the 4 values indicating the level
  or layer for a group of variables. See :ref:`level`. When parsing, an array
  shorter than 4 elements is considered ``null``-padded. ``level`` is missing
  for station values (see :ref:`station_values`).
* ``timerange`` (Array of 3 x (Number or null)): the 3 values indicating the time range 
  for a group of variables. See :ref:`timerange`. When parsing, an array
  shorter than 3 elements is considered ``null``-padded. ``timerange`` is missing
  for station values (see :ref:`station_values`).
* ``vars`` (Object): station variables, or variables at this level/layer and
  time range.

The ``vars`` Object contains an entry per variable, whose key is the ``Bxxyyy``
variable code (see :ref:`varcode`). Each entry is in turn an *Object* with
these keywords:

* ``v`` (String or Number or null): variable value. For values with decimal
  scaling (like ``B12101``), the number is written properly scaled and with the
  correct number of decimal digits. Trailing zeroes after the decimal dot are
  not written, and missing trailing decimal digits are assumed to be zero.
* ``a``: (Object): attributes for the variable (see :ref:`attributes`). If the
  variable has no attributes, the ``a`` key can be omitted. The attributes
  object is a mapping of ``Bxxyyy`` variable codes to variable values as
  Strings, Numbers, or nulls, as with the ``v`` values.


Example JSON message
--------------------

This is the BUFR file ``extra/bufr/gts-synop-linate.bufr`` exported to JSON and
indented for readability::

  {
    "version": "0.1",
    "network": "synop",
    "ident": null,
    "lon": 927833,
    "lat": 4544944,
    "date": "2015-05-27T06:00:00Z",
    "data": [
      {
        "vars": {
          "B01001": { "v": 16 },
          "B01002": { "v": 80 },
          "B01019": { "v": "MILANO/LINATE" },
          "B02001": { "v": 1 },
          "B02002": { "v": 12 },
          "B04001": { "v": 2015 },
          "B04002": { "v": 5 },
          "B04003": { "v": 27 },
          "B04004": { "v": 6 },
          "B04005": { "v": 0 },
          "B05001": { "v": 45.44944 },
          "B06001": { "v": 9.27833 },
          "B07030": { "v": 108 },
          "B07031": { "v": 103 }
        }
      },
      {
        "timerange": [ 1, 0, 43200 ],
        "level": [ 1, null, null, null ],
        "vars": {
          "B13011": { "v": 4, "a": { "B07032": 2 } }
        }
      },
      {
        "timerange": [ 1, 0, 86400 ],
        "level": [ 1, null, null, null ],
        "vars": {
          "B13011": { "v": 4.4, "a": { "B07032": 2 } }
        }
      },
      {
        "timerange": [ 3, 0, 43200 ],
        "level": [ 1, null, null, null ],
        "vars": {
          "B12101": { "v": 284.75, "a": { "B07032": 2 } }
        }
      },
      {
        "timerange": [ 4, 0, 10800 ],
        "level": [ 1, null, null, null ],
        "vars": {
          "B10060": { "v": 150, "a": { "B07031": 103 } }
        }
      },
      {
        "timerange": [ 205, 0, 10800 ],
        "level": [ 1, null, null, null ],
        "vars": {
          "B10063": { "v": 3, "a": { "B07031": 103 } }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 1, null, null, null ],
        "vars": {
          "B10004": { "v": 100310, "a": { "B07031": 103 } },
          "B20001": { "v": 20000, "a": { "B07032": 2 } },
          "B20003": { "v": 508 },
          "B20062": { "v": 1 }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 101, null, null, null ],
        "vars": {
          "B10051": { "v": 101530, "a": { "B07031": 103 } }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 103, 2000, null, null ],
        "vars": {
          "B12101": { "v": 289.85 },
          "B12103": { "v": 286.45 }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 103, 10000, null, null ],
        "vars": {
          "B11001": { "v": 220 },
          "B11002": { "v": 1 }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 256, null, 258, 0 ],
        "vars": {
          "B08002": { "v": 7 },
          "B20011": { "v": 1 },
          "B20013": { "v": 1750 }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 256, null, 258, 1 ],
        "vars": {
          "B20012": { "v": 34 }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 256, null, 258, 2 ],
        "vars": {
          "B20012": { "v": 20 }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 256, null, 258, 3 ],
        "vars": {
          "B20012": { "v": 10 }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 256, null, 259, 1 ],
        "vars": {
          "B08002": { "v": 1 },
          "B20011": { "v": 1 },
          "B20012": { "v": 6 },
          "B20013": { "v": 1500 }
        }
      },
      {
        "timerange": [ 254, 0, 0 ],
        "level": [ 256, null, null, null ],
        "vars": {
          "B20010": { "v": 12 }
        }
      }
    ]
  }
