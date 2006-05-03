      program check_fdballe

ccc *****************************************
ccc * Test suite for DBALLE Fortran bindings
ccc *****************************************

      integer dbahandle, handle,i,i1,i2,ival,saved_id,N
      real rval
      real*8 dval
      character param*10,cval*255
      external testcb

      call idba_error_set_callback(0, testcb, 2, i)

c     Database login
      call idba_presentati(dbahandle, "test", "enrico", "")
c     Open a session
      call idba_preparati(dbahandle, handle,
     $    "rewrite", "rewrite", "rewrite")

c     Clear the database
c     call idba_scopa(handle, "") 

c     do i=1,10000
c       call idba_seti(handle, "lat", i)
c     enddo

c     do i=1,10000
c       call idba_setd(handle, "lat", i * 1D-1)
c     enddo

c     do i=1,10000
c       call idba_setr(handle, "lat", i * 0.1)
c     enddo

c     do i=1,10000
c       call idba_setc(handle, "name", "ciao")
c     enddo

c     do i=1,10000
c       call idba_enqi(handle, "lat", ival)
c     enddo

c     do i=1,10000
c       call idba_enqd(handle, "lat", dval)
c     enddo

c     do i=1,10000
c       call idba_enqr(handle, "lat", rval)
c     enddo

c     do i=1,10000
c       call idba_enqc(handle, "name", cval)
c     enddo

c     do i=1,10000
c       call idba_unsetall(handle);
c     enddo

      call idba_unsetall(handle)
      do i=1,1000
        call idba_setc (handle,"rep_memo","synop")
        call idba_seti (handle,"year",2005)
        call idba_seti (handle,"month",2)
        call idba_seti (handle,"day",1)
        call idba_seti (handle,"hour",1)
        call idba_seti (handle,"min",2)
        call idba_seti (handle,"sec",0)
        call idba_seti (handle,"pindicator",4)
        call idba_seti (handle,"p1",2)
        call idba_seti (handle,"p2",3)
        call idba_setc (handle,"var","B12345")
        call idba_voglioquesto (handle,N)
        do i1=1,N
          call idba_dammelo (handle,param)
           call idba_enqi (handle,"leveltype", ival)
           call idba_enqi (handle,"l1", ival)
           call idba_enqi (handle,"l2", ival)
           call idba_enqr (handle,"lat",rval)
           call idba_enqr (handle,"lon",rval)
           call idba_enqi (handle,"height", ival)
           call idba_enqi (handle,"mobile", ival)
           call idba_enqi (handle,"block", ival)
           call idba_enqi (handle,"ana_id", ival)
           call idba_enqr (handle,param,rval)
        enddo
      enddo

      call idba_fatto(handle)
      call idba_arrivederci(dbahandle)

      print*,"check_mem: terminated."

c      call exit(0)
    
      end

ccc ********************
ccc * Utility functions
ccc ********************

c     Compute the length of a string
      integer function istrlen(string)
      character string*(*)
      istrlen = len(string)
      do while ((string(istrlen:istrlen).eq." " .or.
     $     string(istrlen:istrlen).eq."").and.
     $     istrlen.gt.0)
         istrlen = istrlen - 1
      enddo
      return
      end

c     Print an error if the given logical value is false
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
