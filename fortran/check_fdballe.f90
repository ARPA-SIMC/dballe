      program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      include "dballef.h"

      integer dbahandle, handle,i,i1,i2,i3,i4,i5,i6,ival,saved_id
      real rval
      real*8 dval
      character param*10,cval*255
      external testcb

!      call fdba_error_set_callback(0, testcb, 2, i)

!     Database login
      call idba_presentati(dbahandle, "test", "enrico", "")
      call ensure_no_error("presentati")

!     Open a session
      call idba_preparati(dbahandle, handle, "write", "write", "write")
      call ensure_no_error("preparati")

!     Clear the database
      call idba_scopa(handle, "") 
      call ensure_no_error("scopa")

!     Insert some data
      call idba_setd(handle, "lat", 30D00)
      call ensure_no_error("setd 0")
      call idba_setr(handle, "lon", 10.0)
      call ensure_no_error("setr 1")
      call idba_setc(handle, "mobile", "0")
      call ensure_no_error("setc 2")

!     { "year_ident", "2003" },
!     { "month_ident", "3" },
!     { "day_ident", "23" },
!     { "hour_ident", "12" },
!     { "min_ident", "30" },
!     { "height", "42" },
!     { "heightbaro", "234" },
!     { "block", "1" },
!     { "station", "52" },
!     { "name", "Cippo Lippo" },

      call idba_seti(handle, "year", 2006)
      call ensure_no_error("seti 3")
      call idba_seti(handle, "month", 1)
      call ensure_no_error("seti 4")
      call idba_seti(handle, "day", 2)
      call ensure_no_error("seti 5")
      call idba_seti(handle, "hour", 3)
      call ensure_no_error("seti 6")
      call idba_seti(handle, "min", 4)
      call ensure_no_error("seti 7")

      call idba_setc(handle, "leveltype1", "1")
      call ensure_no_error("setc 8")
      call idba_seti(handle, "l1", 1)
      call ensure_no_error("seti 9")
      call idba_setc(handle, "leveltype2", "1")
      call ensure_no_error("seti 10")
      call idba_seti(handle, "l2", 1)
      call ensure_no_error("seti 11")

      call idba_seti(handle, "pindicator", 20)
      call ensure_no_error("seti 12")
      call idba_seti(handle, "p1", 1)
      call ensure_no_error("seti 13")
      call idba_seti(handle, "p2", 1)
      call ensure_no_error("seti 14")
      
      call idba_seti(handle, "rep_cod", 1)
      call ensure_no_error("seti 15")

      call idba_setc(handle, "B01011", "DB-All.e!")
      call ensure_no_error("setc 16")

!     Perform the insert
      call idba_prendilo(handle)
      call ensure_no_error("first prendilo")

!     Try to read the id of the pseudoana data just inserted
      call idba_enqi(handle, "ana_id", i)
      call ensure_no_error("enqi ana_id")
!     Try to read the context id of the data just inserted
      call idba_enqi(handle, "context_id", i)
      call ensure_no_error("enqi context_id")

!     Insert some QC flags
      call idba_setc(handle, "*B33002", "1")
      call ensure_no_error("critica setc 0")
      call idba_setc(handle, "*B33003", "t")
      call ensure_no_error("critica setc 1")
      call idba_setc(handle, "*var", "B01011")
      call idba_critica(handle)
!      call idba_critica(handle, "B01011")
      call ensure_no_error("prima critica")

!     Query back the data
!      call idba_ricominciamo(handle)
      call idba_unsetall(handle)
      call ensure_no_error("ricominciamo")
      call idba_setr(handle, "latmin", 20.0)
      call ensure_no_error("query setf 0")
      call idba_setr(handle, "latmax", 50.0)
      call ensure_no_error("query setf 1")
      call idba_setr(handle, "lonmin",  5.0)
      call ensure_no_error("query setf 2")
      call idba_setr(handle, "lonmax", 20.0)
      call ensure_no_error("query setf 3")
      call idba_voglioquesto(handle, i)
      call ensure_no_error("voglioquesto")

      call ensure("voglioquesto result", i.eq.1)

