.. _python_how_explorer:

Load/update/save an Explorer
============================

This is an example python code that loads the contents of a
:class:`dballe.Explorer` from a JSON file, adds information from various `BUFR`
files, and saves is back to JSON::

   #!/usr/bin/python3
   import dballe
   import os
   import sys

   with dballe.Explorer("data.json") as explorer:
      with explorer.update() as updater:
          # Import files listed on command line
          importer = dballe.Importer("BUFR")
          for fname in sys.argv[1:]:
              print(f"Load {fname}…")
              with importer.from_file(fname) as f:
                  updater.add_messages(f)

Remove the ``.json`` extension to use an indexed on-disk database, if Xapian
support is available.


Work with a subset of an Explorer
=================================

This an example that creates a new :class:`dballe.Explorer` with a selection of
the data of an existing one::

   with dballe.Explorer() as e:
      # …fill e…
      e.set_filter(...)

      e1 = dballe.Explorer()
      with e1.update() as updater:
          updater.add_explorer(e)
