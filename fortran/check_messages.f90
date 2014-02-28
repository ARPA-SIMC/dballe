      program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest

      include "dballef.h"

      integer :: dbahandle, handle,i,i1,i2,i3,i4,i5,i6,ival,saved_id
      real :: rval
      double precision :: dval
      character (len=10) :: param
      character (len=255) :: cval
      character (len=512) :: infile
      external :: testcb

!      call fdba_error_set_callback(0, testcb, 2, i)

      call idba_presentati(dbahandle, "mem:", char(0), char(0))
      call ensure_no_error("presentati")

      call idba_preparati(dbahandle, handle, "write", "write", "write")
      call ensure_no_error("preparati")

      call getenv("DBA_TESTDATA", infile)
      infile = trim(infile) // "/bufr/db-messages1.bufr";

!     Database login
      call idba_messages_open(handle, infile, "rb", "BUFR", "")
      call ensure_no_error("messages_open")

!     Open a session
      call idba_unsetall(handle)
      call ensure_no_error("unsetall")

!     Query the first message/subset
      call idba_remove_all(handle)
      call idba_messages_read_next(handle, i)
      call ensure_no_error("messages_read_next 1")
      call ensure("messages_read_next result 1", i.eq.1)
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto msg1")
      call ensure("voglioquesto result msg1", i.eq.88)
!     Querying again gives the same data
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto msg1 dup")
      call ensure("voglioquesto result msg1", i.eq.88)
!     Advance to the next message
      call idba_remove_all(handle)
      call idba_messages_read_next(handle, i)
      call ensure_no_error("messages_read_next 2")
      call ensure("messages_read_next result 2", i.eq.1)
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 2")
      call ensure("voglioquesto result msg2", i.eq.9)

!     And the other messages
      call idba_remove_all(handle)
      call idba_messages_read_next(handle, i)
      call ensure_no_error("messages_read_next 3")
      call ensure("messages_read_next result 3", i.eq.1)
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 3")
      call ensure("voglioquesto result msg3", i.eq.193)

      call idba_remove_all(handle)
      call idba_messages_read_next(handle, i)
      call ensure_no_error("messages_read_next 4")
      call ensure("messages_read_next result 4", i.eq.1)
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 4")
      call ensure("voglioquesto result msg4", i.eq.182)

      call idba_remove_all(handle)
      call idba_messages_read_next(handle, i)
      call ensure_no_error("messages_read_next 5")
      call ensure("messages_read_next result 5", i.eq.1)
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 5")
      call ensure("voglioquesto result msg5", i.eq.170)

      call idba_remove_all(handle)
      call idba_messages_read_next(handle, i)
      call ensure_no_error("messages_read_next 6")
      call ensure("messages_read_next result 6", i.eq.1)
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 6")
      call ensure("voglioquesto result msg6", i.eq.184)

      call idba_remove_all(handle)
      call idba_messages_read_next(handle, i)
      call ensure_no_error("messages_read_next 7")
      call ensure("messages_read_next result 7", i.eq.1)
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 7")
      call ensure("voglioquesto result msg7", i.eq.256)

      call idba_remove_all(handle)
      call idba_messages_read_next(handle, i)
      call ensure_no_error("messages_read_next 8")
      call ensure("messages_read_next result 8", i.eq.1)
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 8")
      call ensure("voglioquesto result msg8", i.eq.213)

      call idba_remove_all(handle)
      call idba_messages_read_next(handle, i)
      call ensure_no_error("messages_read_next 9")
      call ensure("messages_read_next result 9", i.eq.0)

      call idba_fatto(handle)
      call ensure_no_error("fatto")

      call idba_arrivederci(dbahandle)
      call ensure_no_error("arrivederci")

!     If we made it so far, exit with no error
      print*,"check_messages: all tests succeed."

      call exit (0)
    
      end program

      include "check-utils.h"

