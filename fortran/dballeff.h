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
use dballef

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

interface

! Init/Shutdown routines
   
   integer function idba_messages_open_input(handle, filename, mode, format, simplified)
     integer, intent(in) :: handle
     character (len=*), intent(in) :: filename,mode,format
     logical, intent(in) :: simplified
   end function idba_messages_open_input

   integer function idba_messages_open_output(handle, filename, mode, format)
     integer, intent(in) :: handle
     character (len=*), intent(in) :: filename,mode,format
   end function idba_messages_open_output

   integer function idba_messages_read_next(handle, status)
     integer, intent(in) :: handle
     logical, intent(out) :: status
   end function idba_messages_read_next

   integer function idba_messages_write_next(handle, template_name)
     integer, intent(in) :: handle
     character (len=*), intent(in) :: template_name
   end function idba_messages_write_next



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
