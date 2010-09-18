/*
 * dballe/wr_exporters/base - Base infrastructure for wreport exporters
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

#ifndef DBALLE_MSG_WREXPORTER_BASE_H
#define DBALLE_MSG_WREXPORTER_BASE_H

#include <dballe/msg/wr_codec.h>
#include <dballe/msg/msg.h>
#include <stdint.h>
#include <map>
#include <string>

namespace wreport {
struct Subset;
struct Bulletin;
struct Var;
}

namespace dballe {
namespace msg {
namespace wr {

struct TemplateRegistry;

class Template
{
protected:
    virtual void setupBulletin(wreport::Bulletin& bulletin);
    virtual void to_subset(const Msg& msg, wreport::Subset& subset);

public:
    const Exporter::Options& opts;
    const Msgs& msgs;
    const Msg* msg;     // Msg being read
    wreport::Subset* subset; // Subset being written

    Template(const Exporter::Options& opts, const Msgs& msgs)
        : opts(opts), msgs(msgs), msg(0), subset(0) {}
    virtual ~Template() {}

    virtual const char* name() const = 0;
    virtual const char* description() const = 0;
    virtual void to_bulletin(wreport::Bulletin& bulletin);
};

typedef std::auto_ptr<Template> (*TemplateFactory)(const Exporter::Options& opts, const Msgs& msgs);

struct TemplateRegistry : public std::map<std::string, TemplateFactory>
{
    static const TemplateRegistry& get();
    static TemplateFactory get(const std::string& name);
};

} // namespace wr
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
