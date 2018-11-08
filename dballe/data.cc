#include "data.h"
#include "core/data.h"

using namespace wreport;
using namespace std;

namespace dballe {

std::unique_ptr<Data> Data::create()
{
    return unique_ptr<Data>(new core::Data);
}

}

