program test


integer :: idbhandle,handle,handle_,i
integer :: idbhandle2,handle2,handle2_,anaid


include "dballef.h"



do i =1, 2000

  call idba_presentati(idbhandle,"sqlite:tmp.sqlite","","")
!  print *,"Presentati",idbhandle
  call idba_preparati (idbhandle,handle,"write","write","write")
!  print *,"Preparati",handle
  call idba_preparati (idbhandle,handle_,"write","write","write")
!  print *,"Preparati_",handle_

  call idba_presentati(idbhandle2,"sqlite:tmp.sqlite","","")
!  print *,"Presentati2",idbhandle2
  call idba_preparati (idbhandle2,handle2,"write","write","write")
!  print *,"Preparati2",handle2
  call idba_preparati (idbhandle2,handle2_,"write","write","write")
!  print *,"Preparati2",handle2_



!     Insert data about a station
  call idba_set (handle, "lat", 11.345)
  call idba_set (handle, "lon", 44.678)
  call idba_set (handle, "height", 23)
  call idba_prendilo (handle)

!     Read the station ID for the station we just inserted
  call idba_enq (handle, "ana_id", anaid)

!     Reset the input data
  call idba_unsetall (handle)

!     Add data to the station we just inserted
  call idba_set (handle, "ana_id", anaid)
  call idba_setlevel (handle, 100, 1, 0, 0)
  call idba_settimerange (handle, 0, 0, 0)
  call idba_setdate (handle, 2006, 06, 20, 19, 30, 0)
  call idba_seti (handle, "t", 21)
  call idba_setc (handle, "B12345", "ciao")
  call idba_prendilo (handle)
 
!  print *,"Fatto2",handle2
  call idba_fatto(handle2)
!  print *,"Fatto2_",handle2_
  call idba_fatto(handle2_)
!  print *,"Arrivederci2",idbhandle2
  call idba_arrivederci(idbhandle2)

!  print *,"Fatto_",handle_
  call idba_fatto(handle_)
!  print *,"Fatto",handle
  call idba_fatto(handle)
!  print *,"Arrivederci",idbhandle
  call idba_arrivederci(idbhandle)
end do

end program test
