/*
 * db/v5/repinfo - repinfo table management
 *
 * Copyright (C) 2005--2014  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include "repinfo.h"
#include "dballe/db/sql.h"
#include "dballe/db/odbc/internals.h"
//#include "dballe/db/odbc/repinfo.h"
#include "dballe/db/sqlite/internals.h"
//#include "dballe/db/sqlite/repinfo.h"
#include "dballe/core/record.h"
#include "dballe/core/csv.h"
#include <wreport/error.h>
#include <algorithm>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace sql {

Repinfo::Repinfo(Connection& conn)
    : conn(conn)
{
}

#if 0
std::unique_ptr<Repinfo> Repinfo::create(Connection& conn)
{
    if (ODBCConnection* c = dynamic_cast<ODBCConnection*>(&conn))
        return unique_ptr<Repinfo>(new ODBCRepinfo(*c));
    else if (SQLiteConnection* c = dynamic_cast<SQLiteConnection*>(&conn))
        return unique_ptr<Repinfo>(new SQLiteRepinfo(*c));
    else
        throw error_unimplemented("v5 DB repinfo not yet implemented for non-ODBC connectors");
}
#endif

void Repinfo::to_record(int id, Record& rec)
{
    const repinfo::Cache* c = get_by_id(id);
    if (c)
    {
        rec.key(DBA_KEY_REP_MEMO).setc(c->memo.c_str());
        rec.key(DBA_KEY_PRIORITY).seti(c->prio);
    } else {
        rec.key(DBA_KEY_REP_MEMO).unset();
        rec.key(DBA_KEY_PRIORITY).unset();
    }
}

const char* Repinfo::get_rep_memo(int id)
{
    if (const repinfo::Cache* c = get_by_id(id))
        return c->memo.c_str();
    error_notfound::throwf("rep_memo not found for report code %d", id);
}

int Repinfo::get_id(const char* memo)
{
    char lc_memo[20];
    int i;
    for (i = 0; i < 19 && memo[i]; ++i)
        lc_memo[i] = tolower(memo[i]);
    lc_memo[i] = 0;

    if (memo_idx.empty()) rebuild_memo_idx();

    int pos = cache_find_by_memo(lc_memo);
    if (pos == -1)
        error_notfound::throwf("looking for repinfo corresponding to '%s'", memo);
    return memo_idx[pos].id;
}

int Repinfo::get_priority(int id)
{
    const repinfo::Cache* ri_entry = get_by_id(id);
    return ri_entry ? ri_entry->prio : INT_MAX;
}

std::map<std::string, int> Repinfo::get_priorities()
{
    std::map<std::string, int> res;
    for (std::vector<repinfo::Cache>::const_iterator i = cache.begin();
            i != cache.end(); ++i)
        res[i->memo] = i->prio;
    return res;
}

int Repinfo::obtain_id(const char* memo)
{
    char lc_memo[20];
    int i;
    for (i = 0; i < 19 && memo[i]; ++i)
        lc_memo[i] = tolower(memo[i]);
    lc_memo[i] = 0;

    if (memo_idx.empty()) rebuild_memo_idx();

    int pos = cache_find_by_memo(lc_memo);
    if (pos == -1)
    {
        insert_auto_entry(memo);
        read_cache();
        return get_id(memo);
    }
    return memo_idx[pos].id;
}

std::vector<int> Repinfo::ids_by_prio(const Record& rec)
{
    int prio = rec.get(DBA_KEY_PRIORITY, MISSING_INT);
    int priomin = rec.get(DBA_KEY_PRIOMIN, MISSING_INT);
    int priomax = rec.get(DBA_KEY_PRIOMAX, MISSING_INT);

    vector<int> res;
    for (std::vector<repinfo::Cache>::const_iterator i = cache.begin();
            i != cache.end(); ++i)
    {
        if (prio != MISSING_INT && i->prio != prio) continue;
        if (priomin != MISSING_INT && i->prio < priomin) continue;
        if (priomax != MISSING_INT && i->prio > priomax) continue;
        res.push_back(i->id);
    }

    return res;
}

const repinfo::Cache* Repinfo::get_by_id(unsigned id) const
{
    int pos = cache_find_by_id(id);
    return pos == -1 ? NULL : &(cache[pos]);
}

const repinfo::Cache* Repinfo::get_by_memo(const char* memo) const
{
    int pos = cache_find_by_memo(memo);
    if (pos == -1) return NULL;
    return get_by_id(memo_idx[pos].id);
}

int Repinfo::cache_find_by_id(unsigned id) const
{
    /* Binary search the ID */
    int begin, end;

    begin = -1, end = cache.size();
    while (end - begin > 1)
    {
        int cur = (end + begin) / 2;
        if (cache[cur].id > id)
            end = cur;
        else
            begin = cur;
    }
    if (begin == -1 || cache[begin].id != id)
        return -1;
    else
        return begin;
}

int Repinfo::cache_find_by_memo(const char* memo) const
{
    /* Binary search the memo index */
    int begin, end;

    begin = -1, end = cache.size();
    while (end - begin > 1)
    {
        int cur = (end + begin) / 2;
        if (memo_idx[cur].memo > memo)
            end = cur;
        else
            begin = cur;
    }
    if (begin == -1 || memo_idx[begin].memo != memo)
        return -1;
    else
        return begin;
}

