/*
 * This example shows how to read a BUFR file.
 */

/*
 * This file provides error management routines.  All DB-All.e functions that
 * can fail return a result of type dba_err that can be tested for success.
 */
#include <dballe/core/error.h>

/*
 * This module provides the functions to read a file with BUFR messages (or any
 * other type of messages supported), and extract the messages one by one.
 */
#include <dballe/core/file.h>

/*
 * This module provides support for deconding a BUFR message and reading the
 * decoded values
 */
#include <dballe/bufrex/msg.h>

/* Other standard include files */
#include <stdio.h>

/* This function (defined later) dumps the subset of a BUFR message to standard
 * output */
dba_err dump_subset(bufrex_subset s);

/*
 * This function dumps the contents of the BUFR file to standard output.
 *
 * The function also shows a simple way to implement in C something similar to
 * C++ exceptions.
 */
dba_err do_dump(const char* filename)
{
	/* We use this to check the error status of the various functions */
	dba_err err = DBA_OK;
	/* This is the dba_file that we use to read the BUFR messages */
	dba_file input = NULL;
	/* This will contain the messages that we read from the file */
	dba_rawmsg rawmsg = NULL;
	/* This will contain the decoded message */
	bufrex_msg msg = NULL;

	/*
	 * Create a dba_file object, open for reading BUFR messages.
	 *
	 * Note that all functions that return some value, but can also fail,
	 * return the error status as the return value of the function, and return
	 * the value as a parameter passed by pointer.
	 *
	 * Also, all parameters passed by pointer to read values are always at the
	 * end of the parameter list.
	 */
	err = dba_file_create(BUFR, filename, "r", &input);
	if (err != DBA_OK)
		goto cleanup;

	/* Checking the return value every time is cumbersome.  For this reason,
	 * dballe/core/error.h provides two useful macros: DBA_RUN_OR_RETURN and
	 * DBA_RUN_OR_GOTO.  The first returns the error code of the function if
	 * the function failed, the second performs a goto to a given label if the
	 * function failed.
	 *
	 * For example, the entire dba_file_create invocation above can be
	 * rewritten as just:
	 * \code
	 * DBA_RUN_OR_GOTO(cleanup, dba_file_create(BUFR, filename, "r", &input));
	 * \endcode
	 *
	 * I'll use it from now on.
	 */

	/*
	 * Create the dba_rawmsg.  This holds a BUFR message in its encoded form,
	 * along with some information about the length of the message, and the
	 * offset in the file where it was found.
	 */
	DBA_RUN_OR_GOTO(cleanup, dba_rawmsg_create(&rawmsg));

	/*
	 * Create the bufrex_msg.  This holds the decoded data of a BUFR message.
	 */
	DBA_RUN_OR_GOTO(cleanup, bufrex_msg_create(BUFREX_BUFR, &msg));

	/* Now that we have all the tools ready, we can proceed to read the
	 * messages and dump them */
	while (1)
	{
		int i;

		/* Read a message from the file.  Since dba_file_read can fail with all
		 * the sort of errors that happen during I/O, it needs an integer
		 * passed by pointer to tell us when it reached the end of file */
		int found;
		DBA_RUN_OR_GOTO(cleanup, dba_file_read(input, rawmsg, &found));
		/* When there are no more messages to read, we are done */
		if (!found) break;

		/* Ok, we now have a message in its encoded form inside rawmsg.
		 * This will decode it */
		DBA_RUN_OR_GOTO(cleanup, bufrex_msg_decode(msg, rawmsg));

		/* Now it's basically a matter of having a look at what is inside the
		 * structure pointed by bufr_msg and take what we need */

		/* Some of the header information */
		printf("Message type %d, subtype %d\n", msg->type, msg->subtype);
		printf("Reference time %04d-%02d-%02d %02d:%02d\n",
			msg->rep_year, msg->rep_month, msg->rep_day, msg->rep_hour, msg->rep_minute);
		printf("The message contains %d subsets of data\n", msg->subsets_count);

		/* BUFR messages contain one or more subsets with the actual data. */
		for (i = 0; i < msg->subsets_count; ++i)
		{
			printf("Subset %d/%d:\n", i, msg->subsets_count);
			DBA_RUN_OR_GOTO(cleanup, dump_subset(msg->subsets[i]));
		}

		/* Output an empty line as a separator before the next message */
		printf("\n");
	}

cleanup:
	/* 
	 * Cleanup code: we release all the resources acquired by the function.
	 *
	 * We always check if every resource was acquired or not: this allows us to
	 * clean up the resources even if the function above was executed only
	 * partially.
	 */

	if (input != NULL)
		/* Note that all _delete cannot fail, and therefore don't return an error
		 * message */
		dba_file_delete(input);

	if (rawmsg != NULL) dba_rawmsg_delete(rawmsg);
	if (msg != NULL) bufrex_msg_delete(msg);

	/* If there was no error, we return dba_error_ok() that also resets
	 * the library error information.  If there was an error, we return the
	 * error code and leave the library error information unchanged, so that
	 * the caller can evaluate them.
	 */
	return err == DBA_OK ? dba_error_ok() : err;
}

