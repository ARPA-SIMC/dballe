#include "dbapi.h"
#include "dballe/file.h"
#include "dballe/importer.h"
#include "dballe/exporter.h"
#include "dballe/message.h"
#include "dballe/values.h"
#include "dballe/core/query.h"
#include "dballe/core/data.h"
#include "dballe/db/db.h"
#include "dballe/msg/msg.h"
#include <cstring>

using namespace wreport;
using namespace std;

namespace dballe {
namespace fortran {

struct InputFile
{
    File* input = nullptr;
    Importer* importer = nullptr;
    std::vector<std::shared_ptr<dballe::Message>> current_msg;
    unsigned current_msg_idx = 0;
    impl::DBImportOptions opts;

    InputFile(Encoding format, bool simplified)
    {
        impl::ImporterOptions importer_options;
        importer_options.simplified = simplified;
        input = File::create(format, stdin, false, "(stdin)").release();
        importer = Importer::create(format, importer_options).release();
    }
    InputFile(const char* fname, Encoding format, bool simplified)
    {
        impl::ImporterOptions importer_options;
        importer_options.simplified = simplified;
        input = File::create(format, fname, "rb").release();
        importer = Importer::create(format, importer_options).release();
    }
    ~InputFile()
    {
        if (input) delete input;
        if (importer) delete importer;
    }

    bool next()
    {
        if (current_msg_idx + 1 < current_msg.size())
            // Move to the next message
            ++current_msg_idx;
        else
        {
            // Read data
            BinaryMessage rmsg = input->read();
            if (!rmsg)
                return false;

            // Parse and interpret data
            current_msg.clear();
            current_msg = importer->from_binary(rmsg);

            // Move to the first message
            current_msg_idx = 0;
        }

        return true;
    }

    const Message& msg() const
    {
        return *current_msg[current_msg_idx];
    }
};

struct OutputFile
{
    File* output = nullptr;

    OutputFile(const char* mode, Encoding format)
    {
        output = File::create(format, stdout, false, "(stdout)").release();
    }
    OutputFile(const char* fname, const char* mode, Encoding format)
    {
        output = File::create(format, fname, mode).release();
    }
    ~OutputFile()
    {
        if (output) delete output;
    }
};

namespace {

struct QuantesonoOperation : public CursorOperation<CursorStation>
{
    const DbAPI& api;

    QuantesonoOperation(const DbAPI& api)
        : api(api)
    {
    }

    int run()
    {
        cursor = api.tr->query_stations(api.input_query);
        return cursor->remaining();
    }

    bool next_station() override
    {
        return cursor->next();
    }

    void query_attributes(Attributes& dest) override { throw error_consistency("query_attributes cannot be called after query_stations/next_station"); }
    void critica(Values& qcinput) override { throw error_consistency("critica cannot be called after query_stations/next_station"); }
    void scusa() override { throw error_consistency("scusa cannot be called after query_stations/next_station"); }
};

template<typename Cursor>
struct CursorTraits {};

template<>
struct CursorTraits<db::CursorStationData>
{
    static inline std::unique_ptr<db::CursorStationData> query(db::Transaction& tr, const core::Query& query)
    {
        return std::unique_ptr<db::CursorStationData>(dynamic_cast<db::CursorStationData*>(tr.query_station_data(query).release()));
    }
    static inline void attr_insert(db::Transaction& tr, int id, const Values& values)
    {
        tr.attr_insert_station(id, values);
    }
    static inline void attr_remove(db::Transaction& tr, int id, const std::vector<wreport::Varcode>& attrs)
    {
        tr.attr_remove_station(id, attrs);
    }
};

template<>
struct CursorTraits<db::CursorData>
{
    static inline std::unique_ptr<db::CursorData> query(db::Transaction& tr, const core::Query& query)
    {
        return std::unique_ptr<db::CursorData>(dynamic_cast<db::CursorData*>(tr.query_data(query).release()));
    }
    static inline void attr_insert(db::Transaction& tr, int id, const Values& values)
    {
        tr.attr_insert_data(id, values);
    }
    static inline void attr_remove(db::Transaction& tr, int id, const std::vector<wreport::Varcode>& attrs)
    {
        tr.attr_remove_data(id, attrs);
    }
};

template<typename Cursor>
struct VoglioquestoOperation : public CursorOperation<Cursor>
{
    const DbAPI& api;
    bool valid_cached_attrs = false;
    bool next_data_ended = false;

    VoglioquestoOperation(const DbAPI& api)
        : api(api)
    {
    }

    int run()
    {
        this->cursor.reset(CursorTraits<Cursor>::query(*api.tr, api.input_query).release());
        return this->cursor->remaining();
    }

    wreport::Varcode next_data() override
    {
        if (next_data_ended) return 0;

        if (this->cursor->next())
        {
            valid_cached_attrs = true;
            return this->cursor->get_varcode();
        } else {
            next_data_ended = true;
            return 0;
        }
    }
    void query_attributes(Attributes& dest) override
    {
        if (next_data_ended) throw error_consistency("query_attributes called after next_data returned end of data");
        function<void(unique_ptr<Var>&&)> consumer;
        if (api.selected_attr_codes.empty())
        {
            consumer = [&](unique_ptr<Var>&& var) {
                dest.values.set(std::move(*var));
            };
        } else {
            consumer = [&](unique_ptr<Var>&& var) {
                for (auto code: api.selected_attr_codes)
                    if (code == var->code())
                    {
                        dest.values.set(std::move(*var));
                        break;
                    }
            };
        }
        dest.values.clear();
        this->cursor->attr_query(consumer, !valid_cached_attrs);
        dest.has_new_values();
    }
    void critica(Values& qcinput) override
    {
        if (next_data_ended) throw error_consistency("critica called after next_data returned end of data");
        CursorTraits<Cursor>::attr_insert(*api.tr, this->cursor->attr_reference_id(), qcinput);
        valid_cached_attrs = false;
    }
    void scusa() override
    {
        if (next_data_ended) throw error_consistency("scusa called after next_data returned end of data");
        CursorTraits<Cursor>::attr_remove(*api.tr, this->cursor->attr_reference_id(), api.selected_attr_codes);
        valid_cached_attrs = false;
    }
};

/// Store information about the database ID of a variable
struct VarID
{
    wreport::Varcode code;
    // True if it is a station value
    bool station;
    size_t id;
    VarID(wreport::Varcode code, bool station, size_t id) : code(code), station(station), id(id) {}
};

struct PrendiloOperation : public Operation
{
    /// Store database variable IDs for all last inserted variables
    DbAPI& api;
    std::vector<VarID> last_inserted_varids;
    wreport::Varcode varcode = 0;
    int last_inserted_station_id = API::missing_int;
    int last_inserted_data_id = API::missing_int;
    impl::DBInsertOptions opts;

    PrendiloOperation(DbAPI& api)
        : api(api)
    {
        opts.can_replace = (api.perms & DbAPI::PERM_DATA_WRITE) != 0;
        opts.can_add_stations = (api.perms & DbAPI::PERM_ANA_WRITE) != 0;
    }

    void set_varcode(wreport::Varcode varcode) override { this->varcode = varcode; }

    void run()
    {
        // db::Transaction& tr, dballe::core::Data& input, bool station_context, unsigned perms)
        last_inserted_varids.clear();
        if (api.station_context)
        {
            api.tr->insert_station_data(api.input_data, opts);
            for (const auto& v: api.input_data.values)
                last_inserted_varids.push_back(VarID(v.code(), true, v.data_id));
        } else {
            api.tr->insert_data(api.input_data, opts);
            for (const auto& v: api.input_data.values)
                last_inserted_varids.push_back(VarID(v.code(), false, v.data_id));
        }
        last_inserted_station_id = api.input_data.station.id;
        if (api.input_data.values.size() == 1)
            last_inserted_data_id = api.input_data.values.begin()->data_id;
        else
            last_inserted_data_id = API::missing_int;
    }
    void query_attributes(Attributes& dest) override
    {
        throw error_consistency("query_attributes cannot be called after a insert_data");
    }
    void critica(Values& qcinput) override
    {
        int data_id = MISSING_INT;
        bool is_station = false;
        // Lookup the variable we act on from the results of last insert_data
        if (last_inserted_varids.size() == 1)
        {
            data_id = last_inserted_varids[0].id;
            is_station = last_inserted_varids[0].station;
        } else {
            if (varcode == 0)
                throw error_consistency("please set *var_related before calling critica after setting multiple variables in a single insert_data");
            for (const auto& i: last_inserted_varids)
                if (i.code == varcode)
                {
                    data_id = i.id;
                    is_station = i.station;
                    break;
                }
            if (data_id == MISSING_INT)
                error_consistency::throwf("cannot insert attributes for *var_related=%01d%02d%03d: the last insert_data inserted %zd variables, none of which match *var_related", WR_VAR_FXY(varcode), last_inserted_varids.size());
        }
        if (is_station)
            api.tr->attr_insert_station(data_id, qcinput);
        else
            api.tr->attr_insert_data(data_id, qcinput);
    }
    void scusa() override
    {
        throw error_consistency("scusa cannot be called after a insert_data");
    }
    int enqi(const char* param) const override
    {
        if (strcmp(param, "ana_id") == 0)
        {
            return last_inserted_station_id;
        } else if (strcmp(param, "context_id") == 0) {
            return last_inserted_data_id;
        } else
            wreport::error_consistency::throwf("enqi %s cannot be called after a insert_data", param);
    }
    double enqd(const char* param) const override { throw wreport::error_consistency("enqd cannot be called after a insert_data"); }
    bool enqc(const char* param, std::string& res) const override { throw wreport::error_consistency("enqc cannot be called after a insert_data"); }
    void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) const override { throw wreport::error_consistency("enqlevel cannot be called after a insert_data"); }
    void enqtimerange(int& ptype, int& p1, int& p2) const override { throw wreport::error_consistency("enqtimerange cannot be called after a insert_data"); }
    void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec) const override { throw wreport::error_consistency("enqdate cannot be called after a insert_data"); }
};

struct VaridOperation : public Operation
{
    DbAPI& api;
    /// Varcode of the data variable
    wreport::Varcode varcode;
    /// Database ID of the data variable
    int varid;

