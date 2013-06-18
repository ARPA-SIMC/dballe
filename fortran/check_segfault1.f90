program test


integer :: idbhandle1,handle1,handle1a,i
integer :: idbhandle2,handle2,handle2a,anaid
integer :: error_handle


include "dballef.h"

call idba_error_set_callback(0, idba_default_error_handler, 42, error_handle)

call idba_presentati(idbhandle1,"sqlite:tmp.sqlite","","")
call idba_preparati (idbhandle1,handle1,"write","write","write")
call idba_scopa (handle1, "")
call idba_fatto(handle1)
call idba_arrivederci(idbhandle1)

do i =1, 2000

  call idba_presentati(idbhandle1,"sqlite:tmp.sqlite","","")
!  print *,"Presentati",idbhandle1
  call idba_preparati (idbhandle1,handle1,"write","write","write")
!  print *,"Preparati",handle1
  call idba_preparati (idbhandle1,handle1a,"write","write","write")
!  print *,"Preparati_",handle1a

  call idba_presentati(idbhandle2,"sqlite:tmp.sqlite","","")
!  print *,"Presentati2",idbhandle2
  call idba_preparati (idbhandle2,handle2,"write","write","write")
!  print *,"Preparati2",handle2
  call idba_preparati (idbhandle2,handle2a,"write","write","write")
!  print *,"Preparati2",handle2a



!     Insert data about a station
  call idba_set (handle1, "rep_memo", "synop")
  call idba_set (handle1, "lat", 11.345)
  call idba_set (handle1, "lon", 44.678)
  call idba_set (handle1, "height", 23.0)
  call idba_setcontextana (handle1);
  call idba_prendilo (handle1)

!     Read the station ID for the station we just inserted
  call idba_enq (handle1, "*ana_id", anaid)

!     Reset the input data
  call idba_unsetall (handle1)

!     Add data to the station we just inserted
  call idba_set (handle1, "ana_id", anaid)
  call idba_set (handle1, "rep_memo", "synop")
  call idba_setlevel (handle1, 100, 1, 0, 0)
  call idba_settimerange (handle1, 0, 0, 0)
  call idba_setdate (handle1, 2006, 06, 20, 19, 30, 0)
  call idba_set (handle1, "B12101", 12.34)
  call idba_prendilo (handle1)
 
!  print *,"Fatto2",handle2
  call idba_fatto(handle2)
!  print *,"Fatto2_",handle2a
  call idba_fatto(handle2a)
!  print *,"Arrivederci2",idbhandle2
  call idba_arrivederci(idbhandle2)

!  print *,"Fatto_",handle1a
  call idba_fatto(handle1a)
!  print *,"Fatto",handle1
  call idba_fatto(handle1)
!  print *,"Arrivederci",idbhandle1
  call idba_arrivederci(idbhandle1)
end do

end program test
