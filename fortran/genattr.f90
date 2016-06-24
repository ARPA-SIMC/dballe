program tmp

include "dballeff.h"

integer,parameter :: nvar=5
real :: field(nvar),obsinc(nvar)
character(len=6) :: var(nvar)
INTEGER :: handle,handle_err,ana_id,debug,i


! gestione degli errori
ierr = idba_error_set_callback(0,idba_default_error_handler,debug,handle_err)

ierr = idba_messaggi(handle, "tmp.bufr", "w", "BUFR")


                                     ! vital statistics data
ierr = idba_setcontextana (handle)
ierr = idba_set (handle,"rep_memo","generic")
ierr = idba_set (handle,"lat",44.5)
ierr = idba_set (handle,"lon",10.0)

ierr = idba_set (handle,"mobile",0)
ierr = idba_set (handle,"block",16)
ierr = idba_set (handle,"station",144)

!!!! e' cosi' per compatibilita' db
ierr = idba_prendilo (handle)
ierr = idba_enqi (handle,"ana_id",ana_id)

ierr = idba_unsetall (handle)

ierr = idba_setc (handle,"rep_memo","generic")
ierr = idba_seti (handle,"ana_id",ana_id)
!!!!

ierr = idba_settimerange (handle,254,0,0)
ierr = idba_setdate (handle,2010,04,11,12,0,0)
ierr = idba_setlevel (handle,100,50000,0,0)

var(1)="B11003"
var(2)="B11004"
var(3)="B12101"
var(4)="B13003"
var(5)="B07004"
  
field(1)=5.5
field(2)=6.6
field(3)=273.15
field(4)=50.
field(5)=55000

obsinc(1)=.5
obsinc(2)=.6
obsinc(3)=10.
obsinc(4)=5.
obsinc(5)=100.

ierr = idba_set(handle,"B08001",1)
ierr = idba_prendilo(handle)
!ierr = idba_unset(handle,"B08001")

print *,"-------------------"

                                ! cicle on 5 variables to write original data and obsevation increments
do i=1,nvar
        
  print *,var(i),field(i),obsinc(i)

                                ! add or rewrite new data
  ierr = idba_set(handle,var(i),field(i))
  ierr = idba_prendilo(handle)
  ierr = idba_set(handle,"*B33198",obsinc(i))
  ierr = idba_critica(handle)
  ierr = idba_unset(handle,"*B33198")
  ierr = idba_unset(handle,var(i))
          
end do

ierr = idba_set(handle,'query',"message generic")
ierr = idba_prendilo (handle)

                                ! end session and connection
ierr = idba_fatto(handle)


end program tmp
