      program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest
      use dballef

      integer :: dbahandle, handle,i,i1,i2,i3,i4,i5,i6,ival,saved_id
      real :: rval
      double precision :: dval
      character (len=10) :: param
      character (len=255) :: cval
      external :: testcb

!      call fdba_error_set_callback(0, testcb, 2, i)

!     Database login
      call dbinit(dbahandle)

!     Open a session
      ierr = idba_begin(dbahandle, handle, "write", "write", "write")
      call ensure_no_error("begin")

!     Clear the database
      ierr = idba_reinit_db(handle, "")
      call ensure_no_error("reinit_db")

!     Insert some data
      ierr = idba_setd(handle, "lat", 30D00)
      call ensure_no_error("setd 0")
      ierr = idba_setr(handle, "lon", 10.0)
      call ensure_no_error("setr 1")
      ierr = idba_setc(handle, "mobile", "0")
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

      ierr = idba_seti(handle, "year", 2006)
      call ensure_no_error("seti 3")
      ierr = idba_seti(handle, "month", 1)
      call ensure_no_error("seti 4")
      ierr = idba_seti(handle, "day", 2)
      call ensure_no_error("seti 5")
      ierr = idba_seti(handle, "hour", 3)
      call ensure_no_error("seti 6")
      ierr = idba_seti(handle, "min", 4)
      call ensure_no_error("seti 7")

      ierr = idba_setc(handle, "leveltype1", "1")
      call ensure_no_error("setc 8")
      ierr = idba_seti(handle, "l1", 1)
      call ensure_no_error("seti 9")
      ierr = idba_setc(handle, "leveltype2", "1")
      call ensure_no_error("seti 10")
      ierr = idba_seti(handle, "l2", 1)
      call ensure_no_error("seti 11")

      ierr = idba_seti(handle, "pindicator", 20)
      call ensure_no_error("seti 12")
      ierr = idba_seti(handle, "p1", 1)
      call ensure_no_error("seti 13")
      ierr = idba_seti(handle, "p2", 1)
      call ensure_no_error("seti 14")
      
      ierr = idba_setc(handle, "rep_memo", "synop")
      call ensure_no_error("seti 15")

      ierr = idba_setc(handle, "B01011", "DB-All.e!")
      call ensure_no_error("setc 16")

!     Perform the insert
      ierr = idba_insert_data(handle)
      call ensure_no_error("first insert_data")

!     Try to read the id of the pseudoana data just inserted
      ierr = idba_enqi(handle, "ana_id", i)
      call ensure_no_error("enqi ana_id")
!     Try to read the context id of the data just inserted
      ierr = idba_enqi(handle, "context_id", i)
      call ensure_no_error("enqi context_id")

!     Insert some QC flags
      ierr = idba_setc(handle, "*B33002", "1")
      call ensure_no_error("insert_attributes setc 0")
      ierr = idba_setc(handle, "*B33003", "1")
      call ensure_no_error("insert_attributes setc 1")
      ierr = idba_setc(handle, "*var_related", "B01011")
      call ensure_no_error("insert_attributes setc 2")
      ierr = idba_insert_attributes(handle)
!      ierr = idba_insert_attributes(handle, "B01011")
      call ensure_no_error("prima insert_attributes")

!     Query back the data
      ierr = idba_unsetall(handle)
      call ensure_no_error("unsetall")
      ierr = idba_setr(handle, "latmin", 20.0)
      call ensure_no_error("query setf 0")
      ierr = idba_setr(handle, "latmax", 50.0)
      call ensure_no_error("query setf 1")
      ierr = idba_setr(handle, "lonmin",  5.0)
      call ensure_no_error("query setf 2")
      ierr = idba_setr(handle, "lonmax", 20.0)
      call ensure_no_error("query setf 3")
      ierr = idba_query_data(handle, i)
      call ensure_no_error("query_data")

      call ensure("query_data result", i.eq.1)

