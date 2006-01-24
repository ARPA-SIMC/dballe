#include "Benchmark.h"

#include <dballe/dba_file.h>
#include <dballe/core/dba_record.h>
#include <dballe/io/dba_rawmsg.h>
#include <dballe/bufrex/bufrex.h>
#include <dballe/bufrex/bufrex_raw.h>
#include <dballe/dba_marshal.h>

#include <vector>


/* Some utility random generator functions */

static inline int rnd(int min, int max)
{
	return min + (int) (max * (rand() / (RAND_MAX + 1.0)));
}

static inline double rnd(double min, double max)
{
	return min + (int) (max * (rand() / (RAND_MAX + 1.0)));
}

static inline std::string rnd(int len)
{
	std::string res;
	int max = rnd(1, len);
	for (int i = 0; i < max; i++)
		res += (char)rnd('a', 'z');
	return res;
}

static inline bool rnd(double prob)
{
	return (rnd(0, 100) < prob*100) ? true : false;
}

/* Random message generation functions */

const static dba_varcode generator_varcodes[] = {
	DBA_VAR(0,  1,   1),
	DBA_VAR(0,  1,   2),
	DBA_VAR(0,  1,   8),
	DBA_VAR(0,  1,  11),
	DBA_VAR(0,  1,  12),
	DBA_VAR(0,  1,  13),
	DBA_VAR(0,  2,   1),
	DBA_VAR(0,  2,   2),
	DBA_VAR(0,  2,   5),
	DBA_VAR(0,  2,  11),
	DBA_VAR(0,  2,  12),
	DBA_VAR(0,  2,  61),
	DBA_VAR(0,  2,  62),
	DBA_VAR(0,  2,  63),
	DBA_VAR(0,  2,  70),
	DBA_VAR(0,  4,   1),
	DBA_VAR(0,  4,   2),
	DBA_VAR(0,  4,   3),
	DBA_VAR(0,  4,   4),
	DBA_VAR(0,  4,   5),
	DBA_VAR(0,  5,   1),
	DBA_VAR(0,  6,   1),
	DBA_VAR(0,  7,   1),
	DBA_VAR(0,  7,   2),
	DBA_VAR(0,  7,   4),
	DBA_VAR(0,  7,  31),
	DBA_VAR(0,  8,   1),
	DBA_VAR(0,  8,   4),
	DBA_VAR(0,  8,  21),
	DBA_VAR(0, 10,   3),
	DBA_VAR(0, 10,   4),
	DBA_VAR(0, 10,  51),
	DBA_VAR(0, 10,  61),
	DBA_VAR(0, 10,  63),
	DBA_VAR(0, 10, 197),
	DBA_VAR(0, 11,   1),
	DBA_VAR(0, 11,   2),
	DBA_VAR(0, 11,   3),
	DBA_VAR(0, 11,   4),
};

class generator
{
	std::vector<dba_record> reused_pseudoana;
	std::vector<dba_record> reused_context;

public:
	~generator()
	{
		for (std::vector<dba_record>::iterator i = reused_pseudoana.begin();
				i != reused_pseudoana.end(); i++)
			dba_record_delete(*i);
		for (std::vector<dba_record>::iterator i = reused_context.begin();
				i != reused_context.end(); i++)
			dba_record_delete(*i);
	}

