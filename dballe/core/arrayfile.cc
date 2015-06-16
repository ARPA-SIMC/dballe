#include "arrayfile.h"

using namespace std;

namespace dballe {
namespace core {

ArrayFile::ArrayFile(Encoding type)
    : File("array", NULL, false), file_type(type), current(0)
{
}

ArrayFile::~ArrayFile()
{
}

File::Encoding ArrayFile::encoding() const { return file_type; }

void ArrayFile::write(const std::string& msg)
{
    msgs.push_back(BinaryMessage(file_type));
    msgs.back().data = msg;
    msgs.back().pathname = m_name;
    msgs.back().offset = msgs.size() - 1;
    msgs.back().index = msgs.size() - 1;
}

BinaryMessage ArrayFile::read()
{
    if (current >= msgs.size())
        return BinaryMessage(file_type);
    else
        return msgs[current++];
}

}
}
