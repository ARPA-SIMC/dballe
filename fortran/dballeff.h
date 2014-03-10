 !
 ! Interface file for DB-ALLe
 !
 ! Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
 !
 ! This program is free software; you can redistribute it and/or modify
 ! it under the terms of the GNU General Public License as published by
 ! the Free Software Foundation; either version 2 of the License.
 !
 ! This program is distributed in the hope that it will be useful,
 ! but WITHOUT ANY WARRANTY; without even the implied warranty of
 ! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ! GNU General Public License for more details.
 !
 ! You should have received a copy of the GNU General Public License
 ! along with this program; if not, write to the Free Software
 ! Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 !
 ! Author: Enrico Zini <enrico@enricozini.com>
 !

! missing value

INTEGER, PARAMETER :: &
dba_int_b    = SELECTED_INT_KIND(1), & ! Byte  integer
dba_int_s    = SELECTED_INT_KIND(4), & ! Short integer
dba_int_l    = SELECTED_INT_KIND(8)    ! Long  integer

INTEGER, PARAMETER :: &
dba_fp_s = SELECTED_REAL_KIND(6), & ! Single precision
dba_fp_d = SELECTED_REAL_KIND(15)   ! Double precision


REAL, PARAMETER :: DBA_MVR = HUGE(1.0)
!REAL(dba_kind=dba_fp_s), PARAMETER ::  = HUGE(1.0_dba_fp_s)
REAL(kind=dba_fp_d), PARAMETER :: DBA_MVD = HUGE(1.0_dba_fp_d)
!REAL(kind=dba_fp_d), PARAMETER :: DBA_MVD = 1.79769D308 
INTEGER, PARAMETER :: DBA_MVI = HUGE(0)
INTEGER(kind=dba_int_b), PARAMETER :: DBA_MVB = HUGE(0_dba_int_b)
CHARACTER(len=1), PARAMETER :: DBA_MVC = char(0)


! Get/Set routines

interface idba_enq

   integer function idba_enqb(handle,param,value)
     integer, intent(in) :: handle
     integer (kind=1),intent(out) :: value
     character (len=*), intent(in) :: param
   end function idba_enqb
   
   integer function idba_enqi(handle,param,value)
     integer, intent(in) :: handle
     integer, intent(out) :: value
     character (len=*), intent(in) :: param
   end function idba_enqi
   
   integer function idba_enqr(handle,param,value)
     integer, intent(in) :: handle
     real, intent(out) :: value
     character (len=*), intent(in) :: param
   end function idba_enqr
   
   integer function idba_enqd(handle,param,value)
     integer, intent(in) :: handle
     double precision, intent(out) :: value
     character (len=*), intent(in) :: param
   end function idba_enqd
   
   integer function idba_enqc(handle,param,value)
     integer, intent(in) :: handle
     character (len=*), intent(in) :: param
     character (len=*), intent(out) :: value
   end function idba_enqc
     
end interface

  
interface idba_set
     
   integer function idba_setb(handle,param,value)
     integer, intent(in) :: handle
     integer (kind=1), intent(in) :: value
     character (len=*), intent(in) :: param
   end function idba_setb
   
   integer function idba_seti(handle,param,value)
     integer, intent(in) :: handle,value
     character (len=*), intent(in) :: param
   end function idba_seti
  
   integer function idba_setr(handle,param,value)
     integer, intent(in) :: handle
     real, intent(in) :: value
     character (len=*), intent(in) :: param
   end function idba_setr

   integer function idba_setd(handle,param,value)
     integer, intent(in) :: handle
     double precision, intent(in) :: value
     character (len=*), intent(in) :: param
   end function idba_setd

   integer function idba_setc(handle,param,value)
     integer, intent(in) :: handle
     character (len=*), intent(in) :: param,value
   end function idba_setc


end interface


interface

! Error handling routines

   subroutine idba_error_message(message)
     character (len=*), intent(out) :: message
   end subroutine idba_error_message

   subroutine idba_error_context(message)
     character (len=*), intent(out) :: message
   end subroutine idba_error_context

   subroutine idba_error_details(message)
     character (len=*), intent(out) :: message
   end subroutine idba_error_details

   integer function idba_error_code()
   end function idba_error_code
   
   integer function idba_error_set_callback(code,func,data,handle)
     integer, intent(in) :: code
     external :: func
     integer, intent(in) :: data
     integer, intent(out) :: handle
   end function idba_error_set_callback

   integer function idba_error_remove_callback(handle)
     integer, intent(in) :: handle
   end function idba_error_remove_callback

   integer function idba_default_error_handler(debug)
     logical, intent(in) :: debug
   end function idba_default_error_handler

   integer function idba_error_handler_tolerating_overflows(debug)
     logical, intent(in) :: debug
   end function idba_error_handler_tolerating_overflows


