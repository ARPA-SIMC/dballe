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
MODULE dballef
USE,INTRINSIC :: iso_c_binding
IMPLICIT NONE

! TODO:
! - check the missing value constants
! - decide whether the _orig interfaces should become private
! - restore the intent's in/out

! definition of missing values
INTEGER, PARAMETER :: &
 dba_int_b = c_signed_char, & ! Byte  integer
 dba_int_i = c_int     ! Integer

INTEGER, PARAMETER :: &
 dba_fp_s = c_float, & ! Single precision
 dba_fp_d = c_double   ! Double precision

REAL(kind=dba_fp_s), PARAMETER :: DBA_MVR = HUGE(1.0_dba_fp_s)
REAL(kind=dba_fp_d), PARAMETER :: DBA_MVD = HUGE(1.0_dba_fp_d)
INTEGER(kind=dba_int_b), PARAMETER :: DBA_MVB = HUGE(0_dba_int_b)
INTEGER(kind=dba_int_i), PARAMETER :: DBA_MVI = HUGE(0_dba_int_i)
CHARACTER(kind=c_char,len=1), PARAMETER :: DBA_MVC = CHAR(0)


! interface to generic dba functions
INTERFACE
  FUNCTION idba_presentati_orig(dbahandle, url) BIND(C,name='idba_presentati')
  IMPORT
  INTEGER(kind=c_int) :: dbahandle
  CHARACTER(kind=c_char) :: url(*)
  INTEGER(kind=c_int) :: idba_presentati_orig
  END FUNCTION idba_presentati_orig
END INTERFACE

INTERFACE
  FUNCTION idba_arrivederci(dbahandle) BIND(C,name='idba_arrivederci')
  IMPORT
  INTEGER(kind=c_int) :: dbahandle
  END FUNCTION idba_arrivederci
END INTERFACE

INTERFACE
  FUNCTION idba_preparati_orig(dbahandle, handle, anaflag, dataflag, attrflag) BIND(C,name='idba_preparati')
  IMPORT
  INTEGER(kind=c_int),VALUE :: dbahandle
  INTEGER(kind=c_int) :: handle
  CHARACTER(kind=c_char) :: anaflag(*)
  CHARACTER(kind=c_char) :: dataflag(*)
  CHARACTER(kind=c_char) :: attrflag(*)
  INTEGER(kind=c_int) :: idba_preparati_orig
  END FUNCTION idba_preparati_orig
END INTERFACE

INTERFACE
  FUNCTION idba_messaggi_orig(handle, filename, mode, typ) BIND(C,name='idba_messaggi')
  IMPORT
  INTEGER(kind=c_int) :: handle
  CHARACTER(kind=c_char) :: filename(*)
  CHARACTER(kind=c_char) :: mode(*)
  CHARACTER(kind=c_char) :: typ(*)
  INTEGER(kind=c_int) :: idba_messaggi_orig
  END FUNCTION idba_messaggi_orig
END INTERFACE

INTERFACE
  FUNCTION idba_fatto(dbahandle) BIND(C,name='idba_fatto')
  IMPORT
  INTEGER(kind=c_int) :: dbahandle
  INTEGER(kind=c_int) :: idba_fatto
  END FUNCTION idba_fatto
END INTERFACE

INTERFACE
  FUNCTION idba_remove_all(handle) BIND(C,name='idba_remove_all')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: idba_remove_all
  END FUNCTION idba_remove_all
END INTERFACE

INTERFACE
  FUNCTION idba_seti_orig(handle, param, val) BIND(C,name='idba_seti')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  INTEGER(kind=c_int) :: val
  INTEGER(kind=c_int) :: idba_seti_orig
  END FUNCTION idba_seti_orig
END INTERFACE
  
INTERFACE
  FUNCTION idba_setb_orig(handle, param, val) BIND(C,name='idba_setb')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  INTEGER(kind=c_signed_char) :: val
  INTEGER(kind=c_int) :: idba_setb_orig
  END FUNCTION idba_setb_orig
END INTERFACE
  
INTERFACE
  FUNCTION idba_setr_orig(handle, param, val) BIND(C,name='idba_setr')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  REAL(kind=c_float) :: val
  INTEGER(kind=c_int) :: idba_setr_orig
  END FUNCTION idba_setr_orig
END INTERFACE
  
