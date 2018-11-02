#ifndef DBALLE_DB_H
#define DBALLE_DB_H

#include <dballe/fwd.h>
#include <memory>

namespace dballe {

struct DB: public std::enable_shared_from_this<DB>
{
    virtual ~DB();

    /**
     * Create from an url-like specification, as described in
     * doc/fapi_connect.md
     *
     * @param url
     *   The url-like connection descriptor
     */
    static std::shared_ptr<DB> connect_from_url(const std::string& url);
};

}

#endif
