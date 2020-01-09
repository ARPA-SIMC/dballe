.. _python_how_convertvars:

Convert values from one B code to another
=========================================

This is an example python code that converts B12101 ("temperature/dry-bulb
temperature") values to B22049 ("sea-surface temperature"), performing unit
conversions if needed::

    # Select B12101 values and convert them to B22049
    with tr.query_data({"var": "B12101"}) as cur:
        self.assertEqual(cur.remaining, 26)
        for rec in cur:
            data = rec.data
            # This removes the existing value from the database
            rec.remove()
            # This converts units automatically
            data["B22049"] = data["B12101"]
            # Remove B12101 from data, to avoid reinserting it
            del data["B12101"]
            tr.insert_data(data)
