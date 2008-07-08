      program dump_dballe

ccc *********************************************
ccc * Dump the contents of a weather message file
ccc *********************************************

      integer handle, nstaz, ndata, nattr
      integer i, i1, i2, tmp
      integer id,height,codrete
      character fname*256,cname*20,rete*20,value*255,avalue*255
      character btable*10,starbtable*10
      real*8 dlat,dlon
      external errorrep

      call idba_error_set_callback(0, errorrep, 2, i)

c     Open a session
      call getarg(1,fname)
      call idba_preparati_msg(handle, fname, "r", "AUTO")

c     Query all the stations
      do while (.true.)
        call idba_quantesono(handle, nstaz)
        write (*,*) nstaz," stazioni:"
        if (nstaz .eq. 0) exit

        do i=1, nstaz
          call idba_elencamele(handle)
          call idba_enqc(handle, "name", cname)
          call idba_enqi(handle, "ana_id", id)
          call idba_enqd(handle, "lat", dlat)
          call idba_enqd(handle, "lon", dlon)
          call idba_enqi(handle, "height", height)
          call idba_enqc(handle,"rep_memo",rete)
          call idba_enqi(handle,"rep_cod",codrete)
          write (*,*) "Staz ",id," (",dlat,",",dlon,") '",
     $        cname(:istrlen(cname)),"' h:",height,
     $        " ",rep_memo,":",rep_cod
          call idba_seti(handle,"ana_id",id)
          call idba_voglioquesto(handle,ndata)
          write (*,*) " ",ndata," dati:"
          do i1=1, ndata
            call idba_dammelo(handle,btable)
            call idba_enqc(handle,btable,value)
            write (*,*) '  var ',btable(:istrlen(btable)),": ",
     $        value(:istrlen(btable))

            call idba_voglioancora (handle,nattr)
            write (*,*) "   ",nattr," attributi:"
            do i2=1, nattr
              call idba_ancora(handle,starbtable)
              call idba_enqc(handle,starbtable,avalue)
              write(*,*) "    attr ",starbtable(:istrlen(starbtable)),
     $          ": ",avalue(:istrlen(avalue))
            enddo
          enddo
        enddo
      enddo

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
