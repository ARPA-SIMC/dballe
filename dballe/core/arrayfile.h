#ifndef DBALLE_CORE_ARRAYFILE_H
#define DBALLE_CORE_ARRAYFILE_H

/** @file
 * @ingroup core
 * In-memory versions of File, to be used for testing,
 */

#include <dballe/core/file.h>
#include <vector>

namespace dballe {
namespace core {

class ArrayFile : public dballe::core::File
{
protected:
    Encoding file_type;

public:
    std::vector<BinaryMessage> msgs;
    /// Current reading offset in msgs
    unsigned current;

    ArrayFile(Encoding type);
    virtual ~ArrayFile();

    Encoding encoding() const override;
    BinaryMessage read() override;
    void write(const std::string& msg) override;
};

}
}

#endif
