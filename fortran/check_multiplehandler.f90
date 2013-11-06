      program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

      use dbtest

      implicit none
      include "dballef.h"

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
      call idba_preparati(dbahandler, handler, "read", "read", "read")
      call ensure_no_error("preparati read")

!     Open a session
      call idba_preparati(dbahandlew, handlew, "write", "write", "write")
      call ensure_no_error("preparati write")


!     Clear the database
      call idba_scopa(handlew, "") 
      call ensure_no_error("scopa")

!     Insert some data
      call idba_setd(handlew, "lat", 30D00)
      call ensure_no_error("setd 0")
      call idba_setr(handlew, "lon", 10.0)
      call ensure_no_error("setr 1")
      call idba_setc(handlew, "mobile", "0")
      call ensure_no_error("setc 2")

      call idba_seti(handlew, "year", 2006)
      call ensure_no_error("seti 3")
      call idba_seti(handlew, "month", 1)
      call ensure_no_error("seti 4")
      call idba_seti(handlew, "day", 2)
      call ensure_no_error("seti 5")
      call idba_seti(handlew, "hour", 3)
      call ensure_no_error("seti 6")
      call idba_seti(handlew, "min", 4)
      call ensure_no_error("seti 7")

      call idba_setc(handlew, "leveltype1", "1")
      call ensure_no_error("setc 8")
      call idba_seti(handlew, "l1", 1)
      call ensure_no_error("seti 9")
      call idba_setc(handlew, "leveltype2", "1")
      call ensure_no_error("seti 10")
      call idba_seti(handlew, "l2", 1)
      call ensure_no_error("seti 11")

      call idba_seti(handlew, "pindicator", 20)
      call ensure_no_error("seti 12")
      call idba_seti(handlew, "p1", 1)
      call ensure_no_error("seti 13")
      call idba_seti(handlew, "p2", 1)
      call ensure_no_error("seti 14")
      
      call idba_setc(handlew, "rep_memo", "synop")
      call ensure_no_error("seti 15")

      call idba_setc(handlew, "B01011", "DB-All.e!")
      call ensure_no_error("setc 16")

!     Perform the insert
      call idba_prendilo(handlew)
      call ensure_no_error("first prendilo")

!     Query back the data
!      call idba_ricominciamo(handle)
      call idba_unsetall(handler)
      call ensure_no_error("ricominciamo")
      call idba_setr(handler, "latmin", 20.0)
      call ensure_no_error("query setf 0")
      call idba_setr(handler, "latmax", 50.0)
      call ensure_no_error("query setf 1")
      call idba_setr(handler, "lonmin",  5.0)
      call ensure_no_error("query setf 2")
      call idba_setr(handler, "lonmax", 20.0)
      call ensure_no_error("query setf 3")
      call idba_voglioquesto(handler, i)
      call ensure_no_error("voglioquesto")

      call ensure("voglioquesto result", i.eq.1)

!     Read the results of the query
!     TODO: make a for loop instead
      do while (i.gt.0)
         call idba_dammelo(handler, param)
         call ensure_no_error("dammelo")
         call idba_enqc(handler, param, cval)
         call ensure_no_error("dammelo enqc 0")
         call idba_enqi(handler, "year", ival)
         call ensure_no_error("dammelo enqi 1")
         call idba_enqr(handler, "lat", rval)
         call ensure_no_error("dammelo enqr 2")
         call idba_enqd(handler, "lon", dval)
         call ensure_no_error("dammelo enqd 3")
         call idba_enqc(handler, "lon", cval)
         call ensure_no_error("dammelo enqc 4")
         call idba_enqdate(handler, i1, i2, i3, i4, i5, i6)
         call ensure_no_error("dammelo enqdate")
         call ensure("enqdate i1", i1.eq.2006)
         call ensure("enqdate i2", i2.eq.1)
         call ensure("enqdate i3", i3.eq.2)
         call ensure("enqdate i4", i4.eq.3)
         call ensure("enqdate i5", i5.eq.4)
         call ensure("enqdate i6", i6.eq.0)
         call idba_enqlevel(handler, i1, i2, i3, i4)
         call ensure_no_error("dammelo enqlevel")
         call ensure("enqlevel i1", i1.eq.1)
         call ensure("enqlevel i2", i2.eq.1)
         call ensure("enqlevel i3", i3.eq.1)
         call ensure("enqlevel i4", i4.eq.1)
         call idba_enqtimerange(handler, i1, i2, i3)
         call ensure_no_error("dammelo enqtimerange")
         call ensure("enqdate i1", i1.eq.20)
         call ensure("enqdate i2", i2.eq.1)
         call ensure("enqdate i3", i3.eq.1)
         
         i = i - 1


!     Insert some data
         call idba_setd(handlew, "lat", 30D00)
         call ensure_no_error("setd 0")
         call idba_setr(handlew, "lon", 10.0)
         call ensure_no_error("setr 1")
         call idba_setc(handlew, "mobile", "0")
         call ensure_no_error("setc 2")

         call idba_seti(handlew, "year", 2007)
         call ensure_no_error("seti 3")
         call idba_seti(handlew, "month", 1)
         call ensure_no_error("seti 4")
         call idba_seti(handlew, "day", 2)
         call ensure_no_error("seti 5")
         call idba_seti(handlew, "hour", 3)
         call ensure_no_error("seti 6")
         call idba_seti(handlew, "min", 4)
         call ensure_no_error("seti 7")
         
         call idba_setc(handlew, "leveltype1", "1")
         call ensure_no_error("setc 8")
         call idba_seti(handlew, "l1", 1)
         call ensure_no_error("seti 9")
         call idba_setc(handlew, "leveltype2", "1")
         call ensure_no_error("seti 10")
         call idba_seti(handlew, "l2", 1)
         call ensure_no_error("seti 11")
         
         call idba_seti(handlew, "pindicator", 20)
         call ensure_no_error("seti 12")
         call idba_seti(handlew, "p1", 1)
         call ensure_no_error("seti 13")
         call idba_seti(handlew, "p2", 1)
         call ensure_no_error("seti 14")
         
         call idba_setc(handlew, "rep_memo", "synop")
         call ensure_no_error("seti 15")
         
         call idba_setc(handlew, "B01011", "DB-All.e!")
         call ensure_no_error("setc 16")
         
                                !     Perform the insert
         call idba_prendilo(handlew)
         call ensure_no_error("first prendilo")


      enddo

      call idba_fatto(handlew)
      call ensure_no_error("fatto")

      call idba_fatto(handler)
      call ensure_no_error("fatto")

! use only in multiple dbhandlers case
!!$      call idba_arrivederci(dbahandlew)
!!$      call ensure_no_error("arrivederci w")

      call idba_arrivederci(dbahandler)
      call ensure_no_error("arrivederci r")


!     If we made it so far, exit with no error
      print*,"check_fdballe: all tests succeed."

      call exit (0)
    
      end program

      include "check-utils.h"

