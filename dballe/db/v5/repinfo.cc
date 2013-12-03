/*
 * db/v5/repinfo - repinfo table management
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

#include "repinfo.h"
#include "dballe/db/db.h"
#include "dballe/db/internals.h"
#include "dballe/core/defs.h"
#include "dballe/core/record.h"
#include "dballe/core/csv.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sql.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v5 {

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

Repinfo::Repinfo(Connection* conn)
	: conn(conn)
{
	read_cache();
}

Repinfo::~Repinfo()
{
}

void Repinfo::read_cache()
{
	db::Statement stm(*conn);

	DBALLE_SQL_C_UINT_TYPE id;
	char memo[20]; SQLLEN memo_ind;
	char description[255]; SQLLEN description_ind;
	DBALLE_SQL_C_SINT_TYPE prio;
	char descriptor[6]; SQLLEN descriptor_ind;
	DBALLE_SQL_C_UINT_TYPE tablea;

	invalidate_cache();

	stm.bind_out(1, id);
	stm.bind_out(2, memo, sizeof(memo), memo_ind);
	stm.bind_out(3, description, sizeof(description), description_ind);
	stm.bind_out(4, prio);
	stm.bind_out(5, descriptor, sizeof(descriptor), descriptor_ind);
	stm.bind_out(6, tablea);

	stm.exec_direct("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");

	/* Get the results and save them in the record */
	while (stm.fetch())
		cache_append(id, memo, description, prio, descriptor, tablea);

	/* Rebuild the memo index as well */
	rebuild_memo_idx();
}

