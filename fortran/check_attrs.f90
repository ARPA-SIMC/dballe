      program check_attrs

      use dbtest
      use dballef

! *****************************************
! * Dump the contents of a dballe database
! *****************************************

      integer dbahandle, handle, handleinit, nattr
      integer ierr
      character btable*10
      external errorrep

!      ierr = idba_error_set_callback(0, errorrep, 2, i)

!     Database login
      call dbinit(dbahandle)

!     Open a session
      ierr = idba_preparati(dbahandle, handleinit, &
             "write", "write", "write")
      call ensure_no_error("preparati handleinit")

      ierr = idba_scopa(handleinit, DBA_MVC)
      call ensure_no_error("scopa")

!     Insert test data
      ierr = idba_setd(handleinit, "lat", 12.345D00)
      call ensure_no_error("init 1")
      ierr = idba_setd(handleinit, "lon", 12.345D00)
      call ensure_no_error("init 2")
      ierr = idba_setlevel(handleinit, 1, 0, 0, 0)
      call ensure_no_error("init 3")
      ierr = idba_settimerange(handleinit, 0, 0, 0)
      call ensure_no_error("init 4")
      ierr = idba_setdate(handleinit, 2007, 06, 13, 0, 0, 0)
      call ensure_no_error("init 5")
      ierr = idba_setd(handleinit, "B12101", 12.345D0)
      call ensure_no_error("init 6")
      ierr = idba_setd(handleinit, "B12103", 23.456D0)
      call ensure_no_error("init 6")
      ierr = idba_setc(handleinit, "rep_memo", 'synop')
      call ensure_no_error("init 6b")
      ierr = idba_prendilo(handleinit)
      call ensure_no_error("prendilo")

      ierr = idba_setr(handleinit, "*B33007", 75.0)
      call ensure_no_error("init 8")
      ierr = idba_setc(handleinit, "*var_related", "B12101")
      call ensure_no_error("init 9")
      ierr = idba_critica(handleinit)
      call ensure_no_error("critica 1")

      ierr = idba_setr(handleinit, "*B33040", 80.0)
      call ensure_no_error("init 11")
      ierr = idba_setc(handleinit, "*var_related", "B12101")
      call ensure_no_error("init 12")
      ierr = idba_critica(handleinit)
      call ensure_no_error("critica 2")

      ierr = idba_setr(handleinit, "*B33036", 90.0)
      call ensure_no_error("init 14")
      ierr = idba_setc(handleinit, "*var_related", "B12101")
      call ensure_no_error("init 15")
      ierr = idba_critica(handleinit)
      call ensure_no_error("critica 3")
      ierr = idba_fatto(handleinit)
      call ensure_no_error("fatto 1")

      ! Read back the data
      ierr = idba_preparati(dbahandle, handle, "read", "read", "read")
      call ensure_no_error("preparati handle")

      ierr = idba_setc(handle, "var", "B12101")
      call ensure_no_error("query set 1")
      ierr = idba_voglioquesto(handle, nattr)
      call ensure("I need 1 var in output", nattr.eq.1)
      call ensure_no_error("query voglioquesto 1")
      ierr = idba_dammelo(handle, btable)
      call ensure_no_error("query dammelo 1")

      call ensure_no_error("query set 2")
      ierr = idba_voglioancora(handle, nattr)
      call ensure_no_error("query voglioancora 1")
      call ensure("I need 3 values", nattr.eq.3)

      ierr = idba_setc(handle, "*var", "*B33040")
      call ensure_no_error("query set 3")
      ierr = idba_voglioancora(handle, nattr)
      call ensure_no_error("query voglioancora 2")
      call ensure("I need 1 values", nattr.eq.1)

      ierr = idba_unset(handle, "*var")
      call ensure_no_error("query set 4")
      ierr = idba_setc(handle, "*varlist", "*B33007,*B33036")
      call ensure_no_error("query set 5")
      ierr = idba_voglioancora(handle, nattr)
      call ensure_no_error("query voglioancora 3")
      call ensure("I need 2 values", nattr.eq.2)

      ierr = idba_fatto(handleinit)
      ierr = idba_fatto(handle)
      ierr = idba_arrivederci(dbahandle)

      call exit (0)
    
      end

      include "check-utils.h"

