#ifndef DBALLE_DB_H
#define DBALLE_DB_H

#include <dballe/fwd.h>
#include <wreport/var.h>
#include <memory>
#include <vector>

namespace dballe {

/**
 * Options controlling how messages are imported in the database.
 *
 * To allow to add members this structure without breaking the ABI, creation of
 * new instances is restricted to DBImportOptions::create().
 */
struct DBImportOptions
{
    /**
     * Report name to use to import data.
     *
     * If left empty (default), then it will be chosen automatically based on
     * the message type.
     */
    std::string report;

    /// Import variable attributes
    bool import_attributes = false;

    /**
     * Update station information.
     *
     * If set to true, station information is merged with existing data in the
     * database. If false (default), station information is imported only when
     * the station did not exist in the database.
     */
    bool update_station = false;

    /**
     * Replace existing data.
     *
     * If set to true, causes existing information already in the database to
     * be overwritten. If false (default), trying to import a message which
     * contains data already present in the database causes the import to fail.
     */
    bool overwrite = false;

    static std::unique_ptr<DBImportOptions> create();

    static const DBImportOptions defaults;

    friend class DB;
    friend class Transaction;
protected:
    DBImportOptions() = default;
    DBImportOptions(const DBImportOptions&) = default;
    DBImportOptions(DBImportOptions&&) = default;
    DBImportOptions& operator=(const DBImportOptions&) = default;
    DBImportOptions& operator=(DBImportOptions&&) = default;
};


/**
 * Options controlling how values are inserted in the database
 *
 * To allow to add members this structure without breaking the ABI, creation of
 * new instances is restricted to DBInsertOptions::create().
 */
struct DBInsertOptions
{
    /// If true, then existing data can be rewritten, else data can only be added.
    bool can_replace = false;

    /**
     * If false, it will not create a missing station record, and only data for
     * existing stations can be added. If true, then if we are inserting data
     * for a station that does not yet exists in the database, it will be
     * created.
     */
    bool can_add_stations = true;

    static std::unique_ptr<DBInsertOptions> create();

    static const DBInsertOptions defaults;

protected:
    DBInsertOptions() = default;
    DBInsertOptions(const DBInsertOptions&) = default;
    DBInsertOptions(DBInsertOptions&&) = default;
    DBInsertOptions& operator=(const DBInsertOptions&) = default;
    DBInsertOptions& operator=(DBInsertOptions&&) = default;
};


class Transaction : public std::enable_shared_from_this<Transaction>
{
public:
    virtual ~Transaction();

    /// Commit this transaction
    virtual void commit() = 0;

    /// Roll back this transaction
    virtual void rollback() = 0;

    /// Roll back this transaction
    virtual void rollback_nothrow() noexcept = 0;

    /**
     * Start a query on the station variables archive.
     *
     * The cursor will iterate over unique lat, lon, ident triples, and will
     * contain all station vars. If a station var exists twice on two different
     * networks, only one will be present: the one of the network with the
     * highest priority.
     *
     * @param query
     *   The Query selecting the stations to return
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorStation> query_stations(const Query& query) = 0;

    /**
     * Query the station variables in the database.
     *
     * When multiple values per variable are present, the results will be presented
     * in increasing order of priority.
     *
     * @param query
     *   The Query selecting the station data to return
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorStationData> query_station_data(const Query& query) = 0;

    /**
     * Query the database.
     *
     * When multiple values per variable are present, the results will be presented
     * in increasing order of priority.
     *
     * @param query
     *   The Query selecting the data to return
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorData> query_data(const Query& query) = 0;

    /**
     * Query a summary of what the result would be for a query.
     *
     * @param query
     *   The Query selecting the data to summarise
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorSummary> query_summary(const Query& query) = 0;

    /**
     * Query the database returning the matching data as Message objects.
     *
     * @param query
     *   The Query selecting the data that will go into the Message objects
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorMessage> query_messages(const Query& query) = 0;

    /**
     * Remove all data from the database.
     *
     * This is faster than remove() with an empty record, and unlike reset() it
     * preserves existing report information.
     *
     * @param transaction
     *   The current active transaction.
     */
    virtual void remove_all() = 0;

    /**
     * Remove data from the database
     *
     * @param query
     *   The query selecting the data to remove
     */
    virtual void remove_station_data(const Query& query) = 0;

    /**
     * Remove data from the database
     *
     * @param query
     *   The query selecting the data to remove
     */
    virtual void remove_data(const Query& query) = 0;

    /**
     * Import a Message into the DB-All.e database
     *
     * @param message
     *   The Message to import
     * @param opts
     *   Options controlling the import process
     */
    virtual void import_message(const Message& message, const DBImportOptions& opts=DBImportOptions::defaults) = 0;

