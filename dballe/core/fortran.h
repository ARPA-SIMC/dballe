#ifndef DBALLE_CORE_FORTRAN_H
#define DBALLE_CORE_FORTRAN_H

#include <string>

namespace dballe {
namespace impl {

void string_to_fortran(const char* str, char* buf, unsigned buf_len);
void string_to_fortran(const std::string& str, char* buf, unsigned buf_len);


}
}

#endif
