#ifndef DBALLE_DATA_H
#define DBALLE_DATA_H

#include <dballe/fwd.h>
#include <dballe/types.h>
#include <memory>
#include <wreport/var.h>

namespace dballe {

/**
 * Key/value store where keys are strings and values are wreport variables.
 *
 * Keys are defined from a known vocabulary, where each key has an associated
 * wreport::Varinfo structure.
 */
class Data
{
public:
    Data()            = default;
    Data(const Data&) = default;
    Data(Data&&)      = default;
    virtual ~Data() {}
    Data& operator=(const Data& o) = default;
    Data& operator=(Data&& o)      = default;

    /// Create a new Record
    static std::unique_ptr<Data> create();

    /// Remove all contents from the record
    virtual void clear() = 0;

    /// Unset all database IDs in station and values
    virtual void clear_ids() = 0;

    /// Remove all Bxxyyy keys from the record, leaving the rest intact
    virtual void clear_vars() = 0;

    /// Check if two records are the same
    virtual bool operator==(const Data& rec) const = 0;

    /// Check if two records differ
    virtual bool operator!=(const Data& rec) const = 0;

    /// Print the contents of this record to the given stream
    virtual void print(FILE* out) const = 0;
};

} // namespace dballe
#endif
