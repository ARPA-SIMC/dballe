.. _python_how_dumpdb:

Dump the contents of the database
=================================

This example python code dumps the content of a database::

    # Connect to the database
    db = dballe.DB.connect_from_file("db.sqlite")

    # Start a transaction
    with db.transaction() as tr:
        for srow in tr.query_stations():
            print(f"* Station: {srow['station']}")

            print("  Station values:")
            for row in tr.query_station_data({"ana_id": srow["ana_id"]}):
                var = row["variable"]
                print(f"    {var.code} {var.info.desc}: {var.format('undefined')}")

            print("  Measured values:")
            cur_datetime = None
            cur_level = None
            cur_trange = None
            for row in tr.query_data({"ana_id": srow["ana_id"]}):
                if cur_datetime != row["datetime"]:
                    print(f"    {row['datetime']}:")
                    cur_datetime = row["datetime"]
                if cur_level != row["level"]:
                    print(f"      At level: {row['level'].describe()}")
                    cur_level = row["level"]
                    cur_trange = None
                if cur_trange != row["trange"]:
                    print(f"      At time range: {row['trange'].describe()}")
                    cur_trange = row["trange"]
                var = row["variable"]
                print(f"        {var.code} {var.info.desc}: {var.format('undefined')}")
