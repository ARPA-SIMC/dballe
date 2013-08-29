/*
 * db/v6/lev_tr - lev_tr table management
 *
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

#include "lev_tr.h"
#include "dballe/db/internals.h"
#include "db.h"
#include <dballe/core/defs.h>
#include <dballe/core/record.h>
#include <dballe/msg/msg.h>

#include <map>
#include <sstream>
#include <cstring>
#include <sql.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v6 {

LevTr::LevTr(DB& db)
    : db(db), sstm(0), sdstm(0), istm(0), dstm(0)
{
    const char* select_query =
        "SELECT id FROM lev_tr WHERE"
        "     ltype1=? AND l1=? AND ltype2=? AND l2=?"
        " AND ptype=? AND p1=? AND p2=?";
    const char* select_data_query =
        "SELECT ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr WHERE id=?";
    const char* insert_query =
        "INSERT INTO lev_tr (ltype1, l1, ltype2, l2, ptype, p1, p2) VALUES (?, ?, ?, ?, ?, ?, ?)";
    const char* remove_query =
        "DELETE FROM lev_tr WHERE id=?";

    /* Override queries for some databases */
    switch (db.conn->server_type)
    {
        case ORACLE:
            insert_query = "INSERT INTO lev_tr VALUES (seq_lev_tr.NextVal, ?, ?, ?, ?, ?, ?, ?)";
            break;
        default: break;
    }

    /* Create the statement for select fixed */
    sstm = new Statement(*db.conn);
    sstm->bind_in(1, ltype1);
    sstm->bind_in(2, l1);
    sstm->bind_in(3, ltype2);
    sstm->bind_in(4, l2);
    sstm->bind_in(5, pind);
    sstm->bind_in(6, p1);
    sstm->bind_in(7, p2);
    sstm->bind_out(1, id);
    sstm->prepare(select_query);

    /* Create the statement for select data */
    sdstm = new Statement(*db.conn);
    sdstm->bind_in(1, id);
    sdstm->bind_out(1, ltype1);
    sdstm->bind_out(2, l1);
    sdstm->bind_out(3, ltype2);
    sdstm->bind_out(4, l2);
    sdstm->bind_out(5, pind);
    sdstm->bind_out(6, p1);
    sdstm->bind_out(7, p2);
    sdstm->prepare(select_data_query);

    /* Create the statement for insert */
    istm = new Statement(*db.conn);
    istm->bind_in(1, ltype1);
    istm->bind_in(2, l1);
    istm->bind_in(3, ltype2);
    istm->bind_in(4, l2);
    istm->bind_in(5, pind);
    istm->bind_in(6, p1);
    istm->bind_in(7, p2);
    istm->prepare(insert_query);

    /* Create the statement for remove */
    dstm = new Statement(*db.conn);
    dstm->bind_in(1, id);
    dstm->prepare(remove_query);
}

LevTr::~LevTr()
{
    if (sstm) delete sstm;
    if (sdstm) delete sdstm;
    if (istm) delete istm;
    if (dstm) delete dstm;
}

int LevTr::get_id()
{
    sstm->execute();

    /* Get the result */
    int res;
    if (sstm->fetch_expecting_one())
        res = id;
    else
        res = -1;

    return res;
}

void LevTr::get_data(int qid)
{
    id = qid;
    sdstm->execute();
    if (!sdstm->fetch_expecting_one())
        error_notfound::throwf("no data found for lev_tr id %d", qid);
}

int LevTr::insert()
{
    istm->execute_and_close();
    return db.last_lev_tr_insert_id();
}

void LevTr::remove()
{
    dstm->execute_and_close();
}

void LevTr::dump(FILE* out)
{
    DBALLE_SQL_C_SINT_TYPE id;
    DBALLE_SQL_C_SINT_TYPE ltype1;
    DBALLE_SQL_C_SINT_TYPE l1;
    DBALLE_SQL_C_SINT_TYPE ltype2;
    DBALLE_SQL_C_SINT_TYPE l2;
    DBALLE_SQL_C_SINT_TYPE pind;
    DBALLE_SQL_C_SINT_TYPE p1;
    DBALLE_SQL_C_SINT_TYPE p2;

    Statement stm(*db.conn);
    stm.bind_out(1, id);
    stm.bind_out(2, ltype1);
    stm.bind_out(3, l1);
    stm.bind_out(4, ltype2);
    stm.bind_out(5, l2);
    stm.bind_out(6, pind);
    stm.bind_out(7, p1);
    stm.bind_out(8, p2);
    stm.exec_direct("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr ORDER BY id");
    int count;
    fprintf(out, "dump of table lev_tr:\n");
    fprintf(out, "   id   lev              tr\n");
    for (count = 0; stm.fetch(); ++count)
    {
        fprintf(out, " %4d ", (int)id);
        {
            stringstream str;
            str << Level(ltype1, l1, ltype2, l2);
            fprintf(out, "%-20s ", str.str().c_str());
        }
        {
            stringstream str;
            str << Trange(pind, p1, p2);
            fprintf(out, "%-10s\n", str.str().c_str());
        }
    }
    fprintf(out, "%d element%s in table lev_tr\n", count, count != 1 ? "s" : "");
}


