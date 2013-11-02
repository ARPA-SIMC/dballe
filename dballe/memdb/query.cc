#include "query.h"
#include <algorithm>

using namespace std;

namespace dballe {
namespace memdb {

Results::Results()
    : select_all(true)
{
}

void Results::intersect(size_t pos)
{
    if (select_all)
    {
        values.push_back(pos);
        select_all = false;
        return;
    }

    vector<size_t>::const_iterator i = lower_bound(values.begin(), values.end(), pos);
    bool found = i != values.end();
    values.clear();
    if (found)
        values.push_back(pos);
}

}
}


