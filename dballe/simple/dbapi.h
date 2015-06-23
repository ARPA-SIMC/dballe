#ifndef FDBA_DBAPI_H
#define FDBA_DBAPI_H

#include "commonapi.h"
#include <dballe/file.h>

namespace dballe {
struct DB;

namespace db {
struct CursorStation;
struct CursorValue;
}

namespace fortran {

struct InputFile;
struct OutputFile;

class DbAPI : public CommonAPIImplementation
{
protected:
    DB& db;
    db::CursorStation* ana_cur;
    db::CursorValue* query_cur;
    InputFile* input_file;
    OutputFile* output_file;

public:
    DbAPI(DB& db, const char* anaflag, const char* dataflag, const char* attrflag);
    virtual ~DbAPI();

    virtual int enqi(const char* param);

    virtual void scopa(const char* repinfofile = 0);
    virtual void remove_all();

    virtual int quantesono();
    virtual void elencamele();

    virtual int voglioquesto();
    virtual const char* dammelo();

    virtual void prendilo();
    virtual void dimenticami();

    virtual int voglioancora();

    virtual void critica();
    virtual void scusa();

    virtual void messages_open_input(const char* filename, const char* mode, File::Encoding format, bool simplified=true);
    virtual void messages_open_output(const char* filename, const char* mode, File::Encoding format);
    virtual bool messages_read_next();
    virtual void messages_write_next(const char* template_name=0);
};

}
}

#endif
