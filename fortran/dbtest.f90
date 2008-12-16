      module dbtest

      contains

      subroutine dbinit(dbahandle)
      integer::dbahandle
      character (len=160) :: dsn,user,pass,testname
        call getenv("DBA_TEST_DSN", dsn)
        call getenv("DBA_TEST_USER", user)
        call getenv("DBA_TEST_PASS", pass)
        call getarg(0, testname)
 
        if (dsn=="") then
          print *,trim(testname),": DBA_TEST_DSN not set: test skipped"
          call exit(0)
        end if

        call idba_presentati(dbahandle, dsn, user, pass)
        call ensure_no_error("presentati")
      endsubroutine dbinit

!     Continue execution only if there was no error
      subroutine ensure_no_error(message)
      character (len=*) :: message
      integer :: idba_error_code, ier
      character (len=1000) :: buf

!      print *,"siamo a ",message
      ier = idba_error_code()
      if (ier.ne.0) then
         print *,ier," in ",message
         call idba_error_message(buf)
         print *,trim(buf)
         call idba_error_context(buf)
         print *,trim(buf)
         call idba_error_details(buf)
         print *,trim(buf)
         call exit (1)
      end if
      return
      endsubroutine ensure_no_error

!     Print an error if the given logical value is false
      subroutine ensure(message, value)
      character (len=*) :: message
      logical :: value

      if (.not.value) then
         print *,"Check failed in ",message
         call exit (1)
      end if
      return
      endsubroutine ensure

      endmodule dbtest
