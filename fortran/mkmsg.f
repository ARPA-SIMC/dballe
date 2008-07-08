      program mkmsg

ccc ****************************
ccc * Create a test message file
ccc ****************************

      integer handle, nstaz, ndata, nattr
      integer i, i1, i2, tmp
      integer id,height,codrete
      character fname*256,encoding*10,cname*20,rete*20,value*255
      character btable*10,starbtable*10
      real*8 dlat,dlon
      external errorrep

      call idba_error_set_callback(0, errorrep, 2, i)

c     Open a session
      call getarg(1,fname)
      call getarg(2,encoding)
      call idba_preparati_msg(handle, fname, "w", encoding)

      call idba_setd(handle, "lat", 44.D0)
      call idba_setd(handle, "lon", 11.D0)
      call idba_setlevel(handle, 102, 2000, 0, 0)
      call idba_settimerange(handle, 254, 0, 0)
      call idba_setdate(handle, 2008, 7, 6, 5, 4, 3)

c     One without setting 'query'
      call idba_seti(handle, "B12001", 2731+120)
      call idba_prendilo(handle)
      call idba_seti(handle, "*B33192", 74)
      call idba_seti(handle, "*B33193", 81)
      call idba_seti(handle, "*B33194", 59)
      call idba_critica(handle)
      call idba_seti(handle, "B12003", 2731+100)
      call idba_prendilo(handle)

c     One setting 'query' to subset
      call idba_setc(handle, "query", "subset")
      call idba_seti(handle, "B12001", 2731+110)
      call idba_prendilo(handle)
      call idba_seti(handle, "*B33192", 47)
      call idba_seti(handle, "*B33193", 18)
      call idba_seti(handle, "*B33194", 95)
      call idba_critica(handle)
      call idba_seti(handle, "B12003", 2731+100)
      call idba_prendilo(handle)

c     One setting 'query' to message, and making a synop
      call idba_setc(handle, "query", "message")
      call idba_seti(handle, "rep_cod", 1)
      call idba_seti(handle, "B12001",  2731+90)
      call idba_prendilo(handle)
      call idba_seti(handle, "*B33007", 81)
      call idba_critica(handle)
      call idba_seti(handle, "B12003",  2731+80)
      call idba_seti(handle, "B13003",  80)
      call idba_prendilo(handle)

      call idba_fatto(handle)

      call exit (0)
    
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

      subroutine errorrep(val)
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