    /**
     * Import Messages into the DB-All.e database
     *
     * @param messages
     *   The messages to import
     * @param opts
     *   Options controlling the import process
     */
    virtual void import_messages(const std::vector<std::shared_ptr<Message>>& messages, const DBImportOptions& opts=DBImportOptions::defaults);

    /**
     * Insert station values into the database
     *
     * The IDs of the station and all variables that were inserted will be
     * stored in vals.
     *
     * @param data
     *   The values to insert.
     * @param opts
     *   Options controlling the insert operation
     */
    virtual void insert_station_data(Data& data, const DBInsertOptions& opts=DBInsertOptions::defaults) = 0;

    /**
     * Insert data values into the database
     *
     * The IDs of the station and all variables that were inserted will be
     * stored in vals.
     *
     * @param data
     *   The values to insert.
     * @param opts
     *   Options controlling the insert operation
     */
    virtual void insert_data(Data& data, const DBInsertOptions& opts=DBInsertOptions::defaults) = 0;
};


struct DB: public std::enable_shared_from_this<DB>
{
    virtual ~DB();

    /**
     * Create from an url-like specification, as described in
     * doc/fapi_connect.md
     *
     * @param url
     *   The url-like connection descriptor
     */
    static std::shared_ptr<DB> connect_from_url(const std::string& url);

    /**
     * Begin a transaction on this database, and return a Transaction object
     * that can be used to commit it.
     */
    virtual std::shared_ptr<dballe::Transaction> transaction(bool readonly=false) = 0;

    /**
     * Start a query on the station variables archive.
     *
     * The cursor will iterate over unique lat, lon, ident triples, and will
     * contain all station vars. If a station var exists twice on two different
     * networks, only one will be present: the one of the network with the
     * highest priority.
     *
     * @param query
     *   The Query selecting the stations to return
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorStation> query_stations(const Query& query);

    /**
     * Query the station variables in the database.
     *
     * When multiple values per variable are present, the results will be presented
     * in increasing order of priority.
     *
     * @param query
     *   The Query selecting the station data to return
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorStationData> query_station_data(const Query& query);

    /**
     * Query the database.
     *
     * When multiple values per variable are present, the results will be presented
     * in increasing order of priority.
     *
     * @param query
     *   The Query selecting the data to return
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorData> query_data(const Query& query);

    /**
     * Query a summary of what the result would be for a query.
     *
     * @param query
     *   The Query selecting the data to summarise
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorSummary> query_summary(const Query& query);

    /**
     * Query the database returning the matching data as Message objects.
     *
     * @param query
     *   The Query selecting the data that will go into the Message objects
     * @return
     *   The cursor to use to iterate over the results
     */
    virtual std::unique_ptr<CursorMessage> query_messages(const Query& query);

    /**
     * Remove all data from the database.
     *
     * This is faster than remove() with an empty record, and unlike reset() it
     * preserves existing report information.
     */
    void remove_all();

    /**
     * Remove data from the database
     *
     * @param query
     *   The query selecting the data to remove
     */
    void remove_station_data(const Query& query);

    /**
     * Remove data from the database
     *
     * @param rec
     *   The query selecting the data to remove
     */
    void remove_data(const Query& query);

    /**
     * Import a Message into the DB-All.e database
     *
     * @param message
     *   The Message to import
     * @param opts
     *   Options controlling the import process
     */
    void import_message(const Message& message, const DBImportOptions& opts=DBImportOptions::defaults);

    /**
     * Import Messages into the DB-All.e database
     *
     * @param messages
     *   The messages to import
     * @param opts
     *   Options controlling the import process
     */
    void import_messages(const std::vector<std::shared_ptr<Message>>& messages, const DBImportOptions& opts=DBImportOptions::defaults);

    /**
     * Insert station values into the database
     *
     * The IDs of the station andl all variables that were inserted will be
     * stored in vals.
     *
     * @param vals
     *   The values to insert.
     * @param opts
     *   Options controlling the insert operation
     */
    void insert_station_data(Data& vals, const DBInsertOptions& opts=DBInsertOptions::defaults);

    /**
     * Insert data values into the database
     *
     * The IDs of the station andl all variables that were inserted will be
     * stored in vals.
     *
     * @param vals
     *   The values to insert.
     * @param can_replace
     *   If true, then existing data can be rewritten, else data can only be added.
     * @param station_can_add
     *   If false, it will not create a missing station record, and only data
     *   for existing stations can be added. If true, then if we are inserting
     *   data for a station that does not yet exists in the database, it will
     *   be created.
     */
    void insert_data(Data& vals, const DBInsertOptions& opts=DBInsertOptions::defaults);
};

}

#endif
