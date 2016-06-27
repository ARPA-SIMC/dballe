MODULE dballef
USE,INTRINSIC :: iso_c_binding
IMPLICIT NONE

INTERFACE
  FUNCTION idba_presentati_orig(dbahandle, url, user, password) BIND(C,name='idba_presentati')
  IMPORT
  INTEGER(kind=c_int) :: dbahandle
  CHARACTER(kind=c_char) :: url(*)
  CHARACTER(kind=c_char) :: user(*)
  CHARACTER(kind=c_char) :: password(*)
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
  INTEGER(kind=c_int) :: dbahandle
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
  SUBROUTINE idba_error_remove_callback(dbahandle) BIND(C,name='idba_remove_callback')
  IMPORT
  INTEGER(kind=c_int) :: dbahandle
  END SUBROUTINE idba_error_remove_callback
END INTERFACE

INTERFACE
  FUNCTION idba_error_code() BIND(C,name='idba_error_code')
  IMPORT
  INTEGER(kind=c_int) :: idba_error_code
  END FUNCTION idba_error_code
END INTERFACE

PRIVATE
PUBLIC idba_presentati, idba_arrivederci, idba_preparati, idba_messaggi, &
 idba_fatto, &
 idba_set, idba_seti, idba_setb, idba_setr, idba_setd, idba_setc, &
 idba_enq, idba_enqi, idba_enqb, idba_enqr, idba_enqd, idba_enqc
PUBLIC idba_unset, idba_unsetb, idba_unsetall, idba_setcontextana, &
 idba_setlevel, idba_settimerange, idba_setdate, idba_setdatemin, idba_setdatemax, &
 idba_enqlevel, idba_enqtimerange, idba_enqdate
PUBLIC idba_scopa, idba_quantesono, idba_elencamele, idba_voglioquesto, idba_dammelo
PUBLIC idba_error_set_callback, idba_default_error_handler, &
 idba_default_error_handle_tolerating_overflows, &
 idba_error_message, idba_error_context, idba_error_details, &
 idba_error_remove_callback, idba_error_code

CONTAINS

! helper function for trimming a fortran character and null terminating it
FUNCTION fchartrimtostr(fchar) RESULT(string)
CHARACTER(len=*),INTENT(in) :: fchar !< Fortran \a CHARACTER variable to convert
CHARACTER(kind=c_char,len=LEN_TRIM(fchar)+1) :: string

string = TRIM(fchar)//CHAR(0)

END FUNCTION fchartrimtostr

! public simplified interface to presentati
FUNCTION idba_presentati(dbahandle, url, user, password)
INTEGER(kind=c_int) :: dbahandle
CHARACTER(kind=c_char,len=*) :: url
CHARACTER(kind=c_char,len=*) :: user
CHARACTER(kind=c_char,len=*) :: password
INTEGER(kind=c_int) :: idba_presentati

idba_presentati = idba_presentati_orig(dbahandle, fchartrimtostr(url), &
 fchartrimtostr(user), fchartrimtostr(password))

END FUNCTION idba_presentati


! public simplified interface to preparati
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


! public simplified interface to messaggi
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


! fortran style interface to error handling functions
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
