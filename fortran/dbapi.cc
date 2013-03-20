/*
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "dbapi.h"
#include <dballe/db/db.h>
//#include <dballe/core/aliases.h>
//#include <dballe/core/verbose.h>
//#include <dballe/db/internals.h>
//#include <cstdlib>

/*
#include <f77.h>
#include <float.h>
#include <limits.h>

#include <stdio.h>  // snprintf
#include <string.h> // strncpy
#include <math.h>
*/

using namespace wreport;
using namespace std;

namespace dballe {
namespace fortran {


DbAPI::DbAPI(DB& db, const char* anaflag, const char* dataflag, const char* attrflag)
    : db(db), ana_cur(0), query_cur(0)
{
    set_permissions(anaflag, dataflag, attrflag);
}

DbAPI::~DbAPI()
{
    if (ana_cur)
    {
        ana_cur->discard_rest();
        delete ana_cur;
    }
    if (query_cur)
    {
        query_cur->discard_rest();
        delete query_cur;
    }
}

void DbAPI::scopa(const char* repinfofile)
{
    if (!(perms & PERM_DATA_WRITE))
        error_consistency::throwf(
            "scopa must be run with the database open in data write mode");
    db.reset(repinfofile);
}

int DbAPI::quantesono()
{
    if (ana_cur != NULL)
    {
        ana_cur->discard_rest();
        delete ana_cur;
        ana_cur = 0;
    }
    ana_cur = db.query_stations(input).release();

#if 0
    if (dba_verbose_is_allowed(DBA_VERB_DB_INPUT))
    {
        dba_verbose(DBA_VERB_DB_INPUT,
                "invoking dba_db_ana_query(db, <input>).  <input> is:\n");
        dba_record_print(input, DBA_VERBOSE_STREAM);
    }
#endif

    return ana_cur->remaining();
}

void DbAPI::elencamele()
{
    if (ana_cur == NULL)
        throw error_consistency("elencamele called without a previous quantesono");

    output.clear();
    if (ana_cur->next())
        ana_cur->to_record(output);
    else
    {
        delete ana_cur;
        ana_cur = NULL;
    }
}

int DbAPI::voglioquesto()
{
    if (query_cur != NULL)
    {
        query_cur->discard_rest();
        delete query_cur;
        query_cur = NULL;
    }
    query_cur = db.query_data(input).release();

#if 0
    if (dba_verbose_is_allowed(DBA_VERB_DB_INPUT))
    {
        dba_verbose(DBA_VERB_DB_INPUT,
                "invoking dba_query(db, <input>).  <input> is:\n");
        dba_record_print(input, DBA_VERBOSE_STREAM);
    }
#endif

    return query_cur->remaining();
}

const char* DbAPI::dammelo()
{
    if (query_cur == NULL)
        throw error_consistency("dammelo called without a previous voglioquesto");

    /* Reset qc record iterator, so that ancora will not return
     * leftover QC values from a previous query */
    qc_iter = -1;

    output.clear();
    if (query_cur->next())
    {
        query_cur->to_record(output);

        /* Set context id and variable name on qcinput so that
         * attribute functions will refer to the last variable read */
        const char* varstr = output.key_peek_value(DBA_KEY_VAR);
        qcinput.set(DBA_KEY_CONTEXT_ID, (int)query_cur->out_context_id);
        qcinput.set(DBA_KEY_VAR_RELATED, varstr);
        return varstr;
    } else {
        delete query_cur;
        query_cur = NULL;
        return 0;
    }
}

void DbAPI::prendilo()
{
    if (perms & PERM_DATA_RO)
        throw error_consistency(
            "idba_prendilo cannot be called with the database open in data readonly mode");

#if 0
    if (dba_verbose_is_allowed(DBA_VERB_DB_INPUT))
    {
        dba_verbose(DBA_VERB_DB_INPUT,
                "invoking dba_insert_or_replace(db, <input>, %d, %d).  <input> is:\n",
                perms & PERM_DATA_WRITE ? 1 : 0,
                perms & PERM_ANA_WRITE ? 1 : 0);
        dba_record_print(input, DBA_VERBOSE_STREAM);
    }
#endif

    db.insert(input, (perms & PERM_DATA_WRITE) != 0, (perms & PERM_ANA_WRITE) != 0);
    int ana_id = input.key_peek(DBA_KEY_ANA_ID)->enqi();
    int context_id = input.key_peek(DBA_KEY_CONTEXT_ID)->enqi();
    // Uncache to prevent confusion on the next insert
    // input.unset(DBA_KEY_ANA_ID); Not needed here, it is reset automatically
    //                              when trying to set a latitude or longitude
    input.unset(DBA_KEY_CONTEXT_ID);

    /* Set the values in the output */
    output.set(DBA_KEY_ANA_ID, ana_id);
    output.set(DBA_KEY_CONTEXT_ID, context_id);

    /* Set context id and variable name on qcinput so that
     * attribute functions will refer to what has been written */
    qcinput.set(DBA_KEY_CONTEXT_ID, context_id);

    /* If there was only one variable in the input, we can pass it on as a
     * default for attribute handling routines; otherwise we unset to mark
     * the ambiguity */
    const vector<Var*> vars = input.vars();
    if (vars.size() == 1)
    {
        Varcode code = vars[0]->code();
        char varname[8];
        snprintf(varname, 7, "B%02d%03d", WR_VAR_X(code), WR_VAR_Y(code));
        qcinput.set(DBA_KEY_VAR_RELATED, varname);
    }
    else
        qcinput.unset(DBA_KEY_VAR_RELATED);
}

void DbAPI::dimenticami()
{
    if (! (perms & PERM_DATA_WRITE))
        throw error_consistency("dimenticami must be called with the database open in data write mode");

    db.remove(input);
}

int DbAPI::voglioancora()
{
    int id_context;
    Varcode id_var;

    /* Retrieve the ID of the data to query */
    get_referred_data_id(&id_context, &id_var);

    /* Retrieve the varcodes of the wanted QC values */
    std::vector<wreport::Varcode> arr;
    read_qc_list(arr);

    /* Do QC query */
    int qc_count = db.query_attrs(id_context, id_var, arr, qcoutput);
    qc_iter = 0;

    clear_qcinput();

    return qc_count;
}

void DbAPI::critica()
{
    if (perms & PERM_ATTR_RO)
        throw error_consistency(
            "critica cannot be called with the database open in attribute readonly mode");

    int id_context;
    Varcode id_var;
    get_referred_data_id(&id_context, &id_var);

    db.attr_insert_or_replace(id_context, id_var, qcinput, (perms & PERM_ATTR_WRITE) != 0);

    clear_qcinput();
}

void DbAPI::scusa()
{
    if (! (perms & PERM_ATTR_WRITE))
        throw error_consistency(
            "scusa must be called with the database open in attribute write mode");

    int id_context;
    Varcode id_var;
    get_referred_data_id(&id_context, &id_var);

    /* Retrieve the varcodes of the wanted QC values */
    std::vector<wreport::Varcode> arr;
    read_qc_list(arr);

    // If arr is still 0, then dba_qc_delete deletes all QC values
    db.attr_remove(id_context, id_var, arr);

    clear_qcinput();
}

}
}

/* vim:set ts=4 sw=4: */
