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

struct lua_State;

namespace dballe {
struct Record;
struct CSVReader;
struct CSVWriter;

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
 * Return a string with the name of a dba_msg_type
 *
 * @param type
 *   The dba_msg_type value to name
 * @return
 *   The name, as a const string.  This function is thread safe.
 */
const char* msg_type_name(MsgType type);

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

    /// Sensor network of origin of the Msg contents
    std::string m_rep_memo;
    /// Reference coordinates for the Msg contents
    Coords m_coords;
    /// Identifier of the contents originator
    Ident m_ident;
    /// Reference time for the Msg contents
    Datetime m_datetime;

public:
    /// Source of the data
    MsgType type;

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
    Datetime get_datetime() const override { return m_datetime; }

    const wreport::Var* get(wreport::Varcode code, const Level& lev, const Trange& tr) const override;

    void print(FILE* out) const override;
    unsigned diff(const Message& msg) const override;

    void set_rep_memo(const std::string& r) { m_rep_memo = r; }
    void set_coords(const Coords& c) { m_coords = c; }
    void set_ident(const Ident& i) { m_ident = i; }
    void set_datetime(const Datetime& dt) { m_datetime = dt; }

    /// Remove all information from Msg
    void clear();

    /**
     * Add a missing context, taking care of its memory management
     *
     * Note: if the context already exists, an exception is thrown
     */
    void add_context(std::unique_ptr<msg::Context>&& ctx);

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
     * Find a contexts given level and timerange found in a shortcut ID
     *
     * @param id
     *   Shortcut ID with the level information to use
     * @return
     *   The context found, or NULL if it was not found.
     */
    const msg::Context* find_context_by_id(int id) const;

    /** 
     * Find a datum given its shortcut ID
     *
     * @param id
     *   Shortcut ID of the value to set.
     * @return
     *   The value found, or NULL if it was not found.
     */
    wreport::Var* edit_by_id(int id);

    /**
     * Add or replace a value
     *
     * @param var
     *   The Var with the value to set
     * @param code
     *   The dba_varcode of the destination value.  If it is different than the
     *   varcode of var, a conversion will be attempted.
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     */
    void set(const wreport::Var& var, wreport::Varcode code, const Level& lev, const Trange& tr);

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
     * Add or replace a value, taking ownership of the source variable without
     * copying it.
     *
     * @param var
     *   The Var with the value to set.  This Msg will take ownership of memory
     *   management.
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     */
    void set(std::unique_ptr<wreport::Var>&& var, const Level& lev, const Trange& tr);

    /**
     * Add or replace an integer value in the dba_msg
     *
     * @param code
     *   The dba_varcode of the destination value..  See @ref vartable.h
     * @param val
     *   The integer value of the data
     * @param conf
     *   The confidence interval of the data, as the value of a B33007 WMO B (per
     *   cent confidence) table entry, that is, a number between 0 and 100
     *   inclusive.  -1 means no confidence interval attribute.
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     */
    void seti(wreport::Varcode code, int val, int conf, const Level& lev, const Trange& tr);

    /**
     * Add or replace a double value in the dba_msg
     *
     * @param code
     *   The dba_varcode of the destination value.  See @ref vartable.h
     * @param val
     *   The double value of the data
     * @param conf
     *   The confidence interval of the data, as the value of a B33007 WMO B (per
     *   cent confidence) table entry, that is, a number between 0 and 100
     *   inclusive.  -1 means no confidence interval attribute.
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     */
    void setd(wreport::Varcode code, double val, int conf, const Level& lev, const Trange& tr);

    /**
     * Add or replace a string value in the dba_msg
     *
     * @param code
     *   The dba_varcode of the destination value.  See @ref vartable.h
     * @param val
     *   The string value of the data
     * @param conf
     *   The confidence interval of the data, as the value of a B33007 WMO B (per
     *   cent confidence) table entry, that is, a number between 0 and 100
     *   inclusive.  -1 means no confidence interval attribute.
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     */
    void setc(wreport::Varcode code, const char* val, int conf, const Level& lev, const Trange& tr);

    /**
     * Copy to dest all the variable in this message that match \a filter
     * TODO: to be implemented
     */
    //void filter(const Record& filter, Msg& dest) const;

    /**
     * Copy a Msg, removing the sounding significance from the level
     * descriptions and packing together the data at the same pressure level.
     *
     * This is used to postprocess data after decoding, where the l2 field of the
     * level description is temporarily used to store the vertical sounding
     * significance, to simplify decoding.
     */
    void sounding_pack_levels(Msg& dst) const;

#if 0
    /**
     * Copy a Msg, adding the sounding significance from the level descriptions
     * and moving the data at the same pressure level to the resulting
     * pseudolevels.
     *
     * This is used to preprocess data before encoding, where the l2 field of the
     * level description is temporarily used to store the vertical sounding
     * significance, to simplify encoding.
     */
    void sounding_unpack_levels(Msg& dst) const;
#endif

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
    static MsgType type_from_repmemo(const char* repmemo);

    /**
     * Get the report code corresponding to the given message source type
     */
    static const char* repmemo_from_type(MsgType type);

#include <dballe/msg/msg-extravars.h>


    /**
     * Push the variable as an object in the lua stack
     */
    void lua_push(struct lua_State* L);

    /**
     * Check that the element at \a idx is a dba_msg
     *
     * @return the dba_msg element, or NULL if the check failed
     */
    static Msg* lua_check(struct lua_State* L, int idx);
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
