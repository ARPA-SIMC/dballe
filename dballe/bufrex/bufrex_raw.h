#ifndef DBALLE_BUFREX_RAW_H
#define DBALLE_BUFREX_RAW_H

#ifdef  __cplusplus
extern "C" {
#endif

/** @file
 * @ingroup bufrex
 * Intermediate representation of parsed values, ordered according to a BUFR or
 * CREX message template.
 */

#include <dballe/err/dba_error.h>
#include <dballe/core/dba_var.h>
#include <dballe/msg/dba_msg.h>
#include <dballe/io/dba_rawmsg.h>
#include <dballe/bufrex/bufrex_dtable.h>

typedef enum { BUFREX_BUFR, BUFREX_CREX } bufrex_type;

struct _bufrex_opcode;

struct _bufrex_bufr_options {
	/* Common Code table C-1 identifying the originating centre */
	int origin;
	/* Version number of master tables used */
	int master_table;
	/* Version number of local tables used to augment the master table */
	int local_table;
};
struct _bufrex_crex_options {
	/* Master table (00 for standard WMO FM95 CREX tables) */
	int master_table;
	/* Table version number */
	int table;
};

struct _bufrex_raw
{
	/* Type of source/target encoding data */
	bufrex_type encoding_type;

	union {
		struct _bufrex_crex_options crex;
		struct _bufrex_bufr_options bufr;
	} opt;

	/* Message category */
	int type;
	/* Message subcategory */
	int subtype;

	/* Edition number */
	int edition;

	/* Representative datetime for this data */
	int rep_year, rep_month, rep_day, rep_hour, rep_minute;

	/* dba_vartable used to lookup B table codes */
	dba_vartable btable;
	/* bufrex_dtable used to lookup D table codes */
	bufrex_dtable dtable;

	/* Decoded variables */
	dba_var* vars;
	/* Number of decoded variables */
	int vars_count;
	/* Size (in dba_var*) of the buffer allocated for vars */
	int vars_alloclen;

	/* Parsed CREX data descriptor section */
	bufrex_opcode datadesc;
	/* Pointer to end of the datadesc chain, used to point to the insertion
	 * point for appends; it always points to a NULL pointer */
	bufrex_opcode* datadesc_last;
};
typedef struct _bufrex_raw* bufrex_raw;

dba_err bufrex_raw_create(bufrex_raw* msg, bufrex_type type);

void bufrex_raw_delete(bufrex_raw msg);

void bufrex_raw_reset(bufrex_raw msg);

/**
 * Get the ID of the table used by this bufrex_raw
 *
 * @retval id
 *   The table id, as a pointer to an internal string.  It must not be
 *   deallocated by the caller.  It is set to NULL when no table has been set.
 * @returns
 *   The error indicator for the function (@see dba_err)
 */
dba_err bufrex_raw_get_table_id(bufrex_raw msg, const char** id);

/**
 * Load a new set of tables to use for encoding this message
 */
dba_err bufrex_raw_load_tables(bufrex_raw msg);

/**
 * Query the WMO B table used for this BUFR/CREX data
 *
 * @param msg
 *   ::bufrex_raw to query
 * @param var
 *   code of the variable to query
 * @retval info
 *   the ::dba_varinfo structure with the results of the query.  The returned
 *   ::dba_varinfo needs to be deallocated using dba_varinfo_delete()
 * @return
 *   The error status (@see dba_err)
 */
dba_err bufrex_raw_query_btable(bufrex_raw msg, dba_varcode code, dba_varinfo* info);

/**
 * Query the WMO D table used for this BUFR/CREX data
 * 
 * @param msg
 *   ::bufrex_raw to query
 * @param var
 *   code of the entry to query
 * @param res
 *   the bufrex_opcode chain that contains the expansion elements
 *   (must be deallocated by the caller using bufrex_opcode_delete)
 * @return
 *   The error status (@see dba_err)
 */
dba_err bufrex_raw_query_dtable(bufrex_raw msg, dba_varcode code, struct _bufrex_opcode** res);

/**
 * Reset the data descriptor section for the message
 *
 * @param msg
 *   The message to act on
 */
void bufrex_raw_reset_datadesc(bufrex_raw msg);

/**
 * Get the data descriptor section of this ::bufrex_raw
 *
 * @param msg
 *   The message to act on
 * @retval res
 *   A copy of the internal list of data descriptors for the data descriptor
 *   section.  It must be deallocated by the caller using
 *   bufrex_opcode_delete()
 * @returns
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err bufrex_raw_get_datadesc(bufrex_raw msg, struct _bufrex_opcode** res);

/**
 * Append one ::dba_varcode to the data descriptor section of the message
 *
 * @param msg
 *   The message to act on
 * @param varcode
 *   The ::dba_varcode to append
 * @returns
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err bufrex_raw_append_datadesc(bufrex_raw msg, dba_varcode varcode);

/**
 * Store a decoded variable in the message, to be encoded later.
 *
 * The function will take ownership of the dba_var, and when the message is
 * destroyed or reset, ::dba_var_delete() will be called on it.
 *
 * \param msg
 *   The message that will hold the variable
 * \param var
 *   The variable to store in the message.  The message will take ownership of
 *   memory management for the variable, which will be deallocated when the
 *   message is deleted or reset.
 * \return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_store_variable(bufrex_raw msg, dba_var var);

/**
 * Store a new variable in the message, copying it from an already existing
 * variable.
 *
 * @param msg
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @param var
 *   The variable holding the value for the variable to add
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_store_variable_var(bufrex_raw msg, dba_varcode code, dba_var var);

/**
 * Store a new variable in the message, providing its value as an int
 *
 * @param msg
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @param val
 *   The value for the variable
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_store_variable_i(bufrex_raw msg, dba_varcode code, int val);

/**
 * Store a new variable in the message, providing its value as a double
 *
 * @param msg
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @param val
 *   The value for the variable
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_store_variable_d(bufrex_raw msg, dba_varcode code, double val);

/**
 * Store a new variable in the message, providing its value as a string
 *
 * @param msg
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @param val
 *   The value for the variable
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_store_variable_c(bufrex_raw msg, dba_varcode code, const char* val);

/**
 * Store a new, undefined variable in the message
 *
 * @param msg
 *   The message that will hold the variable
 * @param code
 *   The ::dba_varcode of the variable to add
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_store_variable_undef(bufrex_raw msg, dba_varcode code);

/**
 * Copy all the attributes from 'var' into the last variable that was
 * previously stored.
 *
 * @param msg
 *   The message to operate on
 * @param var
 *   The variable with the attributes to copy
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_add_attrs(bufrex_raw msg, dba_var var); 

/**
 * Copy decoded variables that are attributes as attributes in the decoded
 * variables they refer to.
 *
 * @param msg
 *   The message to operate on
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_apply_attributes(bufrex_raw msg);

/**
 * Compute and append a data present bitmap
 *
 * @param msg
 *   The message to operate on
 * @param size
 *   The size of the bitmap
 * @param attr
 *   The code of the attribute that the bitmap will represent
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_append_dpb(bufrex_raw msg, int size, dba_varcode attr);

/**
 * Append a fixed-size data present bitmap with all zeros
 *
 * @param msg
 *   The message to operate on
 * @param size
 *   The size of the bitmap
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_append_fixed_dpb(bufrex_raw msg, int size);

/**
 * Scan the first 'size' variables appending the attribute 'attr' when found.
 *
 * The delayed replicator factor with the number of attributes found will also
 * be appended before the attributes.
 *
 * @param msg
 *   The message to operate on
 * @param size
 *   The number of variables to scan
 * @param attr
 *   The code of the attribute to look for
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_append_attrs(bufrex_raw msg, int size, dba_varcode attr);

/**
 * Scan the first 'size' variables appending the attribute 'attr' in any case.
 *
 * Exactly 'size' attributes will be appended, possibly with value 'undef' when
 * they are not present.  No delayed replicator factor is appended.
 *
 * @param msg
 *   The message to operate on
 * @param size
 *   The number of variables to scan
 * @param attr
 *   The code of the attribute to look for
 * @return
 *   The error indicator for the function.  @see dba_err
 */
dba_err bufrex_raw_append_fixed_attrs(bufrex_raw msg, int size, dba_varcode attr);


/**
 * Parse an encoded message into a bufrex_raw
 */
dba_err bufrex_raw_decode(bufrex_raw msg, dba_rawmsg raw);

/**
 * Encode the contents of the bufrex_raw
 */
dba_err bufrex_raw_encode(bufrex_raw msg, dba_rawmsg* raw);

/**
 * Fill in the bufrex_raw with the contents of a dba_msg
 */
dba_err bufrex_raw_from_msg(bufrex_raw raw, dba_msg msg);

/**
 * Fill in a dba_msg with the contents of the bufrex_raw
 */
dba_err bufrex_raw_to_msg(bufrex_raw raw, dba_msg* msg);


/**
 * Encode a BUFR message
 * 
 * @param in
 *   The ::bufrex_raw with the data to encode
 * @param out
 *   The ::dba_rawmsg that will hold the encoded data
 * @param opt
 *   The bufr_options that controls the encoding process.  If NULL is passed,
 *   defaults will be used.
 * @return
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err bufr_encoder_encode(bufrex_raw in, dba_rawmsg out);

/**
 * Decode a BUFR message
 * 
 * @param in
 *   The ::dba_msgraw with the data to decode
 * @param out
 *   The ::bufrex_raw that will hold the decoded data
 * @retval opt
 *   A newly created bufr_options with informations about the decoding process.
 *   If NULL is passed, nothing will be returned.  If a bufr_options is
 *   returned, it will need to be deleted with bufr_options_delete().
 * @return
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err bufr_decoder_decode(dba_rawmsg in, bufrex_raw out);

/**
 * Encode a CREX message
 * 
 * @param in
 *   The ::bufrex_raw with the data to encode
 * @param out
 *   The ::dba_rawmsg that will hold the encoded data
 * @param opt
 *   The bufr_options that controls the encoding process.  If NULL is passed,
 *   defaults will be used.
 * @return
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err crex_encoder_encode(bufrex_raw in, dba_rawmsg out);

/**
 * Decode a CREX message
 * 
 * @param in
 *   The ::dba_msgraw with the data to decode
 * @param out
 *   The ::bufrex_raw that will hold the decoded data
 * @retval opt
 *   A newly created bufr_options with informations about the decoding process.
 *   If NULL is passed, nothing will be returned.  If a bufr_options is
 *   returned, it will need to be deleted with bufr_options_delete().
 * @return
 *   The error indicator for the function.  @see ::dba_err
 */
dba_err crex_decoder_decode(dba_rawmsg in, bufrex_raw out);

/**
 * Infer good type and subtype from a dba_msg
 */
dba_err bufrex_infer_type_subtype(dba_msg msg, int* type, int* subtype);
	
/**
 * Dump the contents of this bufrex_raw
 */
void bufrex_raw_print(bufrex_raw msg, FILE* out);

#ifdef  __cplusplus
}
#endif

/* vim:set ts=4 sw=4: */
#endif
