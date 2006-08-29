
       interface

       subroutine idba_seti(handle,param,value)

       integer :: handle,value
       character (len=*) :: param

       end subroutine idba_seti

       subroutine idba_setr(handle,param,value)

       integer :: handle
       real :: value
       character (len=*) :: param

       end subroutine idba_setr

       subroutine idba_setd(handle,param,value)

       integer :: handle
       double precision :: value
       character (len=*) :: param

       end subroutine idba_setd












       end interface
