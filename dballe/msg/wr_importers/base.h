/*
 * dballe/wr_importers/base - Base infrastructure for wreport importers
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

#ifndef DBALLE_MSG_WRIMPORTER_BASE_H
#define DBALLE_MSG_WRIMPORTER_BASE_H

#include <dballe/msg/wr_codec.h>
#include <dballe/msg/msg.h>
#include <stdint.h>

namespace wreport {
struct Subset;
struct Bulletin;
struct Var;
}

namespace dballe {
namespace msg {
namespace wr {

class Importer
{
protected:
    const import::Options& opts;
    const wreport::Subset* subset;
    Msg* msg;

    virtual void init() {}
    virtual void run() = 0;

public:
    Importer(const import::Options& opts) : opts(opts) {}
    virtual ~Importer() {}

    virtual MsgType scanType(const wreport::Bulletin& bulletin) const = 0;

    void import(const wreport::Subset& subset, Msg& msg)
    {
	    this->subset = &subset;
	    this->msg = &msg;
	    init();
	    run();
    }

    static std::auto_ptr<Importer> createSynop();
    static std::auto_ptr<Importer> createMetar();
    static std::auto_ptr<Importer> createTemp();
    static std::auto_ptr<Importer> createPilot();
    static std::auto_ptr<Importer> createFlight();
    static std::auto_ptr<Importer> createSat();
    static std::auto_ptr<Importer> createPollution();
    static std::auto_ptr<Importer> createGeneric();
};

class WMOImporter : public Importer
{
protected:
    unsigned pos;

    void import_var(const wreport::Var& var);

    virtual void init()
    {
	    pos = 0;
	    Importer::init();
    }

public:
    WMOImporter(const import::Options& opts) : Importer(opts) {}
    virtual ~WMOImporter() {}
};

} // namespace wr
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
