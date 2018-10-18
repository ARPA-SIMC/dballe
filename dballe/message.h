#ifndef DBALLE_MESSAGE_H
#define DBALLE_MESSAGE_H

#include <dballe/fwd.h>
#include <dballe/types.h>
#include <wreport/varinfo.h>
#include <memory>
#include <iosfwd>

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

    /// Return the type of the data in the message
    virtual MessageType get_type() const = 0;

    /// Get the reference Datetime for this message
    virtual Datetime get_datetime() const = 0;

    /// Get the reference coordinates for this message
    virtual Coords get_coords() const = 0;

    /// Get the station identifier for this message
    virtual Ident get_ident() const = 0;

    /// Get the rep_memo for this message
    virtual std::string get_network() const = 0;

    /// Return a copy of this message
    virtual std::unique_ptr<Message> clone() const = 0;

    /**
     * Get a variable given its code, level and time range information.
     *
     * @return
     *   A pointer to the variable, or nullptr if it was not found.
     */
    const wreport::Var* get(const Level& lev, const Trange& tr, wreport::Varcode code) const;

    /**
     * Get a variable given its shortcut name
     *
     * @return
     *   A pointer to the variable, or nullptr if it was not found.
     */
    const wreport::Var* get(const char* shortcut) const;

    /**
     * Get a variable given its shortcut name
     *
     * @return
     *   A pointer to the variable, or nullptr if it was not found.
     */
    const wreport::Var* get(const std::string& shortcut) const;

    /**
     * Add or replace a value
     *
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     * @param code
     *   The Varcode of the destination value.  If it is different than the
     *   varcode of var, a conversion will be attempted.
     * @param var
     *   The Var with the value to set
     */
    void set(const Level& lev, const Trange& tr, wreport::Varcode code, const wreport::Var& var);

    /**
     * Add or replace a value
     *
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     * @param var
     *   The Var with the value to set
     */
    void set(const Level& lev, const Trange& tr, const wreport::Var& var);

    /**
     * Add or replace a value, taking ownership of the source variable without
     * copying it.
     *
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     * @param var
     *   The Var with the value to set.  This Message will take ownership of memory
     *   management.
     */
    void set(const Level& lev, const Trange& tr, std::unique_ptr<wreport::Var> var);

    /**
     * Add or replace a value, taking ownership of the source variable without
     * copying it.
     *
     * @param shortcut
     *   Shortcut name mapping to a (Level, Trange, Varcode) triplet
     * @param var
     *   The Var with the value to set.  This Message will take ownership of memory
     *   management.
     */
    void set(const char* shortcut, std::unique_ptr<wreport::Var> var);

    /**
     * Add or replace a value
     *
     * @param shortcut
     *   Shortcut name mapping to a (Level, Trange, Varcode) triplet
     * @param var
     *   The Var with the value to set. If its varcode is different than the
     *   varcode of the shortcut, a conversion will be attempted.
     */
    void set(const char* shortcut, const wreport::Var& var);

    /**
     * Iterate the contents of the message
     */
    virtual bool foreach_var(std::function<bool(const Level&, const Trange&, const wreport::Var&)>) const = 0;

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

    /**
     * Create a new empty message
     */
    static std::unique_ptr<Message> create(MessageType type);

protected:
    /// Implementation of get(const Level&, const Trange&, wreport::Varcode)
    virtual const wreport::Var* get_impl(const Level& lev, const Trange& tr, wreport::Varcode code) const = 0;

    /// Implementation of set(const Level& const Trange&, std::unique_ptr<wreport::Var>)
    virtual void set_impl(const Level& lev, const Trange& tr, std::unique_ptr<wreport::Var> var) = 0;
};


/**
 * Return a string with the name of a MessageType.
 *
 * @param type
 *   The MessageType value to format
 * @return
 *   The name, as a const string.  This function is thread safe.
 */
const char* format_message_type(MessageType type);


/// Serialize MessageType
std::ostream& operator<<(std::ostream&, const dballe::MessageType&);

}
#endif
