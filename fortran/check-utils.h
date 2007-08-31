!!! ********************
!!! * Utility functions
!!! ********************

!     Compute the length of a string
      integer function istrlen(string)
      character string*(*)
      istrlen = len(string)
      do while ((string(istrlen:istrlen).eq." " .or. &
           string(istrlen:istrlen).eq."").and. &
           istrlen.gt.0)
         istrlen = istrlen - 1
      enddo
      return
      end

!     Continue execution only if there was no error
      subroutine ensure_no_error(message)
      character message*(*)
      integer idba_error_code, ier
      character buf*1000

!      print *,"siamo a ",message
      ier = idba_error_code()
      if (ier.ne.0) then
         print *,ier," in ",message
         call idba_error_message(buf)
         print *,buf(:istrlen(buf))
         call idba_error_context(buf)
         print *,buf(:istrlen(buf))
         call idba_error_details(buf)
         print *,buf(:istrlen(buf))
         call exit (1)
      end if
      return

      end

!     Print an error if the given logical value is false
      subroutine ensure(message, value)
      character message*(*)
      logical value

      if (.not.value) then
         print *,"Check failed in ",message
         call exit (1)
      end if
      return

      end

      subroutine testcb(val)
      integer val
      character buf*1000

      ier = idba_error_code()
      if (ier.ne.0) then
         print *,ier," testcb in ",val
         call idba_error_message(buf)
         print *,buf(:istrlen(buf))
         call idba_error_context(buf)
         print *,buf(:istrlen(buf))
         call idba_error_details(buf)
         print *,buf(:istrlen(buf))
         call exit (1)
      end if
      return

      end
