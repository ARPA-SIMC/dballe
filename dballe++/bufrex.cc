#include <dballe++/bufrex.h>

using namespace std;

namespace dballe {

void BufrexSubset::reset()
{
	bufrex_subset_reset(m_sub);
}

void BufrexSubset::truncate(int idx)
{
	bufrex_subset_truncate(m_sub, idx);
}

void BufrexSubset::appendvar(dba_varcode code, const Var& var)
{
	checked(bufrex_subset_store_variable_var(m_sub, code, var.var()));
}

void BufrexSubset::appendvar(const Var& var)
{
	checked(bufrex_subset_store_variable_var(m_sub, var.code(), var.var()));
}

Var BufrexSubset::getVar(int idx) const
{
	if (idx >= m_sub->vars_count)
		checked(dba_error_notfound("looking for variable %d out of %d in bufrex subset", idx, m_sub->vars_count));
	dba_var res = 0;
	try {
		checked(dba_var_copy(m_sub->vars[idx], &res));
		return Var(res);
	} catch (...) {
		if (res != 0) dba_var_delete(res);
		throw;
	}
}

Bufrex::Bufrex(bufrex_msg msg)
	: m_msg(msg)
{
	m_msg->_refcount = 1;
}

Bufrex::Bufrex(const Bufrex& var)
	: m_msg(var.m_msg)
{
	++(m_msg->_refcount);
}

Bufrex::~Bufrex()
{
	if (--(m_msg->_refcount) == 0)
		bufrex_msg_delete(m_msg);
}

Bufrex& Bufrex::operator=(const Bufrex& msg)
{
	++(msg.m_msg->_refcount);
	if (--(m_msg->_refcount) == 0)
		bufrex_msg_delete(m_msg);
	m_msg = msg.m_msg;
	return *this;
}

Bufrex Bufrex::createBUFR(int centre, int subcentre,
				           int mastertable, int localtable,
				           bool compressed)
{
	bufrex_msg msg;
	checked(bufrex_msg_create(BUFREX_BUFR, &msg));
	msg->opt.bufr.centre = centre;
	msg->opt.bufr.subcentre = subcentre;
	msg->opt.bufr.master_table = mastertable;
	msg->opt.bufr.local_table = localtable;
	msg->opt.bufr.compression = compressed ? 1 : 0;
	msg->edition = 3;
	// Wrap here, so that if load_tables throws, we cleanup properly
	Bufrex res(msg);
	checked(bufrex_msg_load_tables(msg));
	return res;
}

void Bufrex::setTemplate(int type, int subtype, int localsubtype)
{
	m_msg->type = type;
	m_msg->subtype = subtype;
	m_msg->localsubtype = localsubtype;
}

void Bufrex::setEdition(int edition)
{
	m_msg->edition = edition;
}

void Bufrex::setTime(int year, int month, int day, int hour, int minute, int second)
{
	m_msg->rep_year = year;
	m_msg->rep_month = month;
	m_msg->rep_day = day;
	m_msg->rep_hour = hour;
	m_msg->rep_minute = minute;
	m_msg->rep_second = second;
}

void Bufrex::appendDatadesc(dba_varcode code)
{
	checked(bufrex_msg_append_datadesc(m_msg, code));
}

std::string Bufrex::encode()
{
	dba_rawmsg rmsg = 0;
	try {
		checked(bufrex_msg_encode(m_msg, &rmsg));
		string res((const char*)rmsg->buf, (size_t)rmsg->len);
		dba_rawmsg_delete(rmsg);
		return res;
	} catch (...) {
		if (rmsg) dba_rawmsg_delete(rmsg);
		throw;
	}
}

#if 0
ConstSubset Bufrex::getSubset(int idx) const
{
}
#endif

BufrexSubset Bufrex::getSubset(int idx)
{
	if (idx >= m_msg->subsets_count)
		checked(dba_error_notfound("looking for BUFR/CREX subset %d out of %d", idx, m_msg->subsets_count));
	return m_msg->subsets[idx];
}

BufrexSubset Bufrex::append()
{
	bufrex_subset res;
	checked(bufrex_msg_get_subset(m_msg, m_msg->subsets_count, &res));
	return res;
}

}

// vim:set ts=4 sw=4:
