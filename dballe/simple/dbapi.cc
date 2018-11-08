#include "dbapi.h"
#include "dballe/file.h"
#include "dballe/importer.h"
#include "dballe/exporter.h"
#include "dballe/message.h"
#include "dballe/core/query.h"
#include "dballe/core/data.h"
#include "dballe/core/values.h"
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
    Messages current_msg;
    unsigned current_msg_idx = 0;
    DBImportMessageOptions opts;

    InputFile(Encoding format, bool simplified)
    {
        ImporterOptions importer_options;
        importer_options.simplified = simplified;
        input = File::create(format, stdin, false, "(stdin)").release();
        importer = Importer::create(format, importer_options).release();
    }
    InputFile(const char* fname, Encoding format, bool simplified)
    {
        ImporterOptions importer_options;
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

template<typename Cursor>
struct VoglioquestoOperation : public Operation
{
    dballe::Query* query = nullptr;
    Cursor* query_cur = nullptr;
    bool valid_cached_attrs = false;

    VoglioquestoOperation(const dballe::Record& input)
        : query(Query::from_record(input).release())
    {
    }
    ~VoglioquestoOperation()
    {
        if (query_cur) query_cur->discard();
        delete query_cur;
        delete query;
    }
    int voglioquesto(db::Transaction& tr, bool station_context)
    {
        if (station_context)
            query_cur = tr.query_station_data(*query).release();
        else
            query_cur = tr.query_data(*query).release();
        return query_cur->remaining();
    }
    wreport::Varcode dammelo(dballe::Record& output) override
    {
        output.clear();
        if (!query_cur) return 0;

        if (query_cur->next())
        {
            query_cur->to_record(output);
            valid_cached_attrs = true;
            return query_cur->get_varcode();
        } else {
            delete query_cur;
            query_cur = nullptr;
            return 0;
        }
    }
    void voglioancora(db::Transaction& tr, std::vector<wreport::Var>& dest) override
    {
        if (!query_cur) throw error_consistency("voglioancora called after dammelo returned end of data");
        function<void(unique_ptr<Var>&&)> consumer;
        if (selected_attr_codes.empty())
        {
            consumer = [&](unique_ptr<Var>&& var) {
                // TODO: move into qcoutput
                dest.emplace_back(std::move(*var));
            };
        } else {
            consumer = [&](unique_ptr<Var>&& var) {
                for (auto code: selected_attr_codes)
                    if (code == var->code())
                    {
                        // TODO: move into qcoutput
                        dest.emplace_back(std::move(*var));
                        break;
                    }
            };
        }
        query_cur->attr_query(consumer, !valid_cached_attrs);
    }
    void critica(db::Transaction& tr, const Values& qcinput) override
    {
        if (!query_cur) throw error_consistency("critica called after dammelo returned end of data");
        if (dynamic_cast<const db::CursorStationData*>(query_cur))
            tr.attr_insert_station(query_cur->attr_reference_id(), qcinput);
        else
            tr.attr_insert_data(query_cur->attr_reference_id(), qcinput);
        valid_cached_attrs = false;
    }
    void scusa(db::Transaction& tr) override
    {
        if (!query_cur) throw error_consistency("scusa called after dammelo returned end of data");
        if (dynamic_cast<const db::CursorStationData*>(query_cur))
            tr.attr_remove_station(query_cur->attr_reference_id(), selected_attr_codes);
        else
            tr.attr_remove_data(query_cur->attr_reference_id(), selected_attr_codes);
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
    std::vector<VarID> last_inserted_varids;
    wreport::Varcode varcode = 0;

    void set_varcode(wreport::Varcode varcode) override { this->varcode = varcode; }
    int prendilo(db::Transaction& tr, const dballe::Record& input, bool station_context, unsigned perms)
    {
        // fprintf(stderr, "PRENDILO DB\n");
        // tr.dump(stderr);
        // fprintf(stderr, "PRENDILO RECORD\n");
        // input.print(stderr);

        last_inserted_varids.clear();
        if (station_context)
        {
            core::Data data;
            core::Record::downcast(input).to_data(data);
            tr.insert_station_data(data, (perms & DbAPI::PERM_DATA_WRITE) != 0, (perms & DbAPI::PERM_ANA_WRITE) != 0);
            for (const auto& v: data.values)
                last_inserted_varids.push_back(VarID(v.first, true, v.second.data_id));
            return data.station.id;
        } else {
            DataValues dv(input);
            tr.insert_data(dv, (perms & DbAPI::PERM_DATA_WRITE) != 0, (perms & DbAPI::PERM_ANA_WRITE) != 0);
            for (const auto& v: dv.values)
                last_inserted_varids.push_back(VarID(v.first, false, v.second.data_id));
            return dv.station.id;
        }
    }
    void select_attrs(const std::vector<wreport::Varcode>& varcodes) override
    {
        if (!varcodes.empty())
            throw error_consistency("*var and *varlist cannot be set after a prendilo");
    }
    void voglioancora(db::Transaction& tr, std::vector<wreport::Var>& dest) override
    {
        throw error_consistency("voglioancora cannot be called after a prendilo");
    }
    void critica(db::Transaction& tr, const Values& qcinput) override
    {
        int data_id = MISSING_INT;
        bool is_station = false;
        // Lookup the variable we act on from the results of last prendilo
        if (last_inserted_varids.size() == 1)
        {
            data_id = last_inserted_varids[0].id;
            is_station = last_inserted_varids[0].station;
        } else {
            if (varcode == 0)
                throw error_consistency("please set *var_related before calling critica after setting multiple variables in a single prendilo");
            for (const auto& i: last_inserted_varids)
                if (i.code == varcode)
                {
                    data_id = i.id;
                    is_station = i.station;
                    break;
                }
            if (data_id == MISSING_INT)
                error_consistency::throwf("cannot insert attributes for *var_related=%01d%02d%03d: the last prendilo inserted %zd variables, none of which match *var_related", WR_VAR_FXY(varcode), last_inserted_varids.size());
        }
        if (is_station)
            tr.attr_insert_station(data_id, qcinput);
        else
            tr.attr_insert_data(data_id, qcinput);
    }
    void scusa(db::Transaction& tr) override
    {
        throw error_consistency("scusa cannot be called after a prendilo");
    }
};


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

    if (ana_cur)
    {
        ana_cur->discard();
        delete ana_cur;
        ana_cur = nullptr;
    }

    if (commit)
        tr->commit();
}

void DbAPI::fatto()
{
    shutdown(true);
}

int DbAPI::enqi(const char* param)
{
    if (strcmp(param, "*ana_id") == 0)
        return last_inserted_station_id;
    else
        return CommonAPIImplementation::enqi(param);
}

void DbAPI::scopa(const char* repinfofile)
{
    if (!(perms & PERM_DATA_WRITE))
        error_consistency::throwf(
            "scopa must be run with the database open in data write mode");
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

int DbAPI::quantesono()
{
    if (ana_cur != NULL)
    {
        ana_cur->discard();
        delete ana_cur;
        ana_cur = 0;
    }
    auto query = Query::from_record(input);
    ana_cur = tr->query_stations(*query).release();
    delete operation;
    operation = nullptr;

    return ana_cur->remaining();
}

void DbAPI::elencamele()
{
    if (ana_cur == NULL)
        throw error_consistency("elencamele called without a previous quantesono");

    output.clear();
    if (ana_cur->next())
        ana_cur->to_record(output);
    else
    {
        delete ana_cur;
        ana_cur = NULL;
    }
}

int DbAPI::voglioquesto()
{
    delete operation;
    if (station_context)
    {
        VoglioquestoOperation<db::CursorStationData>* op;
        operation = op = new VoglioquestoOperation<db::CursorStationData>(input);
        op->query_cur = dynamic_cast<db::CursorStationData*>(tr->query_station_data(*op->query).release());
        return op->query_cur->remaining();
    }
    else
    {
        VoglioquestoOperation<db::CursorData>* op;
        operation = op = new VoglioquestoOperation<db::CursorData>(input);
        op->query_cur = dynamic_cast<db::CursorData*>(tr->query_data(*op->query).release());
        return op->query_cur->remaining();
    }
}

wreport::Varcode DbAPI::dammelo()
{
    if (!operation) throw error_consistency("dammelo called without a previous voglioquesto");

    /* Reset qc record iterator, so that ancora will not return
     * leftover QC values from a previous query */
    qc_iter = -1;

    return operation->dammelo(output);
}

void DbAPI::prendilo()
{
    if (perms & PERM_DATA_RO)
        throw error_consistency(
            "idba_prendilo cannot be called with the database open in data readonly mode");
    delete operation;
    PrendiloOperation* po;
    operation = po = new PrendiloOperation;
    last_inserted_station_id = po->prendilo(*tr, input, station_context, perms);
    unsetb();
}

void DbAPI::dimenticami()
{
    if (! (perms & PERM_DATA_WRITE))
        throw error_consistency("dimenticami must be called with the database open in data write mode");

    auto query = Query::from_record(input);
    if (station_context)
        tr->remove_station_data(*query);
    else
        tr->remove_data(*query);
    delete operation;
    operation = nullptr;
}

int DbAPI::voglioancora()
{
    // Query attributes
    qcoutput.clear();
    if (!operation) throw error_consistency("voglioancora was not called after a dammelo, or was called with an invalid *context_id or *var_related");
    operation->voglioancora(*tr, qcoutput);
    qc_iter = 0;
    qcinput.clear();
    return qcoutput.size();
}

void DbAPI::critica()
{
    if (perms & PERM_ATTR_RO)
        throw error_consistency(
            "critica cannot be called with the database open in attribute readonly mode");

    if (!operation) throw error_consistency("critica was not called after a dammelo or prendilo, or was called with an invalid *context_id or *var_related");
    operation->critica(*tr, qcinput);
    qcinput.clear();
}

void DbAPI::scusa()
{
    if (! (perms & PERM_ATTR_WRITE))
        throw error_consistency(
            "scusa must be called with the database open in attribute write mode");


    // Retrieve the varcodes of the attributes we want to remove
    if (!operation) throw error_consistency("scusa was not called after a dammelo, or was called with an invalid *context_id or *var_related");
    operation->scusa(*tr);
    qcinput.clear();
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
    ExporterOptions options;
    if (template_name) options.template_name = template_name;
    File& out = *(output_file->output);
    auto exporter = Exporter::create(out.encoding(), options);

    // Do the export with the current filter
    auto query = Query::from_record(input);
    tr->export_msgs(*query, [&](unique_ptr<Message>&& msg) {
        Messages msgs;
        msgs.emplace_back(move(msg));
        out.write(exporter->to_binary(msgs));
        return true;
    });
}

}
}
