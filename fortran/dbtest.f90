      module dbtest

      use test
      use dballef

      contains

      subroutine dbinit(dbahandle)
      integer::dbahandle, handle, ierr
      character (len=255) :: url,testname
        call getenv("DBA_DB", url)
        call getarg(0, testname)

        if (url == "") then
          url = "test:"
        end if

        ! Add ?wipe=1 url argument
        if (index(url, "?") == 0) then
          url = trim(url) // "?wipe=1"
        else
          url = trim(url) // "&wipe=1"
        end if

        ierr = idba_presentati(dbahandle, url)
        call ensure_no_error("presentati")

        ierr = idba_preparati(dbahandle, handle, "write", "write", "write")
        ierr = idba_scopa(handle, char(0))
        ierr = idba_fatto(handle)
      endsubroutine dbinit

      endmodule dbtest
