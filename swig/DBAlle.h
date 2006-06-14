/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef CPLUSPLUS_DBALLE_H
#define CPLUSPLUS_DBALLE_H

#include <DBAError.h>
#include <DBARecord.h>

#include <dballe/dballe.h>

class DBAlle
{
protected:
	dba db;

public:
	DBAlle(const std::string& dsn, const std::string& user, const std::string& password = "")
		: db(0)
	{
		DBA_RUN_OR_THROW(dba_open(dsn.c_str(), user.c_str(), password.c_str(), &db));
	}
	~DBAlle() { if (db) dba_close(db); }
	
	void reset()
	{
		DBA_RUN_OR_THROW(dba_reset(db, NULL));
	}

	void reset(const std::string& repinfofile)
	{
		DBA_RUN_OR_THROW(dba_reset(db, repinfofile.c_str()));
	}

	void insert(DBARecord& rec)
	{
		DBA_RUN_OR_THROW(dba_insert(db, rec.rec));
	}
	
	void insertNew(DBARecord& rec)
	{
		DBA_RUN_OR_THROW(dba_insert_new(db, rec.rec));
	}
	
	void remove(DBARecord& query)
	{
		DBA_RUN_OR_THROW(dba_delete(db, query.rec));
	}
	
	void queryQC(DBARecord& rec, int id_data, dba_varcode* qcs, int qcs_size, DBARecord& qc, int& count)
	{
		DBA_RUN_OR_THROW(dba_qc_query(db, id_data, qcs, qcs_size, qc.rec, &count));
	}

	void insertQC(int id_data, DBARecord& qc)
	{
		DBA_RUN_OR_THROW(dba_qc_insert(db, id_data, qc.rec));
	}

	void insertNewQC(int id_data, DBARecord& qc)
	{
		DBA_RUN_OR_THROW(dba_qc_insert_new(db, id_data, qc.rec));
	}

	void removeQC(DBARecord& rec, int id_data, dba_varcode* qcs = 0, int qcs_size = 0)
	{
		DBA_RUN_OR_THROW(dba_qc_delete(db, id_data, qcs, qcs_size));
	}

	static void init() { DBA_RUN_OR_THROW(dba_init()); }
	static void shutdown() { DBA_RUN_OR_THROW(dba_shutdown()); }
};

#endif
