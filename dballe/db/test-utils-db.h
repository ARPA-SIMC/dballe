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

struct TestRecord
{
    Record data;
    Record station_data;
    std::map<wreport::Varcode, Record> attrs;

    void insert(DB& db, bool can_replace=false);

    /// Returns true if rec matches our station keys
    bool match_station_keys(const Record& rec);
    /// Returns true if rec matches our station/datetime/level/timerange/report keys
    bool match_context_keys(const Record& rec);
    /// Returns true if rec matches our data variable with the given code
    bool match_data_var(wreport::Varcode code, const Record& rec);
};

struct DefaultTestRecord : public TestRecord
{
    DefaultTestRecord()
    {
        data.set(Level(10, 11, 15, 22));
        data.set(Trange(20, 111, 122));
        data.set(DBA_KEY_REP_MEMO, "synop");
        data.set(DBA_KEY_LAT, 12.34560);
        data.set(DBA_KEY_LON, 76.54320);
        data.set_datetime({1945, 4, 25, 8, 0, 0});
        data.set(WR_VAR(0, 1, 11), "DB-All.e!");
        data.set(WR_VAR(0, 1, 12), 300);

        station_data.set(WR_VAR(0, 7, 30), 42);     // Height
        station_data.set(WR_VAR(0, 7, 31), 234);    // Heightbaro
        station_data.set(WR_VAR(0, 1,  1), 1);      // Block
        station_data.set(WR_VAR(0, 1,  2), 52);     // Station
        station_data.set(WR_VAR(0, 1, 19), "Cippo Lippo");  // Name
    }
};

struct db_test
{
    // DB handle
    std::auto_ptr<DB> db;
    db::Format orig_format;

    void disappear();
    bool has_db() const { return db.get() != NULL; }
    void use_db();
    void use_db(db::Format format, bool reset=true);

    db::v5::DB& v5();
    db::v6::DB& v6();

	db_test();
	db_test(db::Format format, bool reset=true);
	~db_test();
};

/// Common bits for db::DB test suites
struct DB_test_base : public db_test
{
    DefaultTestRecord dataset0;
    DefaultTestRecord dataset1;
    TestRecord ds_navile;

    // Work records
    Record insert;
    Record query;
    Record result;
    Record qc;

    DB_test_base();
    DB_test_base(db::Format format);

    void init_records();

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
