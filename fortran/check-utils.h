!!! ********************
!!! * Utility functions
!!! ********************

      subroutine testcb(val)
      USE,INTRINSIC :: iso_c_binding
      include "dballeff.h"
      integer :: val
      character (len=1000) :: buf

      ier = idba_error_code()
      if (ier.ne.0) then
         print *,ier," testcb in ",val
         call idba_error_message(buf)
         print *,trim(buf)
         call idba_error_context(buf)
         print *,trim(buf)
         call idba_error_details(buf)
         print *,trim(buf)
         call exit (1)
      end if
      return
      endsubroutine testcb
