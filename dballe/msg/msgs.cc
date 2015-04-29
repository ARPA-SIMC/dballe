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

#include "msgs.h"
#include <dballe/core/csv.h>
#include <wreport/error.h>
#include <wreport/notes.h>

#include <stdlib.h>
#include <stdio.h>

using namespace std;
using namespace wreport;

namespace dballe {

Msgs::Msgs()
{
}

Msgs::Msgs(const Msgs& msgs)
{
    reserve(msgs.size());
    for (const_iterator i = msgs.begin(); i != msgs.end(); ++i)
        push_back(new Msg(**i));
}

Msgs::~Msgs()
{
	for (iterator i = begin(); i != end(); ++i)
        delete *i;
}

Msgs& Msgs::operator=(const Msgs& msgs)
{
    if (this != &msgs)
    {
        clear();
        reserve(msgs.size());
        for (const_iterator i = msgs.begin(); i != msgs.end(); ++i)
            push_back(new Msg(**i));
    }
    return *this;
}

void Msgs::clear()
{
    for (iterator i = begin(); i != end(); ++i)
        delete *i;
    vector<Msg*>::clear();
}

void Msgs::acquire(const Msg& msg)
{
    push_back(new Msg(msg));
}

void Msgs::acquire(unique_ptr<Msg> msg)
{
    push_back(msg.release());
}

bool Msgs::from_csv(CSVReader& in)
{
    string old_rep;
    bool first = true;
    while (true)
    {
        // Seek to beginning, skipping empty lines
        if (!in.move_to_data())
            return !first;

        if (in.cols.size() != 13)
            error_consistency::throwf("cannot parse CSV line has %zd fields instead of 13", in.cols.size());
        if (first)
        {
            // If we are the first run, initialse old_* markers with the contents of this line
            old_rep = in.cols[2];
            first = false;
        } else if (old_rep != in.cols[2])
            // If Report changes, we are done
            break;

        unique_ptr<Msg> msg(new Msg);
        bool has_next = msg->from_csv(in);
        acquire(std::move(msg));
        if (!has_next)
            break;
    }
    return true;
}

void Msgs::to_csv(std::ostream& out) const
{
    for (const_iterator i = begin(); i != end(); ++i)
        (*i)->to_csv(out);
}

void Msgs::print(FILE* out) const
{
	for (unsigned i = 0; i < size(); ++i)
	{
		fprintf(out, "Subset %d:\n", i);
		(*this)[i]->print(out);
	}
}

unsigned Msgs::diff(const Msgs& msgs) const
{
    unsigned diffs = 0;
    if (size() != msgs.size())
    {
        notes::logf("the message groups contain a different number of messages (first is %zd, second is %zd)\n",
                size(), msgs.size());
        ++diffs;
    }
    unsigned count = size() < msgs.size() ? size() : msgs.size();
    for (unsigned i = 0; i < count; ++i)
        diffs += (*this)[i]->diff(*msgs[i]);
    return diffs;
}


MatchedMsgs::MatchedMsgs(const Msgs& m)
    : m(m)
{
}
MatchedMsgs::~MatchedMsgs()
{
}

matcher::Result MatchedMsgs::match_var_id(int val) const
{
    for (Msgs::const_iterator i = m.begin(); i != m.end(); ++i)
        if (MatchedMsg(**i).match_var_id(val) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsgs::match_station_id(int val) const
{
    for (Msgs::const_iterator i = m.begin(); i != m.end(); ++i)
        if (MatchedMsg(**i).match_station_id(val) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsgs::match_station_wmo(int block, int station) const
{
    for (Msgs::const_iterator i = m.begin(); i != m.end(); ++i)
        if (MatchedMsg(**i).match_station_wmo(block, station) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsgs::match_date(const Datetime& min, const Datetime& max) const
{
    for (Msgs::const_iterator i = m.begin(); i != m.end(); ++i)
        if (MatchedMsg(**i).match_date(min, max) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsgs::match_coords(const Coords& min, const Coords& max) const
{
    for (Msgs::const_iterator i = m.begin(); i != m.end(); ++i)
        if (MatchedMsg(**i).match_coords(min, max) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsgs::match_rep_memo(const char* memo) const
{
    for (Msgs::const_iterator i = m.begin(); i != m.end(); ++i)
        if (MatchedMsg(**i).match_rep_memo(memo) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

}