/* Dump a BUFR subset.  A bufrex_subset (@see subset.h) is little more than an
 * array of dba_var (@see var.h) values.
 *
 * dba_var is a way of representing a value together with all its metadata,
 * including the unit of measurement, the number of significant digits, the
 * description, the allowed range and the type.  A dba_var can also contain no
 * value: in that case the dba_var is said to be "unset", and this is useful to
 * represent the missing values in the BUFR message.
 */
dba_err dump_subset(bufrex_subset s)
{
	int i;
	/* Iterate all the variables in the subset */
	for (i = 0; i < s->vars_count; ++i)
	{
		/* This will hold the metadata of the variable */
		dba_varinfo info;
		/* This will be used later to iterate the attributes of the variable */
		dba_var_attr_iterator iattr;
		/* Shortcut way to address the current variable, to make the code
		 * easier to read */
		dba_var var = s->vars[i];

		/* Start with the position in the subset */
		printf("%3d ", i+i);

		/* This would be a quick way to dump the contents of a dba_var:
		 * dba_var_print(s->vars[i], stdout);
		 * 
		 * However, this is a good opportunity to show how to query a dba_var,
		 * so we should not miss it.
		 *
		 * Every dba_var contains a value serialised as a string, and a
		 * dba_varinfo with all the metadata.
		 *
		 * To see if a variable is set, check if dba_var_value(var) is NULL.
		 *
		 * To read the value, use one of the dba_var_enqi(), dba_var_enqd() and
		 * dba_var_enqc() functions.
		 *
		 * To access the metadata, use dba_var_info(var).
		 */
	
		/* Get the metadata */
		info = dba_var_info(var);
		
		/* Print out some information about the variable */

		/* This is the BUFR variable code.  It's stored packed in 16bits, but
		 * we have macros to unpack it so we can show it in a familiar way */
		printf("%d%02d%03d ", DBA_VAR_F(info->var), DBA_VAR_X(info->var), DBA_VAR_Y(info->var));

		/* Then the variable description */
		printf("%s ", info->desc);

		/* Now the variable value.  This code shows all the main was of
		 * querying a dba_var.  For normal needs, you really just want to test
		 * info->var (or dba_var_code(var) ) for known values, then use the
		 * appropriate dba_var_enq* function for that code.
		 */
		if (dba_var_value(var) == NULL)
		{
			/* Missing value in the BUFR file */
			printf("missing.");
		} else {
			/* See if we are dealing with a string, an integer or a value with
			 * decimal digits */
			if (info->is_string)
			{
				/* It's a string */
				const char* value;
				/* Get the string value.  DBA_RUN_OR_RETURN is a short way of
				 * doing this:
				 *
				 * \code
				 *  dba_err err = dba_var_enqc(var, &value);
				 *  if (err != DBA_OK) return err;
				 * \endcode
				 *
				 * In this function we do not acquire or allocate any
				 * resources, so we can just reutnr the error code in case of
				 * errors.
				 */
				DBA_RUN_OR_RETURN(dba_var_enqc(var, &value));
				printf("%s ", value);
			}
			else if (info->scale == 0)
			{
				/* It's an integer number */
				int value;
				DBA_RUN_OR_RETURN(dba_var_enqi(var, &value));
				printf("%d ", value);
			}
			else
			{
				/* It's a number with decimal digits.  Read it in a double,
				 * then use the metadata to print it nicely using the right
				 * amount of significant digits */
				double value;
				DBA_RUN_OR_RETURN(dba_var_enqd(var, &value));
				printf("%.*f", info->scale > 0 ? info->scale : 0, value);
			}

			/* If the variable has a value, also print the unit of mesurement */
			printf("(%s)", info->unit);
		}
	
		/* Print an end of line at the end of the variable information */
		printf("\n");

		/* Now, a variable can optionally have attributes, such as information
		 * used for quality control.  Let's print that, too */
		for (iattr = dba_var_attr_iterate(var); iattr != NULL;
				iattr = dba_var_attr_iterator_next(iattr))
		{
			/* Attributes are just variables.  Although this would mean that
			 * you could theoretically set attributes for the attributes,
			 * luckily noone does that, so we do not worry about it */
			dba_var attr = dba_var_attr_iterator_attr(iattr);
			/* Print some spaces to align the attribute to its variable */
			printf("    ");
			/* This time we use the shortcut */
			dba_var_print(attr, stdout);
		}
	}

	/* If we have made it so far, then everything is fine.  This will reset the
	 * library error information and return a success indicator */
	return dba_error_ok();
}


int main(int argc, const char* argv[])
{
	/* Ensure that we get the name of a file */
	if (argc == 1)
	{
		fprintf(stderr, "Usage: %s file\n", argv[0]);
		return 1;
	}

	/* Run the dump and check that everthing is successful */
	if (do_dump(argv[1]) != DBA_OK)
	{
		/* If there were problems, we can use the dba_error_print_to_stderr()
		 * convenience function.  If more control is needed, have a look at
		 * dba_error_get_code(), dba_error_get_message() and dba_error_get_context());
		 */
		dba_error_print_to_stderr();
		return 1;
	}

	return 0;
}
