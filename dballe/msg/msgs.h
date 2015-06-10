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
    Msgs(const Msgs& msgs);
    ~Msgs();

    Msgs& operator=(const Msgs& msgs);

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
    void acquire(std::unique_ptr<Msg> msg);

    /**
     * Remove all messages
     */
    void clear();

    /**
     * Read data from a CSV input.
     *
     * Reading stops when Report changes.
     *
     * @return true if some CSV data has been read, false on EOF
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
     * Compute the differences between two Msgs
     *
     * Details of the differences found will be formatted using the notes
     * system (@see notes.h).
     *
     * @param msgs
     *   Msgs to compare to
     * @returns
     *   The number of differences found
     */
    unsigned diff(const Msgs& msgs) const;
};

namespace msg {
struct AcquireMessages : public MsgConsumer
{
    Msgs& out;

    AcquireMessages(Msgs& out) : out(out) {}

    void operator()(std::unique_ptr<Msg> msg)
    {
        out.acquire(std::move(msg));
    }
};
}

/**
 * Match adapter for Msgs
 */
struct MatchedMsgs : public Matched
{
    const Msgs& m;

    MatchedMsgs(const Msgs& m);
    ~MatchedMsgs();

    matcher::Result match_var_id(int val) const override;
    matcher::Result match_station_id(int val) const override;
    matcher::Result match_station_wmo(int block, int station=-1) const override;
    matcher::Result match_date(const Datetime& min, const Datetime& max) const override;
    matcher::Result match_coords(const LatRange& latrange, const LonRange& lonrange) const override;
    matcher::Result match_rep_memo(const char* memo) const override;
};

}

/* vim:set ts=4 sw=4: */
#endif
