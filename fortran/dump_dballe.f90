      program dump_dballe
      USE,INTRINSIC :: iso_c_binding
      include "dballeff.h"

! *****************************************
! * Dump the contents of a dballe database
! *****************************************

      integer dbahandle, handle, handle_ana, nstaz, ndata, nattr
      integer i, i1, i2, tmp, ierr
      integer id,height
      character cname*20,rete*20,value*255,avalue*255
      character btable*10,starbtable*10
      real dlat,dlon
      external errorrep

      ierr = idba_error_set_callback(0, C_FUNLOC(errorrep), 2, i)

!     Database login
      ierr = idba_presentati(dbahandle, "test", "enrico", "")

!     Open a session
      ierr = idba_preparati(dbahandle, handle_ana, "read", "read", "read")
      ierr = idba_preparati(dbahandle, handle, "read", "read", "read")

!     Query all the stations
      ierr = idba_quantesono(handle_ana, nstaz)
      write (*,*) nstaz," stazioni:"

      do i=1, nstaz
        ierr = idba_elencamele(handle_ana)
        ierr = idba_enqc(handle_ana, "name", cname)
        ierr = idba_enqi(handle_ana, "ana_id", id)
        ierr = idba_enqr(handle_ana, "lat", dlat)
        ierr = idba_enqr(handle_ana, "lon", dlon)
        ierr = idba_enqi(handle_ana, "height", height)
        ierr = idba_enqc(handle_ana,"rep_memo",rete)
        write (*,*) "Staz ",id," (",dlat,",",dlon,") '", &
            cname(:istrlen(cname)),"' h:",height, &
            " ",rep_memo
        ierr = idba_seti(handle,"ana_id",id)
        ierr = idba_voglioquesto(handle,ndata)
        write (*,*) " ",ndata," dati:"
        do i1=1, ndata
          ierr = idba_dammelo(handle,btable)
          ierr = idba_enqc(handle,btable,value)
          write (*,*) '  var ',btable(:istrlen(btable)),": ", &
            value(:istrlen(btable))

          ierr = idba_enqi(handle,"!context_id",tmp)
          write (*,*) "  CTX: ",tmp

          ierr = idba_voglioancora (handle,nattr)
          write (*,*) "   ",nattr," attributi:"
          do i2=1, nattr
            ierr = idba_ancora(handle,starbtable)
            ierr = idba_enqc(handle,starbtable,avalue)
            write(*,*) "    attr ",starbtable(:istrlen(starbtable)), &
              ": ",avalue(:istrlen(avalue))
          enddo
        enddo
      enddo

      ierr = idba_fatto(handle_ana)
      ierr = idba_fatto(handle)
      ierr = idba_arrivederci(dbahandle)

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
      USE,INTRINSIC :: iso_c_binding
      include "dballeff.h"
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
