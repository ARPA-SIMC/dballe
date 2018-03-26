      program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest
      use dballef

      integer :: dbahandler, dbahandlew, handler, handlew,i,i1,i2,i3,i4,i5,i6,ival,saved_id
      real :: rval
      double precision :: dval
      character (len=10) :: param
      character (len=255) :: cval
      external :: testcb

!      call fdba_error_set_callback(0, testcb, 2, i)

!     Database login
      call dbinit(dbahandler)

! if I use different handles it do not work for sqlite
!!$!     Database login
!!$      call dbinit(dbahandlew)
      dbahandlew=dbahandler

!     Open a session
      ierr = idba_preparati(dbahandlew, handlew, "write", "write", "write")
      call ensure_no_error("preparati write")


!     Clear the database
      ierr = idba_scopa(handlew, "") 
      call ensure_no_error("scopa")

!     Insert some data
      ierr = idba_setd(handlew, "lat", 30D00)
      call ensure_no_error("setd 0")
      ierr = idba_setr(handlew, "lon", 10.0)
      call ensure_no_error("setr 1")
      ierr = idba_setc(handlew, "mobile", "0")
      call ensure_no_error("setc 2")

      ierr = idba_seti(handlew, "year", 2006)
      call ensure_no_error("seti 3")
      ierr = idba_seti(handlew, "month", 1)
      call ensure_no_error("seti 4")
      ierr = idba_seti(handlew, "day", 2)
      call ensure_no_error("seti 5")
      ierr = idba_seti(handlew, "hour", 3)
      call ensure_no_error("seti 6")
      ierr = idba_seti(handlew, "min", 4)
      call ensure_no_error("seti 7")

      ierr = idba_setc(handlew, "leveltype1", "1")
      call ensure_no_error("setc 8")
      ierr = idba_seti(handlew, "l1", 1)
      call ensure_no_error("seti 9")
      ierr = idba_setc(handlew, "leveltype2", "1")
      call ensure_no_error("seti 10")
      ierr = idba_seti(handlew, "l2", 1)
      call ensure_no_error("seti 11")

      ierr = idba_seti(handlew, "pindicator", 20)
      call ensure_no_error("seti 12")
      ierr = idba_seti(handlew, "p1", 1)
      call ensure_no_error("seti 13")
      ierr = idba_seti(handlew, "p2", 1)
      call ensure_no_error("seti 14")

      ierr = idba_setc(handlew, "rep_memo", "synop")
      call ensure_no_error("seti 15")

      ierr = idba_setc(handlew, "B01011", "DB-All.e!")
      call ensure_no_error("setc 16")

!     Perform the insert
      ierr = idba_prendilo(handlew)
      call ensure_no_error("first prendilo")

      ierr = idba_fatto(handlew)
      call ensure_no_error("fatto")
      ierr = idba_preparati(dbahandlew, handlew, "write", "write", "write")
      call ensure_no_error("second preparati write")

!     Query back the data
      ierr = idba_preparati(dbahandler, handler, "read", "read", "read")
      call ensure_no_error("preparati read")

!      ierr = idba_ricominciamo(handle)
      ierr = idba_unsetall(handler)
      call ensure_no_error("ricominciamo")
      ierr = idba_setr(handler, "latmin", 20.0)
      call ensure_no_error("query setf 0")
      ierr = idba_setr(handler, "latmax", 50.0)
      call ensure_no_error("query setf 1")
      ierr = idba_setr(handler, "lonmin",  5.0)
      call ensure_no_error("query setf 2")
      ierr = idba_setr(handler, "lonmax", 20.0)
      call ensure_no_error("query setf 3")
      ierr = idba_voglioquesto(handler, i)
      call ensure_no_error("voglioquesto")

      call ensure("voglioquesto result", i.eq.1)

