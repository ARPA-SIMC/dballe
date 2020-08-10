.. _python_how_attributes:

Working with attributes
=======================

This is a collection of recipes for working with attributes.


Reading attributes
------------------

One way of reading attributes is while iterating the results of a query::

    with db.transaction() as tr:
        with tr.query_data(...) as cursor:
            for rec in cursor:
                # Print the variable code and value
                print(f"{rec['var']}:", rec["variable"])
    
                # This returns a Dict[str, wreport.Var]
                attrs = rec.query_attrs()
    
                # Print all the attributes
                for name, value in attrs.items():
                    print(f"  {name}:", value)

Note that, since they are not always needed, attributes are not retrieved by
default in queries, and running ``query_attrs`` will make a new query to
retrieve them, potentially adding a database query for each resulting variable.

If you know you are going to also need the attributes, you can add
``query=attrs`` to the query, and attributes will be read together with
variable data, resulting in a more efficient query::

    with db.transaction() as tr:
        with tr.query_data({"query": "attrs"}) as cursor:
            for rec in cursor:
                # Print the variable code and value
                print(f"{rec['var']}:", rec["variable"])
    
                # This returns a Dict[str, wreport.Var]
                # and since query=attrs was used, it won't make an extra
                database query:
                attrs = rec.query_attrs()
    
                # Print all the attributes
                for name, value in attrs.items():
                    print(f"  {name}:", value)

If you are not processing attributes together as you iterate data, you can use
the ``context_id`` output parameter (see :ref:`parms_read_data`) to store
references to variables, that you can use to access their attributes at any
later time::

    interesting_vars = []
    with db.transaction() as tr:
        with tr.query_data(...) as cursor:
            for rec in cursor:
                if ...:
                    interesting_vars.append(rec["context_id")

        for context_id in interesting_vars:
            attrs = tr.attr_query_data(contex_id)
            if attrs["B33196"].enqi() == 1:
                ...

Although querying attributes separately is often not needed, this possibility
becomes useful when modifying attributes. See below.
        

Updating attributes
-------------------

You can insert or remove attributes while iterating the results of a query::

    with db.transaction() as tr:
        with tr.query_data({"var": "B12101"}) as cursor:
            for rec in cursor:
                # Check that the temperature is within a given range
                if rec.enqd() < 150 or req.enqd() > 310:
                    # If it's not, add/update "Data has been invalidated"...
                    rec.insert_attrs({"B33196": 1})
                    # ...and remove "Percent confidence"
                    rec.remove_attrs(["B33007"])

If you are not processing attributes together as you iterate data, you can use
the ``context_id`` output parameter (see :ref:`parms_read_data`) to store
references to variables, that you can use to work on their attributes at a
later time.

For example, this identifies sequences of consecutive data whose values never
change, and invalidates all of them except the last one (see `issue227`_)::

    invalid_list = []
    prev = None
    with db.transaction() as tr:
        for row in tr.query_data(....):
            value = row["variable"].get()
            if prev == value:
                invalid_list.append(row["context_id"])
    
            prev = value
    
        for ctxid in invalid_list:
            tr.attr_insert_data(ctxid, {"B33196": 1})
            tr.attr_remove_data(ctxid, ["B33007"])

.. _issue227: https://github.com/ARPA-SIMC/dballe/issues/227
