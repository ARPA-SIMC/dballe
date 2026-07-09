#ifndef DBALLE_CORE_VALUES_H
#define DBALLE_CORE_VALUES_H

#include <dballe/fwd.h>
#include <vector>
#include <wreport/var.h>

namespace dballe {
namespace core {
namespace value {

struct Encoder
{
    std::vector<uint8_t> buf;

    Encoder();
    void append_uint16(uint16_t val);
    void append_uint32(uint32_t val);
    void append_cstring(const char* val);
    void append(const wreport::Var& var);
    void append_attributes(const wreport::Var& var);
};

struct Decoder
{
    const uint8_t* buf;
    unsigned size;

    Decoder(const std::vector<uint8_t>& buf);
    uint16_t decode_uint16();
    uint32_t decode_uint32();
    const char* decode_cstring();
    std::unique_ptr<wreport::Var> decode_var();

    /**
     * Decode the attributes of var from a buffer
     */
    static void decode_attrs(const std::vector<uint8_t>& buf,
                             wreport::Var& var);
};

} // namespace value
} // namespace core
} // namespace dballe

#endif
