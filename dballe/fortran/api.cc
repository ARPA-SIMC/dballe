#include "api.h"
#include <limits>
#include <cstring>

using namespace std;

namespace dballe {
namespace fortran {

const signed char API::missing_byte = numeric_limits<signed char>::max();
const int API::missing_int = MISSING_INT;
const float API::missing_float = numeric_limits<float>::max();
const double API::missing_double = numeric_limits<double>::max();

void API::to_fortran(const char* str, char* buf, unsigned buf_len)
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

void API::to_fortran(const std::string& str, char* buf, unsigned buf_len)
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