INTERFACE
  FUNCTION idba_setd_orig(handle, param, val) BIND(C,name='idba_setd')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  REAL(kind=c_double) :: val
  INTEGER(kind=c_int) :: idba_setd_orig
  END FUNCTION idba_setd_orig
END INTERFACE

INTERFACE
  FUNCTION idba_setc_orig(handle, param, val) BIND(C,name='idba_setc')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  CHARACTER(kind=c_char) :: val(*)
  INTEGER(kind=c_int) :: idba_setc_orig
  END FUNCTION idba_setc_orig
END INTERFACE

INTERFACE idba_set
  MODULE PROCEDURE idba_seti, idba_setb, idba_setr, idba_setd, idba_setc
END INTERFACE idba_set

INTERFACE
  FUNCTION idba_enqi_orig(handle, param, val) BIND(C,name='idba_enqi')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  INTEGER(kind=c_int) :: val
  INTEGER(kind=c_int) :: idba_enqi_orig
  END FUNCTION idba_enqi_orig
END INTERFACE
  
INTERFACE
  FUNCTION idba_enqb_orig(handle, param, val) BIND(C,name='idba_enqb')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  INTEGER(kind=c_signed_char) :: val
  INTEGER(kind=c_int) :: idba_enqb_orig
  END FUNCTION idba_enqb_orig
END INTERFACE
  
INTERFACE
  FUNCTION idba_enqr_orig(handle, param, val) BIND(C,name='idba_enqr')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  REAL(kind=c_float) :: val
  INTEGER(kind=c_int) :: idba_enqr_orig
  END FUNCTION idba_enqr_orig
END INTERFACE
  
INTERFACE
  FUNCTION idba_enqd_orig(handle, param, val) BIND(C,name='idba_enqd')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  REAL(kind=c_double) :: val
  INTEGER(kind=c_int) :: idba_enqd_orig
  END FUNCTION idba_enqd_orig
END INTERFACE

INTERFACE
  FUNCTION idba_enqc_orig(handle, param, val, val_len) BIND(C,name='idba_enqc')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  CHARACTER(kind=c_char) :: val(*)
  INTEGER(kind=c_int),VALUE :: val_len
  INTEGER(kind=c_int) :: idba_enqc_orig
  END FUNCTION idba_enqc_orig
END INTERFACE

INTERFACE idba_enq
  MODULE PROCEDURE idba_enqi, idba_enqb, idba_enqr, idba_enqd, idba_enqc
END INTERFACE idba_enq

INTERFACE
  FUNCTION idba_unset_orig(handle, param) BIND(C,name='idba_unset')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  INTEGER(kind=c_int) :: idba_unset_orig
  END FUNCTION idba_unset_orig
END INTERFACE

INTERFACE
  FUNCTION idba_unsetb(handle) BIND(C,name='idba_unsetb')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: idba_unsetb
  END FUNCTION idba_unsetb
END INTERFACE

INTERFACE
  FUNCTION idba_unsetall(handle) BIND(C,name='idba_unsetall')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: idba_unsetall
  END FUNCTION idba_unsetall
END INTERFACE

INTERFACE
  FUNCTION idba_setcontextana(handle) BIND(C,name='idba_setcontextana')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: idba_setcontextana
  END FUNCTION idba_setcontextana
END INTERFACE

INTERFACE
  FUNCTION idba_setlevel(handle, ltype1, l1, ltype2, l2) BIND(C,name='idba_setlevel')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),VALUE :: ltype1
  INTEGER(kind=c_int),VALUE :: l1
  INTEGER(kind=c_int),VALUE :: ltype2
  INTEGER(kind=c_int),VALUE :: l2
  INTEGER(kind=c_int) :: idba_setlevel
  END FUNCTION idba_setlevel
END INTERFACE

INTERFACE
  FUNCTION idba_settimerange(handle, ptype, p1, p2) BIND(C,name='idba_settimerange')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),VALUE :: ptype
  INTEGER(kind=c_int),VALUE :: p1
  INTEGER(kind=c_int),VALUE :: p2
  INTEGER(kind=c_int) :: idba_settimerange
  END FUNCTION idba_settimerange
END INTERFACE

