      module dbtest

      use test

      contains

      subroutine dbinit(dbahandle)
      integer::dbahandle, handle
      character (len=160) :: dsn,testname
        call getenv("DBA_DB", dsn)
        call getarg(0, testname)
 
        if (dsn=="") then
          dsn = "test:"
        end if

        call idba_presentati(dbahandle, dsn, char(0), char(0))
        call ensure_no_error("presentati")

        call idba_preparati(dbahandle, handle, "write", "write", "write")
        call idba_scopa(handle, char(0))
        call idba_fatto(handle)
      endsubroutine dbinit

      endmodule dbtest
