      program check_missing

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest
      use dballef

      integer dbahandle, handle, ierr
      integer (kind=dba_int_b):: bval
      external testcb

!      call fdba_error_set_callback(0, testcb, 2, i)

!     Database login
      call dbinit(dbahandle)

!     Open a session
      ierr = idba_preparati(dbahandle, handle, "write", "write", "write")
      call ensure_no_error("preparati")

!     Try setting various kind of variables in various kinds of ways
      bval = 1
      ierr = idba_setb(handle, "lat", bval)
      call ensure_no_error("set key byte")
      ierr = idba_setc(handle, "lat", "1")
      call ensure_no_error("set key char")
      ierr = idba_seti(handle, "lat", 1)
      call ensure_no_error("set key int")
      ierr = idba_setr(handle, "lat", 1.0)
      call ensure_no_error("set key real")
      ierr = idba_setd(handle, "lat", 1.0D0)
      call ensure_no_error("set key double")

      ierr = idba_setb(handle, "B12001", bval)
      call ensure_no_error("set var byte")
      ierr = idba_setc(handle, "B12001", "1")
      call ensure_no_error("set var char")
      ierr = idba_seti(handle, "B12001", 1)
      call ensure_no_error("set var int")
      ierr = idba_setr(handle, "B12001", 1.0)
      call ensure_no_error("set var real")
      ierr = idba_setd(handle, "B12001", 1.0D0)
      call ensure_no_error("set var double")

      ierr = idba_fatto(handle)
      call ensure_no_error("fatto")

      ierr = idba_arrivederci(dbahandle)
      call ensure_no_error("arrivederci")

      call exit (0)

      end program

      include "check-utils.h"
