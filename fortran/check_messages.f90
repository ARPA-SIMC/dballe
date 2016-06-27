      program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest

      include "dballeff.h"

      integer :: dbahandle, handle,i,i1,i2,i3,i4,i5,i6,ival,saved_id
      real :: rval
      double precision :: dval
      character (len=10) :: param
      character (len=255) :: cval
      character (len=512) :: infile
      external :: testcb
      logical :: status

!      call fdba_error_set_callback(0, testcb, 2, i)

      ier = idba_presentati(dbahandle, "mem:")
      call ensure_no_error("presentati")

      ier = idba_preparati(dbahandle, handle, "write", "write", "write")
      call ensure_no_error("preparati")

      call getenv("DBA_TESTDATA", infile)
      infile = trim(infile) // "/bufr/db-messages1.bufr";

!     Database login
      ier = idba_messages_open_input(handle, infile, "rb", "BUFR", .true.)
      call ensure_no_error("messages_open_input")

!     Open a session
      ier = idba_unsetall(handle)
      call ensure_no_error("unsetall")

!     Query the first message/subset
      ier = idba_remove_all(handle)
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 1")
      call ensure("messages_read_next result 1", status)
      ier = idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto msg1")
      call ensure("voglioquesto result msg1", i.eq.88)
!     Querying again gives the same data
      ier = idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto msg1 dup")
      call ensure("voglioquesto result msg1", i.eq.88)
!     Advance to the next message
      ier = idba_remove_all(handle)
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 2")
      call ensure("messages_read_next result 2", status)
      ier = idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 2")
      call ensure("voglioquesto result msg2", i.eq.9)

!     And the other messages
      ier = idba_remove_all(handle)
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 3")
      call ensure("messages_read_next result 3", status)
      ier = idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 3")
      call ensure("voglioquesto result msg3", i.eq.193)

      ier = idba_remove_all(handle)
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 4")
      call ensure("messages_read_next result 4", status)
      ier = idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 4")
      call ensure("voglioquesto result msg4", i.eq.182)

      ier = idba_remove_all(handle)
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 5")
      call ensure("messages_read_next result 5", status)
      ier = idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 5")
      call ensure("voglioquesto result msg5", i.eq.170)

      ier = idba_remove_all(handle)
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 6")
      call ensure("messages_read_next result 6", status)
      ier = idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 6")
      call ensure("voglioquesto result msg6", i.eq.184)

      ier = idba_remove_all(handle)
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 7")
      call ensure("messages_read_next result 7", status)
      ier = idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 7")
      call ensure("voglioquesto result msg7", i.eq.256)

      ier = idba_remove_all(handle)
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 8")
      call ensure("messages_read_next result 8", status)
      ier = idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto 8")
      call ensure("voglioquesto result msg8", i.eq.213)

      ier = idba_remove_all(handle)
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 9")
      call ensure("messages_read_next result 9", .not. status )

      ier = idba_fatto(handle)
      call ensure_no_error("fatto")

      ier = idba_arrivederci(dbahandle)
      call ensure_no_error("arrivederci")

!     If we made it so far, exit with no error
      print*,"check_messages: all tests succeed."

      call exit (0)
    
      end program

      include "check-utils.h"

