#ifndef VERBOSE_H
#define VERBOSE_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup core
 * Debugging aid framework that allows to print, at user request, runtime
 * verbose messages about internal status and operation.
 */

/**
 * The output stream where verbose messages will be sent
 */
#define DBA_VERBOSE_STREAM stderr

/**
 * The various contexts for verbose messages (they will be OR-ed together)
 */
enum {
	DBA_VERB_NONE = 0,
	DBA_VERB_DB_INPUT = 1,
	DBA_VERB_DB_SQL = 2,
};

/**
 * Initialize the verbose printing interface, taking the allowed verbose level
 * from the environment and printing a little informational banner if any
 * level of verbose messages are enabled.
 */
void dba_verbose_init();

/**
 * Return 1 if the given verbose level has been requested in output, else 0
 */
int dba_verbose_is_allowed(int lev);

/**
 * Output a message on the verbose stream, if the given level of verbosity has
 * been requested
 */
void dba_verbose(int lev, const char* fmt, ...);


#ifdef  __cplusplus
}
#endif

#endif
