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
