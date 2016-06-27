      program mkmsg
      use dballef

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

      ierr = idba_setd(handle, "lat", 44.D0)
      ierr = idba_setd(handle, "lon", 11.D0)
      ierr = idba_setlevel(handle, 102, 2000, 0, 0)
      ierr = idba_settimerange(handle, 254, 0, 0)
      ierr = idba_setdate(handle, 2008, 7, 6, 5, 4, 3)

!     One without setting 'query'
      ierr = idba_seti(handle, "B12001", 2731+120)
      ierr = idba_prendilo(handle)
      ierr = idba_seti(handle, "*B33192", 74)
      ierr = idba_seti(handle, "*B33193", 81)
      ierr = idba_seti(handle, "*B33194", 59)
      ierr = idba_critica(handle)
      ierr = idba_seti(handle, "B12003", 2731+100)
      ierr = idba_prendilo(handle)

!     One setting 'query' to subset
      ierr = idba_setc(handle, "query", "subset")
      ierr = idba_seti(handle, "B12001", 2731+110)
      ierr = idba_prendilo(handle)
      ierr = idba_seti(handle, "*B33192", 47)
      ierr = idba_seti(handle, "*B33193", 18)
      ierr = idba_seti(handle, "*B33194", 95)
      ierr = idba_critica(handle)
      ierr = idba_seti(handle, "B12003", 2731+100)
      ierr = idba_prendilo(handle)

!     One setting 'query' to message, and making a synop
      ierr = idba_setc(handle, "query", "message")
      ierr = idba_setc(handle, "rep_memo", "synop")
      ierr = idba_seti(handle, "B12001",  2731+90)
      ierr = idba_prendilo(handle)
      ierr = idba_seti(handle, "*B33007", 81)
      ierr = idba_critica(handle)
      ierr = idba_seti(handle, "B12003",  2731+80)
      ierr = idba_seti(handle, "B13003",  80)
      ierr = idba_prendilo(handle)

!     One setting 'query' to message, and making a synop
      ierr = idba_setc(handle, "query", "message synop")
      ierr = idba_setc(handle, "rep_memo", "synop")
      ierr = idba_seti(handle, "B12001",  2731+90)
      ierr = idba_prendilo(handle)

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
      use dballef
      integer val
      character buf*1000

      ier = idba_error_code()
      if (ier.ne.0) then
         print *,ier," error in ",val
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