!     Read the results of the query
!     TODO: make a for loop instead
      do while (i.gt.0)
         ierr = idba_dammelo(handler, param)
         call ensure_no_error("dammelo")
         ierr = idba_enqc(handler, param, cval)
         call ensure_no_error("dammelo enqc 0")
         ierr = idba_enqi(handler, "year", ival)
         call ensure_no_error("dammelo enqi 1")
         ierr = idba_enqr(handler, "lat", rval)
         call ensure_no_error("dammelo enqr 2")
         ierr = idba_enqd(handler, "lon", dval)
         call ensure_no_error("dammelo enqd 3")
         ierr = idba_enqc(handler, "lon", cval)
         call ensure_no_error("dammelo enqc 4")
         ierr = idba_enqdate(handler, i1, i2, i3, i4, i5, i6)
         call ensure_no_error("dammelo enqdate")
         call ensure("enqdate i1", i1.eq.2006)
         call ensure("enqdate i2", i2.eq.1)
         call ensure("enqdate i3", i3.eq.2)
         call ensure("enqdate i4", i4.eq.3)
         call ensure("enqdate i5", i5.eq.4)
         call ensure("enqdate i6", i6.eq.0)
         ierr = idba_enqlevel(handler, i1, i2, i3, i4)
         call ensure_no_error("dammelo enqlevel")
         call ensure("enqlevel i1", i1.eq.1)
         call ensure("enqlevel i2", i2.eq.1)
         call ensure("enqlevel i3", i3.eq.1)
         call ensure("enqlevel i4", i4.eq.1)
         ierr = idba_enqtimerange(handler, i1, i2, i3)
         call ensure_no_error("dammelo enqtimerange")
         call ensure("enqdate i1", i1.eq.20)
         call ensure("enqdate i2", i2.eq.1)
         call ensure("enqdate i3", i3.eq.1)
         
         i = i - 1


!     Insert some data
         ierr = idba_setd(handlew, "lat", 30D00)
         call ensure_no_error("setd 0")
         ierr = idba_setr(handlew, "lon", 10.0)
         call ensure_no_error("setr 1")
         ierr = idba_setc(handlew, "mobile", "0")
         call ensure_no_error("setc 2")

         ierr = idba_seti(handlew, "year", 2007)
         call ensure_no_error("seti 3")
         ierr = idba_seti(handlew, "month", 1)
         call ensure_no_error("seti 4")
         ierr = idba_seti(handlew, "day", 2)
         call ensure_no_error("seti 5")
         ierr = idba_seti(handlew, "hour", 3)
         call ensure_no_error("seti 6")
         ierr = idba_seti(handlew, "min", 4)
         call ensure_no_error("seti 7")
         
         ierr = idba_setc(handlew, "leveltype1", "1")
         call ensure_no_error("setc 8")
         ierr = idba_seti(handlew, "l1", 1)
         call ensure_no_error("seti 9")
         ierr = idba_setc(handlew, "leveltype2", "1")
         call ensure_no_error("seti 10")
         ierr = idba_seti(handlew, "l2", 1)
         call ensure_no_error("seti 11")
         
         ierr = idba_seti(handlew, "pindicator", 20)
         call ensure_no_error("seti 12")
         ierr = idba_seti(handlew, "p1", 1)
         call ensure_no_error("seti 13")
         ierr = idba_seti(handlew, "p2", 1)
         call ensure_no_error("seti 14")
         
         ierr = idba_setc(handlew, "rep_memo", "synop")
         call ensure_no_error("seti 15")
         
         ierr = idba_setc(handlew, "B01011", "DB-All.e!")
         call ensure_no_error("setc 16")
         
                                !     Perform the insert
         ierr = idba_prendilo(handlew)
         call ensure_no_error("first prendilo")


      enddo

      ierr = idba_fatto(handler)
      call ensure_no_error("fatto")

      ierr = idba_fatto(handlew)
      call ensure_no_error("fatto")

! use only in multiple dbhandlers case
!!$      ierr = idba_arrivederci(dbahandlew)
!!$      call ensure_no_error("arrivederci w")

      ierr = idba_arrivederci(dbahandler)
      call ensure_no_error("arrivederci r")

      call exit (0)
    
      end program

      include "check-utils.h"

