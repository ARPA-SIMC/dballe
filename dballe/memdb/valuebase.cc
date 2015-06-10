#include "valuebase.h"
#include "dballe/core/record.h"
#include <algorithm>

using namespace wreport;
using namespace std;

namespace dballe {
namespace memdb {

ValueBase::~ValueBase()
{
    delete var;
}

void ValueBase::replace(std::unique_ptr<Var> var)
{
    delete this->var;
    this->var = var.release();
}

void ValueBase::replace(const Var& var)
{
    this->var->copy_val_only(var);
}

void ValueBase::query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest) const
{
    for (const Var* a = var->next_attr(); a != NULL; a = a->next_attr())
        dest(newvar(*a));
}

void ValueBase::attr_insert(const Record& attrs)
{
    const auto& vars = core::Record::downcast(attrs).vars();
    for (vector<Var*>::const_iterator i = vars.begin(); i != vars.end(); ++i)
        var->seta(**i);
}

void ValueBase::attr_remove(const std::vector<wreport::Varcode>& qcs)
{
    // FIXME: if qcs is empty, remove all?
    if (qcs.empty())
    {
        var->clear_attrs();
    } else {
        for (vector<Varcode>::const_iterator i = qcs.begin(); i != qcs.end(); ++i)
            var->unseta(*i);
    }
}

}
}
