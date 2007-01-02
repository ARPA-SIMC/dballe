#ifndef DBALLE_CPP_FORMAT_H
#define DBALLE_CPP_FORMAT_H

#include <string>

namespace dballe {

/// Return a string description of a level
std::string describeLevel(int ltype, int l1, int l2);

/// Return a string description of a time range
std::string describeTrange(int ptype, int p1, int p2);

}

#endif
