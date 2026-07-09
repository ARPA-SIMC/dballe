#ifndef DBALLE_MSG_DOMAIN_ERRORS_H
#define DBALLE_MSG_DOMAIN_ERRORS_H

#include <dballe/importer.h>
#include <wreport/options.h>

namespace dballe {
namespace impl {
namespace msg {

/**
 * Locally override wreport options to match the given ImporterOptions request
 */
class WreportVarOptionsForImport
{
    bool old_silent;
#ifdef WREPORT_OPTIONS_HAS_VAR_CLAMP_DOMAIN_ERRORS
    bool old_clamp;
#endif
#ifdef WREPORT_OPTIONS_HAS_VAR_HOOK_DOMAIN_ERRORS
    wreport::options::DomainErrorHook* old_hook;
#endif

public:
    WreportVarOptionsForImport(dballe::ImporterOptions::DomainErrors val);
    ~WreportVarOptionsForImport();
};

/**
 * Hook for out of range values
 */
class TagDomainErrors : public wreport::options::DomainErrorHook
{
    void handle_domain_error_int(wreport::Var& var, int32_t val) override;
    void handle_domain_error_double(wreport::Var& var, double val) override;
};

} // namespace msg
} // namespace impl
} // namespace dballe

#endif
