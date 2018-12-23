      program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest
      use dballef

      integer :: dbahandle, handle,i,i1,i2,i3,i4,i5,i6,ival,saved_id
      real :: rval
      double precision :: dval
      character (len=10) :: param
      character (len=255) :: cval
      external :: testcb

      ierr = idba_connect(dbahandle, &
                  "sqlite:test-check-connect-wipe.sqlite?wipe=1")
      call ensure_no_error("connect 0")

!     A session to insert some data
      ierr = idba_begin(dbahandle, handle, "write", "write", "write")
      call ensure_no_error("begin 0")

      ierr = idba_setd(handle, "lat", 30D00)
      call ensure_no_error("set lat 0")
      ierr = idba_setr(handle, "lon", 10.0)
      call ensure_no_error("set lon 0")
      ierr = idba_setc(handle, "rep_memo", "synop")
      call ensure_no_error("set rep_memo 0")
      ierr = idba_setdate(handle, 2006, 1, 2, 3, 4, 5)
      call ensure_no_error("setdate 0")
      ierr = idba_setlevel(handle, 1, 1, 1, 1)
      call ensure_no_error("setlevel 0")
      ierr = idba_settimerange(handle, 1, 1, 1)
      call ensure_no_error("settimerange 0")
      ierr = idba_setc(handle, "B01011", "DB-All.e!")
      call ensure_no_error("set B01011 0")
      ierr = idba_insert_data(handle)
      call ensure_no_error("insert_data 0")

      ierr = idba_commit(handle)
      call ensure_no_error("end 0:0")

!     A session to reread the data
      ierr = idba_begin(dbahandle, handle, "write", "write", "write")
      call ensure_no_error("begin 0:1")

      ierr = idba_query_data(handle, i)
      call ensure_no_error("query_data")
      call ensure("query_data result", i.eq.1)

      ierr = idba_commit(handle)
      call ensure_no_error("end 0:1")

      ierr = idba_disconnect(dbahandle)
      call ensure_no_error("disconnect 0")


!     Redo connect and try to read data again
      ierr = idba_connect(dbahandle, &
                  "sqlite:test-check-connect-wipe.sqlite")
      call ensure_no_error("connect 1")

      ierr = idba_begin(dbahandle, handle, "write", "write", "write")
      call ensure_no_error("begin 1")

      ierr = idba_query_data(handle, i)
      call ensure_no_error("query_data 1")
      call ensure("query_data result", i.eq.1)

      ierr = idba_commit(handle)
      call ensure_no_error("end 1")

      ierr = idba_disconnect(dbahandle)
      call ensure_no_error("disconnect 1")

      call exit (0)

      end program

      include "check-utils.h"
