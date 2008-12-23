      program check_real0

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest

      include "dballef.h" 
      
      integer :: handle,idbhandle,handle_err
      !data var/ "B22070", "B22074", "B22001", "B22071", "B22042"/
      !integer debug
      !data debug/1/
      
      !call idba_error_set_callback(0,idba_default_error_handler,debug,handle_err)
  
!     Database login
      call dbinit(idbhandle)

!     Open a session
      call idba_preparati(idbhandle,handle,"write","write","write")
      call ensure_no_error("preparati")

!     Clear the database
      call idba_scopa(handle, "") 
      call ensure_no_error("scopa")

!     Insert some data with a 0.0D0 value
      call idba_unsetall (handle)

      call idba_set (handle,"lat",44.2)
      call ensure_no_error("set lat")
      call idba_set (handle,"lon",11.5)
      call ensure_no_error("set lon")
      call idba_set (handle,"mobile",0)
      call ensure_no_error("set mobile")
      call idba_set (handle,"rep_memo","synop")
      call ensure_no_error("set rep_memo")
      call idba_setlevel (handle,1,0,0,0)
      call ensure_no_error("set level")
      call idba_settimerange (handle,0,0,0)
      call ensure_no_error("set timerange")
      call idba_setdate (handle,2007,12,11,12,30,00)
      call ensure_no_error("set date")
     
      call idba_set(handle,"B22070",0.0D0)
      call ensure_no_error("set B22070 to 0.0D0")
  
      call idba_prendilo (handle)
      call ensure_no_error("prendilo")
     
      call idba_fatto(handle)
      call ensure_no_error("fatto")

      call idba_arrivederci(idbhandle)
      call ensure_no_error("arrivederci")
  
      call exit (0)
  
end program check_real0

include "check-utils.h"
