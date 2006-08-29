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

interface


! Init/Shutdown routines

subroutine idba_presentati(dbahandle, dsn, user, password)
	integer, intent(out) :: dbahandle
	character (len=*), intent(in) :: dsn,user,password
end subroutine idba_presentati

subroutine idba_arrivederci(dbahandle)
	integer, intent(in) :: dbahandle
end subroutine idba_arrivederci

subroutine idba_preparati(dbahandle, handle, anaflag, dataflag, attrflag)
	integer, intent(in) :: dbahandle
	integer, intent(out) :: handle
	character (len=*), intent(in) :: anaflag,dataflag,attrflag
end subroutine idba_preparati

subroutine idba_fatto(handle)
	integer, intent(in) :: handle
end subroutine idba_fatto


! Get/Set routines

subroutine idba_enqi(handle,param,value)
       integer, intent(in) :: handle
       integer, intent(out) :: value
       character (len=*), intent(in) :: param
end subroutine idba_enqi

subroutine idba_seti(handle,param,value)
       integer, intent(in) :: handle,value
       character (len=*), intent(in) :: param
end subroutine idba_seti


subroutine idba_enqr(handle,param,value)
       integer, intent(in) :: handle
       real, intent(out) :: value
       character (len=*), intent(in) :: param
end subroutine idba_enqr

subroutine idba_setr(handle,param,value)
       integer, intent(in) :: handle
       real, intent(in) :: value
       character (len=*), intent(in) :: param
end subroutine idba_setr


subroutine idba_enqd(handle,param,value)
       integer, intent(in) :: handle
       double precision, intent(out) :: value
       character (len=*), intent(in) :: param
end subroutine idba_enqd

subroutine idba_setd(handle,param,value)
       integer, intent(in) :: handle
       double precision, intent(in) :: value
       character (len=*), intent(in) :: param
end subroutine idba_setd


subroutine idba_enqc(handle,param,value)
       integer, intent(in) :: handle
       character (len=*), intent(out) :: param,value
end subroutine idba_enqc

subroutine idba_setc(handle,param,value)
       integer, intent(in) :: handle
       character (len=*), intent(in) :: param,value
end subroutine idba_setc


subroutine idba_unset(handle,param)
       integer, intent(in) :: handle
       character (len=*), intent(in) :: param
end subroutine idba_unset

subroutine idba_unsetall(handle)
       integer, intent(in) :: handle
end subroutine idba_unsetall


subroutine idba_setcontextana(handle)
       integer, intent(in) :: handle
end subroutine idba_setcontextana


subroutine idba_enqlevel(handle,ltype,l1,l2)
       integer, intent(in) :: handle
       integer, intent(out) :: ltype,l1,l2
end subroutine idba_enqlevel

subroutine idba_setlevel(handle,ltype,l1,l2)
       integer, intent(in) :: handle,ltype,l1,l2
end subroutine idba_setlevel


subroutine idba_enqtimerange(handle,ptype,p1,p2)
       integer, intent(in) :: handle
       integer, intent(out) :: ptype,p1,p2
end subroutine idba_enqtimerange

subroutine idba_settimerange(handle,ptype,p1,p2)
       integer, intent(in) :: handle,ptype,p1,p2
end subroutine idba_settimerange


subroutine idba_enqdate(handle,year,month,day,hour,min,sec)
       integer, intent(in) :: handle
       integer, intent(out) :: year,month,day,hour,min,sec
end subroutine idba_enqdate

subroutine idba_setdate(handle,year,month,day,hour,min,sec)
       integer, intent(in) :: handle,year,month,day,hour,min,sec
end subroutine idba_setdate

subroutine idba_setdatemin(handle,year,month,day,hour,min,sec)
       integer, intent(in) :: handle,year,month,day,hour,min,sec
end subroutine idba_setdatemin

subroutine idba_setdatemax(handle,year,month,day,hour,min,sec)
       integer, intent(in) :: handle,year,month,day,hour,min,sec
end subroutine idba_setdatemax


! Action routines

subroutine idba_scopa(handle, repinfofile)
	integer, intent(in) :: handle
	character (len=*), intent(in) :: repinfofile
end subroutine idba_scopa


subroutine idba_quantesono(handle, count)
	integer, intent(in) :: handle
	integer, intent(out) :: count
end subroutine idba_quantesono

subroutine idba_elencamele(handle)
	integer, intent(in) :: handle
end subroutine idba_elencamele


subroutine idba_voglioquesto(handle, count)
	integer, intent(in) :: handle
	integer, intent(out) :: count
end subroutine idba_voglioquesto

subroutine idba_dammelo(handle,param)
	integer, intent(in) :: handle
	character (len=*), intent(out) :: param
end subroutine idba_dammelo

subroutine idba_prendilo(handle)
	integer, intent(in) :: handle
end subroutine idba_prendilo

subroutine idba_dimenticami(handle)
	integer, intent(in) :: handle
end subroutine idba_dimenticami


subroutine idba_voglioancora(handle, count)
	integer, intent(in) :: handle
	integer, intent(out) :: count
end subroutine idba_voglioancora

subroutine idba_ancora(handle,param)
	integer, intent(in) :: handle
	character (len=*), intent(out) :: param
end subroutine idba_ancora

subroutine idba_critica(handle)
	integer, intent(in) :: handle
end subroutine idba_critica

subroutine idba_scusa(handle)
	integer, intent(in) :: handle
end subroutine idba_scusa


end interface
