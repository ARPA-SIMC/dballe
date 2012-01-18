program check_fdballe

! *****************************************
! * Test suite for DBALLE Fortran bindings
! *****************************************

include "dballef.h"

integer :: dbahandle, handle,i,i1,i2,i3,i4,i5,i6,ival,saved_id
integer :: debug,handle_err
real :: rval
double precision :: dval
character (len=10) :: param
character (len=255) :: cval
character (len=160) :: dsn

call getenv("DBA_DB", dsn)
 
if (dsn=="") then
  dsn = "sqlite:tmp.sqlite"
end if
print *,dsn

call idba_error_set_callback(0,idba_default_error_handler,1,handle_err)


!     Open a session
call idba_presentati(dbahandle, dsn, char(0), char(0))
call idba_preparati(dbahandle, handle, "write", "write", "write")
!     Clear the database
call idba_scopa(handle, "") 

!     Insert some data
call idba_seti(handle, "rep_cod", 1)
call idba_setd(handle, "lat", 30D00)
call idba_setr(handle, "lon", 10.0)
call idba_setc(handle, "mobile", "0")
call idba_setcontextana(handle)
call idba_prendilo(handle)

!     Try to read the id of the pseudoana data just inserted
call idba_enqi(handle, "ana_id", saved_id)

call idba_unsetall(handle)


call idba_seti(handle, "ana_id", saved_id)

call idba_seti(handle, "year", 2006)
call idba_seti(handle, "month", 1)
call idba_seti(handle, "day", 2)
call idba_seti(handle, "hour", 3)
call idba_seti(handle, "min", 4)

call idba_setc(handle, "leveltype1", "1")
call idba_seti(handle, "l1", 1)
call idba_setc(handle, "leveltype2", "1")
call idba_seti(handle, "l2", 1)

call idba_seti(handle, "pindicator", 20)
call idba_seti(handle, "p1", 1)
call idba_seti(handle, "p2", 1)
      
call idba_seti(handle, "rep_cod", 1)

call idba_setc(handle, "B01011", "DB-All.e!")

!     Perform the insert
call idba_prendilo(handle)

call idba_unset(handle,"B01011")

call idba_setc(handle, "B01011", "seconda")

!     Perform the insert
call idba_prendilo(handle)

call exit (0)

    
end program check_fdballe