    VaridOperation(DbAPI& api, int varid) : api(api), varid(varid) {}
    void run()
    {
    }
    void set_varcode(wreport::Varcode varcode) override { this->varcode = varcode; }
    void query_attributes(Attributes& dest) override
    {
        if (!varid)
            throw error_consistency("query_attributes called with an invalid *context_id");
        // Retrieve the varcodes of the attributes that we want
        function<void(unique_ptr<Var>&&)> consumer;
        if (api.selected_attr_codes.empty())
        {
            consumer = [&](unique_ptr<Var>&& var) {
                dest.values.set(std::move(var));
            };
        } else {
            consumer = [&](unique_ptr<Var>&& var) {
                for (auto code: api.selected_attr_codes)
                    if (code == var->code())
                    {
                        dest.values.set(std::move(var));
                        break;
                    }
            };
        }
        dest.values.clear();
        api.tr->attr_query_data(varid, consumer);
        dest.has_new_values();
    }
    void critica(Values& qcinput) override
    {
        api.tr->attr_insert_data(varid, qcinput);
    }
    void scusa() override
    {
        api.tr->attr_remove_data(varid, api.selected_attr_codes);
    }
    int enqi(const char* param) const override { throw wreport::error_unimplemented("Varid::enqi handle *params or forward to previous operation"); }
    double enqd(const char* param) const override { throw wreport::error_unimplemented("Varid::enqd handle *params or forward to previous operation"); }
    bool enqc(const char* param, std::string& res) const override { throw wreport::error_unimplemented("Varid::enqc handle *params or forward to previous operation"); }
    void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) const override { throw wreport::error_unimplemented("VaridOperation::enqlevel forward to previous operation"); }
    void enqtimerange(int& ptype, int& p1, int& p2) const override { throw wreport::error_unimplemented("VaridOperation::enqtimerange forward to previous operation"); }
    void enqdate(int& year, int& month, int& day, int& hour, int& min, int& sec) const override { throw wreport::error_unimplemented("VaridOperation::enqdata forward to previous operation"); }
};

}


DbAPI::DbAPI(std::shared_ptr<db::Transaction> tr, const char* anaflag, const char* dataflag, const char* attrflag)
    : DbAPI(tr, compute_permissions(anaflag, dataflag, attrflag))
{
}

DbAPI::DbAPI(std::shared_ptr<db::Transaction> tr, unsigned perms)
    : tr(tr)
{
    this->perms = perms;
}

DbAPI::~DbAPI()
{
    shutdown(false);
}

void DbAPI::shutdown(bool commit)
{
    delete input_file;
    input_file = nullptr;

    delete output_file;
    output_file = nullptr;

    delete operation;
    operation = nullptr;

    if (commit)
        tr->commit();
}

void DbAPI::commit()
{
    shutdown(true);
}

void DbAPI::reinit_db(const char* repinfofile)
{
    if (!(perms & PERM_DATA_WRITE))
        error_consistency::throwf(
            "reinit_db must be run with the database open in data write mode");
    tr->remove_all();
    delete operation;
    operation = nullptr;
}

