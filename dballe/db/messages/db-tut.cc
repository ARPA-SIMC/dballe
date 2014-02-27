/*
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

#include "db/test-utils-db.h"
#include "db/messages/db.h"

using namespace dballe;
using namespace dballe::db;
using namespace wreport;
using namespace wibble::tests;
using namespace std;

namespace tut {

struct dbmessages_shar
{
    dbmessages_shar()
    {
    }

    ~dbmessages_shar()
    {
    }
};
TESTGRP(dbmessages);

template<> template<>
void to::test<1>()
{
    // 2 messages, 1 subset each
    db::messages::DB db(BUFR, dballe::tests::datafile("bufr/synotemp.bufr"));

    // At the beginning, the DB is empty
    Record query;
    std::auto_ptr<db::Cursor> cur = db.query_data(query);
    wassert(actual(cur->remaining()) == 0);

    // First message
    wassert(actual(db.next_message()).istrue());
    cur = db.query_data(query);
    wassert(actual(cur->remaining()) == 88);

    // Second message
    wassert(actual(db.next_message()).istrue());
    cur = db.query_data(query);
    wassert(actual(cur->remaining()) == 9);

    // End of messages
    wassert(actual(db.next_message()).isfalse());
    cur = db.query_data(query);
    wassert(actual(cur->remaining()) == 0);
}

template<> template<>
void to::test<2>()
{
    // 1 message, 6 subsets
    db::messages::DB db(BUFR, dballe::tests::datafile("bufr/temp-gts2.bufr"));

    // At the beginning, the DB is empty
    Record query;
    std::auto_ptr<db::Cursor> cur = db.query_data(query);
    wassert(actual(cur->remaining()) == 0);

    // 6 subsets
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 193);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 182);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 170);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 184);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 256);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 213);

    // End of messages
    wassert(actual(db.next_message()).isfalse());
    wassert(actual(db.query_data(query)->remaining()) == 0);
}

template<> template<>
void to::test<3>()
{
    // 2 messages, 2 subsets each
    db::messages::DB db(BUFR, dballe::tests::datafile("bufr/db-messages1.bufr"));

    // At the beginning, the DB is empty
    Record query;
    std::auto_ptr<db::Cursor> cur = db.query_data(query);
    wassert(actual(cur->remaining()) == 0);

    // 6 subsets
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 88);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 9);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 193);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 182);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 170);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 184);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 256);
    wassert(actual(db.next_message()).istrue());
    wassert(actual(db.query_data(query)->remaining()) == 213);

    // End of messages
    wassert(actual(db.next_message()).isfalse());
    wassert(actual(db.query_data(query)->remaining()) == 0);
}

}

/* vim:set ts=4 sw=4: */

