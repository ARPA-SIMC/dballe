#include "repinfo.h"
#include "dballe/db/db.h"
#include "dballe/record.h"
#include "dballe/core/query.h"
#include "dballe/core/record.h"
#include "dballe/core/csv.h"
#include <wreport/error.h>
#include <algorithm>

using namespace wreport;
using namespace std;

namespace dballe {
namespace db {
namespace v7 {

Repinfo::Repinfo(dballe::sql::Connection& conn)
    : conn(conn)
{
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
    if (pos == -1) return -1;
    return memo_idx[pos].id;
}

int Repinfo::get_priority(const std::string& report)
{
    const repinfo::Cache* ri_entry = get_by_memo(report.c_str());
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
        insert_auto_entry(lc_memo);
        read_cache();
        return get_id(lc_memo);
    }
    return memo_idx[pos].id;
}

std::vector<int> Repinfo::ids_by_prio(const core::Query& q)
{
    vector<int> res;
    for (std::vector<repinfo::Cache>::const_iterator i = cache.begin();
            i != cache.end(); ++i)
    {
        if (q.prio_min != MISSING_INT && i->prio < q.prio_min) continue;
        if (q.prio_max != MISSING_INT && i->prio > q.prio_max) continue;
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


    /*
    fprintf(stderr, "POST PARSE:\n");
    for (const auto& e: cache)
        e.dump(stderr);
        */

    return newitems;
}

void Repinfo::update(const char* deffile, int* added, int* deleted, int* updated)
{
    *added = *deleted = *updated = 0;

    // Read the new repinfo data from file
    vector<repinfo::Cache> newitems = read_repinfo_file(deffile);

    // Verify that we are not trying to delete a repinfo entry that is
    // in use
    for (const auto& entry : cache)
    {
        /* Ensure that we are not deleting a repinfo entry that is already in use */
        if (!entry.memo.empty() && entry.new_memo.empty())
            if (id_use_count(entry.id, entry.memo.c_str()) > 0)
                error_consistency::throwf(
                        "trying to delete repinfo entry %u,%s which is currently in use",
                        (unsigned)entry.id, entry.memo.c_str());
    }

    // Perform the changes

    for (const auto& entry : cache)
    {
        if (!entry.memo.empty() && entry.new_memo.empty())
        {
            // Delete the items that were deleted */
            delete_entry(entry.id);
            ++*deleted;
        } else if (!entry.memo.empty() && !entry.new_memo.empty()) {
            // Update the items that were modified
            update_entry(entry);
            ++*updated;
        }
    }

    // Insert the new items
    for (const auto& entry : newitems)
    {
        insert_entry(entry);
        ++*added;
    }

    // Reread the cache
    read_cache();
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

void Cache::dump(FILE* out) const
{
    fprintf(stderr, "%u: %s %s %d %s %u (%s %s %d %s %u)\n",
            id, memo.c_str(), desc.c_str(), prio, descriptor.c_str(), tablea,
            new_memo.c_str(), new_desc.c_str(), new_prio, new_descriptor.c_str(), new_tablea);
}

bool Memoidx::operator<(const Memoidx& val) const
{
    return memo < val.memo;
}

}

}
}
}