!     Read the results of the query
!     TODO: make a for loop instead
      do while (i.gt.0)
         ierr = idba_next_data(handle, param)
         call ensure_no_error("next_data")
         ierr = idba_enqc(handle, param, cval)
         call ensure_no_error("next_data enqc 0")
         ierr = idba_enqi(handle, "year", ival)
         call ensure_no_error("next_data enqi 1")
         ierr = idba_enqr(handle, "lat", rval)
         call ensure_no_error("next_data enqr 2")
         ierr = idba_enqd(handle, "lon", dval)
         call ensure_no_error("next_data enqd 3")
         ierr = idba_enqc(handle, "lon", cval)
         call ensure_no_error("next_data enqc 4")
         ierr = idba_enqdate(handle, i1, i2, i3, i4, i5, i6)
         call ensure_no_error("next_data enqdate")
         call ensure("enqdate i1", i1.eq.2006)
         call ensure("enqdate i2", i2.eq.1)
         call ensure("enqdate i3", i3.eq.2)
         call ensure("enqdate i4", i4.eq.3)
         call ensure("enqdate i5", i5.eq.4)
         call ensure("enqdate i6", i6.eq.0)
         ierr = idba_enqlevel(handle, i1, i2, i3, i4)
         call ensure_no_error("next_data enqlevel")
         call ensure("enqlevel i1", i1.eq.1)
         call ensure("enqlevel i2", i2.eq.1)
         call ensure("enqlevel i3", i3.eq.1)
         call ensure("enqlevel i4", i4.eq.1)
         ierr = idba_enqtimerange(handle, i1, i2, i3)
         call ensure_no_error("next_data enqtimerange")
         call ensure("enqdate i1", i1.eq.20)
         call ensure("enqdate i2", i2.eq.1)
         call ensure("enqdate i3", i3.eq.1)

         ierr = idba_enqi(handle, "ana_id", ival)
         call ensure_no_error("next_data enqi ana_id")
         call ensure("ana_id", ival.eq.1)

         ierr = idba_enqi(handle, "context_id", ival)
         call ensure_no_error("next_data enqi context_id")

!        Save the id for reusing it later
         saved_id = ival

         ierr = idba_query_attributes(handle, i1)
         call ensure_no_error("query_attributes")
         do while (i1.gt.0)
            ierr = idba_next_attribute(handle, param)
            call ensure_no_error("next_attribute")
            ierr = idba_enqc(handle, param, cval)
            call ensure_no_error("next_attribute enqc")
            i1 = i1 - 1
         enddo

!        Perform some useless remove_attributes just to test the parser
         ierr = idba_setc(handle, "*varlist", "*B12345");
         call ensure_no_error("remove_attributes setc 1")
         ierr = idba_remove_attributes(handle);
         call ensure_no_error("remove_attributes 1")
         ierr = idba_setc(handle, "*varlist", "*B12345,*B54321");
         call ensure_no_error("remove_attributes setc 2")
         ierr = idba_remove_attributes(handle);
         call ensure_no_error("remove_attributes 2")

         i = i - 1
      enddo

!     Remove the QC data for saved_data
      ierr = idba_seti(handle, "*context_id", saved_id);
      call ensure_no_error("remove_attributes seti 3")
      ierr = idba_setc(handle, "*var", "*B01011")
      call ensure_no_error("remove_attributes setc 3")
      ierr = idba_remove_attributes(handle);
      call ensure_no_error("remove_attributes 3")

      ierr = idba_unsetall(handle)
      call ensure_no_error("unsetall quantesono 1")
      ierr = idba_quantesono(handle, i)
      call ensure_no_error("quantesono 1")
      call ensure("quantesono 1", i.eq.1)

      ierr = idba_setd(handle, "latmax", 20D00)
      call ensure_no_error("seti quantesono 2")
      ierr = idba_quantesono(handle, i)
      call ensure_no_error("quantesono 2")
      call ensure("quantesono 1", i.eq.0)

      ierr = idba_fatto(handle)
      call ensure_no_error("fatto")

      ierr = idba_arrivederci(dbahandle)
      call ensure_no_error("arrivederci")

      call exit (0)
    
      end program

      include "check-utils.h"

