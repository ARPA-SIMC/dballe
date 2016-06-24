      program mkmsg
      USE,INTRINSIC :: iso_c_binding
      include "dballeff.h"

! ****************************
! * Create a test message file
! ****************************

      integer handle, nstaz, ndata, nattr
      integer i, i1, i2, tmp
      integer id,height,codrete
      character fname*256,encoding*10,cname*20,rete*20,value*255
      character btable*10,starbtable*10
      real*8 dlat,dlon
      external errorrep

      ierr = idba_error_set_callback(0, C_FUNLOC(errorrep), 2, i)

!     Open a session
      call getarg(1,fname)
      call getarg(2,encoding)
      ierr = idba_messaggi(handle, fname, "w", encoding)

!     Write a measured value
      ierr = idba_unsetall(handle)
      ierr = idba_setlevel(handle, 1, 2, 3, 4)
      ierr = idba_settimerange(handle, 5, 6, 7)
      ierr = idba_seti(handle,"B12101",300)
      ierr = idba_prendilo(handle)

!     Write a station value
      ierr = idba_unsetall(handle)
      ierr = idba_setcontextana(handle)
      ierr = idba_setc(handle,"rep_memo","temp")
      ierr = idba_seti(handle,"block",1)
      ierr = idba_setc(handle,'query',"message generic")
      ierr = idba_prendilo(handle)

!     Done
      ierr = idba_fatto(handle)

      call exit (0)
    
      end

! ********************
! * Utility functions
! ********************

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

      subroutine errorrep(val) BIND(C)
      integer val
      character buf*1000

      ier = idba_error_code()
      if (ier.ne.0) then
         print *,ier," error in ",val
         ierr = idba_error_message(buf)
         print *,buf(:istrlen(buf))
         ierr = idba_error_context(buf)
         print *,buf(:istrlen(buf))
         ierr = idba_error_details(buf)
         print *,buf(:istrlen(buf))
         call exit (1)
      end if
      return

      end
