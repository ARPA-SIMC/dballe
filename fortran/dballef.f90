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

PRIVATE
PUBLIC idba_presentati, idba_arrivederci, idba_preparati

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

END MODULE dballef
