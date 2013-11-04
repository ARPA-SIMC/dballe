/*
 * core/varmatch - Variable matcher
 *
 * Copyright (C) 2013  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_CORE_VARMATCH_H
#define DBA_CORE_VARMATCH_H

#include <memory>
#include <wreport/var.h>

namespace dballe {

struct Varmatch
{
    wreport::Varcode code;

    Varmatch(wreport::Varcode code);
    virtual ~Varmatch() {}

    virtual bool operator()(const wreport::Var&) const;

    static std::auto_ptr<Varmatch> parse(const std::string& filter);
};

}

#endif
