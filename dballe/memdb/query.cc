#include "query.h"
#include <algorithm>

using namespace std;

namespace dballe {
namespace memdb {

BaseResults::BaseResults()
    : select_all(true)
{
}

void BaseResults::intersect(size_t pos)
{
    if (select_all)
    {
        indices.push_back(pos);
        select_all = false;
        return;
    }

    vector<size_t>::const_iterator i = lower_bound(indices.begin(), indices.end(), pos);
    bool found = i != indices.end();
    indices.clear();
    if (found)
        indices.push_back(pos);
}

}
}


