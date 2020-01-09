Error management
================

.. toctree::
   :maxdepth: 2

.. highlight:: fortran

* All errors are reported as function return values
* All results are reported as output parameters
* All functions ``idba_set*`` set the input of action routines
* All functions ``idba_enq*`` get the output of action routines

Errors can be handled by checking the return value of every function::

    ! Example error handling
    ierr = idba_connect(dbhandle, "dburl")
    if (ierr.ne.0) then
         ! handle the error...
    end if

Or they can be handled by installing a callback function that is automatically
called in case of error::

    !     How to set a callback
    !       * the first parameter is the error code that triggers the callback (0
    !         means 'trigger on all errors')
    !       * the second parameter is the routine to call when the error happens
    !         (remember to declare the function as 'external'
    !       * the third parameter is a convenience arbitrary integer that will be
    !         passed to the function
    !       * the fourth parameter is used to return a handle that can be used to
    !         remove the callback
    ierr = idba_error_set_callback(0, error_handler, 42, cb_handle)

The previous code will setup DB-ALLe to call ``error_handler`` after any error,
passing it the integer value 42.  The callback can be removed at any time by
calling :c:func:`idba_error_remove_callback`::

    ! How to remove a callback
    ierr = idba_error_remove_callback(cb_handle)

This is an example of a useful error handling function::

    ! The error handler needs to be declared 'external'
    external error_handler

    ! Compute the length of a string
    ! (this is an utility function that is used by the error handler
    !  to print nicer error messages)
    integer function istrlen(string)
    character string*(*)
      istrlen = len(string)
      do while ((string(istrlen:istrlen).eq." " .or. string(istrlen:istrlen).eq."").and. istrlen.gt.0)
         istrlen = istrlen - 1
      enddo
      return
    end

    ! Generic error handler: print all available information
    ! about the error, then exit
    subroutine error_handler(val)
    integer val
    character buf*1000
      print *,ier," testcb in ",val
      call idba_error_message(buf)
      print *,buf(:istrlen(buf))
      call idba_error_context(buf)
      print *,buf(:istrlen(buf))
      call idba_error_details(buf)
      print *,buf(:istrlen(buf))
      call exit (1)
      return
    end

This code introduces three new functions:

* :c:func:`idba_error_message`:
  returns a string describing what type of error has happened.
* :c:func:`idba_error_context`:
  returns a string describing what DB-All.e was trying to do when the error
  happened.
* :c:func:`idba_error_details`:
  returns a detailed description of the error, when available.  If no detailed
  description is available, it returns an empty string.

A similar error handling behaviour can be obtained by using the predefined
convenience function :c:func:`idba_default_error_handler`::

    ! Declare the external function (not necessary if you include dballeff.h)
    external idba_default_error_handler

    ! Use it as the error handling callback
    ierr = idba_error_set_callback(0, idba_default_error_handler, 1, cb_handle)

An alternative error handler called :c:func:`idba_error_handle_tolerating_overflows`
is available: it exists on all errors instead of value overflows, in what case
it prints a warning to standard error and allows the program to continue.  The
overflow error can then be catched, if needed, by inspecting the error code
returned by the DB-All.e function that causes the error.

This is how to use it::

    ! Declare the external function (not necessary if you include dballeff.h)
    external idba_error_handler_tolerating_overflows

    ! Use it as the error handling callback
    ierr = idba_error_set_callback(0, idba_error_handler_tolerating_overflows, 1, cb_handle)
