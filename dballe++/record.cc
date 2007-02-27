#include <dballe++/record.h>
#include <dballe/core/aliases.h>

using namespace std;

namespace dballe {

RecordIterator::RecordIterator(dba_record rec)
    : m_rec(rec), m_kwd(DBA_KEY_ERROR), m_cur(0)
{
    // Scan to the first valid item
    ++*this;
}
bool RecordIterator::operator==(const RecordIterator& rc)
{
    if (m_rec != rc.m_rec)
        return false;
    if (m_kwd == DBA_KEY_COUNT && rc.m_kwd == DBA_KEY_COUNT)
        return m_cur == rc.m_cur;
    else
        return m_kwd == rc.m_kwd;
}
bool RecordIterator::operator!=(const RecordIterator& rc)
{
    if (m_rec != rc.m_rec)
        return true;
    if (m_kwd == DBA_KEY_COUNT && rc.m_kwd == DBA_KEY_COUNT)
        return m_cur != rc.m_cur;
    else
        return m_kwd != rc.m_kwd;
}
RecordIterator& RecordIterator::operator++()
{
    // If we terminated the iteration already, stop now
    if (m_kwd == DBA_KEY_COUNT && m_cur == 0)
        return *this;

    // Increment the keyword until we find a filled one
    while (m_kwd != DBA_KEY_COUNT)
    {
        m_kwd = (dba_keyword)((int)m_kwd + 1);
        if (dba_record_key_peek(m_rec, m_kwd) != NULL)
            break;
    }

    // We found a keyword, we can stop
    if (m_kwd != DBA_KEY_COUNT)
        return *this;

    // Else, iterate on the variables
    if (m_cur == 0)
		m_cur = dba_record_iterate_first(m_rec);
    else
        m_cur = dba_record_iterate_next(m_rec, m_cur);

    return *this;
}
RecordIterator RecordIterator::operator++(int)
{
    RecordIterator res(*this);
    ++*this;
    return res;
}
Var RecordIterator::operator*() const
{
    dba_var res;
    if (m_kwd != DBA_KEY_COUNT)
        checked(dba_record_key_enq(m_rec, m_kwd, &res));
    else
        checked(dba_var_copy(dba_record_cursor_variable(m_cur), &res));
    return res;
}

bool RecordIterator::isKeyword() const
{
    return m_kwd != DBA_KEY_COUNT;
}

const char* RecordIterator::keywordName() const
{
	return dba_record_keyword_name(m_kwd);
}

void Record::throw_notfound(const std::string& parm) const
{
	checked(dba_error_notfound("looking for field '%s'", parm.c_str()));
}
void Record::throw_notfound(dba_keyword parm) const
{
	const char* name = dba_record_keyword_name(parm);
	if (name == NULL)
		checked(dba_error_notfound("looking for field (unknown keyword %d)", parm));
	else
		checked(dba_error_notfound("looking for field \'%s\'", name));
}
void Record::throw_notfound(dba_varcode parm) const
{
	checked(dba_error_notfound("looking for field B%02d%03d", DBA_VAR_X(parm), DBA_VAR_Y(parm)));
}

void Record::set(const std::string& parm, const Var& var)
{
	if (dba_varcode code = dba_varcode_alias_resolve(parm.c_str()))
		varSet(code, var);
	else if (parm[0] != 'B')
		keySet(dba_record_keyword_byname(parm.c_str()), var);
	else
		varSet(stringToVar(parm), var);
}
void Record::seti(const std::string& parm, int value)
{
	if (dba_varcode code = dba_varcode_alias_resolve(parm.c_str()))
		varSeti(code, value);
	else if (parm[0] != 'B')
		keySeti(dba_record_keyword_byname(parm.c_str()), value);
	else
		varSeti(stringToVar(parm), value);
}
void Record::setd(const std::string& parm, double value)
{
	if (dba_varcode code = dba_varcode_alias_resolve(parm.c_str()))
		varSetd(code, value);
	else if (parm[0] != 'B')
		keySetd(dba_record_keyword_byname(parm.c_str()), value);
	else
		varSetd(stringToVar(parm), value);
}
void Record::setc(const std::string& parm, const char* value)
{
	if (dba_varcode code = dba_varcode_alias_resolve(parm.c_str()))
		varSetc(code, value);
	else if (parm[0] != 'B')
		keySetc(dba_record_keyword_byname(parm.c_str()), value);
	else
		varSetc(stringToVar(parm), value);
}
void Record::sets(const std::string& parm, const std::string& value)
{
	if (dba_varcode code = dba_varcode_alias_resolve(parm.c_str()))
		varSets(code, value);
	else if (parm[0] != 'B')
		keySets(dba_record_keyword_byname(parm.c_str()), value);
	else
		varSets(stringToVar(parm), value);
}
void Record::unset(const std::string& parm)
{
	if (dba_varcode code = dba_varcode_alias_resolve(parm.c_str()))
		varUnset(code);
	else if (parm[0] != 'B')
		keyUnset(dba_record_keyword_byname(parm.c_str()));
	else
		varUnset(stringToVar(parm));
}

void Record::dumpToStderr()
{
	dba_record_print(m_rec, stderr);
}

}

// vim:set ts=4 sw=4:
