#ifndef DBALLE_DB_DEFS_H
#define DBALLE_DB_DEFS_H

#include <vector>
#include <wreport/error.h>
#include <wreport/varinfo.h>

namespace dballe {
namespace db {

/**
 * Structure uesd to pass lists of varcodes
 */
typedef std::vector<wreport::Varcode> AttrList;

} // namespace db
} // namespace dballe
#endif
