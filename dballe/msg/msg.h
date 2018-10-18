#ifndef DBA_MSG_H
#define DBA_MSG_H

#include <dballe/message.h>
#include <dballe/var.h>
#include <dballe/core/defs.h>
#include <dballe/core/matcher.h>
#include <dballe/msg/fwd.h>
#include <dballe/msg/vars.h>
#include <stdio.h>
#include <vector>
#include <memory>
#include <iosfwd>

namespace dballe {
struct Record;
struct CSVReader;
struct CSVWriter;

// Compatibility/shortcut from old Messages implementation to new vector of shared_ptr
typedef std::vector<std::shared_ptr<Message>> Messages;

namespace msg {
struct Context;

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

}

/**
 * Storage for related physical data
 */
class Msg : public Message
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

    /**
     * Add a missing context, taking care of its memory management
     *
     * Note: if the context already exists, an exception is thrown
     */
    void add_context(std::unique_ptr<msg::Context>&& ctx);

public:
    /// Source of the data
    MessageType type;

    /** Context in the message */
    std::vector<msg::Context*> data;

    /**
     * Create a new dba_msg
     *
     * By default, type is MSG_GENERIC
     */
    Msg();
    ~Msg();

    Msg(const Msg& m);
    Msg& operator=(const Msg& m);

    /**
     * Return a reference to \a o downcasted as a Msg.
     *
     * Throws an exception if \a o is not a Msg.
     */
    static const Msg& downcast(const Message& o);

    /**
     * Return a reference to \a o downcasted as a Msg.
     *
     * Throws an exception if \a o is not a Msg.
     */
    static Msg& downcast(Message& o);

    /**
     * Returns a pointer to \a o downcasted as a Msg.
     *
     * Throws an exception if \a o is not a Msg.
     */
    static std::shared_ptr<Msg> downcast(std::shared_ptr<Message> o);

    std::unique_ptr<Message> clone() const override;
    Datetime get_datetime() const override;
    Coords get_coords() const override;
    Ident get_ident() const override;
    std::string get_network() const override;
    MessageType get_type() const override { return type; }
    bool foreach_var(std::function<bool(const Level&, const Trange&, const wreport::Var&)>) const override;
    void print(FILE* out) const override;
    unsigned diff(const Message& msg) const override;

    /// Remove all information from Msg
    void clear();

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
    const msg::Context* find_station_context() const;

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

    /// Shortcut to obtain_context(Level(), Trange());
    msg::Context& obtain_station_context();

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

    /** 
     * Find a datum given its shortcut ID
     *
     * @param id
     *   Shortcut ID of the value to set.
     * @return
     *   The value found, or NULL if it was not found.
     */
    const wreport::Var* find_by_id(int id) const;

    /**
     * Add or replace a value
     *
     * @param var
     *   The Var with the value to set
     * @param shortcut
     *   Shortcut ID of the value to set
     */
    void set_by_id(const wreport::Var& var, int shortcut);

    /**
     * Copy a Msg, removing the sounding significance from the level
     * descriptions and packing together the data at the same pressure level.
     *
     * This is used to postprocess data after decoding, where the l2 field of the
     * level description is temporarily used to store the vertical sounding
     * significance, to simplify decoding.
     */
    void sounding_pack_levels(Msg& dst) const;

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
 * Match adapter for Msg
 */
struct MatchedMsg : public Matched
{
    const Msg& m;

    MatchedMsg(const Msg& r);
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
    const Messages& m;

    MatchedMessages(const Messages& m);
    ~MatchedMessages();

    matcher::Result match_var_id(int val) const override;
    matcher::Result match_station_id(int val) const override;
    matcher::Result match_station_wmo(int block, int station=-1) const override;
    matcher::Result match_datetime(const DatetimeRange& range) const override;
    matcher::Result match_coords(const LatRange& latrange, const LonRange& lonrange) const override;
    matcher::Result match_rep_memo(const char* memo) const override;
};

}
#endif
