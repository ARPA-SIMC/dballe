#include "error.h"

namespace dballe {

const char* error::strerror(wreport::ErrorCode code)
{
    return wreport::error::strerror(code);
}

} // namespace dballe
