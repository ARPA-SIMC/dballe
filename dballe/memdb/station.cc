#include "station.h"

using namespace std;

namespace dballe {
namespace memdb {

const Station& Stations::get_station(double lat, double lon, const std::string& report)
{
    // Search
    Coord coords(lat, lon);
    Positions res = by_coord.search(coords);
    for (Positions::const_iterator i = res.begin();
            i != res.end(); ++i)
        if (!stations[*i].mobile && stations[*i].report == report)
            return stations[*i];

    // Station not found, create it
    stations.push_back(Station(coords, report));
    // Index it
    by_coord[coords].insert(stations.size() - 1);
    // And return it
    return stations.back();
}

const Station& Stations::get_station(double lat, double lon, const std::string& ident, const std::string& report)
{
    // Search
    Coord coords(lat, lon);
    Positions res = by_coord.search(coords);
    by_ident.refine(ident, res);
    for (Positions::const_iterator i = res.begin();
            i != res.end(); ++i)
        if (stations[*i].mobile && stations[*i].report == report)
            return stations[*i];

    // Station not found, create it
    stations.push_back(Station(coords, ident, report));
    // Index it
    by_coord[coords].insert(stations.size() - 1);
    by_ident[ident].insert(stations.size() - 1);
    // And return it
    return stations.back();
}

}
}