void Repinfo::insert_auto_entry(const char* memo)
{
    db::Statement stm(*conn);

    DBALLE_SQL_C_UINT_TYPE id;
    stm.bind_out(1, id);
    stm.exec_direct("SELECT MAX(id) FROM repinfo");
    stm.fetch_expecting_one();

    DBALLE_SQL_C_UINT_TYPE prio;
    stm.bind_out(1, prio);
    stm.exec_direct("SELECT MAX(prio) FROM repinfo");
    stm.fetch_expecting_one();

    ++id;
    ++prio;

    stm.bind_in(1, id);
    stm.bind_in(2, memo);
    stm.bind_in(3, memo);
    stm.bind_in(4, prio);

    stm.exec_direct_and_close("INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)"
                    " VALUES (?, ?, ?, ?, '-', 255)");
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

void Repinfo::invalidate_cache()
{
	cache.clear();
	memo_idx.clear();
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

int Repinfo::get_id(const char* memo) const
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

bool Repinfo::has_id(unsigned id) const
{
	return cache_find_by_id(id) != -1;
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

std::vector<int> Repinfo::ids_by_prio(const Record& rec) const
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

std::map<std::string, int> Repinfo::get_priorities() const
{
    std::map<std::string, int> res;
    for (std::vector<repinfo::Cache>::const_iterator i = cache.begin();
            i != cache.end(); ++i)
        res[i->memo] = i->prio;
    return res;
}

#if 0
/*
static void commit_cache_item(struct _dba_db_repinfo_cache* item)
{
	if (item->memo != NULL)
		free(item->memo);
	item->memo = item->new_memo;
	item->new_memo = NULL;

	if (item->desc != NULL)
		free(item->desc);
	item->desc = item->new_desc;
	item->new_desc = NULL;

	if (item->descriptor != NULL)
		free(item->descriptor);
	item->descriptor = item->new_descriptor;
	item->new_descriptor = NULL;
}
*/
#endif


static inline void inplace_tolower(std::string& buf)
{
	for (string::iterator i = buf.begin(); i != buf.end(); ++i)
		*i = tolower(*i);
}

namespace {
struct fd_closer
{
	FILE* fd;
	fd_closer(FILE* fd) : fd(fd) {}
	~fd_closer() { fclose(fd); }
};
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

int Repinfo::id_use_count(unsigned id, const char* name)
{
    DBALLE_SQL_C_UINT_TYPE dbid = id;
    DBALLE_SQL_C_UINT_TYPE count;
    db::Statement stm(*conn);
    stm.prepare("SELECT COUNT(1) FROM context WHERE id_report = ?");
    stm.bind_in(1, dbid);
    stm.bind_out(1, count);
    stm.execute();
    if (!stm.fetch_expecting_one())
        error_consistency::throwf("%s is in cache but not in the database (database externally modified?)", name);
    return count;
}

void Repinfo::update(const char* deffile, int* added, int* deleted, int* updated)
{
	*added = *deleted = *updated = 0;

	// Read the new repinfo data from file
	vector<repinfo::Cache> newitems = read_repinfo_file(deffile);

    // Verify that we are not trying to delete a repinfo entry that is
    // in use
    for (size_t i = 0; i < cache.size(); ++i)
    {
        /* Ensure that we are not deleting a repinfo entry that is already in use */
        if (!cache[i].memo.empty() && cache[i].new_memo.empty())
            if (id_use_count(cache[i].id, cache[i].memo.c_str()) > 0)
                error_consistency::throwf(
                        "trying to delete repinfo entry %u,%s which is currently in use",
                        (unsigned)cache[i].id, cache[i].memo.c_str());
    }

	/* Perform the changes */

	/* Delete the items that were deleted */
	{
		db::Statement stm(*conn);
		stm.prepare("DELETE FROM repinfo WHERE id=?");
		for (size_t i = 0; i < cache.size(); ++i)
			if (!cache[i].memo.empty() && cache[i].new_memo.empty())
			{
				stm.bind_in(1, cache[i].id);
				stm.execute_and_close();

				/* clear_cache_item(&(ri->cache[i])); */
				++*deleted;
			}
	}

	/* Update the items that were modified */
	{
		db::Statement stm(*conn);
		stm.prepare("UPDATE repinfo set memo=?, description=?, prio=?, descriptor=?, tablea=?"
			    "  WHERE id=?");
		for (size_t i = 0; i < cache.size(); ++i)
			if (!cache[i].memo.empty() && !cache[i].new_memo.empty())
			{
				stm.bind_in(1, cache[i].new_memo.c_str());
				stm.bind_in(2, cache[i].new_desc.c_str());
				stm.bind_in(3, cache[i].new_prio);
				stm.bind_in(4, cache[i].new_descriptor.c_str());
				stm.bind_in(5, cache[i].new_tablea);
				stm.bind_in(6, cache[i].id);

				stm.execute_and_close();

				/* commit_cache_item(&(ri->cache[i])); */
				++*updated;
			}
	}

	/* Insert the new items */
	if (!newitems.empty())
	{
		Statement stm(*conn);
		stm.prepare("INSERT INTO repinfo (id, memo, description, prio, descriptor, tablea)"
			    "     VALUES (?, ?, ?, ?, ?, ?)");

		for (vector<repinfo::Cache>::const_iterator i = newitems.begin();
				i != newitems.end(); ++i)
		{
			stm.bind_in(1, i->id);
			stm.bind_in(2, i->new_memo.c_str());
			stm.bind_in(3, i->new_desc.c_str());
			stm.bind_in(4, i->new_prio);
			stm.bind_in(5, i->new_descriptor.c_str());
			stm.bind_in(6, i->new_tablea);

			stm.execute_and_close();

			/* DBA_RUN_OR_GOTO(cleanup, cache_append(ri, id, memo, description, prio, descriptor, tablea)); */
			++*added;
		}
	}

	conn->commit();

	/* Reread the cache */
	read_cache();
}

void Repinfo::dump(FILE* out)
{
    DBALLE_SQL_C_UINT_TYPE id;
    char memo[20]; SQLLEN memo_ind;
    char description[255]; SQLLEN description_ind;
    DBALLE_SQL_C_SINT_TYPE prio;
    char descriptor[6]; SQLLEN descriptor_ind;
    DBALLE_SQL_C_UINT_TYPE tablea;

    db::Statement stm(*conn);

    stm.bind_out(1, id);
    stm.bind_out(2, memo, sizeof(memo), memo_ind);
    stm.bind_out(3, description, sizeof(description), description_ind);
    stm.bind_out(4, prio);
    stm.bind_out(5, descriptor, sizeof(descriptor), descriptor_ind);
    stm.bind_out(6, tablea);

    stm.exec_direct("SELECT id, memo, description, prio, descriptor, tablea FROM repinfo ORDER BY id");

    int count;
    fprintf(out, "dump of table repinfo:\n");
    fprintf(out, "   id   memo   description  prio   desc  tablea\n");
    for (count = 0; stm.fetch(); ++count)
    {
        fprintf(out, " %4d   %s  %s  %d  %s %u\n",
                (int)id,
                memo_ind == SQL_NULL_DATA ? "-" : memo,
                description_ind == SQL_NULL_DATA ? "-" : description,
                (int)prio,
                descriptor_ind == SQL_NULL_DATA ? "-" : descriptor,
                (unsigned)tablea
               );
    }
    fprintf(out, "%d element%s in table repinfo\n", count, count != 1 ? "s" : "");
}


} // namespace v5
} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