INTERFACE
  FUNCTION idba_setdate(handle, year, month, day, hour, minute, second) BIND(C,name='idba_setdate')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),VALUE :: year
  INTEGER(kind=c_int),VALUE :: month
  INTEGER(kind=c_int),VALUE :: day
  INTEGER(kind=c_int),VALUE :: hour
  INTEGER(kind=c_int),VALUE :: minute
  INTEGER(kind=c_int),VALUE :: second
  INTEGER(kind=c_int) :: idba_setdate
  END FUNCTION idba_setdate
END INTERFACE

INTERFACE
  FUNCTION idba_setdatemin(handle, year, month, day, hour, minute, second) BIND(C,name='idba_setdatemin')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),VALUE :: year
  INTEGER(kind=c_int),VALUE :: month
  INTEGER(kind=c_int),VALUE :: day
  INTEGER(kind=c_int),VALUE :: hour
  INTEGER(kind=c_int),VALUE :: minute
  INTEGER(kind=c_int),VALUE :: second
  INTEGER(kind=c_int) :: idba_setdatemin
  END FUNCTION idba_setdatemin
END INTERFACE

INTERFACE
  FUNCTION idba_setdatemax(handle, year, month, day, hour, minute, second) BIND(C,name='idba_setdatemax')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),VALUE :: year
  INTEGER(kind=c_int),VALUE :: month
  INTEGER(kind=c_int),VALUE :: day
  INTEGER(kind=c_int),VALUE :: hour
  INTEGER(kind=c_int),VALUE :: minute
  INTEGER(kind=c_int),VALUE :: second
  INTEGER(kind=c_int) :: idba_setdatemax
  END FUNCTION idba_setdatemax
END INTERFACE

INTERFACE
  FUNCTION idba_enqlevel(handle, ltype1, l1, ltype2, l2) BIND(C,name='idba_enqlevel')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: ltype1
  INTEGER(kind=c_int) :: l1
  INTEGER(kind=c_int) :: ltype2
  INTEGER(kind=c_int) :: l2
  INTEGER(kind=c_int) :: idba_enqlevel
  END FUNCTION idba_enqlevel
END INTERFACE

INTERFACE
  FUNCTION idba_enqtimerange(handle, ptype, p1, p2) BIND(C,name='idba_enqtimerange')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: ptype
  INTEGER(kind=c_int) :: p1
  INTEGER(kind=c_int) :: p2
  INTEGER(kind=c_int) :: idba_enqtimerange
  END FUNCTION idba_enqtimerange
END INTERFACE

INTERFACE
  FUNCTION idba_enqdate(handle, year, month, day, hour, minute, second) BIND(C,name='idba_enqdate')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: year
  INTEGER(kind=c_int) :: month
  INTEGER(kind=c_int) :: day
  INTEGER(kind=c_int) :: hour
  INTEGER(kind=c_int) :: minute
  INTEGER(kind=c_int) :: second
  INTEGER(kind=c_int) :: idba_enqdate
  END FUNCTION idba_enqdate
END INTERFACE


! interface to action functions
INTERFACE
  FUNCTION idba_scopa_orig(handle, repinfofile) BIND(C,name='idba_scopa')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: repinfofile(*)
  INTEGER(kind=c_int) :: idba_scopa_orig
  END FUNCTION idba_scopa_orig
END INTERFACE

INTERFACE
  FUNCTION idba_quantesono(handle, count) BIND(C,name='idba_quantesono')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),INTENT(out) :: count
  INTEGER(kind=c_int) :: idba_quantesono
  END FUNCTION idba_quantesono
END INTERFACE

INTERFACE
  FUNCTION idba_elencamele(handle) BIND(C,name='idba_elencamele')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: idba_elencamele
  END FUNCTION idba_elencamele
END INTERFACE

INTERFACE
  FUNCTION idba_voglioquesto(handle, count) BIND(C,name='idba_voglioquesto')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),INTENT(out) :: count
  INTEGER(kind=c_int) :: idba_voglioquesto
  END FUNCTION idba_voglioquesto
END INTERFACE

INTERFACE
  FUNCTION idba_dammelo_orig(handle, param, param_len) BIND(C,name='idba_dammelo')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  INTEGER(kind=c_int),VALUE :: param_len
  INTEGER(kind=c_int) :: idba_dammelo_orig
  END FUNCTION idba_dammelo_orig
END INTERFACE

