program test


integer :: handle
integer :: error_handle
character(len=255) :: prettyvalue

include "dballef.h"

call idba_error_set_callback(0, idba_default_error_handler, 42, error_handle)

call idba_messaggi(handle,"/dev/null", "w", "BUFR")
call idba_spiegab(handle, "B12101", "1234", prettyvalue)
call idba_fatto(handle)

end program test
