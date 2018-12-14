#include "api.h"
#include "dballe/core/fortran.h"
#include <limits>

using namespace std;

namespace dballe {
namespace fortran {

const signed char API::missing_byte = numeric_limits<signed char>::max();
const int API::missing_int = MISSING_INT;
const float API::missing_float = numeric_limits<float>::max();
const double API::missing_double = numeric_limits<double>::max();

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
