#include "file.h"
#include <wreport/bulletin.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace core {

File::File(const std::string& name, FILE* fd, bool close_on_exit)
    : m_name(name), fd(fd), close_on_exit(close_on_exit), idx(0)
{
}

File::~File()
{
    if (fd && close_on_exit)
        fclose(fd);
}

bool File::foreach(std::function<bool(const BinaryMessage&)> dest)
{
    while (true)
    {
        if (auto bm = read())
        {
            if (!dest(bm))
                return false;
        } else
            break;
    }
    return true;
}

std::string File::resolve_test_data_file(const std::string& name)
{
    // Skip appending the test data path for pathnames starting with ./
    if (name[0] == '.') return name;
    const char* testdatadirenv = getenv("DBA_TESTDATA");
    std::string testdatadir = testdatadirenv ? testdatadirenv : ".";
    return testdatadir + "/" + name;
}

std::unique_ptr<dballe::File> File::open_test_data_file(Encoding type, const std::string& name)
{
    return File::create(type, resolve_test_data_file(name), "r");
}

BinaryMessage BufrFile::read()
{
    BinaryMessage res(BUFR);
    if (BufrBulletin::read(fd, res.data, m_name.c_str(), &res.offset))
    {
        res.pathname = m_name;
        res.index = idx++;
        return res;
    }
    return BinaryMessage(BUFR);
}

void BufrFile::write(const std::string& msg)
{
    BufrBulletin::write(msg, fd, m_name.c_str());
}

BinaryMessage CrexFile::read()
{
    BinaryMessage res(CREX);
    if (CrexBulletin::read(fd, res.data, m_name.c_str(), &res.offset))
    {
        res.pathname = m_name;
        res.index = idx++;
        return res;
    }
    return BinaryMessage(CREX);
}

void CrexFile::write(const std::string& msg)
{
    CrexBulletin::write(msg, fd, m_name.c_str());
}

}
}
