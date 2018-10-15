#ifndef DBALLE_MESSAGE_H
#define DBALLE_MESSAGE_H

#include <dballe/fwd.h>
#include <dballe/types.h>
#include <wreport/varinfo.h>
#include <memory>

namespace wreport {
struct Var;
}

namespace dballe {

/**
 * A bulletin that has been decoded and physically interpreted.
 *
 * Message collects zero or more variables that have been forecast or measured
 * by the same station in the same instant.
 *
 * Each variable is annotated with its vertical level/layer information, and
 * its time range / statistical information.
 *
 * The representation in Message is as connected as possible to physics rather
 * than to observations.
 */
struct Message
{
    virtual ~Message();

    /// Get the reference Datetime for this message
    virtual Datetime get_datetime() const = 0;

    /// Return a copy of this message
    virtual std::unique_ptr<Message> clone() const = 0;

    /**
     * Get a variable given its code, level and time range information.
     *
     * @return
     *   A pointer to the variable, or nullptr if it was not found.
     */
    virtual const wreport::Var* get(wreport::Varcode code, const Level& lev, const Trange& tr) const = 0;

    /// Print all the contents of this message to an output stream
    virtual void print(FILE* out) const = 0;

    /**
     * Compute the differences between two Messages
     *
     * Details of the differences found will be formatted using the wreport
     * notes system (@see wreport/notes.h).
     *
     * @returns
     *   The number of differences found
     */
    virtual unsigned diff(const Message& msg) const = 0;
};

}
#endif
