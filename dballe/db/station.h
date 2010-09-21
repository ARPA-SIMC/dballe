/*
 * db/station - station table management
 *
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

#ifndef DBALLE_DB_STATION_H
#define DBALLE_DB_STATION_H

/** @file
 * @ingroup db
 *
 * Station table management used by the db module.
 */

#include <dballe/db/odbcworkarounds.h>
#include <sqltypes.h>
#include <cstdio>

namespace dballe {
struct DB;

namespace db {
struct Connection;
struct Statement;

/**
 * Precompiled queries to manipulate the station table
 */
struct Station
{
	/**
	 * DB connection.
	 */
	DB& db;

	/** Precompiled select fixed station query */
        db::Statement* sfstm;
	/** Precompiled select mobile station query */
        db::Statement* smstm;
	/** Precompiled select data by station id query */
        db::Statement* sstm;
	/** Precompiled insert query */
        db::Statement* istm;
	/** Precompiled update query */
        db::Statement* ustm;
	/** Precompiled delete query */
        db::Statement* dstm;

	/** Station ID SQL parameter */
	DBALLE_SQL_C_SINT_TYPE id;
	/** Station latitude SQL parameter */
	DBALLE_SQL_C_SINT_TYPE lat;
	/** Station longitude SQL parameter */
	DBALLE_SQL_C_SINT_TYPE lon;
	/** Mobile station identifier SQL parameter */
	char ident[64];
	/** Mobile station identifier indicator */
	SQLLEN ident_ind;

	Station(DB& conn);
	~Station();

        /**
         * Set the mobile station identifier input value for this ::dba_db_station
         *
         * @param ident
         *   Value to use for ident.  NULL can be used to unset ident.
         */
        void set_ident(const char* ident);

        /**
         * Get the station ID given latitude, longitude and mobile identifier
         *
         * @return
         *   Resulting ID of the station
         */
        int get_id();

        /**
         * Get station information given a station ID
         *
         * @param id
         *   ID of the station to query
         */
        void get_data(int id);

        /**
         * Insert a new station entry
         *
         * @retval id
         *   ID of the newly inserted station
         */
        int insert();

        /**
         * Update the information about a station entry
         */
        void update();

        /**
         * Remove a station record
         */
        void remove();

        /**
         * Dump the entire contents of the table to an output stream
         */
        void dump(FILE* out);

private:
	// disallow copy
	Station(const Station&);
	Station& operator=(const Station&);
};

#if 0


#ifdef  __cplusplus
}
#endif

#endif

} // namespace db
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
