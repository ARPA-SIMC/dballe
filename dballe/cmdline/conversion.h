/*
 * Copyright (C) 2005--2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef CONVERSION_H
#define CONVERSION_H

#if 0
#include <dballe/core/file.h>
#include <dballe/core/rawmsg.h>
#include <dballe/msg/msgs.h>
#include <dballe/bufrex/msg.h>
#endif
#include <dballe/cmdline/processor.h>

namespace wreport {
struct Bulletin;
}

namespace dballe {
struct File;

namespace msg {
struct Importer;
struct Exporter;
}

namespace cmdline {

struct Converter : public Action
{
    File* file;
    const char* dest_rep_memo;
    const char* dest_template;
    bool bufr2netcdf_categories;

    msg::Exporter* exporter;

    Converter() : file(0), dest_rep_memo(0), dest_template(0), bufr2netcdf_categories(false), exporter(0) {}
    ~Converter();

    /**
     * Convert the item as configured in the Converter, and write it to the
     * output file
     */
    virtual bool operator()(const cmdline::Item& item);

protected:
    /**
     * Perform conversion at the encoding level only (e.g. BUFR->CREX)
     *
     * @param orig
     *   Original Rawmsg used for its source information, to report errors
     */
    void process_bufrex_msg(const Rawmsg& orig, const wreport::Bulletin& msg);

    /**
     * Perform conversion of decoded data, auto-inferring
     * type/subtype/localsubtype from the Msgs contents
     */
    void process_dba_msg(const Rawmsg& orig, const Msgs& msgs);

    /**
     * Perform conversion of decded data, using the original bulletin for
     * type/subtype/localsubtype information
     */
    void process_dba_msg_from_bulletin(const Rawmsg& orig, const wreport::Bulletin& bulletin, const Msgs& msgs);
};

}
}

#endif
