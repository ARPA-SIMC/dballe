#include "core.h"
#include <dballe/core/defs.h>
#include <math.h>

using namespace std;

namespace dballe {
namespace memdb {

Coord::Coord(double lat, double lon)
    : lat(lround(lat * 100000)), lon(lround(lon * 100000)) {}

double Coord::dlat() const { return (double)lat/100000.0; }
double Coord::dlon() const { return (double)lon/100000.0; }

template class Index<std::string>;
template class Index<Coord>;
template class Index<Level>;
template class Index<Trange>;

}
}


