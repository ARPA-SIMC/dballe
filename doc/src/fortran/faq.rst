FAQ and Troubleshooting
=======================

.. highlight:: fortran

How do I access the station values?
-----------------------------------

There are two ways:

If you know in advances what variables you want to read, you can find them
among the results of :c:func:`idba_next_station`::

    ! Query station data
    ierr = idba_query_stations(handle, count)

    ! Get the informations about a station
    do i=1,count
      ierr = idba_next_station(handle)
      ierr = idba_enqc(handle, "name", cname)
      ierr = idba_enqi(handle, "B02001", type)
      ! ....
    enddo

If you want to see all the extra station data available, you can make an
explicit query for the extra station data using :c:func:`idba_query_data` and
:c:func:`idba_next_data`::

    ierr = idba_seti("ana_id", id)
    ierr = idba_query_data(handle, count)
    do i=1,count
      ierr = idba_next_data(handle, param)
      ! get the value of this variable
      ierr = idba_enqc(handle, param, cvalue)
      print*,param,": ",cvalue
    enddo
