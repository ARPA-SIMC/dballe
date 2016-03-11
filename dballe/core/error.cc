#include "error.h"

namespace dballe {

const char* error::strerror(ErrorCode code)
{
    switch (code)
    {
        case DBALLE_ERR_DB: return "database error";
        default: return wreport::error::strerror((wreport::ErrorCode)code);
    }
}

}