void DbAPI::remove_all()
{
    if (!(perms & PERM_DATA_WRITE))
        error_consistency::throwf(
            "remove_all must be run with the database open in data write mode");
    tr->remove_all();
    delete operation;
    operation = nullptr;
}

void DbAPI::seti(const char* param, int value)
{
    if (param[0] == '*')
    {
        if (strcmp(param + 1, "context_id") == 0)
        {
            if (value != MISSING_INT)
                reset_operation(new VaridOperation(*this, value));
            else
                reset_operation();
            return;
        }
    }
    return CommonAPIImplementation::seti(param, value);
}

int DbAPI::query_stations()
{
    validate_input_query();
    return reset_operation(new QuantesonoOperation(*this));
}

int DbAPI::query_data()
{
    validate_input_query();
    if (station_context)
        return reset_operation(new VoglioquestoOperation<db::CursorStationData>(*this));
    else
        return reset_operation(new VoglioquestoOperation<db::CursorData>(*this));
}

void DbAPI::insert_data()
{
    if (perms & PERM_DATA_RO)
        throw error_consistency(
            "idba_insert_data cannot be called with the database open in data readonly mode");
    input_data.datetime.set_lower_bound();
    reset_operation(new PrendiloOperation(*this));
    unsetb();
}

void DbAPI::remove_data()
{
    if (! (perms & PERM_DATA_WRITE))
        throw error_consistency("remove_data must be called with the database open in data write mode");

    validate_input_query();

    if (station_context)
        tr->remove_station_data(input_query);
    else
        tr->remove_data(input_query);
    delete operation;
    operation = nullptr;
}

void DbAPI::messages_open_input(const char* filename, const char* mode, Encoding format, bool simplified)
{
    // Consistency checks
    if (strchr(mode, 'r') == NULL)
        throw error_consistency("input files should be open with 'r' mode");
    if ((perms & PERM_ANA_RO) || (perms & PERM_DATA_RO))
        throw error_consistency("messages_open must be called on a session with writable station and data");

    // Close existing file, if any
    if (input_file)
    {
        delete input_file;
        input_file = 0;
    }

    // Open new one and set import options
    if (*filename)
        input_file = new InputFile(filename, format, simplified);
    else
        input_file = new InputFile(format, simplified);

    input_file->opts.update_station = true;
    if (perms & PERM_ATTR_WRITE)
        input_file->opts.import_attributes = true;
    if (perms & PERM_DATA_WRITE)
        input_file->opts.overwrite = true;
}

void DbAPI::messages_open_output(const char* filename, const char* mode, Encoding format)
{
    if (strchr(mode, 'w') == NULL && strchr(mode, 'a') == NULL)
        throw error_consistency("output files should be open with 'w' or 'a' mode");

    // Close existing file, if any
    if (output_file)
    {
        delete output_file;
        output_file = 0;
    }

    if (*filename)
        output_file = new OutputFile(filename, mode, format);
    else
        output_file = new OutputFile(mode, format);
}

bool DbAPI::messages_read_next()
{
    if (!input_file)
        throw error_consistency("messages_read_next called but there are no open input files");
    if (!input_file->next())
        return false;
    tr->import_message(input_file->msg(), input_file->opts);
    return true;
}

void DbAPI::messages_write_next(const char* template_name)
{
    // Build an exporter for this template
    impl::ExporterOptions options;
    if (template_name) options.template_name = template_name;
    File& out = *(output_file->output);
    auto exporter = Exporter::create(out.encoding(), options);

    // Do the export with the current filter
    auto cursor = tr->query_messages(input_query);
    while (cursor->next())
    {
        std::vector<std::shared_ptr<dballe::Message>> msgs;
        msgs.emplace_back(cursor->detach_message());
        out.write(exporter->to_binary(msgs));
    }
}

std::unique_ptr<API> DbAPI::fortran_connect(const char* url, const char* anaflag, const char* dataflag, const char* attrflag)
{
    unsigned perms = DbAPI::compute_permissions(anaflag, dataflag, attrflag);
    bool readonly = !(perms & (fortran::DbAPI::PERM_ANA_WRITE | fortran::DbAPI::PERM_DATA_ADD | fortran::DbAPI::PERM_DATA_WRITE | fortran::DbAPI::PERM_ATTR_WRITE));
    auto db = DB::connect_from_url(url);
    auto tr = dynamic_pointer_cast<db::Transaction>(db->transaction(readonly));
    return std::unique_ptr<API>(new fortran::DbAPI(tr, perms));
}

}
}
