#ifndef DBALLE_DB_DEFS_H
#define DBALLE_DB_DEFS_H

#include <wreport/error.h>

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
 * Known database formats
 */
typedef enum {
    V5 = 0,
    V6 = 1,
    MEM = 2,
    MESSAGES = 3,
    V7 = 4,
} Format;

}
}
#endif
