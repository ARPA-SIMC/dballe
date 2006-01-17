#ifndef DBA_ERROR_H
#define DBA_ERROR_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup err
 * Error management module for DBALLE.
 */

/**
 * Return value used to denote success or failure for a function.
 * 
 * It is guaranteed to evaluate to false to indicate success (absence of
 * errors), or to true to indicate failure (presence of errors).
 */
enum _dba_err {
	DBA_OK = 0,
	DBA_ERROR = 1
};
typedef enum _dba_err dba_err;

/**
 * Error code identifying a type of error.  It can be used by code to implement
 * special handling for some kind of errors.
 */
typedef enum {
	DBA_ERR_NONE			=  0,
	DBA_ERR_NOTFOUND		=  1,
	DBA_ERR_TYPE			=  2,
	DBA_ERR_ALLOC			=  3,
	DBA_ERR_ODBC			=  4,
	DBA_ERR_HANDLES			=  5,
	DBA_ERR_TOOLONG			=  6,
	DBA_ERR_SYSTEM			=  7,
	DBA_ERR_CONSISTENCY		=  8,
	DBA_ERR_PARSE			=  9,
	DBA_ERR_WRITE			= 10,
	DBA_ERR_UNIMPLEMENTED	= 11
} dba_err_code;

/**
 * Type of the callback function that can be registered to be invoked when some
 * error happens.
 */
typedef void (*dba_err_callback)(void* data);


/**
 * Run the given function, checking the dba_err result value.  If the function
 * failed, then execute a "return" with that value.
 */
#define DBA_RUN_OR_RETURN(...) do { \
		dba_err err = __VA_ARGS__; \
		if (err != DBA_OK) \
			return err; \
	} while (0)

/**
 * Run the given function, checking the dba_err result value.  If the function
 * failed, then goto the given label
 *
 * This is used to perform some cleanup before returning the error.
 *
 * Example:
 * \code
 *   // Special cleanup on failure
 *   dba_err example(dba_var* result)
 *   {
 *       dba_err err;
 *       dba_var var;
 *       // A temporary variable is needed
 *       DBA_RUN_OR_RETURN(dba_var_create_local(DBA_VAR(0, 1, 2), &var));
 *       DBA_RUN_OR_GOTO(fail, dba_var_setd(var, compute_some_value()));
 *       *result = var;
 *       return dba_error_ok();
 *   fail:
 *       dba_var_delete(var);
 *       return err;
 *   }
 *
 *   // Cleanup is needed in case of both success and failure
 *   dba_err example()
 *   {
 *       dba_err err = DBA_OK;
 *       dba_var var = NULL;
 *		 // perform some computation...
 *       DBA_RUN_OR_GOTO(cleanup, dba_var_create_local(DBA_VAR(0, 1, 2), &var));
 *       DBA_RUN_OR_GOTO(cleanup, dba_var_setd(var, compute_some_value()));
 *   cleanup:
 *       if (var != NULL)
 *           dba_var_delete(var);
 *       return err == DBA_OK ? dba_error_ok() : err;
 *   }
 * \endcode
 */
#define DBA_RUN_OR_GOTO(label, ...) do { \
		err = __VA_ARGS__; \
		if (err != DBA_OK) \
			goto label; \
	} while (0)

/**
 * Return the error \e error.
 *
 * Example:
 * \code
 *   if (idx > max)
 *     DBA_FAIL_RETURN(dba_error_notfound, "looking for item %s", name);
 * \endcode
 */
#define DBA_FAIL_RETURN(...) do { \
		return __VA_ARGS__; \
	} while (0)

/**
 * Report the error \e error by jumping to a label instead of returning.
 *
 * Example:
 * \code
 *   if (idx > max)
 *     DBA_FAIL_GOTO(cleanup, dba_error_notfound, "looking for item %s", name);
 * \endcode
 */
#define DBA_FAIL_GOTO(label, ...) do { \
		err = __VA_ARGS__; \
		goto label; \
	} while (0)

/**
 * Set a generic message.  To be used by other modules to implement their own
 * error functions.
 *
 * @param code
 *   The error code for this error
 * @param context
 *   The context description for this error.  It can be NULL.  If it is
 *   non-NULL, then it will be deallocated by the dba_error module.
 * @param extended_message
 *   The extended message for this error.  It can be NULL.  If it is
 *   non-NULL, then it will be deallocated by the dba_error module.
 * @return
 *   DBA_ERROR
 */
dba_err dba_error_generic0(dba_err_code code, char* context, char* extended_message);

/**
 * Set a generic message.  To be used by other modules to implement their own
 * error functions.
 *
 * @param code
 *   The error code for this error
 * @param fmt
 *   printf-style format string used to build the context informations for this
 *   error
 * @return
 *   DBA_ERROR
 */
dba_err dba_error_generic1(dba_err_code code, const char* fmt, ...);

/**
 * Reports the success of a function.
 *
 * @return
 *   DBA_OK
 */
dba_err dba_error_ok();

/**
 * Reports that a search-like function could not find what was requested.
 *
 * @param fmt
 *   printf-style format string used to build the context informations for this
 *   error
 * @return
 *   DBA_ERROR
 */
#define dba_error_notfound(fmt, ...) dba_error_generic1(DBA_ERR_NOTFOUND, fmt , ## __VA_ARGS__)