!     Read the results of the query
!     TODO: make a for loop instead
      do while (i.gt.0)
         call idba_dammelo(handle, param)
         call ensure_no_error("dammelo")
         call idba_enqc(handle, param, cval)
         call ensure_no_error("dammelo enqc 0")
         call idba_enqi(handle, "year", ival)
         call ensure_no_error("dammelo enqi 1")
         call idba_enqr(handle, "lat", rval)
         call ensure_no_error("dammelo enqr 2")
         call idba_enqd(handle, "lon", dval)
         call ensure_no_error("dammelo enqd 3")
         call idba_enqc(handle, "lon", cval)
         call ensure_no_error("dammelo enqc 4")
         call idba_enqdate(handle, i1, i2, i3, i4, i5, i6)
         call ensure_no_error("dammelo enqdate")
         call ensure("enqdate i1", i1.eq.2006)
         call ensure("enqdate i2", i2.eq.1)
         call ensure("enqdate i3", i3.eq.2)
         call ensure("enqdate i4", i4.eq.3)
         call ensure("enqdate i5", i5.eq.4)
         call ensure("enqdate i6", i6.eq.0)
         call idba_enqlevel(handle, i1, i2, i3, i4)
         call ensure_no_error("dammelo enqlevel")
         call ensure("enqlevel i1", i1.eq.1)
         call ensure("enqlevel i2", i2.eq.1)
         call ensure("enqlevel i3", i3.eq.1)
         call ensure("enqlevel i4", i4.eq.1)
         call idba_enqtimerange(handle, i1, i2, i3)
         call ensure_no_error("dammelo enqtimerange")
         call ensure("enqdate i1", i1.eq.20)
         call ensure("enqdate i2", i2.eq.1)
         call ensure("enqdate i3", i3.eq.1)
         
         call idba_enqi(handle, "ana_id", ival)
         call ensure_no_error("dammelo enqi ana_id")
         call ensure("ana_id", ival.eq.1)

         call idba_enqi(handle, "context_id", ival)
         call ensure_no_error("dammelo enqi context_id")

!        Save the id for reusing it later
         saved_id = ival

         call idba_voglioancora(handle, i1)
         call ensure_no_error("voglioancora")
         do while (i1.gt.0)
            call idba_ancora(handle, param)
            call ensure_no_error("ancora")
            call idba_enqc(handle, param, cval)
            call ensure_no_error("ancora enqc")
            i1 = i1 - 1
         enddo

!        Perform some useless scusa just to test the parser
         call idba_setc(handle, "*varlist", "*B12345");
         call ensure_no_error("scusa setc 1")
         call idba_scusa(handle);
         call ensure_no_error("scusa 1")
         call idba_setc(handle, "*varlist", "*B12345,*B54321");
         call ensure_no_error("scusa setc 2")
         call idba_scusa(handle);
         call ensure_no_error("scusa 2")

         i = i - 1
      enddo

!     Remove the QC data for saved_data
      call idba_seti(handle, "*context_id", saved_id);
      call ensure_no_error("scusa seti 3")
      call idba_setc(handle, "*var", "*B01011")
      call ensure_no_error("scusa setc 3")
      call idba_scusa(handle);
      call ensure_no_error("scusa 3")

      call idba_unsetall(handle)
      call ensure_no_error("unsetall quantesono 1")
      call idba_quantesono(handle, i)
      call ensure_no_error("quantesono 1")
      call ensure("quantesono 1", i.eq.1)

      call idba_setd(handle, "latmax", 20D00)
      call ensure_no_error("seti quantesono 2")
      call idba_quantesono(handle, i)
      call ensure_no_error("quantesono 2")
      call ensure("quantesono 1", i.eq.0)

      call idba_fatto(handle)
      call ensure_no_error("fatto")

      call idba_arrivederci(dbahandle)
      call ensure_no_error("arrivederci")


!     If we made it so far, exit with no error
      print*,"check_fdballe: all tests succeed."

      call exit (0)
    
      end program

      include "check-utils.h"
