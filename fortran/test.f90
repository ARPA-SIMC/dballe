      module test
      use dballef

      contains

!     Continue execution only if there was no error
      subroutine ensure_no_error(message)
      character (len=*) :: message
      integer :: ier
      character (len=1000) :: buf

      ier = idba_error_code()
      if (ier.ne.0) then
         print *,ier," in ",message
         call idba_error_message(buf)
         print *,trim(buf)
         call idba_error_context(buf)
         print *,trim(buf)
         call idba_error_details(buf)
         print *,trim(buf)
         call exit (1)
      end if
      return
      endsubroutine ensure_no_error

!     Print an error if the given logical value is false
      subroutine ensure(message, value)
      character (len=*) :: message
      logical :: value

      if (.not.value) then
         print *,"Check failed in ",message
         call exit (1)
      end if
      return
      endsubroutine ensure

      endmodule test
