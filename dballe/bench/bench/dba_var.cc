#include "Benchmark.h"

#include <dballe/core/dba_var.h>

namespace bench_var {

class create : public Benchmark
{
protected:
	virtual dba_err main()
	{
		static const int iterations = 3000000;
		dba_varinfo info;
		DBA_RUN_OR_RETURN(dba_varinfo_query_local(DBA_VAR(0, 6, 1), &info));

		for (int i = 0; i < iterations; i++)
		{
			dba_var var;
			DBA_RUN_OR_RETURN(dba_var_create(info, &var));
			dba_var_delete(var);
		}
		timing("%d dba_var_create", iterations);

		for (int i = 0; i < iterations; i++)
		{
			dba_var var;
			DBA_RUN_OR_RETURN(dba_var_createi(info, 1234, &var));
			dba_var_delete(var);
		}
		timing("%d dba_var_createi", iterations);

		for (int i = 0; i < iterations; i++)
		{
			dba_var var;
			DBA_RUN_OR_RETURN(dba_var_created(info, 12.34, &var));
			dba_var_delete(var);
		}
		timing("%d dba_var_created", iterations);

		for (int i = 0; i < iterations; i++)
		{
			dba_var var;
			DBA_RUN_OR_RETURN(dba_var_createc(info, "1234", &var));
			dba_var_delete(var);
		}
		timing("%d dba_var_createc", iterations);

		return dba_error_ok();
	}

public:
	create() : Benchmark("create") {}
};

class enqset : public Benchmark
{
protected:
	virtual dba_err main()
	{
		static const int iterations = 3000000;
		dba_varinfo info;
		dba_var var;

		DBA_RUN_OR_RETURN(dba_varinfo_query_local(DBA_VAR(0, 6, 1), &info));
		DBA_RUN_OR_RETURN(dba_var_createi(info, 1234, &var));

		for (int i = 0; i < iterations; i++)
		{
			int ival;
			DBA_RUN_OR_RETURN(dba_var_enqi(var, &ival));
		}
		timing("%d dba_var_enqi", iterations);

		for (int i = 0; i < iterations; i++)
		{
			double dval;
			DBA_RUN_OR_RETURN(dba_var_enqd(var, &dval));
		}
		timing("%d dba_var_enqd", iterations);

		for (int i = 0; i < iterations; i++)
		{
			const char* cval;
			DBA_RUN_OR_RETURN(dba_var_enqc(var, &cval));
		}
		timing("%d dba_var_enqc", iterations);

		for (int i = 0; i < iterations; i++)
			DBA_RUN_OR_RETURN(dba_var_seti(var, 1234));
		timing("%d dba_var_seti", iterations);

		for (int i = 0; i < iterations; i++)
			DBA_RUN_OR_RETURN(dba_var_setd(var, 12.34));
		timing("%d dba_var_setd", iterations);

		for (int i = 0; i < iterations; i++)
			DBA_RUN_OR_RETURN(dba_var_setc(var, "1234"));
		timing("%d dba_var_setc", iterations);

		dba_var_delete(var);

		return dba_error_ok();
	}

public:
	enqset() : Benchmark("enqset") {}
};

class top : public Benchmark
{
public:
	top() : Benchmark("dba_var")
	{
		addChild(new create());
		addChild(new enqset());
	}
};

static RegisterRoot r(new top());

}

/* vim:set ts=4 sw=4: */
