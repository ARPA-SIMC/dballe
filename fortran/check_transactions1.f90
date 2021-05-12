      program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest
      use dballef

      integer :: dbahandle, handle_read, handle_write, ierr, ival
      external :: testcb

!     Database login
      call dbinit(dbahandle)

!     Open a session
      ierr = idba_preparati(dbahandle, handle_write, "write", "write", "write")
      call ensure_no_error("preparati write")

!     Clear the database
      ierr = idba_scopa(handle_write, "") 
      call ensure_no_error("scopa")

!     Insert some data
      ierr = idba_setd(handle_write, "lat", 30D00)
      ierr = idba_setr(handle_write, "lon", 10.0)
      ierr = idba_setc(handle_write, "mobile", "0")
      ierr = idba_setdate(handle_write, 2006, 1, 2, 3, 4, 5)
      ierr = idba_setlevel(handle_write, 1, 1, 1, 1)
      ierr = idba_settimerange(handle_write, 20, 1, 1)
      ierr = idba_setc(handle_write, "rep_memo", "synop")
      ierr = idba_setc(handle_write, "B01011", "DB-All.e!")
      ierr = idba_prendilo(handle_write)
      call ensure_no_error("first prendilo")

      ! do not call idba_fatto yet, no data is read

      ierr = idba_preparati(dbahandle, handle_read, "read", "read", "read")
      call ensure_no_error("preparati read 1")
      ierr = idba_voglioquesto(handle_read, ival)
      call ensure_no_error("voglioquesto 1")
      call ensure("voglioquesto 1 result", ival.eq.0)
      ierr = idba_fatto(handle_read)
      call ensure_no_error("fatto 1")

      ierr = idba_fatto(handle_write)
      call ensure_no_error("fatto w")

      ierr = idba_preparati(dbahandle, handle_read, "read", "read", "read")
      call ensure_no_error("preparati read 2")
      ierr = idba_voglioquesto(handle_read, ival)
      call ensure_no_error("voglioquesto 2")
      call ensure("voglioquesto 2 result", ival.eq.1)
      ierr = idba_fatto(handle_read)
      call ensure_no_error("fatto 2")

      ierr = idba_arrivederci(dbahandle)
      call ensure_no_error("arrivederci r")

      call exit (0)

      end program

      include "check-utils.h"

