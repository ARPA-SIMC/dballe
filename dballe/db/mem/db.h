#ifndef DBA_DB_MEM_H
#define DBA_DB_MEM_H

#include <dballe/db/db.h>
#include <dballe/db/mem/repinfo.h>
#include <dballe/memdb/memdb.h>
#include <wreport/varinfo.h>
#include <string>
#include <vector>
#include <memory>

namespace dballe {
namespace db {

namespace mem {

/// DB-ALLe database, in-memory db implementation
class DB : public dballe::DB
{
public:
    /// In-memory database backend
    Memdb memdb;
    Repinfo repinfo;

protected:
    std::string serialization_dir;

    size_t m_last_station_id;

    /// Store information about the database ID of a variable
    struct VarID
    {
        wreport::Varcode code;
        // True if it is a station value
        bool station;
        size_t id;
        VarID(wreport::Varcode code, bool station, size_t id) : code(code), station(station), id(id) {}
    };

    /// Store database variable IDs for all last inserted variables
    std::vector<VarID> last_insert_varids;

    /// Query stations, returning a list of station IDs
    void raw_query_stations(const core::Query& rec, memdb::Results<memdb::Station>& res);

    /// Query station data, returning a list of Value IDs
    void raw_query_station_data(const core::Query& rec, memdb::Results<memdb::StationValue>& res);

    /// Query data, returning a list of Value IDs
    void raw_query_data(const core::Query& rec, memdb::Results<memdb::Value>& res);
    
public:
    DB();
    DB(const std::string& arg);
    virtual ~DB();

    db::Format format() const override { return MEM; }

    void disappear() override ;

    /**
     * Reset the database, removing all existing DBALLE tables and re-creating them
     * empty.
     *
     * @param repinfo_file
     *   The name of the CSV file with the report type information data to load.
     *   The file is in CSV format with 6 columns: report code, mnemonic id,
     *   description, priority, descriptor, table A category.
     *   If repinfo_file is NULL, then the default of /etc/dballe/repinfo.csv is
     *   used.
     */
    void reset(const char* repinfo_file = 0) override;

    /**
     * Update the repinfo table in the database, with the data found in the given
     * file.
     *
     * @param repinfo_file
     *   The name of the CSV file with the report type information data to load.
     *   The file is in CSV format with 6 columns: report code, mnemonic id,
     *   description, priority, descriptor, table A category.
     *   If repinfo_file is NULL, then the default of /etc/dballe/repinfo.csv is
     *   used.
     * @retval added
     *   The number of repinfo entryes that have been added
     * @retval deleted
     *   The number of repinfo entryes that have been deleted
     * @retval updated
     *   The number of repinfo entryes that have been updated
     */
    void update_repinfo(const char* repinfo_file, int* added, int* deleted, int* updated) override;

    std::map<std::string, int> get_repinfo_priorities() override;

    void insert_station_data(StationValues& vals, bool can_replace, bool station_can_add) override;
    void insert_data(DataValues& vals, bool can_replace, bool station_can_add) override;

    int last_station_id() const override;

    void remove_station_data(const Query& query) override;
    void remove(const Query& rec) override;
    void remove_all() override;

    /**
     * Remove orphan values from the database.
     *
     * Orphan values are currently:
     * \li lev_tr values for which no data exists
     * \li station values for which no lev_tr exists
     *
     * Depending on database size, this routine can take a few minutes to execute.
     */
    void vacuum() override;

    std::unique_ptr<db::CursorStation> query_stations(const Query& query) override;
    std::unique_ptr<db::CursorStationData> query_station_data(const Query& query) override;
    std::unique_ptr<db::CursorData> query_data(const Query& query) override;
    std::unique_ptr<db::CursorSummary> query_summary(const Query& query) override;

    /**
     * Query attributes
     *
     * @param id_data
     *   The database id of the data related to the attributes to retrieve
     * @param id_var
     *   The varcode of the variable related to the attributes to retrieve.  See @ref vartable.h (ignored)
     * @param qcs
     *   The WMO codes of the QC values requested.  If it is empty, then all values
     *   are returned.
     * @param dest
     *   The function that will be called on each attribute retrieved
     * @return
     *   Number of attributes returned in attrs
     */
    void query_attrs(int reference_id, wreport::Varcode id_var,
            std::function<void(std::unique_ptr<wreport::Var>)>&& dest) override;

    void attr_insert(wreport::Varcode id_var, const Values& attrs) override;
    void attr_insert(int id_data, wreport::Varcode id_var, const Values& attrs) override;

    /**
     * Delete QC data for the variable `var' in record `rec' (coming from a previous
     * dba_query)
     *
     * @param id_data
     *   The database id of the lev_tr related to the attributes to remove
     * @param id_var
     *   The varcode of the variable related to the attributes to remove.  See @ref vartable.h (ignored)
     * @param qcs
     *   Array of WMO codes of the attributes to delete.  If empty, all attributes
     *   associated to id_data will be deleted.
     */
    void attr_remove(int id_data, wreport::Varcode id_var, const db::AttrList& qcs) override;

    void import_msg(const Message& msg, const char* repmemo, int flags) override;
    bool export_msgs(const Query& query, std::function<bool(std::unique_ptr<Message>&&)> dest) override;

    /**
     * Dump the entire contents of the database to an output stream
     */
    void dump(FILE* out) override;

    friend class dballe::DB;
};

}
}
}
#endif