INTERFACE
  FUNCTION idba_prendilo(handle) BIND(C,name='idba_prendilo')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: idba_prendilo
  END FUNCTION idba_prendilo
END INTERFACE

INTERFACE
  FUNCTION idba_dimenticami(handle) BIND(C,name='idba_dimenticami')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: idba_dimenticami
  END FUNCTION idba_dimenticami
END INTERFACE

INTERFACE
  FUNCTION idba_voglioancora(handle, count) BIND(C,name='idba_voglioancora')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),INTENT(out) :: count
  INTEGER(kind=c_int) :: idba_voglioancora
  END FUNCTION idba_voglioancora
END INTERFACE

INTERFACE
  FUNCTION idba_ancora_orig(handle, param, param_len) BIND(C,name='idba_ancora')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: param(*)
  INTEGER(kind=c_int),VALUE :: param_len
  INTEGER(kind=c_int) :: idba_ancora_orig
  END FUNCTION idba_ancora_orig
END INTERFACE

INTERFACE
  FUNCTION idba_critica(handle) BIND(C,name='idba_critica')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: idba_critica
  END FUNCTION idba_critica
END INTERFACE

INTERFACE
  FUNCTION idba_scusa(handle) BIND(C,name='idba_scusa')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int) :: idba_scusa
  END FUNCTION idba_scusa
END INTERFACE

INTERFACE
  FUNCTION idba_messages_open_input_orig(handle, filename, mode, form, simplified) BIND(C,name='idba_messages_open_input')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: filename(*)
  CHARACTER(kind=c_char) :: mode(*)
  CHARACTER(kind=c_char) :: form(*)
  INTEGER(kind=c_int),VALUE :: simplified
  INTEGER(kind=c_int) :: idba_messages_open_input_orig
  END FUNCTION idba_messages_open_input_orig
END INTERFACE

INTERFACE
  FUNCTION idba_messages_open_output_orig(handle, filename, mode, form) BIND(C,name='idba_messages_open_output')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: filename(*)
  CHARACTER(kind=c_char) :: mode(*)
  CHARACTER(kind=c_char) :: form(*)
  INTEGER(kind=c_int) :: idba_messages_open_output_orig
  END FUNCTION idba_messages_open_output_orig
END INTERFACE

INTERFACE
  FUNCTION idba_messages_read_next_orig(handle, found) BIND(C,name='idba_messages_read_next')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),INTENT(out) :: found
  INTEGER(kind=c_int) :: idba_messages_read_next_orig
  END FUNCTION idba_messages_read_next_orig
END INTERFACE

INTERFACE
  FUNCTION idba_messages_write_next_orig(handle, template_name) BIND(C,name='idba_messages_write_next')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: template_name(*)
  INTEGER(kind=c_int) :: idba_messages_write_next_orig
  END FUNCTION idba_messages_write_next_orig
END INTERFACE


! interface to pretty-printing functions
INTERFACE
  FUNCTION idba_spiegal_orig(handle, ltype1, l1, ltype2, l2, res, res_len) BIND(C,name='idba_spiegal')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),VALUE :: ltype1
  INTEGER(kind=c_int),VALUE :: l1
  INTEGER(kind=c_int),VALUE :: ltype2
  INTEGER(kind=c_int),VALUE :: l2
  CHARACTER(kind=c_char) :: res(*)
  INTEGER(kind=c_int),VALUE :: res_len
  INTEGER(kind=c_int) :: idba_spiegal_orig
  END FUNCTION idba_spiegal_orig
END INTERFACE

INTERFACE
  FUNCTION idba_spiegat_orig(handle, ptype, p1, p2, res, res_len) BIND(C,name='idba_spiegat')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  INTEGER(kind=c_int),VALUE :: ptype
  INTEGER(kind=c_int),VALUE :: p1
  INTEGER(kind=c_int),VALUE :: p2
  CHARACTER(kind=c_char) :: res(*)
  INTEGER(kind=c_int),VALUE :: res_len
  INTEGER(kind=c_int) :: idba_spiegat_orig
  END FUNCTION idba_spiegat_orig
END INTERFACE

INTERFACE
  FUNCTION idba_spiegab_orig(handle, varcode, var, res, res_len) BIND(C,name='idba_spiegab')
  IMPORT
  INTEGER(kind=c_int),VALUE :: handle
  CHARACTER(kind=c_char) :: varcode(*)
  CHARACTER(kind=c_char) :: var(*)
  CHARACTER(kind=c_char) :: res(*)
  INTEGER(kind=c_int),VALUE :: res_len
  INTEGER(kind=c_int) :: idba_spiegab_orig
  END FUNCTION idba_spiegab_orig
