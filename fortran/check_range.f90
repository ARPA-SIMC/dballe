program check_range

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest
      use dballef
      
      integer :: handle,idbhandle,handle_err, errcode
      real :: rval
      !data var/ "B22070", "B22074", "B22001", "B22071", "B22042"/
      integer debug
      data debug/1/
      
      !call idba_error_set_callback(0,idba_default_error_handler,debug,handle_err)
  
!     Database login
      call dbinit(idbhandle)

!     Open a session
      ierr = idba_preparati(idbhandle,handle,"write","write","write")
      call ensure_no_error("preparati")

!     Check that NaN values are trapped
      rval = 0.
      rval = log(rval) / log(rval)
      ierr = idba_set(handle, "B12103", log(rval))
      errcode = idba_error_code()
      call ensure("set to NaN", errcode == 13)

!     Check that negative reals can be read
      ierr = idba_set(handle, "lon", -12.3456)
      call ensure_no_error("set lon negative")
      ierr = idba_test_input_to_output(handle)
      call ensure_no_error("input to output")
      ierr = idba_enqr(handle, "lon", rval)
      call ensure_no_error("enq lon negative")
      call ensure("read negative real", rval == -12.3456)

      ierr = idba_fatto(handle)
      call ensure_no_error("fatto")

      ierr = idba_arrivederci(idbhandle)
      call ensure_no_error("arrivederci")
  
      call exit (0)
  
end program check_range

include "check-utils.h"
