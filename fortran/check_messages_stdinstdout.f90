      program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest
      USE,INTRINSIC :: iso_c_binding
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

!     Database login
      ier = idba_messages_open_input(handle, "", "rb", "BUFR", .true.)
      call ensure_no_error("messages_open_input")

      ier = idba_messages_open_output(handle, "", "wb", "BUFR")
      call ensure_no_error("messages_open_output")

!     Query the first message/subset
      ier = idba_messages_read_next(handle, status)
      call ensure_no_error("messages_read_next 1")
      call ensure("messages_read_next result 1", status)

      ier = idba_messages_write_next(handle, "")
      call ensure_no_error("messages_write_next 1")

      ier = idba_fatto(handle)
      call ensure_no_error("fatto")

      ier = idba_arrivederci(dbahandle)
      call ensure_no_error("arrivederci")

!     If we made it so far, exit with no error
      print*,"check_messages_stdinstdout: all tests succeed."

      call exit (0)
    
      end program

      include "check-utils.h"

