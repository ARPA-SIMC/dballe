      module dbtest

      use test
      use dballef

      contains

      subroutine dbinit(dbahandle)
      integer::dbahandle, handle, ierr
      character (len=160) :: dsn,testname
        call getenv("DBA_DB", dsn)
        call getarg(0, testname)
 
        if (dsn=="") then
          dsn = "test:"
        end if

        ierr = idba_presentati(dbahandle, dsn, char(0), char(0))
        call ensure_no_error("presentati")

        ierr = idba_preparati(dbahandle, handle, "write", "write", "write")
        call idba_scopa(handle, char(0))
        call idba_fatto(handle)
      endsubroutine dbinit

      endmodule dbtest