! Init/Shutdown routines
   
   integer function idba_presentati(dbahandle, dsn, user, password)
     integer, intent(out) :: dbahandle
     character (len=*), intent(in) :: dsn,user,password
   end function idba_presentati

   integer function idba_arrivederci(dbahandle)
     integer, intent(in) :: dbahandle
   end function idba_arrivederci

   integer function idba_preparati(dbahandle, handle, anaflag, dataflag, attrflag)
     integer, intent(in) :: dbahandle
     integer, intent(out) :: handle
     character (len=*), intent(in) :: anaflag,dataflag,attrflag
   end function idba_preparati

   integer function idba_messaggi(handle, filename, mode, type)
     integer, intent(out) :: handle
     character (len=*), intent(in) :: filename,mode,type
   end function idba_messaggi

   integer function idba_messages_open_input(handle, filename, mode, format, simplified)
     integer, intent(in) :: handle
     character (len=*), intent(in) :: filename,mode,format
     logical, intent(in) :: simplified
   end function idba_messages_open_input

   integer function idba_messages_read_next(handle, status)
     integer, intent(in) :: handle
     logical, intent(out) :: status
   end function idba_messages_read_next

   integer function idba_fatto(handle)
     integer, intent(in) :: handle
   end function idba_fatto



   integer function idba_unset(handle,param)
     integer, intent(in) :: handle
     character (len=*), intent(in) :: param
   end function idba_unset

   integer function idba_unsetall(handle)
     integer, intent(in) :: handle
   end function idba_unsetall

   integer function idba_remove_all(handle)
     integer, intent(in) :: handle
   end function idba_remove_all

   integer function idba_unsetb(handle)
     integer, intent(in) :: handle
   end function idba_unsetb

   integer function idba_setcontextana(handle)
     integer, intent(in) :: handle
   end function idba_setcontextana


   integer function idba_enqlevel(handle,ltype1,l1,ltype2,l2)
     integer, intent(in) :: handle
     integer, intent(out) :: ltype1,l1,ltype2,l2
   end function idba_enqlevel

   integer function idba_setlevel(handle,ltype1,l1,ltype2,l2)
     integer, intent(in) :: handle,ltype1,l1,ltype2,l2
   end function idba_setlevel


   integer function idba_enqtimerange(handle,ptype,p1,p2)
     integer, intent(in) :: handle
     integer, intent(out) :: ptype,p1,p2
   end function idba_enqtimerange

   integer function idba_settimerange(handle,ptype,p1,p2)
     integer, intent(in) :: handle,ptype,p1,p2
   end function idba_settimerange


   integer function idba_enqdate(handle,year,month,day,hour,min,sec)
     integer, intent(in) :: handle
     integer, intent(out) :: year,month,day,hour,min,sec
   end function idba_enqdate

   integer function idba_setdate(handle,year,month,day,hour,min,sec)
     integer, intent(in) :: handle,year,month,day,hour,min,sec
   end function idba_setdate

   integer function idba_setdatemin(handle,year,month,day,hour,min,sec)
     integer, intent(in) :: handle,year,month,day,hour,min,sec
   end function idba_setdatemin

   integer function idba_setdatemax(handle,year,month,day,hour,min,sec)
     integer, intent(in) :: handle,year,month,day,hour,min,sec
   end function idba_setdatemax


! Action routines

   integer function idba_scopa(handle, repinfofile)
     integer, intent(in) :: handle
     character (len=*), intent(in) :: repinfofile
   end function idba_scopa


   integer function idba_quantesono(handle, count)
     integer, intent(in) :: handle
     integer, intent(out) :: count
   end function idba_quantesono

   integer function idba_elencamele(handle)
     integer, intent(in) :: handle
   end function idba_elencamele


   integer function idba_voglioquesto(handle, count)
     integer, intent(in) :: handle
     integer, intent(out) :: count
   end function idba_voglioquesto

   integer function idba_dammelo(handle,param)
     integer, intent(in) :: handle
     character (len=*), intent(out) :: param
   end function idba_dammelo

   integer function idba_prendilo(handle)
     integer, intent(in) :: handle
   end function idba_prendilo

   integer function idba_dimenticami(handle)
     integer, intent(in) :: handle
   end function idba_dimenticami


   integer function idba_voglioancora(handle, count)
     integer, intent(in) :: handle
     integer, intent(out) :: count
   end function idba_voglioancora

   integer function idba_ancora(handle,param)
     integer, intent(in) :: handle
     character (len=*), intent(out) :: param
   end function idba_ancora

   integer function idba_critica(handle)
     integer, intent(in) :: handle
   end function idba_critica

   integer function idba_scusa(handle)
     integer, intent(in) :: handle
   end function idba_scusa


! Pretty printing routines

   integer function idba_spiegal(handle,ltype1,l1,ltype2,l2,result)
     integer, intent(in) :: handle,ltype1,l1,ltype2,l2
     character (len=*), intent(out) :: result
   end function idba_spiegal

   integer function idba_spiegat(handle,ptype,p1,p2,result)
     integer, intent(in) :: handle,ptype,p1,p2
     character (len=*), intent(out) :: result
   end function idba_spiegat

   integer function idba_spiegab(handle,varcode,var,result)
     integer, intent(in) :: handle
     character (len=*), intent(in) :: varcode,var
     character (len=*), intent(out) :: result
   end function idba_spiegab

end interface
