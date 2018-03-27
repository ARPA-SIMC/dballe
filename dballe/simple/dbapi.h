#ifndef FDBA_DBAPI_H
#define FDBA_DBAPI_H

#include "commonapi.h"
#include <dballe/file.h>

namespace dballe {
struct DB;

namespace db {
struct CursorStation;
struct CursorValue;
struct Transaction;
}

namespace fortran {

struct InputFile;
struct OutputFile;

class DbAPI : public CommonAPIImplementation
{
protected:
    std::shared_ptr<db::Transaction> tr;
    db::CursorStation* ana_cur;
    db::CursorValue* query_cur;
    InputFile* input_file;
    OutputFile* output_file;
    int last_inserted_station_id;

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
    std::vector<VarID> last_inserted_varids;

    void shutdown(bool commit);

public:
    DbAPI(std::shared_ptr<db::Transaction> tr, const char* anaflag, const char* dataflag, const char* attrflag);
    virtual ~DbAPI();

    int enqi(const char* param) override;
    void scopa(const char* repinfofile=0) override;
    void remove_all() override;
    int quantesono() override;
    void elencamele() override;
    int voglioquesto() override;
    const char* dammelo() override;
    void prendilo() override;
    void dimenticami() override;
    int voglioancora() override;
    void critica() override;
    void scusa() override;
    void fatto() override;
    void messages_open_input(const char* filename, const char* mode, File::Encoding format, bool simplified=true) override;
    void messages_open_output(const char* filename, const char* mode, File::Encoding format) override;
    bool messages_read_next() override;
    void messages_write_next(const char* template_name=0) override;
};

}
}

#endif
