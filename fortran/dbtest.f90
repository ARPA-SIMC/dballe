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

        ierr = idba_presentati(dbahandle, dsn)
        call ensure_no_error("presentati")

        ierr = idba_preparati(dbahandle, handle, "write", "write", "write")
        ierr = idba_scopa(handle, char(0))
        ierr = idba_fatto(handle)
      endsubroutine dbinit

      endmodule dbtest
