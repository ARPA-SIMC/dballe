#ifndef FDBA_ERROR_H
#define FDBA_ERROR_H

#include <wreport/error.h>

namespace dballe {
struct DB;

namespace fortran {
struct API;

/// Initialise error handlers
void error_init();

/// Digest an exception turning it into a fortran API result code
int error(wreport::error& e);

/// Return a success code, updating the error information accordingly
int success();

}
}
#endif
