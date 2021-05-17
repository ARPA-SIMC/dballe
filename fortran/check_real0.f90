      program check_real0

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest
      use dballef

      integer :: handle,idbhandle, ierr
      !data var/ "B22070", "B22074", "B22001", "B22071", "B22042"/
      !integer debug
      !data debug/1/
      
      !call idba_error_set_callback(0,idba_default_error_handler,debug,handle_err)
  
!     Database login
      call dbinit(idbhandle)

!     Open a session
      ierr = idba_preparati(idbhandle,handle,"write","write","write")
      call ensure_no_error("preparati")

!     Clear the database
      ierr = idba_scopa(handle, "") 
      call ensure_no_error("scopa")

!     Insert some data with a 0.0D0 value
      ierr = idba_unsetall (handle)

      ierr = idba_set (handle,"lat",44.2)
      call ensure_no_error("set lat")
      ierr = idba_set (handle,"lon",11.5)
      call ensure_no_error("set lon")
      ierr = idba_set (handle,"mobile",0)
      call ensure_no_error("set mobile")
      ierr = idba_set (handle,"rep_memo","synop")
      call ensure_no_error("set rep_memo")
      ierr = idba_setlevel (handle,1,0,0,0)
      call ensure_no_error("set level")
      ierr = idba_settimerange (handle,0,0,0)
      call ensure_no_error("set timerange")
      ierr = idba_setdate (handle,2007,12,11,12,30,00)
      call ensure_no_error("set date")
     
      ierr = idba_set(handle,"B22070",0.0D0)
      call ensure_no_error("set B22070 to 0.0D0")
  
      ierr = idba_prendilo (handle)
      call ensure_no_error("prendilo")
     
      ierr = idba_fatto(handle)
      call ensure_no_error("fatto")

      ierr = idba_arrivederci(idbhandle)
      call ensure_no_error("arrivederci")
  
      call exit (0)
  
end program check_real0

include "check-utils.h"
