#ifndef DBALLE_CORE_RECORD_ACCESS_H
#define DBALLE_CORE_RECORD_ACCESS_H

#include <dballe/core/fwd.h>
#include <string>

namespace dballe {
namespace core {

/// Check if a value is set
bool record_isset(const core::Record& rec, const char* key);

/// Remove/unset a key from the record
void record_unset(core::Record& rec, const char* name);

/**
 * Set a core::Record key to an integer value.
 *
 * If the key that is being set has a decimal component (like lat and lon),
 * the integer value represents the units of maximum precision of the
 * field. For example, using seti to set lat to 4500000 is the same as
 * setting it to 45.0.
 */
void record_seti(core::Record& rec, const char* key, int val);

/**
 * Set a core::Record key to a double value.
 */
void record_setd(core::Record& rec, const char* key, double val);

/**
 * Set a core::Record key to a string value.
 *
 * If the key that is being set has a decimal component (like lat and lon),
 * the string is converted to an integer value representing the units of
 * maximum precision of the field. For example, using seti to set lat to
 * "4500000" is the same as setting it to 45.0.
 */
void record_setc(core::Record& rec, const char* key, const char* val);

/**
 * Set a core::Record key to a string value.
 *
 * If the key that is being set has a decimal component (like lat and lon),
 * the string is converted to an integer value representing the units of
 * maximum precision of the field. For example, using seti to set lat to
 * "4500000" is the same as setting it to 45.0.
 */
void record_sets(core::Record& rec, const char* key, const std::string& val);

/**
 * Set a core::Record key to a string value.
 *
 * Contrarily to setc, the string is parsed according to the natural
 * representation for the given key. For example, if lat is set to "45",
 * then it gets the value 45.0.
 *
 * Also, if a Decimal or Integer value is assigned "-", it is unset
 * instead.
 */
void record_setf(core::Record& rec, const char* key, const char* val);

/// Query an integer value, returning def if missing
int record_enqi(const core::Record& rec, const char* key, int def);

/// Query a double value, returning def if missing
double record_enqd(const core::Record& rec, const char* key, double def);

/// Query a double value, returning false if missing
bool record_enqdb(const core::Record& rec, const char* key, double& res);

/// Query a string value, returning def if missing
std::string record_enqs(const core::Record& rec, const char* key, const std::string& def);

/// Query a string value, returning false if missing
bool record_enqsb(const core::Record& rec, const char* key, std::string& res);


}
}

#endif
