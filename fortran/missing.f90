      module missing

! **************************************************************************
! * Global variables with fortran missing values, to let our C bindings know
! **************************************************************************

      include "dballef.h"

      integer :: fortran_missing_int = DBA_MVI
      integer (kind=dba_int_b) :: fortran_missing_byte = DBA_MVB
      real :: fortran_missing_real = DBA_MVR
      real*8 :: fortran_missing_double = DBA_MVD

      end module missing
