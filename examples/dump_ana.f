      program dump_ana

ccc Dump coordinates of items present in pseudoana
ccc
ccc By default, dump all coordinates
ccc If a parameter is given in commandline, then dump only the
ccc coordinates of the mobile station with the given ident

      integer dbahandle, handle,i
      real*8 lat, lon
      character cname*255,ident*64,wantident*64
      external testcb

      call getarg(1, wantident)

      call idba_error_set_callback(0, testcb, 2, i)

c     Database login
      call idba_presentati(dbahandle, "test", "enrico", "")

c     Open a session
      call idba_preparati(dbahandle, handle, "read", "read", "read")

c     Dump the anagraphical data
      call idba_quantesono(handle, i);

      if (wantident.eq."") then
         do while (i.gt.0)
            call idba_elencamele(handle)
            call idba_enqd(handle, "lat", lat)
            call idba_enqd(handle, "lon", lon)
            call idba_enqc(handle, "name", cname)

            print*,lat,lon,cname(:istrlen(cname))

            i = i - 1
         enddo
      else
         do while (i.gt.0)
            call idba_elencamele(handle)
            call idba_enqd(handle, "lat", lat)
            call idba_enqd(handle, "lon", lon)
            call idba_enqc(handle, "name", cname)
            call idba_enqc(handle, "ident", ident)
            if (ident.eq.wantident) then
               print*,lat,lon,cname(:istrlen(cname))
            endif

            i = i - 1
         enddo
      endif

      call idba_fatto(handle)
      call idba_arrivederci()

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
