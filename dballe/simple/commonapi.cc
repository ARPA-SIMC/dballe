#include "commonapi.h"
#include "dballe/var.h"
#include "dballe/types.h"
#include "dballe/db/db.h"
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

Operation::~Operation() {}
void Operation::set_varcode(wreport::Varcode varcode) {}
const char* Operation::dammelo(dballe::Record& output) { throw error_consistency("dammelo called without a previous voglioquesto"); }


struct VaridOperation : public Operation
{
    wreport::Varcode varcode;
    int varid;

    VaridOperation(int varid) : varid(varid) {}
    void set_varcode(wreport::Varcode varcode) override { this->varcode = varcode; }
    void voglioancora(db::Transaction& tr, std::function<void(std::unique_ptr<wreport::Var>&&)> dest) override
    {
        if (!varid)
            throw error_consistency("voglioancora called with an invalid *context_id");
        tr.attr_query_data(varid, dest);
    }
    void critica(db::Transaction& tr, const core::Record& qcinput) override
    {
        tr.attr_insert_data(varid, qcinput);
    }
    void scusa(db::Transaction& tr, const std::vector<wreport::Varcode>& attrs) override
    {
        tr.attr_remove_data(varid, attrs);
    }
};



CommonAPIImplementation::CommonAPIImplementation()
    : perms(0), qc_iter(-1), qc_count(0)
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

Record& CommonAPIImplementation::choose_input_record(const char*& param)
{
    switch (param[0])
    {
        case '*':
            param = param + 1;
            return qcinput;
        default:
            return input;
    }
}

Record& CommonAPIImplementation::choose_output_record(const char*& param)
{
    switch (param[0])
    {
        case '*':
            param = param + 1;
            return qcoutput;
        default:
            return output;
    }
}

void CommonAPIImplementation::test_input_to_output()
{
    output = input;
}

int CommonAPIImplementation::enqi(const char* param)
{
    Record& rec = choose_output_record(param);
    return rec.enq(param, missing_int);
}

signed char CommonAPIImplementation::enqb(const char* param)
{
    Record& rec = choose_output_record(param);
    int value = rec.enq(param, missing_int);
    if (value == missing_int)
        return missing_byte;

    if (value < numeric_limits<signed char>::min()
            || value > numeric_limits<signed char>::max())
        error_consistency::throwf("value queried (%d) does not fit in a byte", value);
    return value;
}

float CommonAPIImplementation::enqr(const char* param)
{
    Record& rec = choose_output_record(param);
    double value = rec.enq(param, missing_double);
    if (value == missing_double)
        return missing_float;

    if (value < -numeric_limits<float>::max()
            || value > numeric_limits<float>::max())
        error_consistency::throwf("value queried (%f) does not fit in a real", value);
    return value;
}

double CommonAPIImplementation::enqd(const char* param)
{
    Record& rec = choose_output_record(param);
    return rec.enq(param, missing_double);
}

const char* CommonAPIImplementation::enqc(const char* param)
{
    Record& rec = choose_output_record(param);
    return rec.enq(param, (const char*)nullptr);
}

void CommonAPIImplementation::seti(const char* param, int value)
{
    if (param[0] == '*')
    {
        if (strcmp(param + 1, "context_id") == 0)
        {
            delete operation;
            if (value == MISSING_INT)
                operation = nullptr;
            else
                operation = new VaridOperation(value);
        } else {
            qcinput.seti(param + 1, value);
        }
    } else
        input.seti(param, value);
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
    Record& rec = choose_input_record(param);
    rec.set(param, value);
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
        } else {
            qcinput.setc(param + 1, value);
        }
    } else
        input.setc(param, value);
}

void CommonAPIImplementation::setcontextana()
{
    input.set_datetime(Datetime());
    input.set_level(Level());
    input.set_trange(Trange());
    station_context = true;
}

