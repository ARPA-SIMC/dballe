.. _parms_filters:

ana_filter, data_filter, attr_filter
====================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

The expressions in ``ana_filter``, ``data_filter``, and ``attr_filter`` can
have one of these formats:

* ``Bxxyyy<value``
* ``Bxxyyy<=value``
* ``Bxxyyy>value``
* ``Bxxyyy>=value``
* ``Bxxyyy=value``
* ``Bxxyyy==value``
* ``Bxxyyy<>value``
* ``Bxxyyy!=value``
* ``value1<Bxxyyy<value2``
* ``value1<=Bxxyyy<=value2``
* ``value1>Bxxyyy>value2``
* ``value1>=Bxxyyy>=value2``

Examples for station values: ``height>=1000``, ``B02001=1``, ``1000<=height<=2000``.

Examples for data values: ``t<260``, ``B22021>2``, ``10<=B22021<=20``.

Examples for attributes: ``conf>70``, ``B33197=0``, ``25<=conf<=50``.

