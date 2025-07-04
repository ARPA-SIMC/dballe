#ifndef DBALLE_MSG_BULLETIN_H
#define DBALLE_MSG_BULLETIN_H

#include <cstdio>

namespace wreport {
struct Bulletin;
}

namespace dballe {
namespace msg {

/**
 * Write bulletins in CSV format to an output stream.
 *
 * Headers will only be written for the first bulletin, and will not be written
 * output_bulletin is never called.
 *
 * The output stream will be left open when the class is destroyed.
 */
class BulletinCSVWriter
{
protected:
    bool first = true;
    FILE* out;

public:
    BulletinCSVWriter(FILE* out);
    ~BulletinCSVWriter();

    void output_bulletin(const wreport::Bulletin& bulletin);
};

} // namespace msg
} // namespace dballe

#endif
