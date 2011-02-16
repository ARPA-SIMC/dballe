      program check_missing

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest

      include "dballef.h"

      integer handle, ival, ndata, n, i
      integer (kind=dba_int_b):: bval
      real rval
      real*8 dval
      character cval*255, btable*8
      character(len=1024) :: fname
      external testcb

!      call fdba_error_set_callback(0, testcb, 2, i)

      call getenv('DBA_TESTDATA', fname)
      fname = trim(fname) // "/bufr/temp-bad5.bufr"

!     Open a session
      call idba_messaggi(handle, fname, "r", "BUFR")
      call ensure_no_error("messaggi")

!     Try reading 'missing' values
      call idba_enqb(handle, "latmin", bval)
      call ensure_no_error("enq key byte")
      call ensure("enq empty key byte", bval.eq.DBA_MVB)
      call idba_enqc(handle, "latmin", cval)
      call ensure_no_error("enq key char")
      call ensure("enq empty key char", cval.eq.DBA_MVC)
      call idba_enqi(handle, "latmin", ival)
      call ensure_no_error("enq key int")
      call ensure("enq empty key int", ival.eq.DBA_MVI)
      call idba_enqr(handle, "latmin", rval)
      call ensure_no_error("enq key real")
      call ensure("enq empty key real", rval.eq.DBA_MVR)
      call idba_enqd(handle, "latmin", dval)
      call ensure_no_error("enq key double")
      call ensure("enq empty key double", dval.eq.DBA_MVD)
      call idba_enq(handle, "latmin", dval)
      call ensure_no_error("enq key auto double")
      call ensure("enq empty key auto double", dval.eq.DBA_MVD)

      call idba_enqb(handle, "B05002", bval)
      call ensure_no_error("enq var byte")
      call ensure("enq empty var byte", bval.eq.DBA_MVB)
      call idba_enqc(handle, "B05002", cval)
      call ensure_no_error("enq var char")
      call ensure("enq empty var char", cval.eq.DBA_MVC)
      call idba_enqi(handle, "B05002", ival)
      call ensure_no_error("enq var int")
      call ensure("enq empty var int", ival.eq.DBA_MVI)
      call idba_enqr(handle, "B05002", rval)
      call ensure_no_error("enq var real")
      call ensure("enq empty var real", rval.eq.DBA_MVR)
      call idba_enqd(handle, "B05002", dval)
      call ensure_no_error("enq var double")
      call ensure("enq empty var double", dval.eq.DBA_MVD)
      call idba_enq(handle, "B05002", dval)
      call ensure_no_error("enq var auto double")
      call ensure("enq empty var auto double", dval.eq.DBA_MVD)

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

      call idba_unsetall(handle)
      call ensure_no_error("unsetall")

      n = 1
      do while ( n > 0 )
        call idba_voglioquesto (handle,n)
        call ensure_no_error("voglioquesto")
      
        do i = 1, n
          call idba_dammelo (handle,btable)
          call ensure_no_error("dammelo")
          call idba_enqd (handle,"B11001",dval)
          call ensure_no_error("enqd from msg")
          call idba_enqr (handle,"B11001",rval)
          call ensure_no_error("enqr from msg")
          call idba_enqi (handle,"B11001",ival)
          call ensure_no_error("enqi from msg")
          ! Value does not fit in a byte
          !call idba_enqb (handle,"B11001",bval)
          !call ensure_no_error("enqb from msg")
        end do
      end do

      call idba_fatto(handle)
      call ensure_no_error("fatto")

!     If we made it so far, exit with no error
      print*,"check_missing: all tests succeed."

      call exit (0)
    
      end program

      include "check-utils.h"
