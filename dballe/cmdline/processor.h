/*
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

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <dballe/core/rawmsg.h>
#include <popt.h>

namespace wreport {
struct Bulletin;
}

namespace dballe {
struct Rawmsg;
struct Msgs;
struct Matcher;

namespace cmdline {

struct grep_t
{
    int category;
    int subcategory;
    int checkdigit;
    int unparsable;
    int parsable;
    const char* index;
    Matcher* matcher;

    grep_t();
    ~grep_t();

    /// Initialise the matcher eating key=val arguments
    void matcher_from_args(poptContext optCon);

    bool match_index(int idx) const;
    bool match_common(const Rawmsg& rmsg, const Msgs* msgs) const;
    bool match_bufrex(const Rawmsg& rmsg, const wreport::Bulletin* rm, const Msgs* msgs) const;
    bool match_bufr(const Rawmsg& rmsg, const wreport::Bulletin* rm, const Msgs* msgs) const;
    bool match_crex(const Rawmsg& rmsg, const wreport::Bulletin* rm, const Msgs* msgs) const;
    bool match_aof(const Rawmsg& rmsg, const Msgs* msgs) const;
};

struct Action
{
	virtual ~Action() {}
	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* bulletin, const Msgs* msgs) = 0;
};

void process_all(poptContext optCon,
		 Encoding type,
		 struct grep_t* grepdata,
		 Action& action);

} // namespace cmdline
} // namespace dballe

#endif
