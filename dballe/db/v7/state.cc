#include "state.h"
#include "dballe/record.h"
#include <algorithm>
#include <ostream>

using namespace std;

namespace dballe {
namespace db {
namespace v7 {

bool LevTrEntry::operator==(const LevTrEntry& o) const
{
    if (id != MISSING_INT && o.id != MISSING_INT)
        return id == o.id;
    if (level != o.level) return false;
    return trange == o.trange;
}

bool LevTrEntry::operator!=(const LevTrEntry& o) const
{
    if (id != MISSING_INT && o.id != MISSING_INT)
        return id != o.id;
    if (level == o.level) return false;
    return trange != o.trange;
}

std::ostream& operator<<(std::ostream& out, const LevTrEntry& l)
{
    out << "(";
    if (l.id == MISSING_INT)
        out << "-";
    else
        out << l.id;
    return out << ":" << l.level << ":" << l.trange;
}

}
}
}
