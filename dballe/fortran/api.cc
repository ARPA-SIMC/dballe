#include "api.h"
#include <wreport/error.h>
#include <cstring>
#include <limits>

using namespace std;

namespace dballe {
namespace fortran {

const signed char API::missing_byte = numeric_limits<signed char>::max();
const int API::missing_int = MISSING_INT;
const float API::missing_float = numeric_limits<float>::max();
const double API::missing_double = numeric_limits<double>::max();

namespace {

// Compute the number of digits of a 32bit unsigned integer
// From http://stackoverflow.com/questions/1489830/efficient-way-to-determine-number-of-digits-in-an-integer
unsigned count_digits(uint32_t x)
{
    if (x >= 10000) {
        if (x >= 10000000) {
            if (x >= 100000000) {
                if (x >= 1000000000)
                    return 10;
                return 9;
            }
            return 8;
        }
        if (x >= 100000) {
            if (x >= 1000000)
                return 7;
            return 6;
        }
        return 5;
    }
    if (x >= 100) {
        if (x >= 1000)
            return 4;
        return 3;
    }
    if (x >= 10)
        return 2;
    return 1;
}

// Adapted from http://tia.mat.br/blog/html/2014/06/23/integer_to_string_conversion.html
size_t uint32_to_str(uint32_t value, unsigned value_digits, char *dst)
{
    static const char digits[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    size_t const length = value_digits;
    size_t next = length - 1;
    while (value >= 100) {
        auto const i = (value % 100) * 2;
        value /= 100;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
        next -= 2;
    }
    // Handle last 1-2 digits
    if (value < 10) {
        dst[next] = '0' + uint32_t(value);
    } else {
        auto i = uint32_t(value) * 2;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
    }
    return length;
}

}

void API::to_fortran(int32_t val, char* buf, unsigned buf_len)
{
    if (!buf_len)
        wreport::error_consistency::throwf("Value %u does not fit in %u digits", (unsigned)val, buf_len);

    char* dest = buf;
    // No need to account for the trailing 0
    uint32_t dec;
    if (val < 0)
    {
        dec = -val;
        buf[0] = '-';
        ++dest;
        --buf_len;
    } else
        dec = val;

    unsigned digits = count_digits(dec);
    if (digits > buf_len)
        wreport::error_consistency::throwf("Value %u does not fit in %u digits", (unsigned)dec, buf_len);
    uint32_to_str(dec, digits, dest);

    // Pad with spaces for Fortran
    if (digits < buf_len)
        memset(buf + digits, ' ', buf_len - digits);
}

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

const char* API::test_enqc(const char* param, unsigned len)
{
    static std::string res;

    char buf[len];
    if (!enqc(param, buf, len))
        return nullptr;

    if (buf[0] == 0)
        return nullptr;

    if (len)
    {
        --len;
        while (len > 0 && buf[len] == ' ')
            --len;
    }

    if (!len && buf[0] == ' ')
        res.clear();
    else
        res.assign(buf, len + 1);

    return res.c_str();
}

}
}