void Repinfo::cache_append(unsigned id, const char* memo, const char* desc, int prio, const char* descriptor, int tablea)
{
    /* Ensure that we are adding things in order */
    if (!cache.empty() && cache.back().id >= id)
        error_consistency::throwf(
                "checking that value to append to repinfo cache (%u) "
                "is greather than the last value in che cache (%u)", id, (unsigned)cache.back().id);

    memo_idx.clear();

    /* Enlarge buffer if needed */
    cache.push_back(repinfo::Cache(id, memo, desc, prio, descriptor, tablea));
}

void Repinfo::rebuild_memo_idx() const
{
    memo_idx.clear();
    memo_idx.resize(cache.size());
    for (size_t i = 0; i < cache.size(); ++i)
    {
        memo_idx[i].memo = cache[i].memo;
        memo_idx[i].id = cache[i].id;
    }
    std::sort(memo_idx.begin(), memo_idx.end());
}

namespace {
struct fd_closer
{
    FILE* fd;
    fd_closer(FILE* fd) : fd(fd) {}
    ~fd_closer() { fclose(fd); }
};

inline void inplace_tolower(std::string& buf)
{
    for (string::iterator i = buf.begin(); i != buf.end(); ++i)
        *i = tolower(*i);
}
}

std::vector<repinfo::Cache> Repinfo::read_repinfo_file(const char* deffile)
{
    if (deffile == 0)
        deffile = DB::default_repinfo_file();

    /* Open the input CSV file */
    FILE* in = fopen(deffile, "r");
    if (in == NULL)
        error_system::throwf("opening file %s", deffile);
    fd_closer closer(in);

    /* Read the CSV file */
    vector<repinfo::Cache> newitems;

    vector<string> columns;
    for (int line = 1; csv_read_next(in, columns); ++line)
    {
        int id, pos;

        if (columns.size() != 6)
            error_parse::throwf(deffile, line, "Expected 6 columns, got %zd", columns.size());

        // Lowercase all rep_memos
        inplace_tolower(columns[1]);

        id = strtol(columns[0].c_str(), 0, 10);
        pos = cache_find_by_id(id);
        if (pos == -1)
        {
            /* New entry */
            newitems.push_back(repinfo::Cache(id, columns[1], columns[2], strtol(columns[3].c_str(), 0, 10), columns[4], strtol(columns[5].c_str(), 0, 10)));
            newitems.back().make_new();
        } else {
            /* Possible update on an existing entry */
            cache[pos].new_memo = columns[1];
            cache[pos].new_desc = columns[2];
            cache[pos].new_prio = strtol(columns[3].c_str(), 0, 10);
            cache[pos].new_descriptor = columns[4];
            cache[pos].new_tablea = strtol(columns[5].c_str(), 0, 10);
        }
    }

    /* Verify conflicts */
    for (size_t i = 0; i < cache.size(); ++i)
    {
        /* Skip empty items or items that will be deleted */
        if (cache[i].memo.empty() || cache[i].new_memo.empty())
            continue;
        if (cache[i].memo != cache[i].new_memo)
            error_consistency::throwf("cannot rename rep_cod %d (previous rep_memo was %s, new rep_memo is %s)",
                    (int)cache[i].id,
                    cache[i].memo.c_str(),
                    cache[i].new_memo.c_str());
        for (size_t j = i + 1; j < cache.size(); ++j)
        {
            /* Skip empty items or items that will be deleted */
            if (cache[j].memo.empty() || cache[j].new_memo.empty())
                continue;
            if (cache[j].new_prio == cache[i].new_prio)
                error_consistency::throwf("%s has the same priority (%d) as %s",
                        cache[j].new_memo.c_str(),
                        (int)cache[j].new_prio,
                        cache[i].new_memo.c_str());
        }
        for (vector<repinfo::Cache>::const_iterator j = newitems.begin();
                j != newitems.end(); ++j)
        {
            if (j->new_prio == cache[i].new_prio)
                error_consistency::throwf("%s has the same priority (%d) as %s",
                        j->new_memo.c_str(),
                        (int)j->new_prio,
                        cache[i].new_memo.c_str());
        }
    }
    for (vector<repinfo::Cache>::const_iterator i = newitems.begin();
            i != newitems.end(); ++i)
    {
        /*fprintf(stderr, "prio %d\n", cur->item.new_prio);*/
        for (vector<repinfo::Cache>::const_iterator j = i + 1;
                j != newitems.end(); ++j)
        {
            if (i->new_prio == j->new_prio)
                error_consistency::throwf("%s has the same priority (%d) as %s",
                        i->new_memo.c_str(), (int)i->new_prio, j->new_memo.c_str());
        }
    }

    return newitems;
}


namespace repinfo {

Cache::Cache(int id, const std::string& memo, const std::string& desc, int prio, const std::string& descriptor, int tablea)
    : id(id), memo(memo), desc(desc), prio(prio), descriptor(descriptor), tablea(tablea),
      new_prio(0), new_tablea(0)
{
}

void Cache::make_new()
{
    new_memo = memo;
    new_desc = desc;
    new_prio = prio;
    new_descriptor = descriptor;
    new_tablea = tablea;
}

bool Memoidx::operator<(const Memoidx& val) const
{
    return memo < val.memo;
}

}

}
}
}
