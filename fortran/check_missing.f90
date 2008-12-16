      program check_missing

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest

      include "dballef.h"

      integer dbahandle, handle, ival
      integer (kind=dba_int_b):: bval
      real rval
      real*8 dval
      character cval*255
      external testcb

!      call fdba_error_set_callback(0, testcb, 2, i)

!     Database login
      call dbinit(dbahandle)

!     Open a session
      call idba_preparati(dbahandle, handle, "write", "write", "write")
      call ensure_no_error("preparati")

!     Try reading 'missing' values
      call idba_enqb(handle, "lat", bval)
      call ensure_no_error("enq key byte")
      call ensure("enq empty key byte", bval.eq.DBA_MVB)
      call idba_enqc(handle, "lat", cval)
      call ensure_no_error("enq key char")
      call ensure("enq empty key char", cval.eq.DBA_MVC)
      call idba_enqi(handle, "lat", ival)
      call ensure_no_error("enq key int")
      call ensure("enq empty key int", ival.eq.DBA_MVI)
      call idba_enqr(handle, "lat", rval)
      call ensure_no_error("enq key real")
      call ensure("enq empty key real", rval.eq.DBA_MVR)
      call idba_enqd(handle, "lat", dval)
      call ensure_no_error("enq key double")
      call ensure("enq empty key double", dval.eq.DBA_MVD)

      call idba_enqb(handle, "B12001", bval)
      call ensure_no_error("enq var byte")
      call ensure("enq empty var byte", bval.eq.DBA_MVB)
      call idba_enqc(handle, "B12001", cval)
      call ensure_no_error("enq var char")
      call ensure("enq empty var char", cval.eq.DBA_MVC)
      call idba_enqi(handle, "B12001", ival)
      call ensure_no_error("enq var int")
      call ensure("enq empty var int", ival.eq.DBA_MVI)
      call idba_enqr(handle, "B12001", rval)
      call ensure_no_error("enq var real")
      call ensure("enq empty var real", rval.eq.DBA_MVR)
      call idba_enqd(handle, "B12001", dval)
      call ensure_no_error("enq var double")
      call ensure("enq empty var double", dval.eq.DBA_MVD)

!     Try using the 'missing' values
      call idba_setb(handle, "lat", DBA_MVB)
      call ensure_no_error("unset key byte")
      call idba_setc(handle, "lat", DBA_MVC)
      call ensure_no_error("unset key char")
      call idba_seti(handle, "lat", DBA_MVI)
      call ensure_no_error("unset key int")
      call idba_setr(handle, "lat", DBA_MVR)
      call ensure_no_error("unset key real")
      call idba_setd(handle, "lat", DBA_MVD)
      call ensure_no_error("unset key double")

      call idba_setb(handle, "B12001", DBA_MVB)
      call ensure_no_error("unset var byte")
      call idba_setc(handle, "B12001", DBA_MVC)
      call ensure_no_error("unset var char")
      call idba_seti(handle, "B12001", DBA_MVI)
      call ensure_no_error("unset var int")
      call idba_setr(handle, "B12001", DBA_MVR)
      call ensure_no_error("unset var real")
      call idba_setd(handle, "B12001", DBA_MVD)
      call ensure_no_error("unset var double")

!     If we made it so far, exit with no error
      print*,"check_missing: all tests succeed."

      call exit (0)
    
      end program

      include "check-utils.h"
