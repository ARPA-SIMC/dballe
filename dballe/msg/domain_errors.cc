#include "domain_errors.h"
#include <wreport/options.h>
#include <wreport/var.h>
#include <dballe/var.h>

namespace dballe {
namespace impl {
namespace msg {

void TagDomainErrors::handle_domain_error_int(wreport::Var& var, int32_t val)
{
    var.set(val < var.info()->imin ? var.info()->imin : var.info()->imax);
    var.seta(newvar(WR_VAR(0, 33, 192), 0));
}

void TagDomainErrors::handle_domain_error_double(wreport::Var& var, double val)
{
    var.set(val < var.info()->dmin ? var.info()->dmin : var.info()->dmax);
    var.seta(newvar(WR_VAR(0, 33, 192), 0));
}

TagDomainErrors domain_errors_tag;


WreportVarOptionsForImport::WreportVarOptionsForImport(dballe::ImporterOptions::DomainErrors val)
{
    old_silent = wreport::options::var_silent_domain_errors;
    wreport::options::var_silent_domain_errors = false;
#ifdef WREPORT_OPTIONS_HAS_VAR_CLAMP_DOMAIN_ERRORS
    old_clamp = wreport::options::var_clamp_domain_errors;
    wreport::options::var_clamp_domain_errors = false;
#endif
#ifdef WREPORT_OPTIONS_HAS_VAR_HOOK_DOMAIN_ERRORS
    old_hook = wreport::options::var_hook_domain_errors;
    wreport::options::var_hook_domain_errors = nullptr;
#endif

    switch (val)
    {
        case ImporterOptions::DomainErrors::THROW:
            break;
        case ImporterOptions::DomainErrors::UNSET:
            wreport::options::var_silent_domain_errors = true;
            break;
        case ImporterOptions::DomainErrors::CLAMP:
#ifdef WREPORT_OPTIONS_HAS_VAR_CLAMP_DOMAIN_ERRORS
            wreport::options::var_clamp_domain_errors = true;
#else
            throw std::runtime_error("the current version of wreport does not support clamping domain errors");
#endif
            break;
        case ImporterOptions::DomainErrors::TAG:
#ifdef WREPORT_OPTIONS_HAS_VAR_HOOK_DOMAIN_ERRORS
            wreport::options::var_hook_domain_errors = &domain_errors_tag;
#else
            throw std::runtime_error("the current version of wreport does not support domain error hooks");
#endif
            break;
    }
}

WreportVarOptionsForImport::~WreportVarOptionsForImport()
{
    wreport::options::var_silent_domain_errors = old_silent;
#ifdef WREPORT_OPTIONS_HAS_VAR_CLAMP_DOMAIN_ERRORS
    wreport::options::var_clamp_domain_errors = old_clamp;
#endif
#ifdef WREPORT_OPTIONS_HAS_VAR_HOOK_DOMAIN_ERRORS
    wreport::options::var_hook_domain_errors = old_hook;
#endif
}

}
}
}
