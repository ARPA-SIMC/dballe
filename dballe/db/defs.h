#ifndef DBALLE_DB_DEFS_H
#define DBALLE_DB_DEFS_H

#include <wreport/error.h>
#include <wreport/varinfo.h>
#include <vector>

/**
 * Flags controlling message import
 * @{
 */
/* Import the attributes. */
#define DBA_IMPORT_ATTRS		1
/* Attempt to merge pseudoana extra information into the existing ones. */
#define DBA_IMPORT_FULL_PSEUDOANA	2
/* Message data will overwrite existing values; otherwise, trying to insert
 * existing data will cause an error. */
#define DBA_IMPORT_OVERWRITE		8
/// @}

namespace dballe {
namespace db {

/**
 * Structure uesd to pass lists of varcodes
 */
typedef std::vector<wreport::Varcode> AttrList;

}
}
#endif
