#ifndef DBA_MSG_CONTEXT_H
#define DBA_MSG_CONTEXT_H

/** @file
 * @ingroup msg
 *
 * Sorted storage for all the dba_msg_datum present on one level.
 */

#include <dballe/var.h>
#include <dballe/types.h>
#include <vector>
#include <memory>

namespace dballe {
namespace msg {

/**
 * Store an array of physical data all on the same level
 */
class Context
{
protected:
    /**
     * Return the index of the var `code' in l, or -1 if it was not found
     */
    int find_index(wreport::Varcode code) const;

public:
    Level level;
    Trange trange;

    /**
     * The variables in this context
     */
    std::vector<wreport::Var*> data;

    Context(const Level& lev, const Trange& tr);
    Context(const Context& c);
    ~Context();

    Context& operator=(const Context& src);

    /// @return true if this is the station context, else false
    bool is_station() const;

    /**
     * Compare two dba_msg_context strutures, for use in sorting.
     *
     * @param ctx
     *   First context to compare
     * @return
     *   -1 if l1 < l2, 0 if l1 == l2, 1 if l1 > l2
     */
    int compare(const Context& ctx) const;

    /**
     * Compare a Context struture with level and time range information, for
     * use in sorting.
     *
     * @return
     *   -1 if l < ltype,l1,l2; 0 if l == ltype,l1,l2; 1 if l > ltype,l1,l2
     */
    int compare(const Level& lev, const Trange& tr) const;

    /**
     * Add a Var to the level
     *
     * If a variable exists with the same code, it is replaced
     *
     * @param var
     *   The variable to add or replace.
     */
    void set(const wreport::Var& var);

    /**
     * Add a Var to the level
     *
     * If a variable exists with the same code, it is replaced
     *
     * The Context will take ownership of memory management for \a var
     *
     * @param var
     *   The variable to add or replace.
     */
    void set(std::unique_ptr<wreport::Var> var);

    /**
     * Add or replace an integer value
     *
     * @param code
     *   The wreport::Varcode of the destination value.
     * @param val
     *   The integer value of the data
     */
    void seti(wreport::Varcode code, int val);

    /**
     * Add or replace a double value
     *
     * @param code
     *   The wreport::Varcode of the destination value.
     * @param val
     *   The double value of the data
     */
    void setd(wreport::Varcode code, double val);

    /**
     * Add or replace a string value
     *
     * @param code
     *   The wreport::Varcode of the destination value.
     * @param val
     *   The string value of the data
     */
    void setc(wreport::Varcode code, const char* val);

    /**
     * Find a variable given its varcode
     *
     * @param code
     *   The wreport::Varcode of the variable to query.  See @ref vartable.h
     * @return
     *   The variable found, or NULL if it was not found.
     */
    const wreport::Var* find(wreport::Varcode code) const;

    /**
     * Find a variable given its varcode
     *
     * @param code
     *   The wreport::Varcode of the variable to query.  See @ref vartable.h
     * @return
     *   The variable found, or NULL if it was not found.
     */
    wreport::Var* edit(wreport::Varcode code);

    /**
     * Remove a variable given its varcode
     *
     * @param code
     *   The wreport::Varcode of the variable to query.  See @ref vartable.h
     * @return
     *   True if the variable was removed, false if it was not found.
     */
    bool remove(wreport::Varcode code);

    /** 
     * Find a variable given its shortcut ID
     *
     * @param id
     *   Shortcut ID of the value to set (see @ref vars.h)
     * @return
     *   The variable found, or NULL if it was not found.
     */
    const wreport::Var* find_by_id(int id) const;

    /**
     * If this context is the right context for a vertical sounding significance
     * and contains a vertical sounding significance variable, return it. Else,
     * return NULL.
     */
    const wreport::Var* find_vsig() const;

    /**
     * Dump all the contents of the context to the given stream
     *
     * @param out
     *   The stream to dump the contents of the level to.
     */
    void print(FILE* out) const;

    /**
     * Compute the differences between two contexts
     *
     * Details of the differences found will be formatted using the notes
     * system (@see notes.h).
     *
     * @param ctx
     *   Context to compare with this one
     * @returns
     *   The number of differences found
     */
    unsigned diff(const Context& ctx) const;
};

}
}

#endif