	dba_err fill_pseudoana(dba_record rec)
	{
		dba_record ana;
		if (reused_pseudoana.empty() || rnd(0.3))
		{
			DBA_RUN_OR_RETURN(dba_record_create(&ana));

			/* Pseudoana */
			DBA_RUN_OR_RETURN(dba_record_key_setd(ana, DBA_KEY_LAT, rnd(-180, 180)));
			DBA_RUN_OR_RETURN(dba_record_key_setd(ana, DBA_KEY_LON, rnd(-180, 180)));
			if (rnd(0.8))
			{
				DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_BLOCK, rnd(0, 99)));
				DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_STATION, rnd(0, 999)));
				DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_MOBILE, 0));
			} else {
				DBA_RUN_OR_RETURN(dba_record_key_setc(ana, DBA_KEY_IDENT, rnd(10).c_str()));
				DBA_RUN_OR_RETURN(dba_record_key_seti(ana, DBA_KEY_MOBILE, 1));
			}
			DBA_RUN_OR_RETURN(dba_record_key_setd(ana, DBA_KEY_HEIGHT, rnd(1, 3000)));
			DBA_RUN_OR_RETURN(dba_record_key_setc(ana, DBA_KEY_NAME, rnd(20).c_str()));

			reused_pseudoana.push_back(ana);
		} else {
			ana = reused_pseudoana[rnd(0, reused_pseudoana.size() - 1)];
		}
		DBA_RUN_OR_RETURN(dba_record_add(rec, ana));
		return dba_error_ok();
	}

	dba_err fill_context(dba_record rec)
	{
		dba_record ctx;
		if (reused_context.empty() || rnd(0.7))
		{
			DBA_RUN_OR_RETURN(dba_record_create(&ctx));

			/* Context */
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_YEAR, rnd(2002, 2005)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_MONTH, rnd(1, 12)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_DAY, rnd(1, 28)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_HOUR, rnd(0, 23)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_MIN, rnd(0, 59)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_LEVELTYPE, rnd(0, 300)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_L1, rnd(0, 100000)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_L2, rnd(0, 100000)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_PINDICATOR, rnd(0, 300)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_P1, rnd(0, 100000)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_P2, rnd(0, 100000)));
			DBA_RUN_OR_RETURN(dba_record_key_seti(ctx, DBA_KEY_REP_COD, rnd(0, 20)));

			reused_context.push_back(ctx);
		} else {
			ctx = reused_context[rnd(0, reused_context.size() - 1)];
		}
		DBA_RUN_OR_RETURN(dba_record_add(rec, ctx));
		return dba_error_ok();
	}

	dba_err fill_record(dba_record rec)
	{
		DBA_RUN_OR_RETURN(fill_pseudoana(rec));
		DBA_RUN_OR_RETURN(fill_context(rec));
		DBA_RUN_OR_RETURN(dba_record_var_setc(rec, generator_varcodes[rnd(0, sizeof(generator_varcodes) / sizeof(dba_varcode))], "11111"));
		return dba_error_ok();
	}
};


/* Message reading functions */

class dba_raw_consumer
{
public:
	virtual ~dba_raw_consumer() {}

	virtual dba_err consume(dba_rawmsg raw) = 0;
};

static dba_err read_file(dba_encoding type, const std::string& name, dba_raw_consumer& cons)
{
	dba_err err = DBA_OK;
	dba_file file = 0;
	dba_rawmsg raw = 0;
	int found;

	DBA_RUN_OR_GOTO(cleanup, dba_file_create(&file, type, name.c_str(), "r"));
	DBA_RUN_OR_GOTO(cleanup, dba_rawmsg_create(&raw));

	DBA_RUN_OR_GOTO(cleanup, dba_file_read_raw(file, raw, &found));
	while (found)
	{
		DBA_RUN_OR_GOTO(cleanup, cons.consume(raw));
		DBA_RUN_OR_GOTO(cleanup, dba_file_read_raw(file, raw, &found));
	}

cleanup:
	if (file) dba_file_delete(file);
	if (raw) dba_rawmsg_delete(raw);
	return err == DBA_OK ? dba_error_ok() : err;
}

class msg_vector : public dba_raw_consumer, public std::vector<dba_msg>
{
public:
	virtual ~msg_vector()
	{
		for (iterator i = begin(); i != end(); i++)
			dba_msg_delete(*i);
	}
		
	virtual dba_err consume(dba_rawmsg raw)
	{
		dba_msg msg;

		DBA_RUN_OR_RETURN(dba_marshal_decode(raw, &msg));
		push_back(msg);

		return dba_error_ok();
	}
};
	
class bufrex_vector : public dba_raw_consumer, public std::vector<bufrex_raw>
{
public:
	virtual ~bufrex_vector()
	{
		for (iterator i = begin(); i != end(); i++)
			bufrex_raw_delete(*i);
	}
		
	virtual dba_err consume(dba_rawmsg raw)
	{
		dba_err err = DBA_OK;
		bufrex_raw msg;

		switch (raw->encoding)
		{
			case BUFR:
				DBA_RUN_OR_RETURN(bufrex_raw_create(&msg, BUFREX_BUFR));
				break;
			case CREX:
				DBA_RUN_OR_RETURN(bufrex_raw_create(&msg, BUFREX_CREX));
				break;
			default:
				return dba_error_consistency("unhandled message type");
		}
		DBA_RUN_OR_GOTO(fail, bufrex_raw_decode(msg, raw));
		push_back(msg);

		return dba_error_ok();

	fail:
		bufrex_raw_delete(msg);
		return err;
	}
};

/* vim:set ts=4 sw=4: */
