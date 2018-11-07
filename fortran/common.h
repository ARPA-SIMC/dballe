#ifndef FDBA_COMMON_H
#define FDBA_COMMON_H

#include <string>

namespace dballe {
namespace fortran {

/**
 * Copy the string str to a fortran string buf of length len.
 *
 * If str is nullptr, buf is set to the agreed string missing value of the
 * DB-All-e API.
 */
void cstring_to_fortran(const char* str, char* buf, unsigned buf_len);

/**
 * Copy the string str to a fortran string buf of length len.
 */
void cstring_to_fortran(const std::string& str, char* buf, unsigned buf_len);

}
}
#endif
