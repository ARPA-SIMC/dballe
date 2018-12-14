#include "fortran.h"
#include <cstring>

namespace dballe {
namespace impl {

void string_to_fortran(const char* str, char* buf, unsigned buf_len)
{
    // Copy the result values
    size_t len;
    if (buf_len == 0)
        len = 0;
    else if (str)
    {
        len = strlen(str);
        if (len > buf_len)
            len = buf_len;
        memcpy(buf, str, len);
    } else {
        // The missing string value has been defined as a
        // null byte plus blank padding.
        buf[0] = 0;
        len = 1;
    }

    if (len < buf_len)
        memset(buf + len, ' ', buf_len - len);
}

void string_to_fortran(const std::string& str, char* buf, unsigned buf_len)
{
    // Copy the result values
    size_t len;
    if (buf_len == 0)
        len = 0;
    else
    {
        len = str.size();
        if (len > buf_len)
            len = buf_len;
        memcpy(buf, str.data(), len);
    }

    if (len < buf_len)
        memset(buf + len, ' ', buf_len - len);
}
}
}
