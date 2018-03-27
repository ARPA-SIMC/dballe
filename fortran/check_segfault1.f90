program test

use dballef

integer :: idbhandle1,handle1
integer :: i,anaid
integer :: error_handle

ierr = idba_error_set_callback(0, C_FUNLOC(idba_default_error_handler), 42, error_handle)

ierr = idba_presentati(idbhandle1,"sqlite:tmp.sqlite?wipe=1")
ierr = idba_preparati (idbhandle1,handle1,"write","write","write")
ierr = idba_scopa (handle1, "")
ierr = idba_fatto(handle1)
ierr = idba_arrivederci(idbhandle1)

do i = 1, 1030

  ierr = idba_presentati(idbhandle1,"sqlite:tmp.sqlite")
!  print *,"Presentati",idbhandle1
  ierr = idba_preparati (idbhandle1,handle1,"write","write","write")
!  print *,"Preparati",handle1

!     Insert data about a station
  ierr = idba_set (handle1, "rep_memo", "synop")
  ierr = idba_set (handle1, "lat", 11.345)
  ierr = idba_set (handle1, "lon", 44.678)
  ierr = idba_set (handle1, "height", 23.0)
  ierr = idba_setcontextana (handle1);
  ierr = idba_prendilo (handle1)

!  print *,"Fatto",handle1
  ierr = idba_fatto(handle1)
!  print *,"Arrivederci",idbhandle1
  ierr = idba_arrivederci(idbhandle1)
end do

end program test
