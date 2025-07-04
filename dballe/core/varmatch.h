#ifndef DBA_CORE_VARMATCH_H
#define DBA_CORE_VARMATCH_H

#include <memory>
#include <wreport/var.h>

namespace dballe {

/**
 * Match a variable code and value
 */
struct Varmatch
{
    wreport::Varcode code;

    Varmatch(wreport::Varcode code);
    virtual ~Varmatch() {}

    virtual bool operator()(const wreport::Var&) const;

    /**
     * Parse variable matcher from a string in the form
     * Bxxyyy{<|<=|=|>=|>}value or value<=Bxxyyy<=value
     */
    static std::unique_ptr<Varmatch> parse(const std::string& filter);
};

} // namespace dballe

#endif
