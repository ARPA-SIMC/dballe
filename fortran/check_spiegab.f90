program test

use dballef

integer :: handle
integer :: error_handle
character(len=255) :: prettyvalue

ierr = idba_error_set_callback(0, C_FUNLOC(idba_default_error_handler), 42, error_handle)

ierr = idba_messaggi(handle,"/dev/null", "w", "BUFR")
ierr = idba_spiegab(handle, "B12101", "1234", prettyvalue)
ierr = idba_fatto(handle)

end program test
