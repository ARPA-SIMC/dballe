/*
 * memdb/valuebase - Common implementation for StationValue and Value
 *
 * Copyright (C) 2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBA_MEMDB_VALUEBASE_H
#define DBA_MEMDB_VALUEBASE_H

#include <wreport/var.h>
#include <vector>
#include <memory>

namespace dballe {
struct Values;
struct Record;

namespace memdb {

/// Station information
struct ValueBase
{
    wreport::Var* var;

    ValueBase(std::unique_ptr<wreport::Var> var)
        : var(var.release()) {}
    ~ValueBase();

    void query_attrs(std::function<void(std::unique_ptr<wreport::Var>)> dest) const;

    void attr_insert(const Record& attrs);
    void attr_insert(const Values& attrs);
    void attr_remove(const std::vector<wreport::Varcode>& qcs);

    /// Replace the variable with the given one
    void replace(std::unique_ptr<wreport::Var> var);

    /// Replace the value with the one of the given variable
    void replace(const wreport::Var& var);

private:
    ValueBase(const ValueBase&);
    ValueBase& operator=(const ValueBase&);
};

}
}

#endif
