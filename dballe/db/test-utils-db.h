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

#include <dballe/msg/test-utils-msg.h>
#include <dballe/db/db.h>
#include <sqltypes.h>

namespace dballe {
namespace db {
namespace v5 {
class DB;
}
namespace v6 {
class DB;
}
}

namespace tests {

struct db_test
{
    // DB handle
    std::auto_ptr<DB> db;

    bool has_db() const { return db.get() != NULL; }
    void use_db();

    db::v5::DB& v5();
    db::v6::DB& v6();

	db_test(bool reset=true);
	~db_test();
};

/// Common bits for db::DB test suites
struct DB_test_base : public db_test
{
    // Records with test data
    Record sampleAna;
    Record extraAna;
    Record sampleBase;
    Record sample0;
    Record sample00;
    Record sample01;
    Record sample1;
    Record sample10;
    Record sample11;

    // Work records
    Record insert;
    Record query;
    Record result;
    Record qc;

    DB_test_base();

    void populate_database();
};

static inline SQL_TIMESTAMP_STRUCT mkts(int year, int month, int day, int hour, int minute, int second)
{
	SQL_TIMESTAMP_STRUCT res;
	res.year = year;
	res.month = month;
	res.day = day;
	res.hour = hour;
	res.minute = minute;
	res.second = second;
	res.fraction = 0;
	return res;
}

} // namespace tests
} // namespace dballe

namespace std {

static inline bool operator!=(const SQL_TIMESTAMP_STRUCT& a, const SQL_TIMESTAMP_STRUCT& b)
{
	return a.year != b.year || a.month != b.month || a.day != b.day || a.hour != b.hour || a.minute != b.minute || a.second != b.second || a.fraction != b.fraction;
}

static inline std::ostream& operator<<(std::ostream& o, const SQL_TIMESTAMP_STRUCT& t)
{
	char buf[20];
	snprintf(buf, 20, "%04d-%02d-%02d %02d:%02d:%02d.%d", t.year, t.month, t.day, t.hour, t.minute, t.second, t.fraction);
	o << buf;
	return o;
}

}

// vim:set ts=4 sw=4:
