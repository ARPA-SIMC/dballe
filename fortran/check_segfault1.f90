program test


integer :: idbhandle1,handle1
integer :: i,anaid
integer :: error_handle


include "dballef.h"

call idba_error_set_callback(0, idba_default_error_handler, 42, error_handle)

call idba_presentati(idbhandle1,"sqlite:tmp.sqlite","","")
call idba_preparati (idbhandle1,handle1,"write","write","write")
call idba_scopa (handle1, "")
call idba_fatto(handle1)
call idba_arrivederci(idbhandle1)

do i = 1, 1030

  call idba_presentati(idbhandle1,"sqlite:tmp.sqlite","","")
!  print *,"Presentati",idbhandle1
  call idba_preparati (idbhandle1,handle1,"write","write","write")
!  print *,"Preparati",handle1

!     Insert data about a station
  call idba_set (handle1, "rep_memo", "synop")
  call idba_set (handle1, "lat", 11.345)
  call idba_set (handle1, "lon", 44.678)
  call idba_set (handle1, "height", 23.0)
  call idba_setcontextana (handle1);
  call idba_prendilo (handle1)

!  print *,"Fatto",handle1
  call idba_fatto(handle1)
!  print *,"Arrivederci",idbhandle1
  call idba_arrivederci(idbhandle1)
end do

end program test
