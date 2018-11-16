#ifndef DBALLE_MSG_H
#define DBALLE_MSG_H

#include <dballe/message.h>
#include <dballe/var.h>
#include <dballe/core/fwd.h>
#include <dballe/msg/fwd.h>
#include <dballe/core/defs.h>
#include <dballe/core/matcher.h>
#include <dballe/msg/context.h>
#include <stdio.h>
#include <vector>
#include <memory>
#include <iosfwd>

namespace dballe {
struct CSVReader;
struct CSVWriter;

namespace impl {
// Compatibility/shortcut from old Messages implementation to new vector of shared_ptr
typedef std::vector<std::shared_ptr<dballe::Message>> Messages;

namespace msg {

/**
 * Read data from a CSV input.
 *
 * Reading stops when Report changes.
 */
Messages messages_from_csv(CSVReader& in);

/**
 * Output in CSV format
 */
void messages_to_csv(const Messages& msgs, CSVWriter& out);

/**
 * Compute the differences between two Messages
 *
 * Details of the differences found will be formatted using the wreport
 * notes system (@see wreport/notes.h).
 *
 * @returns
 *   The number of differences found
 */
unsigned messages_diff(const Messages& msgs1, const Messages& msgs2);

/// Print all the contents of all the messages to an output stream
void messages_print(const Messages& msgs, FILE* out);


class Contexts
{
public:
    typedef std::vector<msg::Context>::const_iterator const_iterator;
    typedef std::vector<msg::Context>::iterator iterator;
    typedef std::vector<msg::Context>::const_reverse_iterator const_reverse_iterator;
    typedef std::vector<msg::Context>::reverse_iterator reverse_iterator;

protected:
    std::vector<msg::Context> m_contexts;

    iterator insert_new(const Level& level, const Trange& trange);

public:
    Contexts() = default;
    Contexts(const Contexts&) = default;
    Contexts(Contexts&&) = default;
    Contexts& operator=(const Contexts&) = default;
    Contexts& operator=(Contexts&&) = default;

    const_iterator begin() const { return m_contexts.begin(); }
    const_iterator end() const { return m_contexts.end(); }
    iterator begin() { return m_contexts.begin(); }
    iterator end() { return m_contexts.end(); }
    const_reverse_iterator rbegin() const { return m_contexts.rbegin(); }
    const_reverse_iterator rend() const { return m_contexts.rend(); }
    const_iterator cbegin() const { return m_contexts.cbegin(); }
    const_iterator cend() const { return m_contexts.cend(); }

    const_iterator find(const Level& level, const Trange& trange) const;
    iterator find(const Level& level, const Trange& trange);

    iterator obtain(const Level& level, const Trange& trange);
    bool drop(const Level& level, const Trange& trange);

    size_t size() const { return m_contexts.size(); }
    bool empty() const { return m_contexts.empty(); }
    void clear() { return m_contexts.clear(); }
    void reserve(typename std::vector<Value>::size_type size) { m_contexts.reserve(size); }
    iterator erase(iterator pos) { return m_contexts.erase(pos); }
    iterator erase(const_iterator pos) { return m_contexts.erase(pos); }
};

}


/**
 * Storage for related physical data
 */
class Message : public dballe::Message
{
protected:
    /**
     * Return the index of the given context, or -1 if it was not found
     */
    int find_index(const Level& lev, const Trange& tr) const;

    const wreport::Var* get_impl(const Level& lev, const Trange& tr, wreport::Varcode code) const override;
    void set_impl(const Level& lev, const Trange& tr, std::unique_ptr<wreport::Var> var) override;

    void seti(const Level& lev, const Trange& tr, wreport::Varcode code, int val, int conf);
    void setd(const Level& lev, const Trange& tr, wreport::Varcode code, double val, int conf);
    void setc(const Level& lev, const Trange& tr, wreport::Varcode code, const char* val, int conf);

public:
    /// Source of the data
    MessageType type = MessageType::GENERIC;
    Values station_data;
    msg::Contexts data;

    Message() = default;
    Message(const Message&) = default;
    Message(Message&&) = default;
    Message& operator=(const Message& m) = default;
    Message& operator=(Message&& m) = default;

    /**
     * Return a reference to \a o downcasted as an impl::Message.
     *
     * Throws an exception if \a o is not an impl::Message.
     */
    static const Message& downcast(const dballe::Message& o);

    /**
     * Return a reference to \a o downcasted as an impl::Message.
     *
     * Throws an exception if \a o is not an impl::Message.
     */
    static Message& downcast(dballe::Message& o);

    /**
     * Returns a pointer to \a o downcasted as an impl::Message.
     *
     * Throws an exception if \a o is not an impl::Message.
     */
    static std::shared_ptr<Message> downcast(std::shared_ptr<dballe::Message> o);

    std::unique_ptr<dballe::Message> clone() const override;
    Datetime get_datetime() const override;
    Coords get_coords() const override;
    Ident get_ident() const override;
    std::string get_report() const override;
    MessageType get_type() const override { return type; }
    bool foreach_var(std::function<bool(const Level&, const Trange&, const wreport::Var&)>) const override;
    void print(FILE* out) const override;
    unsigned diff(const dballe::Message& msg) const override;

