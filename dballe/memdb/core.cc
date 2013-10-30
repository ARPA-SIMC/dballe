#include "core.h"
#include <dballe/core/defs.h>

using namespace std;

namespace dballe {
namespace memdb {

template class Index<std::string>;
template class Index<Coord>;
template class Index<Level>;
template class Index<Trange>;

}
}


