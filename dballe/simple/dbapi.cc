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
#include <cstring>

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

int DbAPI::enqi(const char* param)
{
    if (strcmp(param, "*ana_id") == 0)
        return db.last_station_id();
    else
        return CommonAPIImplementation::enqi(param);
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
        const char* varstr = output.key_peek_value(DBA_KEY_VAR);

        // Remember the varcode and reference ID for the next attribute
        // operations
        attr_varid = WR_STRING_TO_VAR(varstr + 1);
        attr_reference_id = output.get(DBA_KEY_CONTEXT_ID).enqi();

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

    // Mark the attr reference id as invalid, so that a critica() will call
    // attr_insert without the reference id
    attr_reference_id = -1;

    // If there was only one variable in the input, make it a valid default for
    // the next critica
    const vector<Var*> vars = input.vars();
    if (vars.size() == 1)
        attr_varid = vars[0]->code();
    else
        attr_varid = 0;
}

void DbAPI::dimenticami()
{
    if (! (perms & PERM_DATA_WRITE))
        throw error_consistency("dimenticami must be called with the database open in data write mode");

    db.remove(input);
}

int DbAPI::voglioancora()
{
    if (attr_reference_id == -1 || attr_varid == 0)
        throw error_consistency("voglioancora must be called after a dammelo, or after setting *context_id and *var_related");

    // Retrieve the varcodes of the attributes that we want
    std::vector<wreport::Varcode> arr;
    read_qc_list(arr);

    // Query attributes
    int qc_count = db.query_attrs(attr_reference_id, attr_varid, arr, qcoutput);
    qc_iter = 0;

    qcinput.clear();

    return qc_count;
}

void DbAPI::critica()
{
    if (perms & PERM_ATTR_RO)
        throw error_consistency(
            "critica cannot be called with the database open in attribute readonly mode");

    if (attr_varid == 0)
        throw error_consistency("critica must be called after a dammelo, a prendilo, or after setting *context_id and *var_related");

    if (attr_reference_id == -1)
        db.attr_insert(attr_varid, qcinput);
    else
        db.attr_insert(attr_reference_id, attr_varid, qcinput);

    qcinput.clear();
}

void DbAPI::scusa()
{
    if (! (perms & PERM_ATTR_WRITE))
        throw error_consistency(
            "scusa must be called with the database open in attribute write mode");

    if (attr_reference_id == -1 || attr_varid == 0)
        throw error_consistency("voglioancora must be called after a dammelo, or after setting *context_id and *var_related");

    // Retrieve the varcodes of the attributes we want to remove
    std::vector<wreport::Varcode> arr;
    read_qc_list(arr);

    db.attr_remove(attr_reference_id, attr_varid, arr);

    qcinput.clear();
}

}
}

/* vim:set ts=4 sw=4: */
