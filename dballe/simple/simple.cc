#include "simple.h"
#include <limits>

using namespace std;

namespace dballe {
namespace fortran {

const signed char API::missing_byte = numeric_limits<signed char>::max();
const int API::missing_int = MISSING_INT;
const float API::missing_float = numeric_limits<float>::max();
const double API::missing_double = numeric_limits<double>::max();

}
}
