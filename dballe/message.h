#ifndef DBALLE_MESSAGE_H
#define DBALLE_MESSAGE_H

#include <dballe/types.h>
#include <wreport/varinfo.h>
#include <vector>
#include <memory>
#include <iterator>

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

/**
 * Ordered collection of messages.
 *
 * This supports many encode/decode operations, that work on group of similar
 * Messages.
 */
class Messages
{
protected:
    std::vector<Message*> msgs;

    // Convert an iterator over pointers to an iterator over values
    template<typename VAL, typename WRAPPED>
    class base_iterator : public std::iterator<std::random_access_iterator_tag, VAL, typename WRAPPED::difference_type, VAL*, VAL&>
    {
    protected:
        WRAPPED current;

    public:
        base_iterator() {}
        explicit base_iterator(const WRAPPED& o) : current(o) {}

        // Forward iterator requirements
        inline VAL& operator*() const { return **current; }
        inline VAL* operator->() const { return *current; }
        inline base_iterator& operator++() { ++current; return *this; }
        inline base_iterator operator++(int) { return base_iterator(current++); }

        // Bidirectional iterator requirements
        inline base_iterator& operator--() { --current; return *this; }
        inline base_iterator operator--(int) { return base_iterator(current--); }

        // Random access iterator requirements
        inline VAL& operator[](typename WRAPPED::difference_type n) const { return *current[n]; }
        inline base_iterator& operator+=(typename WRAPPED::difference_type n) { current += n; return *this; }
        inline base_iterator operator+(typename WRAPPED::difference_type n) const { return base_iterator(current + n); }
        inline base_iterator& operator-=(typename WRAPPED::difference_type n) { current -= n; return *this; }
        inline base_iterator operator-(typename WRAPPED::difference_type n) const { return base_iterator(current - n); }

        // Forward iterator requirements
        template<typename O> inline bool operator==(const O& o) const { return current == o.current; }
        template<typename O> inline bool operator!=(const O& o) const { return current != o.current; }
        template<typename O> inline bool operator<(const O& o) const { return current < o.current; }
        template<typename O> inline bool operator<=(const O& o) const { return current <= o.current; }
        template<typename O> inline bool operator>(const O& o) const { return current > o.current; }
        template<typename O> inline bool operator>=(const O& o) const { return current >= o.current; }
        template<typename O> inline typename WRAPPED::difference_type operator-(const O& o) const { return current - o.current; }
    };

public:
    typedef base_iterator<Message, std::vector<Message*>::iterator> iterator;
    typedef base_iterator<const Message, std::vector<Message*>::const_iterator> const_iterator;

    Messages();
    Messages(const Messages& o);
    Messages(Messages&& o);
    ~Messages();

    Messages& operator=(const Messages& o);
    Messages& operator=(Messages&& o);

    iterator begin() { return iterator(msgs.begin()); }
    iterator end() { return iterator(msgs.end()); }
    const_iterator begin() const { return const_iterator(msgs.begin()); }
    const_iterator end() const { return const_iterator(msgs.end()); }

    Message& operator[](size_t pos) { return *msgs[pos]; }
    const Message& operator[](size_t pos) const { return *msgs[pos]; }

    /// Check if the collection is empty
    bool empty() const;

    /// Return the number of messages
    size_t size() const;

    /// Append a copy of the message
    void append(const Message& msg);

    /// Append a message, taking over its memory management.
    void append(std::unique_ptr<Message>&& msg);

    /// Remove all messages
    void clear();

    /// Print all the contents of all the messages to an output stream
    void print(FILE* out) const;

    /**
     * Compute the differences between two Messages
     *
     * Details of the differences found will be formatted using the wreport
     * notes system (@see wreport/notes.h).
     *
     * @param msgs
     *   Messages to compare to
     * @returns
     *   The number of differences found
     */
    unsigned diff(const Messages& msgs) const;
};

}
#endif
