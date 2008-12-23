program check_range

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest

      include "dballef.h" 
      
      integer :: handle,idbhandle,handle_err, errcode
      real :: rval
      !data var/ "B22070", "B22074", "B22001", "B22071", "B22042"/
      !integer debug
      !data debug/1/
      
      !call idba_error_set_callback(0,idba_default_error_handler,debug,handle_err)
  
!     Database login
      call dbinit(idbhandle)

!     Open a session
      call idba_preparati(idbhandle,handle,"write","write","write")
      call ensure_no_error("preparati")

!     Check that NaN values are trapped
      rval = 0.
      rval = log(rval) / log(rval)
      call idba_set(handle, "B12003", log(rval))
      errcode = idba_error_code()
      call ensure("set to NaN", errcode == 6)

!     Check that negative reals can be read
      call idba_set(handle, "lon", -12.3456)
      call ensure_no_error("set lon negative")
      call idba_test_input_to_output(handle)
      call ensure_no_error("input to output")
      call idba_enqr(handle, "lon", rval)
      call ensure_no_error("enq lon negative")
      call ensure("read negative real", rval == -12.3456)

      call idba_fatto(handle)
      call ensure_no_error("fatto")

      call idba_arrivederci(idbhandle)
      call ensure_no_error("arrivederci")
  
      call exit (0)
  
end program check_range

include "check-utils.h"
