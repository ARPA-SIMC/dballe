/*
 * dballe/msg - Hold an interpreted weather bulletin
 *
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#ifndef DBA_MSG_H
#define DBA_MSG_H

/** @file
 * @ingroup msg
 *
 * Abstraction for a weather report message which is independent from the
 * encoding, used to make sense of decoded information and to carry data between
 * the various import and export modules of DB-ALLe.
 * 
 * The internal representation is as connected as possible to physics rather than
 * to observations.  dba_msg is a container for related weather information,
 * stored in a nonambiguous way.
 *
 * To understand what is the difference betwee ::dba_msg and other ways of
 * representing weather data, it is important to keep in mind how a value is
 * usually defined in the various encodings:
 * 
 * \li by previous values and position, as in the AOF encoding
 * \li by previous values, data descriptor and sometimes position, as in the BUFR
 *     encoding
 * \li by measure type and physical coordinates, as in ::dba_msg and the DB-ALLe
 *     database
 * 
 * ::dba_msg contains values as tuples (variable, level layer, time range).
 * 
 * The variable is represented by a dba_var.  The dba_varcode of the dba_var
 * refers to a local B table which lists physical measurements unambiguously.
 * 
 * Level layer is a triple (level type, l1, l2) and time range is a triple
 * (pindicator, p1, p2).  The values of these two triples follow what is used in
 * the GRIB encoding plus some local extensions, and an explanation of their
 * possible value is found in the document "DB-ALLe Guide of the Fortran API".
 * 
 * Importers and exporters have to implement a mapping between their
 * representation and the unambiguous physical representation.  Luckily this is
 * necessarily possible, because the ultimate purpose of the various message
 * encodings is to correctly transmit those physical data.
 *
 * Since to work with the full physical coordinates one needs to specify a lot of
 * different parameters in order to identify a value (BLocal value, level layer,
 * time range), there is a var.h module available with shortcut functions to
 * the values that are used more commonly.
 */

#include <dballe/core/var.h>
#include <dballe/core/defs.h>
#include <dballe/core/matcher.h>
#include <dballe/msg/vars.h>
#include <stdio.h>
#include <vector>
#include <memory>
#include <iosfwd>

struct lua_State;

namespace dballe {

struct Record;
struct CSVReader;

namespace msg {
struct Context;
}

/**
 * Source of the data
 */
enum MsgType {
    MSG_GENERIC,    /**< Data from unspecified source */
    MSG_SYNOP,      /**< Synop measured data */
    MSG_PILOT,      /**< Pilot sounding data */
    MSG_TEMP,       /**< Temp sounding data */
    MSG_TEMP_SHIP,  /**< Temp ship sounding data */
    MSG_AIREP,      /**< Airep airplane data */
    MSG_AMDAR,      /**< Amdar airplane data */
    MSG_ACARS,      /**< Acars airplane data */
    MSG_SHIP,       /**< Ship measured data */
    MSG_BUOY,       /**< Buoy measured data */
    MSG_METAR,      /**< Metar data */
    MSG_SAT,        /**< Satellite data */
    MSG_POLLUTION   /**< Pollution data */
};

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
class Msg
{
protected:
    /**
     * Return the index of the given context, or -1 if it was not found
     */
    int find_index(const Level& lev, const Trange& tr) const;

public:
    /** Source of the data */
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

    /// Remove all information from Msg
    void clear();

    /**
     * Add a missing context, taking care of its memory management
     *
     * Note: if the context already exists, an exception is thrown
     */
    void add_context(std::unique_ptr<msg::Context> ctx);

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

    /// Shortcut to obtain_context(Level::ana(), Trange::ana());
    msg::Context& obtain_station_context();

    /**
     * Find a variable given its description
     *
     * @param code
     *   The ::dba_varcode of the variable to query. See @ref vartable.h
     * @param lev
     *   The Level to query
     * @param tr
     *   The Trange to query
     * @return
     *   The variable found, or NULL if it was not found.
     */
    const wreport::Var* find(wreport::Varcode code, const Level& lev, const Trange& tr) const;

    /**
     * Find a variable given its description
     *
     * @param code
     *   The ::dba_varcode of the variable to query. See @ref vartable.h
     * @param lev
     *   The Level to query
     * @param tr
     *   The Trange to query
     * @return
     *   The variable found, or NULL if it was not found.
     */
    wreport::Var* edit(wreport::Varcode code, const Level& lev, const Trange& tr);

    /** 
     * Find a datum given its shortcut ID
     *
     * @param msg
     *   The message to query
     * @param id
     *   Shortcut ID of the value to set (see @ref vars.h)
     * @return
     *   The value found, or NULL if it was not found.
     */
    const wreport::Var* find_by_id(int id) const;

    /** 
     * Find a contexts given level and timerange found in a shortcut ID
     *
     * @param msg
     *   The message to query
     * @param id
     *   Shortcut ID with the level information to use
     * @return
     *   The context found, or NULL if it was not found.
     */
    const msg::Context* find_context_by_id(int id) const;

    /** 
     * Find a datum given its shortcut ID
     *
     * @param msg
     *   The message to query
     * @param id
     *   Shortcut ID of the value to set (see @ref vars.h)
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
     * @param msg
     *   The Var with the value to set.  This Msg will take ownership of memory
     *   management.
     * @param lev
     *   The Level of the value
     * @param tr
     *   The Trange of the value
     */
    void set(std::unique_ptr<wreport::Var> var, const Level& lev, const Trange& tr);

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

    /// Set the date from a string in the format "YYYY-MM-DD HH:MM:SS"
    void set_date(const char* date);

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
     * Parse the date set in the Msg.
     *
     * This function will examine the values year, month, day, hour, min and
     * sec, and will compute the lower bound of the datetime they represent.
     *
     * @retval values
     *   An array of 6 integers that will be filled with the minimum year, month,
     *   day, hour, minute and seconds.
     */
    Datetime get_datetime() const;

    /**
     * Read data from a CSV input.
     *
     * Reading stops when one of Longitude, Latitude, Report or Date changes.
     *
     * @return true if some CSV data has been found, false on EOF
     */
    bool from_csv(CSVReader& in);

    /**
     * Output in CSV format
     */
    void to_csv(std::ostream& out) const;

    /// Output the CSV header
    static void csv_header(std::ostream& out);

    /**
     * Dump all the contents of the message to the given stream
     *
     * @param out
     *   The stream to dump the contents of the dba_msg to.
     */
    void print(FILE* out) const;

    /**
     * Compute the differences between two Msg
     *
     * Details of the differences found will be formatted using the notes
     * system (@see notes.h).
     *
     * @param msg
     *   Message to compare this one to
     * @returns
     *   The number of differences found
     */
    unsigned diff(const Msg& msg) const;

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
 * Consumer interface used to stream messages as they are produced
 */
struct MsgConsumer
{
    virtual ~MsgConsumer() {}
    virtual void operator()(std::unique_ptr<Msg>) = 0;
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
    matcher::Result match_date(const Datetime& min, const Datetime& max) const override;
    matcher::Result match_coords(const Coords& min, const Coords& max) const override;
    matcher::Result match_rep_memo(const char* memo) const override;
};

}

#endif
