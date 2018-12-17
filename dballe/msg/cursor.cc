#include "cursor.h"
#include "dballe/core/fortran.h"

namespace dballe {
namespace impl {
namespace msg {

bool CursorStation::enqi(const char* key, unsigned len, int& res) const
{
    impl::Enqi enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

bool CursorStation::enqd(const char* key, unsigned len, double& res) const
{
    impl::Enqd enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

bool CursorStation::enqs(const char* key, unsigned len, std::string& res) const
{
    impl::Enqs enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

bool CursorStation::enqf(const char* key, unsigned len, std::string& res) const
{
    impl::Enqf enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}


bool CursorStationData::enqi(const char* key, unsigned len, int& res) const
{
    impl::Enqi enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

bool CursorStationData::enqd(const char* key, unsigned len, double& res) const
{
    impl::Enqd enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

bool CursorStationData::enqs(const char* key, unsigned len, std::string& res) const
{
    impl::Enqs enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

bool CursorStationData::enqf(const char* key, unsigned len, std::string& res) const
{
    impl::Enqf enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}


bool CursorData::enqi(const char* key, unsigned len, int& res) const
{
    impl::Enqi enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

bool CursorData::enqd(const char* key, unsigned len, double& res) const
{
    impl::Enqd enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

bool CursorData::enqs(const char* key, unsigned len, std::string& res) const
{
    impl::Enqs enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

bool CursorData::enqf(const char* key, unsigned len, std::string& res) const
{
    impl::Enqf enq(key, len);
    enq_generic(enq);
    if (enq.missing)
        return false;
    res = enq.res;
    return true;
}

}
}
}

#include "cursor-access.tcc"
