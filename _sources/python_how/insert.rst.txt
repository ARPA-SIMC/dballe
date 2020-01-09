.. _python_how_insert:

Insert data into a db
=====================

This is an example python code that inserts a station, some station data, and
some data into a database.

::

    with db.transaction() as tr:
        # Insert 2 new variables in the database.
        #
        # Setting can_add_stations to True automatically creates a new station
        # if needed.
        tr.insert_data({
            "report": "synop",
            "lat": 44.5, "lon": 11.4,
            "level": dballe.Level(1),
            "trange": dballe.Trange(254),
            "datetime": datetime.datetime(2013, 4, 25, 12, 0, 0),
            "B12101": 22.4,
            "B12103": 17.2,
        }, can_add_stations=True)

        # Insert data for this station
        tr.insert_station_data({
            "report": "synop",
            "lat": 44.5, "lon": 11.4,
            "B07030": 123.4,
            "B01019": "Test Station",
        })