END INTERFACE

INTERFACE
  FUNCTION idba_test_input_to_output(dbahandle) BIND(C,name='idba_test_input_to_output')
  IMPORT
  INTEGER(kind=c_int),VALUE :: dbahandle
  INTEGER(kind=c_int) :: idba_test_input_to_output
  END FUNCTION idba_test_input_to_output
END INTERFACE


! interfaces to error handling functions
INTERFACE
  FUNCTION idba_error_set_callback(code, func, dat, handle) BIND(C,name='idba_error_set_callback')
  IMPORT
  INTEGER(kind=c_int),VALUE :: code
  TYPE(c_funptr),VALUE :: func
  INTEGER(kind=c_int),VALUE :: dat
  INTEGER(kind=c_int) :: handle
  END FUNCTION idba_error_set_callback
END INTERFACE

INTERFACE
  FUNCTION idba_default_error_handler(debug) BIND(C,name='idba_default_error_handler')
  IMPORT
  INTEGER(kind=c_int) :: debug
  END FUNCTION idba_default_error_handler
END INTERFACE

INTERFACE
  FUNCTION idba_default_error_handle_tolerating_overflows(debug) BIND(C,name='idba_default_error_handle_tolerating_overflows')
  IMPORT
  INTEGER(kind=c_int) :: debug
  END FUNCTION idba_default_error_handle_tolerating_overflows
END INTERFACE

INTERFACE
  SUBROUTINE idba_error_message_orig(message, message_len) BIND(C,name='idba_error_message')
  IMPORT
  CHARACTER(kind=c_char) :: message(*)
  INTEGER(kind=c_int),VALUE :: message_len
  END SUBROUTINE idba_error_message_orig
END INTERFACE

INTERFACE
  SUBROUTINE idba_error_context_orig(message, message_len) BIND(C,name='idba_error_context')
  IMPORT
  CHARACTER(kind=c_char) :: message(*)
  INTEGER(kind=c_int),VALUE :: message_len
  END SUBROUTINE idba_error_context_orig
END INTERFACE

INTERFACE
  SUBROUTINE idba_error_details_orig(message, message_len) BIND(C,name='idba_error_details')
  IMPORT
  CHARACTER(kind=c_char) :: message(*)
  INTEGER(kind=c_int),VALUE :: message_len
  END SUBROUTINE idba_error_details_orig
END INTERFACE

INTERFACE
  FUNCTION idba_error_remove_callback(dbahandle) BIND(C,name='idba_error_remove_callback')
  IMPORT
  INTEGER(kind=c_int) :: dbahandle
  INTEGER(kind=c_int) :: idba_error_remove_callback
  END FUNCTION idba_error_remove_callback
END INTERFACE

INTERFACE
  FUNCTION idba_error_code() BIND(C,name='idba_error_code')
  IMPORT
  INTEGER(kind=c_int) :: idba_error_code
  END FUNCTION idba_error_code
END INTERFACE

PUBLIC
PRIVATE fchartrimtostr

CONTAINS

! helper function for trimming a fortran character and null terminating it
! used for intent(in) string arguments, intent(out) are converted by C code
FUNCTION fchartrimtostr(fchar) RESULT(string)
CHARACTER(len=*),INTENT(in) :: fchar !< Fortran \a CHARACTER variable to convert
CHARACTER(kind=c_char,len=LEN_TRIM(fchar)+1) :: string

string = TRIM(fchar)//CHAR(0)

END FUNCTION fchartrimtostr


! fortran-style interface to generic dba functions
FUNCTION idba_presentati(dbahandle, url)
INTEGER(kind=c_int) :: dbahandle
CHARACTER(kind=c_char,len=*) :: url
INTEGER(kind=c_int) :: idba_presentati

idba_presentati = idba_presentati_orig(dbahandle, fchartrimtostr(url))

END FUNCTION idba_presentati

FUNCTION idba_preparati(dbahandle, handle, anaflag, dataflag, attrflag)
INTEGER(kind=c_int) :: dbahandle
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: anaflag
CHARACTER(kind=c_char,len=*) :: dataflag
CHARACTER(kind=c_char,len=*) :: attrflag
INTEGER(kind=c_int) :: idba_preparati

