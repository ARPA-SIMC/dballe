#include <dballe++/record.h>

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

void Record::dumpToStderr()
{
	dba_record_print(m_rec, stderr);
}

}

// vim:set ts=4 sw=4:
