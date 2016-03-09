#include <dballe/msg/context.h>
#include <dballe/msg/vars.h>
#include <wreport/notes.h>
#include <dballe/core/ostream.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {

Context::Context(const Level& lev, const Trange& tr)
    : level(lev), trange(tr)
{
}

Context::Context(const Context& c)
    : level(c.level), trange(c.trange)
{
    // Reserve space for the new vars
    data.reserve(c.data.size());
    
    // Copy the variables
    for (vector<Var*>::const_iterator i = c.data.begin();
            i != c.data.end(); ++i)
        data.push_back(new Var(**i));
}

Context::~Context()
{
    for (vector<Var*>::iterator i = data.begin();
            i != data.end(); ++i)
        delete *i;
}

Context& Context::operator=(const Context& src)
{
    // Manage a = a
    if (this == &src) return *this;

    level = src.level;
    trange = src.trange;

    // Delete existing vars
    for (vector<Var*>::iterator i = data.begin();
            i != data.end(); ++i)
        delete *i;
    data.clear();

    // Reserve space for the new vars
    data.reserve(src.data.size());
    
    // Copy the variables
    for (vector<Var*>::const_iterator i = src.data.begin();
            i != src.data.end(); ++i)
        data.push_back(new Var(**i));
    return *this;
}

bool Context::is_station() const
{
    return level == Level() && trange == Trange();
}

int Context::compare(const Context& ctx) const
{
    int res;
    if ((res = level.compare(ctx.level))) return res;
    return trange.compare(ctx.trange);
}

int Context::compare(const Level& lev, const Trange& tr) const
{
    int res;
    if ((res = level.compare(lev))) return res;
    return trange.compare(tr);
}

void Context::set(const Var& var)
{
    set(unique_ptr<Var>(new Var(var)));
}

void Context::seti(wreport::Varcode code, int val)
{
    set(newvar(code, val));
}

void Context::setd(wreport::Varcode code, double val)
{
    set(newvar(code, val));
}

void Context::setc(wreport::Varcode code, const char* val)
{
    set(newvar(code, val));
}

void Context::set(unique_ptr<Var> var)
{
    Varcode code = var->code();
    int idx = find_index(code);

    if (idx != -1)
    {
        /* Replace the variable */
        delete data[idx];
    }
    else
    {
        /* Add the value */

        /* Enlarge the buffer */
        data.resize(data.size() + 1);

        /* Insertionsort.  Crude, but our datasets should be too small for an
         * RB-Tree to be worth */
        for (idx = data.size() - 1; idx > 0; --idx)
            if (data[idx - 1]->code() > code)
                data[idx] = data[idx - 1];
            else
                break;
    }
    data[idx] = var.release();
}

int Context::find_index(Varcode code) const
{
    /* Binary search */
    int low = 0, high = data.size() - 1;
    while (low <= high)
    {
        int middle = low + (high - low)/2;
        int cmp = (int)code - (int)data[middle]->code();
        if (cmp < 0)
            high = middle - 1;
        else if (cmp > 0)
            low = middle + 1;
        else
            return middle;
    }

    return -1;
}

const Var* Context::find(Varcode code) const
{
    int idx = find_index(code);
    return (idx == -1) ? NULL : data[idx];
}

Var* Context::edit(Varcode code)
{
    int idx = find_index(code);
    return (idx == -1) ? NULL : data[idx];
}

bool Context::remove(Varcode code)
{
    int idx = find_index(code);
    if (idx == -1) return false;
    Var* res = data[idx];
    data.erase(data.begin() + idx);
    delete res;
    return true;
}

const Var* Context::find_by_id(int id) const
{
    return find(shortcutTable[id].code);
}

void Context::print(FILE* out) const
{
    fprintf(out, "Level ");
    level.print(out, "-", " tr ");
    trange.print(out, "-", " ");

    if (data.size() > 0)
    {
        fprintf(out, "%zd vars:\n", data.size());
        for (vector<Var*>::const_iterator i = data.begin(); i != data.end(); ++i)
            (*i)->print(out);
    } else
        fprintf(out, "exists but is empty.\n");
}

static void context_summary(const msg::Context& c, ostream& out)
{
    out << "c(" << c.level << ", " << c.trange << ")";
}

static void var_summary(const Var& var, ostream& out)
{
    out << varcode_format(var.code()) << "[" << var.info()->desc << "]";
}

static void context_var_summary(const msg::Context& c, const Var& var, ostream& out)
{
    out << "Variable ";
    context_summary(c, out);
    out << " ";
    var_summary(var, out);
}

unsigned Context::diff(const Context& ctx) const
{
    if (level != ctx.level || trange != ctx.trange)
    {
        notes::log() << "the contexts are different (first is "
            << level << ", " << trange
            << " second is "
            << ctx.level << ", " << ctx.trange
            << ")" << endl;
        return 1;
    }

    size_t i1 = 0, i2 = 0;
    unsigned diffs = 0;
    while (i1 < data.size() || i2 < ctx.data.size())
    {
        if (i1 == data.size())
        {
            context_var_summary(ctx, *ctx.data[i2], notes::log());
            notes::log() << " exists only in the second message" << endl;
            ++i2;
            ++diffs;
        } else if (i2 == ctx.data.size()) {
            context_var_summary(*this, *data[i1], notes::log());
            notes::log() << " exists only in the first message" << endl;
            ++i1;
            ++diffs;
        } else {
            int cmp = (int)data[i1]->code() - (int)ctx.data[i2]->code();
            if (cmp == 0)
            {
                diffs += data[i1]->diff(*ctx.data[i2]);
                ++i1;
                ++i2;
            } else if (cmp < 0) {
                if (data[i1]->isset())
                {
                    context_var_summary(*this, *data[i1], notes::log());
                    notes::log() << " exists only in the first message" << endl;
                    ++diffs;
                }
                ++i1;
            } else {
                if (ctx.data[i2]->isset())
                {
                    context_var_summary(ctx, *ctx.data[i2], notes::log());
                    notes::log() << " exists only in the second message" << endl;
                    ++diffs;
                }
                ++i2;
            }
        }
    }
    return diffs;
}

const Var* Context::find_vsig() const
{
    // Check if we have the right context information
    if ((level.ltype1 != 100 && level.ltype1 != 102 && level.ltype1 != 103) || trange != Trange::instant())
        return NULL;
    // Look for VSS variable
    const Var* res = find(WR_VAR(0, 8, 42));
    if (res == NULL) return NULL;

    // Ensure it is not undefined
    if (!res->isset()) return NULL;

    // Finally return it
    return res;
}

}
}
