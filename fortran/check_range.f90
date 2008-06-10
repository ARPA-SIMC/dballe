program check_range

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      include "dballef.h" 
      
      integer :: handle,idbhandle,handle_err, errcode
      real :: rval
      !data var/ "B22070", "B22074", "B22001", "B22071", "B22042"/
      !integer debug
      !data debug/1/
      
      !call idba_error_set_callback(0,idba_default_error_handler,debug,handle_err)
  
!     Database login
      call idba_presentati(idbhandle, 'test', 'enrico', '')
      call ensure_no_error("presentati")

!     Open a session
      call idba_preparati(idbhandle,handle,"write","write","write")
      call ensure_no_error("preparati")

!     Check that NaN values are trapped
      rval = 0.
      call idba_set(handle, "B12003", log(rval))
      errcode = idba_error_code()
      print*,"CCACACA",errcode
      call ensure("did not fail", errcode == 6)

      call idba_fatto(handle)
      call ensure_no_error("fatto")

      call idba_arrivederci(idbhandle)
      call ensure_no_error("arrivederci")
  
      call exit (0)
  
end program check_range

include "check-utils.h"
