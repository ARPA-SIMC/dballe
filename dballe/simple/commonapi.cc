#include "commonapi.h"
#include "dballe/var.h"
#include "dballe/types.h"
#include "dballe/core/record-access.h"
#include <stdio.h>  // snprintf
#include <limits>
#include <cstdlib>
#include <cstring>
#include <strings.h>

using namespace dballe;
using namespace wreport;
using namespace std;

namespace dballe {
namespace fortran {

wreport::Varcode Attributes::next()
{
    if (!valid)
        throw error_consistency("ancora called without a previous voglioancora");
    if (current == values.end())
        throw error_notfound("ancora called with no (or no more) results available");

    Varcode res = current->code();
    ++current;
    return res;
}

void Attributes::invalidate()
{
    valid = false;
    values.clear();
}

void Attributes::has_new_values()
{
    valid = true;
    current = values.begin();
}

Operation::~Operation() {}
void Operation::set_varcode(wreport::Varcode varcode) {}
bool Operation::elencamele() { throw error_consistency("elencamele called without a previous quantesono"); }
wreport::Varcode Operation::dammelo() { throw error_consistency("dammelo called without a previous voglioquesto"); }

signed char Operation::enqb(const char* param) const
{
    int value = enqi(param);

    if (value == API::missing_int)
        return API::missing_byte;

    if (value < numeric_limits<signed char>::min()
            || value > numeric_limits<signed char>::max())
        error_consistency::throwf("value queried (%d) does not fit in a byte", value);
    return value;
}

float Operation::enqr(const char* param) const
{
    double value = enqd(param);

    if (value == API::missing_double)
        return API::missing_float;

    if (value < -numeric_limits<float>::max()
            || value > numeric_limits<float>::max())
        error_consistency::throwf("value queried (%f) does not fit in a real", value);
    return value;
}


CommonAPIImplementation::CommonAPIImplementation()
{
}

CommonAPIImplementation::~CommonAPIImplementation()
{
    delete operation;
}

unsigned CommonAPIImplementation::compute_permissions(const char* anaflag, const char* dataflag, const char* attrflag)
{
    unsigned perms = 0;

    if (strcasecmp("read",  anaflag) == 0)
        perms |= PERM_ANA_RO;
    if (strcasecmp("write", anaflag) == 0)
        perms |= PERM_ANA_WRITE;
    if (strcasecmp("read",  dataflag) == 0)
        perms |= PERM_DATA_RO;
    if (strcasecmp("add",   dataflag) == 0)
        perms |= PERM_DATA_ADD;
    if (strcasecmp("write", dataflag) == 0)
        perms |= PERM_DATA_WRITE;
    if (strcasecmp("read",  attrflag) == 0)
        perms |= PERM_ATTR_RO;
    if (strcasecmp("write", attrflag) == 0)
        perms |= PERM_ATTR_WRITE;

    if ((perms & (PERM_ANA_RO | PERM_ANA_WRITE)) == 0)
        throw error_consistency("pseudoana should be opened in either 'read' or 'write' mode");
    if ((perms & (PERM_DATA_RO | PERM_DATA_ADD | PERM_DATA_WRITE)) == 0)
        throw error_consistency("data should be opened in one of 'read', 'add' or 'write' mode");
    if ((perms & (PERM_ATTR_RO | PERM_ATTR_WRITE)) == 0)
        throw error_consistency("attr should be opened in either 'read' or 'write' mode");

    if (perms & PERM_ANA_RO && perms & PERM_DATA_WRITE)
        throw error_consistency("when data is 'write' ana must also be set to 'write', because deleting data can potentially also delete pseudoana");
    /*
    // Check disabled: allowing importing data without attributes is more
    // important than a dubious corner case
    if (perms & PERM_ATTR_RO && perms & PERM_DATA_WRITE)
        throw error_consistency("when data is 'write' attr must also be set to 'write', because deleting data also deletes its attributes");
    */

    return perms;
}

int CommonAPIImplementation::enqi(const char* param)
{
    if (param[0] == '*')
    {
        if (strcmp(param + 1, "context_id") == 0)
            // TODO: it seems that this always returned missing, by querying a Record that was never set
            return missing_int;
        else
        {
            if (!qcoutput.valid)
                error_consistency::throwf("enqi %s can only be called after a voglioancora", param);
            wreport::Varcode code = resolve_varcode(param + 1);
            return qcoutput.values.enq(code, API::missing_int);
        }
    }
    if (!operation) return missing_int;
    return operation->enqi(param);
}

signed char CommonAPIImplementation::enqb(const char* param)
{
    if (!operation) return missing_byte;
    return operation->enqb(param);
}

float CommonAPIImplementation::enqr(const char* param)
{
    if (!operation) return missing_float;
    return operation->enqr(param);
}

double CommonAPIImplementation::enqd(const char* param)
{
    if (param[0] == '*')
    {
        if (!qcoutput.valid)
            error_consistency::throwf("enqd %s can only be called after a voglioancora", param);
        wreport::Varcode code = resolve_varcode(param + 1);
        return qcoutput.values.enq(code, API::missing_double);
    }
    if (!operation) return missing_double;
    return operation->enqd(param);
}

bool CommonAPIImplementation::enqc(const char* param, std::string& res)
{
    if (param[0] == '*')
    {
        if (!qcoutput.valid)
            error_consistency::throwf("enqc %s can only be called after a voglioancora", param);
        wreport::Varcode code = resolve_varcode(param + 1);
        const Var* var = qcoutput.values.get_var(code);
        if (!var) return false;
        if (!var->isset()) return false;
        res = var->enqc();
        return true;
    }
    if (!operation) return false;
    return operation->enqc(param, res);
}

void CommonAPIImplementation::enqlevel(int& ltype1, int& l1, int& ltype2, int& l2)
{
    if (!operation)
        ltype1 = l1 = ltype2 = l2 = MISSING_INT;
    else
        return operation->enqlevel(ltype1, l1, ltype2, l2);
}

void CommonAPIImplementation::enqtimerange(int& pind, int& p1, int& p2)
{
    if (!operation)
        pind = p1 = p2 = MISSING_INT;
    else
        return operation->enqtimerange(pind, p1, p2);
}

void CommonAPIImplementation::enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
    if (!operation)
        year = month = day = hour = min = sec = MISSING_INT;
    else
        return operation->enqdate(year, month, day, hour, min, sec);
}


void CommonAPIImplementation::seti(const char* param, int value)
{
    if (param[0] == '*')
    {
        qcinput.set(resolve_varcode(param + 1), value);
        return;
    }
    if (!_seti(param, strlen(param), value))
        input_data.values.set(resolve_varcode(param), value);
}

void CommonAPIImplementation::setb(const char* param, signed char value)
{
    return seti(param, value);
}

void CommonAPIImplementation::setr(const char* param, float value)
{
    return setd(param, value);
}

void CommonAPIImplementation::setd(const char* param, double value)
{
    if (param[0] == '*')
    {
        qcinput.set(resolve_varcode(param + 1), value);
        return;
    }
    if (!_setd(param, strlen(param), value))
        input_data.values.set(resolve_varcode(param), value);
}

void CommonAPIImplementation::setc(const char* param, const char* value)
{
    if (param[0] == '*')
    {
        if (strcmp(param + 1, "var_related") == 0)
        {
            if (!operation)
                throw error_consistency("*var_related set without context_id, or before any dammelo or prendilo");
            operation->set_varcode(resolve_varcode(value));
        } else if (strcmp(param + 1, "var") == 0) {
            if (!operation)
                throw error_consistency("*var set without context_id, or before any dammelo or prendilo");
            std::vector<wreport::Varcode> varcodes { resolve_varcode(value + 1) };
            operation->select_attrs(varcodes);
        } else if (strcmp(param + 1, "varlist") == 0) {
            if (!operation)
                throw error_consistency("*varlist set without context_id, or before any dammelo or prendilo");
            std::vector<wreport::Varcode> varcodes;
            size_t pos = 0;
            while (true)
            {
                size_t len = strcspn(value + pos, ",");
                if (len == 0) break;

                if (*(value + pos) != '*')
                    throw error_consistency("QC value names must start with '*'");
                varcodes.push_back(resolve_varcode(value + pos + 1));

                if (!*(value + pos + len))
                    break;
                pos += len + 1;
            }
            operation->select_attrs(varcodes);
        } else {
            // Set varcode=value
            qcinput.set(resolve_varcode(param + 1), value);
        }
        return;
    }
    if (!_setc(param, strlen(param), value))
        input_data.values.set(resolve_varcode(param), value);
}

void CommonAPIImplementation::setcontextana()
{
    input_query.datetime = DatetimeRange();
    input_data.datetime = Datetime();
    input_query.level = input_data.level = Level();
    input_query.trange = input_data.trange = Trange();
    station_context = true;
}

void CommonAPIImplementation::setlevel(int ltype1, int l1, int ltype2, int l2)
{
    Level level(ltype1, l1, ltype2, l2);
    if (!level.is_missing())
        station_context = false;
    input_query.level = input_data.level = level;
}

void CommonAPIImplementation::settimerange(int ptype, int p1, int p2)
{
    Trange trange(ptype, p1, p2);
    if (!trange.is_missing())
        station_context = false;
    input_query.trange = input_data.trange = trange;
}

void CommonAPIImplementation::setdate(int year, int month, int day, int hour, int min, int sec)
{
    Datetime dt(year, month, day, hour, min, sec);
    if (!dt.is_missing())
        station_context = false;
    input_query.datetime.min = input_query.datetime.max = input_data.datetime = dt;
}

void CommonAPIImplementation::setdatemin(int year, int month, int day, int hour, int min, int sec)
{
    input_query.datetime.min = Datetime(year, month, day, hour, min, sec);
    if (!input_query.datetime.min.is_missing())
        station_context = true;
}

void CommonAPIImplementation::setdatemax(int year, int month, int day, int hour, int min, int sec)
{
    input_query.datetime.max = Datetime(year, month, day, hour, min, sec);
    if (!input_query.datetime.max.is_missing())
        station_context = true;
}

void CommonAPIImplementation::unset(const char* param)
{
    if (param[0] == '*')
    {
        if (strcmp(param + 1, "var_related") == 0)
        {
            if (!operation)
                throw error_consistency("*var_related set without context_id, or before any dammelo or prendilo");
            operation->set_varcode(0);
        } else if (strcmp(param + 1, "var") == 0) {
            if (!operation) return;
            operation->select_attrs(std::vector<wreport::Varcode>());
        } else if (strcmp(param + 1, "varlist") == 0) {
            if (!operation) return;
            operation->select_attrs(std::vector<wreport::Varcode>());
        } else {
            qcinput.unset(resolve_varcode(param + 1));
        }
        return;
    }
    if (!_unset(param, strlen(param)))
        input_data.values.unset(resolve_varcode(param));
}

void CommonAPIImplementation::unsetall()
{
    qcinput.clear();
    input_query.clear();
    input_data.clear();
    if (operation)
    {
        operation->set_varcode(0);
        operation->select_attrs(std::vector<wreport::Varcode>());
    }
    station_context = false;
}

void CommonAPIImplementation::unsetb()
{
    qcinput.clear();
    input_data.clear_vars();
}

const char* CommonAPIImplementation::spiegal(int ltype1, int l1, int ltype2, int l2)
{
    cached_spiega = Level(ltype1, l1, ltype2, l2).describe();
    return cached_spiega.c_str();
}

const char* CommonAPIImplementation::spiegat(int ptype, int p1, int p2)
{
    cached_spiega = Trange(ptype, p1, p2).describe();
    return cached_spiega.c_str();
}

const char* CommonAPIImplementation::spiegab(const char* varcode, const char* value)
{
    Varinfo info = varinfo(WR_STRING_TO_VAR(varcode + 1));
    Var var(info, value);

    char buf[1024];
    switch (info->type)
    {
        case Vartype::String:
            snprintf(buf, 1023, "%s (%s) %s", var.enqc(), info->unit, info->desc);
            break;
        case Vartype::Binary:
            snprintf(buf, 1023, "%s (%s) %s", var.enqc(), info->unit, info->desc);
            break;
        case Vartype::Integer:
        case Vartype::Decimal:
            snprintf(buf, 1023, "%.*f (%s) %s", info->scale > 0 ? info->scale : 0, var.enqd(), info->unit, info->desc);
            break;
    }
    cached_spiega = buf;
    return cached_spiega.c_str();
}

void CommonAPIImplementation::elencamele()
{
    if (!operation)
        throw error_consistency("elencamele called without a previous quantesono");
    if (!operation->elencamele())
        reset_operation();
}

wreport::Varcode CommonAPIImplementation::dammelo()
{
    if (!operation) throw error_consistency("dammelo called without a previous voglioquesto");
    qcoutput.invalidate();
    return operation->dammelo();
}

int CommonAPIImplementation::voglioancora()
{
    // Query attributes
    if (!operation) throw error_consistency("voglioancora was not called after a dammelo, or was called with an invalid *context_id or *var_related");
    operation->voglioancora(qcoutput);
    qcinput.clear();
    return qcoutput.values.size();
}

const char* CommonAPIImplementation::ancora()
{
    static char parm[10] = "*";
    Varcode code = qcoutput.next();
    format_bcode(code, parm + 1);
    return parm;
}

void CommonAPIImplementation::critica()
{
    if (perms & PERM_ATTR_RO)
        throw error_consistency(
            "critica cannot be called with the database open in attribute readonly mode");

    if (!operation) throw error_consistency("critica was not called after a dammelo or prendilo, or was called with an invalid *context_id or *var_related");
    operation->critica(qcinput);
    qcinput.clear();
}

void CommonAPIImplementation::scusa()
{
    if (! (perms & PERM_ATTR_WRITE))
        throw error_consistency(
            "scusa must be called with the database open in attribute write mode");


    // Retrieve the varcodes of the attributes we want to remove
    if (!operation) throw error_consistency("scusa was not called after a dammelo, or was called with an invalid *context_id or *var_related");
    operation->scusa();
    qcinput.clear();
}

void CommonAPIImplementation::fatto()
{
}

}
}
