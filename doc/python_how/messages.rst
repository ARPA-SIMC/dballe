.. _python_how_messages:

Working with messages
=====================

These are exmples of Python code that reads messages and adds their content to
a database.

::

    # Fully expanded iteration
    importer = dballe.Importer("BUFR")
    with db.transaction() as tr:
        with dballe.File(pathname) as f:
            for binmsg in f:
                for msg in importer.from_binary(binmsg):
                    tr.import_messages(msg)

    # All subsets of a BUFR at the same time
    importer = dballe.Importer("BUFR")
    with db.transaction() as tr:
        with dballe.File(pathname) as f:
            for binmsg in f:
                tr.import_messages(importer.from_binary(binmsg))

    # Iterate all decoded messages with dballe.Importer.from_file
    importer = dballe.Importer("BUFR")
    with db.transaction() as tr:
        with dballe.File(pathname) as f:
            self.db.import_messages(importer.from_file(fp))

    # Transaction.import_messages() supports importing only some variables
    importer = dballe.Importer("BUFR")
    with db.transaction() as tr:
       with dballe.File(pathname) as f:
           tr.import_messages(importer.from_file(f), varlist="B11001,B11002")

This is an example Python code that queries a database and exports the results
as BUFR messages.

::

    exporter = dballe.Exporter("BUFR")
    with open("test.bufr", "wb") as out:
        with db.transaction() as tr:
            for cur in tr.query_messages({...}):
                binmsg = exporter.to_binary(cur.message)
                out.write(binmsg)
