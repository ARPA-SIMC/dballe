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

