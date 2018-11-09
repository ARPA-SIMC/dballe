#ifndef DBALLE_DB_V7_DATAV7_H
#define DBALLE_DB_V7_DATAV7_H

#include <dballe/fwd.h>
#include <dballe/core/fwd.h>
#include <dballe/core/defs.h>
#include <dballe/core/values.h>
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
namespace db {
namespace v7 {

template<typename Traits>
class DataCommon
{
protected:
    typedef typename Traits::BatchValue BatchValue;
    static const char* table_name;

    v7::Transaction& tr;

    /**
     * Load attributes from the database into a Values
     */
    void read_attrs_into_values(Tracer<>& trc, int id_data, core::Values& values);

    /**
     * Replace the attributes of a variable with those in Values
     */
    virtual void write_attrs(Tracer<>& trc, int id_data, const core::Values& values) = 0;

    /**
     * Remove all attributes from a variable
     */
    virtual void remove_all_attrs(Tracer<>& trc, int id_data) = 0;

public:
    DataCommon(v7::Transaction& tr) : tr(tr) {}
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
    virtual void read_attrs(Tracer<>& trc, int id_data, std::function<void(std::unique_ptr<wreport::Var>)> dest) = 0;

    /**
     * Merge the given attributes with the existing attributes of the given
     * variable:
     *
     * * Existing attributes not in attrs are preserved.
     * * Existing attributes in attrs are overwritten.
     * * New attributes in attrs are inesrted.
     */
    void merge_attrs(Tracer<>& trc, int id_data, const core::Values& attrs);

    /**
     * Remove the given attributes from the given variable, if they exist.
     */
    void remove_attrs(Tracer<>& trc, int data_id, const db::AttrList& attrs);

    /// Bulk variable update
    virtual void update(Tracer<>& trc, std::vector<typename Traits::BatchValue>& vars, bool with_attrs) = 0;

    /// Run the query to delete all records selected by the given QueryBuilder
    virtual void remove(Tracer<>& trc, const v7::IdQueryBuilder& qb) = 0;

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
    using DataCommon<StationDataTraits>::DataCommon;

    /// Bulk variable insert
    virtual void insert(Tracer<>& trc, int id_station, std::vector<batch::StationDatum>& vars, bool with_attrs) = 0;

    /// Query contents of the data table
    virtual void query(Tracer<>& trc, int id_station, std::function<void(int id, wreport::Varcode code)> dest) = 0;

    /**
     * Run a station data query, iterating on the resulting variables
     */
    virtual void run_station_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_data, std::unique_ptr<wreport::Var> var)>) = 0;
};

struct Data : public DataCommon<DataTraits>
{
    using DataCommon<DataTraits>::DataCommon;

    /// Bulk variable insert
    virtual void insert(Tracer<>& trc, int id_station, const Datetime& datetime, std::vector<batch::MeasuredDatum>& vars, bool with_attrs) = 0;

    /// Query contents of the data table
    virtual void query(Tracer<>& trc, int id_station, const Datetime& datetime, std::function<void(int id, int id_levtr, wreport::Varcode code)> dest) = 0;

    /**
     * Run a data query, iterating on the resulting variables
     */
    virtual void run_data_query(Tracer<>& trc, const v7::DataQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_levtr, const Datetime& datetime, int id_data, std::unique_ptr<wreport::Var> var)>) = 0;

    /**
     * Run a summary query, iterating on the resulting variables
     */
    virtual void run_summary_query(Tracer<>& trc, const v7::SummaryQueryBuilder& qb, std::function<void(const dballe::DBStation& station, int id_levtr, wreport::Varcode code, const DatetimeRange& datetime, size_t size)>) = 0;
};

}
}
}
#endif
