program dump_dballe

! ****************************************************
! * Dump the contents of a weather messages in a file
! ****************************************************

implicit none

integer :: handle, nstaz, ndata, nattr
integer :: i, i1, i2, type1, l1, type2, l2
integer ::  height, rep_cod, year, month, day, hour, minute, second
character(len=255) :: fname,prettyvalue
character(len=20)  :: cname, rep_memo,value, avalue
character(len=10) :: btable, starbtable
doubleprecision ::dlat,dlon

include "dballe/dballef.h"
external errorrep
call getarg(1,fname)

call idba_error_set_callback(0, errorrep, 0, i)

!     Open a session
call idba_messaggi(handle, fname, "r", "BUFR")

!     Query all the stations
do while (.true.)
  call idba_quantesono(handle, nstaz)
  if (nstaz .eq. 0) exit

  call idba_elencamele(handle)
  call idba_enq(handle, "name", cname)
  call idba_enq(handle, "lat", dlat)
  call idba_enq(handle, "lon", dlon)
  call idba_enq(handle, "height", height)
  call idba_enq(handle,"rep_memo",rep_memo)

  write (*,*) "Staz: ",trim(cname)," (",dlat,",",dlon,")"," h:",height," network: ",rep_memo
  !call idba_set(handle,"varlist","B12101,B11002") ! only on DB is valid

  call idba_voglioquesto(handle,ndata)
  !write (*,*) " ",ndata," dati:"                  ! only on DB is valid
  do i1=1, ndata
    call idba_dammelo(handle,btable)
    if (btable /= "B12101" .and. btable /= "B11002") cycle 

    call idba_enqdate(handle, year, month, day, hour, minute, second)
    call idba_enqlevel(handle, type1, l1, type2, l2)
    call idba_enq(handle,btable,value)

    print*,"----"
    write (*,*) "date time: ",year, month, day, hour, minute
    call idba_spiegal(handle,type1,l1,type2,l2,prettyvalue)
    write (*,*) trim(prettyvalue)
    call idba_spiegab(handle,btable,value,prettyvalue)
    write (*,*) trim(prettyvalue)
    
    call idba_voglioancora (handle,nattr)
    if (nattr > 0) then
      write (*,*) "   ",nattr," attributi:"
      do i2=1, nattr
        call idba_ancora(handle,starbtable)
        call idba_enq(handle,starbtable,avalue)
        write(*,*) "    attr ",trim(starbtable),": ",avalue
      enddo
    end if
  enddo
enddo

call idba_fatto(handle)
      
call exit (0)

end program dump_dballe

subroutine errorrep(val)
integer :: val
character(len=1000) :: buf

ier = idba_error_code()
if (ier.ne.0) then
  print *,ier," error in ",val
  call idba_error_message(buf)
  print *,trim(buf)
  call idba_error_context(buf)
  print *,trim(buf)
  call idba_error_details(buf)
  print *,trim(buf)
  call exit (1)
end if
return

end subroutine errorrep

