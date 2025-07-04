#include "file.h"
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <wreport/bulletin.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace core {

File::File(const std::string& name, FILE* fd, bool close_on_exit)
    : m_name(name), fd(fd), close_on_exit(close_on_exit), idx(0)
{
}

File::~File() { close(); }

void File::close()
{
    if (fd && close_on_exit)
    {
        fclose(fd);
        fd = nullptr;
    }
}

bool File::foreach (std::function<bool(const BinaryMessage&)> dest)
{
    while (true)
    {
        if (auto bm = read())
        {
            if (!dest(bm))
                return false;
        }
        else
            break;
    }
    return true;
}

std::string File::resolve_test_data_file(const std::string& name)
{
    // Skip appending the test data path for pathnames starting with ./
    if (name[0] == '.')
        return name;
    const char* testdatadirenv = getenv("DBA_TESTDATA");
    std::string testdatadir    = testdatadirenv ? testdatadirenv : ".";
    return testdatadir + "/" + name;
}

std::unique_ptr<dballe::File> File::open_test_data_file(Encoding type,
                                                        const std::string& name)
{
    return File::create(type, resolve_test_data_file(name), "r");
}

BinaryMessage BufrFile::read()
{
    if (fd == nullptr)
        throw error_consistency("cannot read from a closed file");
    BinaryMessage res(Encoding::BUFR);
    if (BufrBulletin::read(fd, res.data, m_name.c_str(), &res.offset))
    {
        res.pathname = m_name;
        res.index    = idx++;
        return res;
    }
    return BinaryMessage(Encoding::BUFR);
}

void BufrFile::write(const std::string& msg)
{
    if (fd == nullptr)
        throw error_consistency("cannot write to a closed file");
    BufrBulletin::write(msg, fd, m_name.c_str());
}

BinaryMessage CrexFile::read()
{
    if (fd == nullptr)
        throw error_consistency("cannot read from a closed file");
    BinaryMessage res(Encoding::CREX);
    if (CrexBulletin::read(fd, res.data, m_name.c_str(), &res.offset))
    {
        res.pathname = m_name;
        res.index    = idx++;
        return res;
    }
    return BinaryMessage(Encoding::CREX);
}

void CrexFile::write(const std::string& msg)
{
    if (fd == nullptr)
        throw error_consistency("cannot write to a closed file");
    CrexBulletin::write(msg, fd, m_name.c_str());
}

BinaryMessage JsonFile::read()
{
    if (fd == nullptr)
        throw error_consistency("cannot read from a closed file");

    BinaryMessage res(Encoding::JSON);
    long offset = ftell(fd);
    int c;
    while ((c = getc(fd)) != EOF)
    {
        if (c == '\n')
            break;
        res.data += c;
    }
    if (res.data.empty())
        return res;

    res.pathname = m_name;
    res.index    = idx++;
    res.offset   = offset;
    return res;
}

void JsonFile::write(const std::string& msg)
{
    if (fd == nullptr)
        throw error_consistency("cannot write to a closed file");
    fwrite(msg.data(), msg.size(), 1, fd);
    if (ferror(fd))
        error_system::throwf("cannot write JSON line to %s", m_name.c_str());
    // No need to add a newline, as the JSON exporter already does
    // if (putc('\n', fd) == EOF)
    //     error_system::throwf("cannot write JSON line terminator to %s",
    //     m_name.c_str());
}

} // namespace core
} // namespace dballe
