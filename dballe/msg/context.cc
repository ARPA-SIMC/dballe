#include "dballe/msg/context.h"
#include "dballe/msg/vars.h"
#include <wreport/notes.h>
#include <sstream>
#include <cstdlib>
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {
namespace impl {
namespace msg {

Context::Context(const Level& lev, const Trange& tr)
    : level(lev), trange(tr)
{
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

const Var* Context::find_by_id(int id) const
{
    return values.maybe_var(shortcutTable[id].code);
}

void Context::print(FILE* out) const
{
    fprintf(out, "Level ");
    level.print(out, "-", " tr ");
    trange.print(out, "-", " ");
    values.print(out);
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

    Values::const_iterator i1 = values.begin();
    Values::const_iterator i2 = ctx.values.begin();
    unsigned diffs = 0;
    while (i1 != values.end() && i2 != ctx.values.end())
    {
        int cmp = (int)i1->code() - (int)i2->code();
        if (cmp == 0)
        {
            diffs += (*i1)->diff(**i2);
            ++i1;
            ++i2;
        } else if (cmp < 0) {
            if ((*i1)->isset())
            {
                context_var_summary(*this, **i1, notes::log());
                notes::log() << " exists only in the first message" << endl;
                ++diffs;
            }
            ++i1;
        } else {
            if ((*i2)->isset())
            {
                context_var_summary(ctx, **i2, notes::log());
                notes::log() << " exists only in the second message" << endl;
                ++diffs;
            }
            ++i2;
        }
    }
    while (i1 != values.end())
    {
        context_var_summary(*this, **i1, notes::log());
        notes::log() << " exists only in the first message" << endl;
        ++i1;
        ++diffs;
    }
    while (i2 != ctx.values.end())
    {
        context_var_summary(ctx, **i2, notes::log());
        notes::log() << " exists only in the second message" << endl;
        ++i2;
        ++diffs;
    }
    return diffs;
}

const Var* Context::find_vsig() const
{
    // Check if we have the right context information
    if ((level.ltype1 != 100 && level.ltype1 != 102 && level.ltype1 != 103) || trange != Trange::instant())
        return NULL;
    // Look for VSS variable
    const Var* res = values.maybe_var(WR_VAR(0, 8, 42));
    if (!res) return nullptr;

    // Ensure it is not undefined
    if (!res->isset()) return nullptr;

    // Finally return it
    return res;
}

}
}
}
