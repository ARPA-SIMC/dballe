      program check_attrs

      use dbtest

      include "dballef.h"

! *****************************************
! * Dump the contents of a dballe database
! *****************************************

      integer dbahandle, handle, handleinit, nstaz, ndata, nattr
      integer i, i1, i2, tmp
      integer id,height,codrete
      character cname*20,rete*20,value*255,avalue*255
      character btable*10,starbtable*10
      real dlat,dlon
      external errorrep

!      call idba_error_set_callback(0, errorrep, 2, i)

!     Database login
      call dbinit(dbahandle)

!     Open a session
      call idba_preparati(dbahandle, handle, "read", "read", "read")
      call ensure_no_error("preparati handle")
      call idba_preparati(dbahandle, handleinit, &
             "write", "write", "write")
      call ensure_no_error("preparati handleinit")

      call idba_scopa(handleinit, DBA_MVC)
      call ensure_no_error("scopa")

!     Insert test data
      call idba_setd(handleinit, "lat", 12.345D00)
      call ensure_no_error("init 1")
      call idba_setd(handleinit, "lon", 12.345D00)
      call ensure_no_error("init 2")
      call idba_setlevel(handleinit, 1, 0, 0, 0)
      call ensure_no_error("init 3")
      call idba_settimerange(handleinit, 0, 0, 0)
      call ensure_no_error("init 4")
      call idba_setdate(handleinit, 2007, 06, 13, 0, 0, 0)
      call ensure_no_error("init 5")
      call idba_setd(handleinit, "B12001", 12.345D0)
      call ensure_no_error("init 6")
      call idba_setc(handleinit, "rep_memo", 'synop')
      call ensure_no_error("init 6b")
      call idba_prendilo(handleinit)
      call ensure_no_error("prendilo")

      call idba_enqi(handleinit, "context_id", i)
      call ensure_no_error("init 7")

      call idba_setr(handleinit, "*B33007", 75.0)
      call ensure_no_error("init 8")
      call idba_setc(handleinit, "*var_related", "B12001")
      call ensure_no_error("init 9")
      call idba_critica(handleinit)
      call ensure_no_error("critica 1")

      call idba_setr(handleinit, "*B33040", 80.0)
      call ensure_no_error("init 11")
      call idba_setc(handleinit, "*var_related", "B12001")
      call ensure_no_error("init 12")
      call idba_critica(handleinit)
      call ensure_no_error("critica 2")

      call idba_setr(handleinit, "*B33036", 90.0)
      call ensure_no_error("init 14")
      call idba_setc(handleinit, "*var_related", "B12001")
      call ensure_no_error("init 15")
      call idba_critica(handleinit)
      call ensure_no_error("critica 3")

      call idba_seti(handle, "*context_id", i)
      call ensure_no_error("query set 1")
      call idba_setc(handle, "*var_related", "B12001")
      call ensure_no_error("query set 2")
      call idba_voglioancora(handle, nattr)
      call ensure_no_error("query voglioancora 1")
      call ensure("I need 3 values", nattr.eq.3)

      call idba_setc(handle, "*var", "*B33040")
      call ensure_no_error("query set 3")
      call idba_voglioancora(handle, nattr)
      call ensure_no_error("query voglioancora 2")
      call ensure("I need 1 values", nattr.eq.1)

      call idba_unset(handle, "*var")
      call ensure_no_error("query set 4")
      call idba_setc(handle, "*varlist", "*B33007,*B33036")
      call ensure_no_error("query set 5")
      call idba_voglioancora(handle, nattr)
      call ensure_no_error("query voglioancora 3")
      call ensure("I need 2 values", nattr.eq.2)

      call idba_fatto(handleinit)
      call idba_fatto(handle)
      call idba_arrivederci(dbahandle)

!     If we made it so far, exit with no error
      print*,"check_attrs: all tests succeed."

      call exit (0)
    
      end

      include "check-utils.h"

