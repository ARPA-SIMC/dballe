/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <stdlib.h>
#include <stdio.h>

using namespace std;

namespace dballe {

Msgs::Msgs()
{
}

Msgs::~Msgs()
{
	for (iterator i = begin(); i != end(); ++i)
        delete *i;
}

void Msgs::acquire(const Msg& msg)
{
    push_back(new Msg(msg));
}

void Msgs::acquire(auto_ptr<Msg> msg)
{
    push_back(msg.release());
}

void Msgs::print(FILE* out) const
{
	for (unsigned i = 0; i < size(); ++i)
	{
		fprintf(out, "Subset %d:\n", i);
		(*this)[i]->print(out);
	}
}

unsigned Msgs::diff(const Msgs& msgs, FILE* out) const
{
    unsigned diffs = 0;
	if (size() != msgs.size())
	{
		fprintf(out, "the message groups contain a different number of messages (first is %zd, second is %zd)\n",
				size(), msgs.size());
		++diffs;
	}
	unsigned count = size() < msgs.size() ? size() : msgs.size();
	for (unsigned i = 0; i < count; ++i)
		diffs += (*this)[i]->diff(*msgs[i], out);
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

matcher::Result MatchedMsgs::match_date(const int* min, const int* max) const
{
    for (Msgs::const_iterator i = m.begin(); i != m.end(); ++i)
        if (MatchedMsg(**i).match_date(min, max) == matcher::MATCH_YES)
            return matcher::MATCH_YES;
    return matcher::MATCH_NA;
}

matcher::Result MatchedMsgs::match_coords(int latmin, int latmax, int lonmin, int lonmax) const
{
    for (Msgs::const_iterator i = m.begin(); i != m.end(); ++i)
        if (MatchedMsg(**i).match_coords(latmin, latmax, lonmin, lonmax) == matcher::MATCH_YES)
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

} // namespace dballe

/* vim:set ts=4 sw=4: */
