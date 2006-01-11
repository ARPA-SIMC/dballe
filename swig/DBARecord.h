#ifndef CPLUSPLUS_DBALLE_RECORD_H
#define CPLUSPLUS_DBALLE_RECORD_H

#include <DBAError.h>
#include <dballe/core/dba_record.h>

class DBAlle;

class DBARecord
{
protected:
	dba_record rec;

public:
	DBARecord()
	{
		DBA_RUN_OR_THROW(dba_record_create(&rec));
	}
	~DBARecord() { dba_record_delete(rec); }

	int enqi(const std::string& param) const
	{
		int res;
		DBA_RUN_OR_THROW(dba_enqi(rec, param.c_str(), &res));
		return res;
	}
	float enqr(const std::string& param) const
	{
		float res;
		DBA_RUN_OR_THROW(dba_enqr(rec, param.c_str(), &res));
		return res;
	}
	double enqd(const std::string& param) const
	{
		double res;
		DBA_RUN_OR_THROW(dba_enqd(rec, param.c_str(), &res));
		return res;
	}
	std::string enqc(const std::string& param) const
	{
		const char* res;
		DBA_RUN_OR_THROW(dba_enqc(rec, param.c_str(), &res));
		return res;
	}

	void seti(const std::string& param, int val)
	{
		DBA_RUN_OR_THROW(dba_seti(rec, param.c_str(), val));
	}
	void setr(const std::string& param, float val)
	{
		DBA_RUN_OR_THROW(dba_setr(rec, param.c_str(), val));
	}
	void setd(const std::string& param, double val)
	{
		DBA_RUN_OR_THROW(dba_setd(rec, param.c_str(), val));
	}
	void setc(const std::string& param, const std::string& val)
	{
		DBA_RUN_OR_THROW(dba_setc(rec, param.c_str(), val.c_str()));
	}

	void set(const std::string& param, int val)
	{
		DBA_RUN_OR_THROW(dba_seti(rec, param.c_str(), val));
	}
	void set(const std::string& param, float val)
	{
		DBA_RUN_OR_THROW(dba_setr(rec, param.c_str(), val));
	}
	void set(const std::string& param, double val)
	{
		DBA_RUN_OR_THROW(dba_setd(rec, param.c_str(), val));
	}
	void set(const std::string& param, const std::string& val)
	{
		DBA_RUN_OR_THROW(dba_setc(rec, param.c_str(), val.c_str()));
	}

	friend class DBAlle;
};

#endif
