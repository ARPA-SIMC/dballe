#ifndef DBA_MSG_CONTEXT_H
#define DBA_MSG_CONTEXT_H

/** @file
 * @ingroup msg
 *
 * Sorted storage for all the dba_msg_datum present on one level.
 */

#include <dballe/var.h>
#include <dballe/types.h>
#include <dballe/values.h>
#include <vector>
#include <memory>

namespace dballe {
namespace impl {
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
    Values values;

    Context(const Level& lev, const Trange& tr);
    Context(const Context& c) = default;
    Context(Context&& c) = default;

    Context& operator=(const Context& src) = default;
    Context& operator=(Context&& src) = default;

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
     *   -1 if l < lev, tr; 0 if l == lev, tr; 1 if l > lev, tr
     */
    int compare(const Level& lev, const Trange& tr) const;

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
}

#endif
