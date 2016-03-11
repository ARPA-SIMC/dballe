#ifndef DBALLE_CORE_ERROR_H
#define DBALLE_CORE_ERROR_H

#include <wreport/error.h>

namespace dballe {

enum ErrorCode {
    // Database error
    DBALLE_ERR_DB          = 1001,
};

/// Base exception for DB-All.e errors
struct error : public wreport::error
{
    /**
     * String description for an error code.
     *
     * It delegates to wreport::error::strerror for codes not known to
     * DB-All.e, so it can be used instead of wreport::error::strerror.
     */
    static const char* strerror(ErrorCode code);
};

/// Error in case of failed database operations
struct error_db : public error
{
    wreport::ErrorCode code() const noexcept override { return (wreport::ErrorCode)DBALLE_ERR_DB; }
};

}

#endif
