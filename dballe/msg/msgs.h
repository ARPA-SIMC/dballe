/*
 * msg/msgs - Hold a group of similar Msg
 *
 * Copyright (C) 2005--2011  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_MSG_MSGS_H
#define DBA_MSG_MSGS_H

/** @file
 * @ingroup msg
 * Dynamic array of dba_msg
 */

#include <dballe/msg/msg.h>
#include <vector>
#include <memory>

namespace dballe {
struct CSVReader;

/** Dynamic array of dba_msg */
struct Msgs : public std::vector<Msg*>
{
    Msgs();
    ~Msgs();

    /**
     * Append a copy of the message to the array.
     *
     * @param msg
     *   The message to append.
     */
    void acquire(const Msg& msg);

    /**
     * Append a message to the array, taking over its memory management.
     *
     * @param msg
     *   The message to append.  The Msgs will take over memory
     *   management for it.
     */
    void acquire(std::auto_ptr<Msg> msg);

    /**
     * Read data from a CSV input.
     *
     * Reading stops when Report changes.
     *
     * @return true if there is more data to read in \a in, false on EOF
     */
    bool from_csv(CSVReader& in);

    /**
     * Output in CSV format
     */
    void to_csv(std::ostream& out) const;

    /**
     * Dump all the contents of the message to the given stream
     *
     * @param out
     *   The stream to dump the contents of the dba_msg to.
     */
    void print(FILE* out) const;

    /**
     * Print the differences between two dba_msgs to a stream
     *
     * @param msgs
     *   Msgs to compare to
     * @param out
     *   The stream to dump a description of the differences to.
     * @returns
     *   The number of differences found.
     */
    unsigned diff(const Msgs& msgs, FILE* out) const;
};

/**
 * Match adapter for Msgs
 */
struct MatchedMsgs : public Matched
{
    const Msgs& m;

    MatchedMsgs(const Msgs& m);
    ~MatchedMsgs();

    virtual matcher::Result match_var_id(int val) const;
    virtual matcher::Result match_station_id(int val) const;
    virtual matcher::Result match_station_wmo(int block, int station=-1) const;
    virtual matcher::Result match_date(const int* min, const int* max) const;
    virtual matcher::Result match_coords(int latmin, int latmax, int lonmin, int lonmax) const;
    virtual matcher::Result match_rep_memo(const char* memo) const;
};

}

/* vim:set ts=4 sw=4: */
#endif
