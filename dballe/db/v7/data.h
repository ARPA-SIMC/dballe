#ifndef DBALLE_DB_V7_DATAV7_H
#define DBALLE_DB_V7_DATAV7_H

#include <dballe/core/defs.h>
#include <dballe/sql/fwd.h>
#include <dballe/db/defs.h>
#include <dballe/db/v7/fwd.h>
#include <wreport/var.h>
#include <memory>
#include <vector>
#include <list>
#include <cstdio>
#include <functional>

namespace dballe {
struct Record;
struct Values;

namespace db {
namespace v7 {

template<typename Traits>
class DataCommon
{
protected:
    typedef typename Traits::BatchValue BatchValue;
    static const char* table_name;

    /**
     * Load attributes from the database into a Values
     */
    void read_attrs_into_values(int id_data, Values& values);

    /**
     * Replace the attributes of a variable with those in Values
     */
    virtual void write_attrs(int id_data, const Values& values) = 0;

    /**
     * Remove all attributes from a variable
     */
    virtual void remove_all_attrs(int id_data) = 0;

public:
    virtual ~DataCommon() {}

    /**
     * Load from the database all the attributes for var
     *
     * @param id_data
     *   ID of the data row for the value of which we will read attributes
     * @param dest
     *   Function that will be called to consume the attrbutes as they are
     *   loaded.
     */
    virtual void read_attrs(int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) = 0;

    /**
     * Merge the given attributes with the existing attributes of the given
     * variable:
     *
     * * Existing attributes not in attrs are preserved.
     * * Existing attributes in attrs are overwritten.
     * * New attributes in attrs are inesrted.
     */
    void merge_attrs(int id_data, const Values& attrs);

    /**
     * Remove the given attributes from the given variable, if they exist.
     */
    void remove_attrs(int data_id, const db::AttrList& attrs);

    /// Bulk variable update
    virtual void update(dballe::db::v7::Transaction& t, std::vector<typename Traits::BatchValue>& vars, bool with_attrs) = 0;

    /// Run the query to delete all records selected by the given QueryBuilder
    virtual void remove(const v7::IdQueryBuilder& qb) = 0;

    /// Dump the entire contents of the table to an output stream
    virtual void dump(FILE* out) = 0;

    virtual void clear_cache() = 0;
};


struct StationDataDumper
{
    unsigned count = 0;
    FILE* out;

    StationDataDumper(FILE* out);

    void print_head();
    void print_row(int id, int id_station, wreport::Varcode code, const char* val, const std::vector<uint8_t>& attrs);
    void print_tail();
};

struct DataDumper
{
    unsigned count = 0;
    FILE* out;

    DataDumper(FILE* out);

    void print_head();
    void print_row(int id, int id_station, int id_levtr, const Datetime& dt, wreport::Varcode code, const char* val, const std::vector<uint8_t>& attrs);
    void print_tail();
};

struct StationDataTraits
{
    typedef batch::StationDatum BatchValue;
    static const char* table_name;
};

struct DataTraits
{
    typedef batch::MeasuredDatum BatchValue;
    static const char* table_name;
};

extern template class DataCommon<StationDataTraits>;
extern template class DataCommon<DataTraits>;

struct StationData : public DataCommon<StationDataTraits>
{
    /// Bulk variable insert
    virtual void insert(dballe::db::v7::Transaction& t, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs) = 0;

    /// Query contents of the data table
    virtual void query(int id_station, std::function<void(int id, wreport::Varcode code)> dest) = 0;
};

struct Data : public DataCommon<DataTraits>
{
    /// Bulk variable insert
    virtual void insert(dballe::db::v7::Transaction& t, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs) = 0;

    /// Query contents of the data table
    virtual void query(int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest) = 0;
};

}
}
}
#endif
