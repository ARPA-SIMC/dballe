#ifndef FDBA_DBAPI_H
#define FDBA_DBAPI_H

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

#include "commonapi.h"

namespace dballe {
struct DB;

namespace db {
struct Cursor;
}

namespace fortran {

class DbAPI : public CommonAPIImplementation
{
protected:
    DB& db;
    db::Cursor* ana_cur;
    db::Cursor* query_cur;

public:
    DbAPI(DB& db, const char* anaflag, const char* dataflag, const char* attrflag);
    virtual ~DbAPI();

    virtual int enqi(const char* param);

    virtual void scopa(const char* repinfofile = 0);

    virtual int quantesono();
    virtual void elencamele();

    virtual int voglioquesto();
    virtual const char* dammelo();

    virtual void prendilo();
    virtual void dimenticami();

    virtual int voglioancora();

    virtual void critica();
    virtual void scusa();
};

}
}

/* vim:set ts=4 sw=4: */
#endif