idba_preparati = idba_preparati_orig(dbahandle, handle, fchartrimtostr(anaflag), &
 fchartrimtostr(dataflag), fchartrimtostr(attrflag))

END FUNCTION idba_preparati

FUNCTION idba_messaggi(handle, filename, mode, typ)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: filename
CHARACTER(kind=c_char,len=*) :: mode
CHARACTER(kind=c_char,len=*) :: typ
INTEGER(kind=c_int) :: idba_messaggi

idba_messaggi = idba_messaggi_orig(handle, fchartrimtostr(filename), &
 fchartrimtostr(mode), fchartrimtostr(typ))

END FUNCTION idba_messaggi

FUNCTION idba_seti(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
INTEGER(kind=c_int) :: val
INTEGER(kind=c_int) :: idba_seti

idba_seti = idba_seti_orig(handle, fchartrimtostr(param), val)

END FUNCTION idba_seti

FUNCTION idba_setb(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
INTEGER(kind=c_signed_char) :: val
INTEGER(kind=c_int) :: idba_setb

idba_setb = idba_setb_orig(handle, fchartrimtostr(param), val)

END FUNCTION idba_setb

FUNCTION idba_setr(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
REAL(kind=c_float) :: val
INTEGER(kind=c_int) :: idba_setr

idba_setr = idba_setr_orig(handle, fchartrimtostr(param), val)

END FUNCTION idba_setr

FUNCTION idba_setd(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
REAL(kind=c_double) :: val
INTEGER(kind=c_int) :: idba_setd

idba_setd = idba_setd_orig(handle, fchartrimtostr(param), val)

END FUNCTION idba_setd

FUNCTION idba_setc(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
CHARACTER(kind=c_char,len=*) :: val
INTEGER(kind=c_int) :: idba_setc

idba_setc = idba_setc_orig(handle, fchartrimtostr(param), fchartrimtostr(val))

END FUNCTION idba_setc

FUNCTION idba_enqi(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
INTEGER(kind=c_int) :: val
INTEGER(kind=c_int) :: idba_enqi

idba_enqi = idba_enqi_orig(handle, fchartrimtostr(param), val)

END FUNCTION idba_enqi

FUNCTION idba_enqb(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
INTEGER(kind=c_signed_char) :: val
INTEGER(kind=c_int) :: idba_enqb

idba_enqb = idba_enqb_orig(handle, fchartrimtostr(param), val)

END FUNCTION idba_enqb

FUNCTION idba_enqr(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
REAL(kind=c_float) :: val
INTEGER(kind=c_int) :: idba_enqr

idba_enqr = idba_enqr_orig(handle, fchartrimtostr(param), val)

END FUNCTION idba_enqr

FUNCTION idba_enqd(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
REAL(kind=c_double) :: val
INTEGER(kind=c_int) :: idba_enqd

idba_enqd = idba_enqd_orig(handle, fchartrimtostr(param), val)

END FUNCTION idba_enqd

FUNCTION idba_enqc(handle, param, val)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
CHARACTER(kind=c_char,len=*) :: val
INTEGER(kind=c_int) :: idba_enqc


idba_enqc = idba_enqc_orig(handle, fchartrimtostr(param), val, LEN(val))

END FUNCTION idba_enqc


FUNCTION idba_unset(handle, param)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
INTEGER(kind=c_int) :: idba_unset

idba_unset = idba_unset_orig(handle, fchartrimtostr(param))

END FUNCTION idba_unset


! fortran-style interface to action functions
FUNCTION idba_scopa(handle, repinfofile)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: repinfofile
INTEGER(kind=c_int) :: idba_scopa

idba_scopa = idba_scopa_orig(handle, fchartrimtostr(repinfofile))

END FUNCTION idba_scopa

FUNCTION idba_dammelo(handle, param)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
INTEGER(kind=c_int) :: idba_dammelo

idba_dammelo = idba_dammelo_orig(handle, param, LEN(param))

END FUNCTION idba_dammelo

FUNCTION idba_ancora(handle, param)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: param
INTEGER(kind=c_int) :: idba_ancora

idba_ancora = idba_ancora_orig(handle, param, LEN(param))

END FUNCTION idba_ancora

FUNCTION idba_messages_open_input(handle, filename, mode, form, simplified)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: filename
CHARACTER(kind=c_char,len=*) :: mode
CHARACTER(kind=c_char,len=*) :: form
LOGICAL,INTENT(in) :: simplified
INTEGER(kind=c_int) :: idba_messages_open_input
INTEGER(kind=c_int) :: lsimplified

lsimplified = 0
IF (simplified) lsimplified = 1

idba_messages_open_input = idba_messages_open_input_orig(handle, &
 fchartrimtostr(filename), fchartrimtostr(mode), fchartrimtostr(form), lsimplified)

END FUNCTION idba_messages_open_input

FUNCTION idba_messages_open_output(handle, filename, mode, form)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: filename
CHARACTER(kind=c_char,len=*) :: mode
CHARACTER(kind=c_char,len=*) :: form

INTEGER(kind=c_int) :: idba_messages_open_output

idba_messages_open_output = idba_messages_open_output_orig(handle, &
 fchartrimtostr(filename), fchartrimtostr(mode), fchartrimtostr(form))

END FUNCTION idba_messages_open_output

FUNCTION idba_messages_read_next(handle, found)
INTEGER(kind=c_int) :: handle
LOGICAL,INTENT(out) :: found
INTEGER(kind=c_int) :: idba_messages_read_next
INTEGER :: lfound

idba_messages_read_next = idba_messages_read_next_orig(handle, lfound)
found = lfound /= 0 ! int to logical

END FUNCTION idba_messages_read_next

FUNCTION idba_messages_write_next(handle, template_name)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*) :: template_name
INTEGER(kind=c_int) :: idba_messages_write_next

idba_messages_write_next = idba_messages_write_next_orig(handle, &
 fchartrimtostr(template_name))

END FUNCTION idba_messages_write_next


! fortran-style interface to pretty-printing functions
FUNCTION idba_spiegal(handle, ltype1, l1, ltype2, l2, res)
INTEGER(kind=c_int) :: handle
INTEGER(kind=c_int) :: ltype1
INTEGER(kind=c_int) :: l1
INTEGER(kind=c_int) :: ltype2
INTEGER(kind=c_int) :: l2
CHARACTER(kind=c_char,len=*),INTENT(out) :: res
INTEGER(kind=c_int) :: idba_spiegal

idba_spiegal = idba_spiegal_orig(handle, ltype1, l1, ltype2, l2, res, LEN(res))

END FUNCTION idba_spiegal

FUNCTION idba_spiegat(handle, ptype, p1, p2, res)
INTEGER(kind=c_int) :: handle
INTEGER(kind=c_int) :: ptype
INTEGER(kind=c_int) :: p1
INTEGER(kind=c_int) :: p2
CHARACTER(kind=c_char,len=*),INTENT(out) :: res
INTEGER(kind=c_int) :: idba_spiegat

idba_spiegat = idba_spiegat_orig(handle, ptype, p1, p2, res, LEN(res))

END FUNCTION idba_spiegat

FUNCTION idba_spiegab(handle, varcode, var, res)
INTEGER(kind=c_int) :: handle
CHARACTER(kind=c_char,len=*),INTENT(in) :: varcode
CHARACTER(kind=c_char,len=*),INTENT(in) :: var
CHARACTER(kind=c_char,len=*),INTENT(out) :: res
INTEGER(kind=c_int) :: idba_spiegab

idba_spiegab = idba_spiegab_orig(handle, fchartrimtostr(varcode), &
 fchartrimtostr(var), res, LEN(res))

END FUNCTION idba_spiegab


! fortran-style interface to error handling functions
SUBROUTINE idba_error_message(message)
CHARACTER(kind=c_char,len=*),INTENT(out) :: message

CALL idba_error_message_orig(message, LEN(message))

END SUBROUTINE idba_error_message

SUBROUTINE idba_error_context(message)
CHARACTER(kind=c_char,len=*),INTENT(out) :: message

CALL idba_error_context_orig(message, LEN(message))

END SUBROUTINE idba_error_context

SUBROUTINE idba_error_details(message)
CHARACTER(kind=c_char,len=*),INTENT(out) :: message

CALL idba_error_details_orig(message, LEN(message))

END SUBROUTINE idba_error_details


END MODULE dballef