void CommonAPIImplementation::enqlevel(int& ltype1, int& l1, int& ltype2, int& l2)
{
    ltype1 = output.enq("leveltype1", API::missing_int);
    l1 = output.enq("l1", API::missing_int);
    ltype2 = output.enq("leveltype2", API::missing_int);
    l2 = output.enq("l2", API::missing_int);
}

void CommonAPIImplementation::setlevel(int ltype1, int l1, int ltype2, int l2)
{
    Level level(ltype1, l1, ltype2, l2);
    if (!level.is_missing())
        station_context = false;
    input.set_level(level);
}

void CommonAPIImplementation::enqtimerange(int& ptype, int& p1, int& p2)
{
    ptype = output.enq("pindicator", API::missing_int);
    p1 = output.enq("p1", API::missing_int);
    p2 = output.enq("p2", API::missing_int);
}

void CommonAPIImplementation::settimerange(int ptype, int p1, int p2)
{
    Trange trange(ptype, p1, p2);
    if (!trange.is_missing())
        station_context = false;
    input.set_trange(trange);
}

void CommonAPIImplementation::enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec)
{
    year = output.enq("year", API::missing_int);
    month = output.enq("month", API::missing_int);
    day = output.enq("day", API::missing_int);
    hour = output.enq("hour", API::missing_int);
    min = output.enq("min", API::missing_int);
    sec = output.enq("sec", API::missing_int);
}

void CommonAPIImplementation::setdate(int year, int month, int day, int hour, int min, int sec)
{
    Datetime dt(year, month, day, hour, min, sec);
    if (!dt.is_missing())
        station_context = false;
    input.set_datetime(dt);
}

void CommonAPIImplementation::setdatemin(int year, int month, int day, int hour, int min, int sec)
{
    DatetimeRange dtr = input.get_datetimerange();
    dtr.min = Datetime(year, month, day, hour, min, sec);
    if (!dtr.min.is_missing())
        station_context = true;
    input.set_datetimerange(dtr);
}

void CommonAPIImplementation::setdatemax(int year, int month, int day, int hour, int min, int sec)
{
    DatetimeRange dtr = input.get_datetimerange();
    dtr.max = Datetime(year, month, day, hour, min, sec);
    if (!dtr.max.is_missing())
        station_context = true;
    input.set_datetimerange(dtr);
}

void CommonAPIImplementation::unset(const char* param)
{
    Record& rec = choose_input_record(param);
    rec.unset(param);
}

void CommonAPIImplementation::unsetall()
{
    qcinput.clear();
    input.clear();
    station_context = false;
}

void CommonAPIImplementation::unsetb()
{
    qcinput.clear_vars();
    input.clear_vars();
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

const char* CommonAPIImplementation::ancora()
{
    static char parm[10] = "*";

    if (qc_iter < 0)
        throw error_consistency("ancora called without a previous voglioancora");
    if ((unsigned)qc_iter >= qcoutput.vars().size())
        throw error_notfound("ancora called with no (or no more) results available");

    Varcode var = qcoutput.vars()[qc_iter]->code();
    format_bcode(var, parm + 1);

    /* Get next value from qc */
    ++qc_iter;

    return parm;
}

void CommonAPIImplementation::fatto()
{
}

void CommonAPIImplementation::read_qc_list(vector<Varcode>& res_arr) const
{
    res_arr.clear();
    if (const char* val = qcinput.enq("var", (const char*)nullptr))
    {
        // Get only the QC values in *varlist
        if (*val != '*')
            throw error_consistency("QC values must start with '*'");

        res_arr.push_back(resolve_varcode(val + 1));
        return;
    }

    if (const char* val = qcinput.enq("varlist", (const char*)nullptr))
    {
        // Get only the QC values in *varlist
        size_t pos;
        size_t len;

        for (pos = 0; (len = strcspn(val + pos, ",")) > 0; pos += len + 1)
        {
            if (*(val + pos) != '*')
                throw error_consistency("QC value names must start with '*'");
            res_arr.push_back(resolve_varcode(val + pos + 1));
        }
    }
}

}
}
