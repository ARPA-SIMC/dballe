#ifndef DBALLE_FORTRAN_DBAPI_H
#define DBALLE_FORTRAN_DBAPI_H

#include "commonapi.h"
#include <dballe/file.h>

namespace dballe {
struct DB;

namespace db {
struct CursorStation;
struct Transaction;
} // namespace db

namespace fortran {

struct InputFile;
struct OutputFile;

struct DbAPI : public CommonAPIImplementation
{
protected:
    void shutdown(bool commit);

public:
    std::shared_ptr<db::Transaction> tr;
    InputFile* input_file   = nullptr;
    OutputFile* output_file = nullptr;

    DbAPI(std::shared_ptr<db::Transaction> tr, const char* anaflag,
          const char* dataflag, const char* attrflag);
    DbAPI(std::shared_ptr<db::Transaction> tr, unsigned perms);
    virtual ~DbAPI();

    void seti(const char* param, int value) override;
    void reinit_db(const char* repinfofile = nullptr) override;
    void remove_all() override;
    int query_stations() override;
    int query_data() override;
    void insert_data() override;
    void remove_data() override;
    void commit() override;
    void messages_open_input(const char* filename, const char* mode,
                             Encoding format, bool simplified = true) override;
    void messages_open_output(const char* filename, const char* mode,
                              Encoding format) override;
    bool messages_read_next() override;
    void messages_write_next(const char* template_name = 0) override;

    static std::unique_ptr<API> fortran_connect(const DBConnectOptions& options,
                                                const char* anaflag,
                                                const char* dataflag,
                                                const char* attrflag);

    friend class Operation;
};

} // namespace fortran
} // namespace dballe

#endif
