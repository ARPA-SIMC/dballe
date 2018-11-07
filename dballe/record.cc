#include "record.h"
#include "core/record.h"
#include "var.h"

using namespace wreport;
using namespace std;

namespace dballe {

std::unique_ptr<Record> Record::create()
{
    return unique_ptr<Record>(new core::Record);
}

}