    /// Reset the messages as if it was just created
    void clear();

    using dballe::Message::get;
    using dballe::Message::set;

    /**
     * Find a datum given its shortcut
     *
     * @param shortcut
     *   Shortcut of the value to set.
     * @return
     *   The value found, or nullptr if it was not found.
     */
    const wreport::Var* get(const Shortcut& shortcut) const;

    /**
     * Add or replace a value
     *
     * @param shortcut
     *   Shortcut ID of the value to set
     * @param var
     *   The Var with the value to set
     */
    void set(const Shortcut& shortcut, const wreport::Var& var);

    /**
     * Shortcut to set year...second variables in a single call
     */
    void set_datetime(const Datetime& dt);

    /**
     * Remove a context from the message
     *
     * @return true if the context was removed, false if it did not exist
     */
    bool remove_context(const Level& lev, const Trange& tr);

    /**
     * Find a msg::Context given its description
     *
     * @param lev
     *   The Level to query
     * @param tr
     *   The Trange to query
     * @return
     *   The context found, or NULL if it was not found.
     */
    const msg::Context* find_context(const Level& lev, const Trange& tr) const;

    /**
     * Find the station info context
     *
     * @return
     *   The context found, or NULL if it was not found.
     */
    const Values& find_station_context() const;

    /**
     * Find a msg::Context given its description
     *
     * @param lev
     *   The Level to query
     * @param tr
     *   The Trange to query
     * @return
     *   The context found, or NULL if it was not found.
     */
    msg::Context* edit_context(const Level& lev, const Trange& tr);

    /**
     * Find a msg::Context given its description, creating it if it does not
     * exist
     *
     * @param lev
     *   The Level to query
     * @param tr
     *   The Trange to query
     * @return
     *   The context found
     */
    msg::Context& obtain_context(const Level& lev, const Trange& tr);

    /**
     * Find a variable given its description
     *
     * @param code
     *   The wreport::Varcode of the variable to query.
     * @param lev
     *   The Level to query
     * @param tr
     *   The Trange to query
     * @return
     *   The variable found, or NULL if it was not found.
     */
    wreport::Var* edit(wreport::Varcode code, const Level& lev, const Trange& tr);

#if 0
    /**
     * Remove a variable given its description
     *
     * @param code
     *   The wreport::Varcode of the variable to query.
     * @param lev
     *   The Level to query
     * @param tr
     *   The Trange to query
     * @returns
     *   True if the variable was removed, false if it was not found.
     */
    bool remove(wreport::Varcode code, const Level& lev, const Trange& tr);
#endif

    /**
     * Copy a Message, removing the sounding significance from the level
     * descriptions and packing together the data at the same pressure level.
     *
     * This is used to postprocess data after decoding, where the l2 field of the
     * level description is temporarily used to store the vertical sounding
     * significance, to simplify decoding.
     */
    void sounding_pack_levels(Message& dst) const;

    /**
     * Read data from a CSV input.
     *
     * Reading stops when one of Longitude, Latitude, Report or Date changes.
     *
     * @return true if some CSV data has been found, false on EOF
     */
    bool from_csv(CSVReader& in);

    /// Output in CSV format
    void to_csv(CSVWriter& out) const;

    std::unique_ptr<CursorStation> query_stations(const Query& query) const override;
    std::unique_ptr<CursorStationData> query_station_data(const Query& query) const override;
    std::unique_ptr<CursorData> query_data(const Query& query) const override;
    std::unique_ptr<CursorData> query_station_and_data(const Query& query) const;

    /// Output the CSV header
    static void csv_header(CSVWriter& out);

    /**
     * Get the message source type corresponding to the given report code
     */
    static MessageType type_from_repmemo(const char* repmemo);

    /**
     * Get the report code corresponding to the given message source type
     */
    static const char* repmemo_from_type(MessageType type);

#include <dballe/msg/msg-extravars.h>
};


/**
 * Match adapter for impl::Message
 */
struct MatchedMsg : public Matched
{
    const impl::Message& m;

    MatchedMsg(const impl::Message& r);
    ~MatchedMsg();

    matcher::Result match_var_id(int val) const override;
    matcher::Result match_station_id(int val) const override;
    matcher::Result match_station_wmo(int block, int station=-1) const override;
    matcher::Result match_datetime(const DatetimeRange& range) const override;
    matcher::Result match_coords(const LatRange& latrange, const LonRange& lonrange) const override;
    matcher::Result match_rep_memo(const char* memo) const override;
};


/**
 * Match adapter for Messages
 */
struct MatchedMessages : public Matched
{
    const std::vector<std::shared_ptr<dballe::Message>>& m;

    MatchedMessages(const std::vector<std::shared_ptr<dballe::Message>>& m);
    ~MatchedMessages();

    matcher::Result match_var_id(int val) const override;
    matcher::Result match_station_id(int val) const override;
    matcher::Result match_station_wmo(int block, int station=-1) const override;
    matcher::Result match_datetime(const DatetimeRange& range) const override;
    matcher::Result match_coords(const LatRange& latrange, const LonRange& lonrange) const override;
    matcher::Result match_rep_memo(const char* memo) const override;
};

}
}
#endif
