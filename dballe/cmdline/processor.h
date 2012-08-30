/*
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

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include <dballe/core/rawmsg.h>
#include <dballe/core/record.h>
#include <dballe/msg/codec.h>
#include <list>
#include <string>

namespace wreport {
struct Bulletin;
}

namespace dballe {
struct Rawmsg;
struct Msgs;
struct Matcher;

namespace cmdline {

struct Item
{
    unsigned idx;
    Rawmsg* rmsg;
    wreport::Bulletin* bulletin;
    Msgs* msgs;

    Item();
    ~Item();

    /// Decode all that can be decoded
    void decode(msg::Importer& imp, bool print_errors=false);

    /// Set the value of msgs, possibly replacing the previous one
    void set_msgs(Msgs* new_msgs);
};

struct Action
{
    virtual ~Action() {}
    virtual void operator()(const Item& item) = 0;
};

struct Filter
{
    msg::Exporter::Options export_opts;
    int category;
    int subcategory;
    int checkdigit;
    int unparsable;
    int parsable;
    const char* index;
    Matcher* matcher;

    Filter();
    ~Filter();

    /// Reset to the empty matcher
    void matcher_reset();

    /// Initialise the matcher from a record
    void matcher_from_record(const Record& query);

    bool match_index(int idx) const;
    bool match_common(const Rawmsg& rmsg, const Msgs* msgs) const;
    bool match_msgs(const Msgs& msgs) const;
    bool match_bufrex(const Rawmsg& rmsg, const wreport::Bulletin* rm, const Msgs* msgs) const;
    bool match_bufr(const Rawmsg& rmsg, const wreport::Bulletin* rm, const Msgs* msgs) const;
    bool match_crex(const Rawmsg& rmsg, const wreport::Bulletin* rm, const Msgs* msgs) const;
    bool match_aof(const Rawmsg& rmsg, const Msgs* msgs) const;
    bool match_item(const Item& item) const;
};

class Reader
{
protected:
    void read_csv(const std::list<std::string>& fnames, Action& action);
    void read_file(const std::list<std::string>& fnames, Action& action);

public:
    const char* input_type;
    msg::Importer::Options import_opts;
    Filter filter;
    bool verbose;

    Reader();

    void read(const std::list<std::string>& fnames, Action& action);
};

} // namespace cmdline
} // namespace dballe

#endif