LevTrCache::~LevTrCache() {}

struct MapLevTrCache : public LevTrCache
{
    struct SQLOut
    {
        DBALLE_SQL_C_SINT_TYPE id;
        DBALLE_SQL_C_SINT_TYPE ltype1;
        DBALLE_SQL_C_SINT_TYPE l1;
        DBALLE_SQL_C_SINT_TYPE ltype2;
        DBALLE_SQL_C_SINT_TYPE l2;
        DBALLE_SQL_C_SINT_TYPE pind;
        DBALLE_SQL_C_SINT_TYPE p1;
        DBALLE_SQL_C_SINT_TYPE p2;
    };

    struct Item
    {
        int ltype1;
        int l1;
        int ltype2;
        int l2;
        int pind;
        int p1;
        int p2;
        Item(const SQLOut& o)
            : ltype1(o.ltype1), l1(o.l1),
              ltype2(o.ltype2), l2(o.l2),
              pind(o.pind), p1(o.p1), p2(o.p2) {}

        void to_record(Record& rec) const
        {
            rec.set(DBA_KEY_LEVELTYPE1, ltype1);
            rec.set(DBA_KEY_L1, l1);
            rec.set(DBA_KEY_LEVELTYPE2, ltype2);
            rec.set(DBA_KEY_L2, l2);
            rec.set(DBA_KEY_PINDICATOR, pind);
            rec.set(DBA_KEY_P1, p1);
            rec.set(DBA_KEY_P2, p2);
        }

        Level lev() const
        {
            return Level(ltype1, l1, ltype2, l2);
        }

        Trange tr() const
        {
            return Trange(pind, p1, p2);
        }
    };

    struct FetchStatement
    {
        // Output values
        SQLOut out;
        DB& db;
        Statement stm;

        FetchStatement(DB& db)
            : db(db), stm(*db.conn)
        {
            // Create the fetch query
            stm.bind_in(1, out.id);
            stm.bind_out(1, out.ltype1);
            stm.bind_out(2, out.l1);
            stm.bind_out(3, out.ltype2);
            stm.bind_out(4, out.l2);
            stm.bind_out(5, out.pind);
            stm.bind_out(6, out.p1);
            stm.bind_out(7, out.p2);
            stm.prepare("SELECT ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr WHERE id=?");
        }

        void prefetch(map<int, Item>& cache)
        {
            // Prefetch everything
            Statement stm(*db.conn);
            stm.bind_out(1, out.id);
            stm.bind_out(2, out.ltype1);
            stm.bind_out(3, out.l1);
            stm.bind_out(4, out.ltype2);
            stm.bind_out(5, out.l2);
            stm.bind_out(6, out.pind);
            stm.bind_out(7, out.p1);
            stm.bind_out(8, out.p2);
            stm.exec_direct("SELECT id, ltype1, l1, ltype2, l2, ptype, p1, p2 FROM lev_tr");
            while (stm.fetch())
                cache.insert(make_pair((int)out.id, Item(out)));
        }

        bool get(int id)
        {
            out.id = id;
            stm.execute();
            if (!stm.fetch_expecting_one())
                return false;
            return true;
        }

    };

    mutable map<int, Item> cache;
    mutable FetchStatement stm;


    MapLevTrCache(LevTr& lt)
        : stm(lt.db)
    {
        // TODO: make prefetch optional if needed, controlled by an env
        //       variable
        stm.prefetch(cache);
    }

    const Item* get(int id) const
    {
        map<int, Item>::const_iterator i = cache.find(id);

        // Cache hit
        if (i != cache.end()) return &(i->second);

        // Miss: try the DB
        if (!stm.get(id))
            return 0;

        // Fill cache
        pair<map<int, Item>::iterator, bool> res = cache.insert(make_pair(id, Item(stm.out)));
        return &(res.first->second);
    }

    bool to_rec(int id, Record& rec)
    {
        const Item* i = get(id);
        if (!i) return 0;
        i->to_record(rec);
        return true;
    }

    Level to_level(int id) const
    {
        const Item* i = get(id);
        if (!i) return Level();
        return i->lev();
    }

    Trange to_trange(int id) const
    {
        const Item* i = get(id);
        if (!i) return Trange();
        return i->tr();
    }

    msg::Context* to_msg(int id, Msg& msg)
    {
        const Item* i = get(id);
        if (!i) return 0;
        msg::Context& res = msg.obtain_context(i->lev(), i->tr());
        return &res;
    }

    void invalidate()
    {
        cache.clear();
    }

    void dump(FILE* out) const
    {
        fprintf(out, "%zd elements in level/timerange cache:\n", cache.size());
        for (map<int, Item>::const_iterator i = cache.begin(); i != cache.end(); ++i)
        {
            stringstream str;
            str << i->second.lev();
            str << " ";
            str << i->second.tr();
            fprintf(out, "  %d: %s\n", i->first, str.str().c_str());
        }
    }
};


std::auto_ptr<LevTrCache> LevTrCache::create(LevTr& lt)
{
    // TODO: check env vars to select alternate caching implementations, such
    // as a hit/miss that queries single items from the DB when not in cache
    return auto_ptr<LevTrCache>(new MapLevTrCache(lt));
}

} // namespace v6
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
