/*
 * memdb/match - Record-by-record match
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

#include "match.h"
#include "stationvalue.h"
#include "value.h"
#include "dballe/core/varmatch.h"

using namespace std;
using namespace wreport;

namespace dballe {
namespace memdb {
namespace match {

template<typename T>
Varcodes<T>::Varcodes(std::set<wreport::Varcode> codes) : codes(codes) {}

template<typename T>
bool Varcodes<T>::operator()(const T& val) const
{
    return codes.find(val.var->code()) != codes.end();
}

template<typename T>
DataFilter<T>::DataFilter(const std::string& expr) : match(Varmatch::parse(expr).release()) {}

template<typename T>
DataFilter<T>::~DataFilter() { delete match; }

template<typename T>
bool DataFilter<T>::operator()(const T& val) const
{
    return (*match)(*val.var);
}

template<typename T>
AttrFilter<T>::AttrFilter(const std::string& expr) : match(Varmatch::parse(expr).release()) {}

template<typename T>
AttrFilter<T>::~AttrFilter() { delete match; }

template<typename T>
bool AttrFilter<T>::operator()(const T& val) const
{
    const Var* a = val.var->enqa(match->code);
    if (!a) return false;
    return (*match)(*a);
}

template class Varcode<Value>;
template class Varcode<StationValue>;
template class Varcodes<Value>;
template class Varcodes<StationValue>;
template class DataFilter<Value>;
template class DataFilter<StationValue>;
template class AttrFilter<Value>;
template class AttrFilter<StationValue>;

}
}
}