/**
 * For functions handling data with multiple types, reports a mismatch
 * between the type requested and the type found.
 *
 * @param fmt
 *   printf-style format string used to build the context informations for this
 *   error
 * @return
 *   DBA_ERROR
 */
#define dba_error_type(fmt, ...) dba_error_generic1(DBA_ERR_TYPE, fmt , ## __VA_ARGS__)

/**
 * Reports that memory allocation has failed.
 *
 * @return
 *   DBA_ERROR
 */
dba_err dba_error_alloc(const char* message);

/**
 * For functions working with handles, reports a problem with handling handles,
 * such as impossibility to allocate a new one, or an invalid handle being
 * passed to the function.
 *
 * @param fmt
 *   printf-style format string used to build the context informations for this
 *   error
 * @return
 *   DBA_ERROR
 */
#define dba_error_handles(fmt, ...) dba_error_generic1(DBA_ERR_HANDLES, fmt , ## __VA_ARGS__)

/**
 * Report an error with a buffer being to short for the data it needs to fit.
 *
 * @param fmt
 *   printf-style format string used to build the context informations for this
 *   error
 * @return
 *   DBA_ERROR
 */
#define dba_error_toolong(fmt, ...) dba_error_generic1(DBA_ERR_TOOLONG, fmt , ## __VA_ARGS__)

/**
 * Report a system error message.  The message description will be looked up
 * using the current value of errno.
 *
 * @param fmt
 *   printf-style format string used to build the context informations for this
 *   error
 * @return
 *   DBA_ERROR
 */
dba_err dba_error_system(const char* fmt, ...);

/**
 * Report an error when a consistency check failed.
 *
 * @param fmt
 *   printf-style format string used to build the context informations for this
 *   error
 * @return
 *   DBA_ERROR
 */
#define dba_error_consistency(fmt, ...) dba_error_generic1(DBA_ERR_CONSISTENCY, fmt , ## __VA_ARGS__)

/**
 * Report an error when parsing informations.
 *
 * @param file
 *   The file that is being parsed
 * @param line
 *   The line of the file where the problem has been found
 * @param fmt
 *   printf-style format string used to build the context informations for this
 *   error
 * @return
 *   DBA_ERROR
 */
dba_err dba_error_parse(const char* file, int line, const char* fmt, ...);

/**
 * Reports that a feature is still not implemented.
 *
 * @param fmt
 *   printf-style format string used to build the context informations for this
 *   error
 * @return
 *   DBA_ERROR
 */
#define dba_error_unimplemented(fmt, ...) dba_error_generic1(DBA_ERR_UNIMPLEMENTED, fmt , ## __VA_ARGS__)


/**
 * Get the error code for the last error.
 *
 * @return
 *   The error code (@see dba_err_code)
 */
dba_err_code dba_error_get_code();

/**
 * Get the error message for the last error.
 *
 * @return
 *   The short error message, which describes the error code.  It can never be
 *   NULL.
 */
const char* dba_error_get_message();

/**
 * Get the description of the context in which the last error happened.
 *
 * @return
 *   A string with the context description.  It can never be NULL.
 */
const char* dba_error_get_context();

/**
 * Get the details about the last error happening.  This is used in those cases
 * in which it is possible to find out various kinds of detailed informations
 * about an error.
 *
 * @return
 *   A string with the details description.  It can be NULL.
 */
const char* dba_error_get_details();

/**
 * Get the stack backtrace at the time the last error happened.
 *
 * @return
 *   A string with the backtrace. It can be NULL.
 */
const char* dba_error_get_backtrace();

/**
 * Set a callback to be invoked when an error of a specific kind happens.
 *
 * @param code
 *   The error code (@see ::dba_err_code) of the error that triggers this
 *   callback.  If DBA_ERR_NONE is used, then the callback is invoked on all
 *   errors. 
 * @param cb
 *   The function to be called.
 * @param data
 *   An arbitrary data that is passed verbatim to the callback function when
 *   invoked.
 */
void dba_error_set_callback(dba_err_code code, dba_err_callback cb, void* data);

/**
 * Removes a callback that has been set previously.  The same values of code,
 * cb and data given when setting the callback must be provided for the
 * callback to be identified.
 *
 * @param code
 *   The 'code' parameter used when setting the callback to be deleted.
 * @param cb
 *   The 'cb' parameter used when setting the callback to be deleted.
 * @param data
 *   The 'data' parameter used when setting the callback to be deleted.
 */
void dba_error_remove_callback(dba_err_code code, dba_err_callback cb, void* data);


struct _dba_error_info;
/**
 * Opaque structure that is used to save all the error reporting informations,
 * to be restored later
 */
typedef struct _dba_error_info* dba_error_info;

/**
 * Save all error informations inside info, to be restored later with dba_error_state_set
 *
 * @param info
 *   The dba_error_info where the informations will be saved.  If info points to NULL,
 *   it will be allocated, else it will be considered as an old dba_error_info
 *   to reuse.  In both cases, it will need to be deallocated with
 *   dba_error_state_delete.
 */
void dba_error_state_get(dba_error_info* info);

/**
 * Restore saved error informations
 *
 * @param info
 *   The dba_error_info where the state was previously saved by dba_error_state_get
 */
void dba_error_state_set(dba_error_info info);

/**
 * Deallocate a dba_error_info
 */
void dba_error_state_delete(dba_error_info* info);


#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
