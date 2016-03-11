#include "attrv6.h"
#include "internals.h"
#include <algorithm>
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace sql {

AttrV6::~AttrV6() {}

void AttrV6::insert_attributes(dballe::sql::Transaction& t, int id_data, const wreport::Var& var, UpdateMode update_mode)
{
    bulk::InsertAttrsV6 attrs;
    attrs.add_all(var, id_data);
    if (attrs.empty()) return;
    insert(t, attrs, update_mode);
}


namespace bulk {

void InsertAttrsV6::add_all(const wreport::Var& var, int id_data)
{
    for (const Var* attr = var.next_attr(); attr != NULL; attr = attr->next_attr())
        if (attr->isset())
            emplace_back(attr, id_data);
}

AnnotateAttrsV6::AnnotateAttrsV6(InsertAttrsV6& attrs)
    : attrs(attrs)
{
    std::sort(attrs.begin(), attrs.end());
    iter = attrs.begin();
}

bool AnnotateAttrsV6::annotate(int id_data, Varcode code, const char* value)
{
    //fprintf(stderr, "ANNOTATE ");
    while (iter != attrs.end())
    {
        //fprintf(stderr, "id_data: %d/%d  id_levtr: %d/%d  varcode: %d/%d  value: %s/%s: ", id_data, iter->id_data, id_levtr, iter->id_levtr, code, iter->var->code(), value, iter->var->value());

        // This attribute is not on our list: stop here and wait for a new one
        if (id_data < iter->id_data)
        {
            //fprintf(stderr, "levtr lower than ours, wait for next\n");
            return true;
        }

        // iter points to a attribute that is not currently in the DB
        if (id_data > iter->id_data)
        {
            //fprintf(stderr, "levtr higher than ours, insert this\n");
            do_insert = true;
            iter->set_needs_insert();
            ++iter;
            continue;
        }

        // id_levtr is the same

        // This attribute is not on our list: stop here and wait for a new one
        if (code < iter->attr->code())
        {
            //fprintf(stderr, "varcode lower than ours, wait for next\n");
            return true;
        }

        // iter points to a attribute that is not currently in the DB
        if (code > iter->attr->code())
        {
            //fprintf(stderr, "varcode higher than ours, insert this\n");
            do_insert = true;
            iter->set_needs_insert();
            ++iter;
            continue;
        }

        // iter points to an attribute that is also in the DB

        // If the value is different, we need to update
        if (strcmp(value, iter->attr->enqc()) != 0)
        {
            //fprintf(stderr, "needs_update ");
            iter->set_needs_update();
            do_update = true;
        }

        // We processed this attribute: stop here and wait for a new one
        ++iter;
        //fprintf(stderr, "wait for next\n");
        return true;
    }

    // We have no more attribute to consider: signal the caller that they can
    // stop iterating if they wish.
    //fprintf(stderr, "done.\n");
    return false;
}

void AnnotateAttrsV6::annotate_end()
{
    // Mark all remaining attribute as needing insert
    for ( ; iter != attrs.end(); ++iter)
    {
        //fprintf(stderr, "LEFTOVER: id_levtr: %d  varcode: %d  value: %s\n", iter->id_levtr, iter->var->code(), iter->var->value());
        iter->set_needs_insert();
        do_insert = true;
    }
}

void AnnotateAttrsV6::dump(FILE* out) const
{
    fprintf(out, "Needs insert: %d, needs update: %d\n", do_insert, do_update);
    attrs.dump(out);
}

void AttrV6::dump(FILE* out) const
{
    char flags[5];
    format_flags(flags);
    fprintf(out, "flags:%s %01d%02d%03d(%d): %s\n",
            flags, WR_VAR_F(attr->code()), WR_VAR_X(attr->code()), WR_VAR_Y(attr->code()),
            (int)(attr->code()),
            attr->isset() ? attr->enqc() : "(null)");
}

void InsertAttrsV6::dump(FILE* out) const
{
    for (unsigned i = 0; i < size(); ++i)
    {
        fprintf(out, "%3u/%3zd: ", i, size());
        (*this)[i].dump(out);
    }
}

}

}
}
}

