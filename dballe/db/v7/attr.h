#ifndef DBALLE_DB_V7_ATTRV7_H
#define DBALLE_DB_V7_ATTRV7_H

#include <dballe/db/v7/data.h>
#include <dballe/sql/fwd.h>
#include <wreport/var.h>
#include <functional>
#include <vector>
#include <memory>
#include <cstdio>

namespace dballe {
struct Record;

namespace db {
namespace v7 {
struct AttributeList;

namespace bulk {
struct InsertAttrsV7;
}

/**
 * Precompiled queries to manipulate the attr table
 */
struct Attr
{
public:
    enum UpdateMode {
        UPDATE,
        IGNORE,
        ERROR,
    };

    virtual ~Attr();

    /// Insert all attributes of the given variable
    void insert_attributes(dballe::Transaction& t, int id_data, const wreport::Var& var, UpdateMode update_mode=UPDATE);

    /// Bulk attribute insert
    virtual void insert(dballe::Transaction& t, v7::bulk::InsertAttrsV7& vars, UpdateMode update_mode=UPDATE) = 0;

    /**
     * Load from the database all the attributes for var
     *
     * @param id_data
     *   ID of the data row for the value of which we will read attributes
     * @param dest
     *   Function that will be called to consume the attrbutes as they are
     *   loaded.
     */
    virtual void read(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) = 0;

    /**
     * Dump the entire contents of the table to an output stream
     */
    virtual void dump(FILE* out) = 0;
};

namespace bulk {

/**
 * Workflow information about an attribute variable listed for bulk
 * insert/update
 */
struct AttrV7 : public Item
{
    int id_data;
    const wreport::Var* attr;

    AttrV7(const wreport::Var* attr, int id_data=-1)
        : id_data(id_data), attr(attr)
    {
    }
    bool operator<(const AttrV7& v) const
    {
        if (int d = id_data - v.id_data) return d < 0;
        return attr->code() < v.attr->code();
    }

    void dump(FILE* out) const;
};

struct InsertAttrsV7 : public std::vector<AttrV7>
{
    void add(const wreport::Var* attr, int id_data)
    {
        emplace_back(attr, id_data);
    }

    // Add all attributes of the given variable
    void add_all(const wreport::Var& var, int id_data);

    void dump(FILE* out) const;
};

/**
 * Helper class for annotating AttrV7 variables with the current status of
 * the database.
 */
struct AnnotateAttrsV7
{
    InsertAttrsV7& attrs;
    InsertAttrsV7::iterator iter;
    bool do_insert = false;
    bool do_update = false;

    AnnotateAttrsV7(InsertAttrsV7& attrs);

    bool annotate(int id_data, wreport::Varcode code, const char* value);
    void annotate_end();

    void dump(FILE* out) const;
};

}


}
}
}

#endif

