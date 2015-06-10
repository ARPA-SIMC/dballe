#include "record.h"
#include "core/record.h"
#include "core/var.h"

using namespace wreport;
using namespace std;

namespace dballe {

std::unique_ptr<Record> Record::create()
{
    return unique_ptr<Record>(new core::Record);
}

const Var& Record::operator[](const char* key) const
{
    const Var* res = get(key);
    if (!res)
        error_notfound::throwf("key '%s' not found in record", key);
    if (!res->isset())
        error_notfound::throwf("key '%s' is unset in record", key);
    return *res;
}

bool Record::isset(const char* key) const
{
    const Var* res = get(key);
    if (!res) return false;
    if (!res->isset()) return false;
    return true;
}

wreport::Varinfo Record::key_info(const char* key)
{
    using namespace dballe::core;
    dba_keyword k = core::Record::keyword_byname(key);
    if (k != DBA_KEY_ERROR)
        return core::Record::keyword_info(k);
    return varinfo(resolve_varcode_safe(key));
}

wreport::Varinfo Record::key_info(const std::string& key)
{
    using namespace dballe::core;
    dba_keyword k = core::Record::keyword_byname_len(key.data(), key.size());
    if (k != DBA_KEY_ERROR)
        return core::Record::keyword_info(k);
    return varinfo(resolve_varcode_safe(key));
}

}
