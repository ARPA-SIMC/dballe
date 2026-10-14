Deleting data
=============

.. toctree::
   :maxdepth: 2

.. highlight:: fortran


Clearing the database
---------------------

You can initialise or reinitialise the database using :ref:`idba_reinit_db`::

   ! Start the work with a clean database
   ierr = idba_reinit_db(handle, "repinfo.csv")

:ref:`idba_reinit_db` clears the database if it exists, then recreates all the
needed tables.  Finally, it populates the informations about the reports (such
as the available report types, their mnemonics and their priority) using the
data in the file given as argument.

The file is in CSV format, with 6 columns:

1. Report code (corresponding to parameter ``rep_cod``)
2. Mnemonic name (corresponding to parameter ``rep_memo``, forced to lowercase)
3. Report description
4. Report priority (corresponding to parameter ``priority``)
5. Ignored
6. Ignored

If ``""`` is given instead of the file name, :ref:`idba_reinit_db` will read the
data from ``/etc/repinfo.csv``.

.. highlight:: csv

This is an example of the contents of the file::

    01,synop,report synottico,100,oss,0
    02,metar,metar,80,oss,0
    03,temp,radiosondaggio,100,oss,2
    04,ana_lm,valori analizzati LM,-1,ana,255
    05,ana,analisi,-10,pre,255
    06,pre_cleps_box1.5maxel001,previsti cosmo leps box 1.5 gradi valore max elemento 1,-1,pre,255
    07,pre_lmn_box1.5med,previzione Lokal Model nudging box 1.5 gradi valore medio,-1,pre,255
    08,pre_lmp_spnp0,previsione Lkal Model prognostica interpolato punto piu' vicino,-1,pre,255
    09,boe,dati omdametrici,100,oss,31

.. highlight:: fortran

:ref:`idba_reinit_db` will not work unless ``rewrite`` has been enabled for the
data when opening the database.


Deleting data
-------------

Data is deleted using :ref:`idba_remove_data`::

    ! Delete all data from the station with id 4 in year 2002
    ierr = idba_seti(handle, "ana_id", 4)
    ierr = idba_seti(handle, "year", 2002)
    ierr = idba_remove_data(handle)

This code introduces a new function:

* :ref:`idba_remove_data`: deletes all the data found in the extremes specified in input.

:ref:`idba_remove_data` will not work unless ``rewrite`` has been enabled for
the data when opening the database.
