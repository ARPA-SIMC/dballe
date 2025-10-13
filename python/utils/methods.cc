#include "methods.h"
#include <string>

namespace dballe::python {

std::string build_method_doc(const char* name, const char* signature,
                             const char* returns, const char* summary,
                             const char* doc)
{
    std::string res;
    unsigned doc_indent = 0;
    if (doc)
    {
        // Look up doc indentation
        // Count the leading spaces of the first non-empty line
        unsigned indent = 0;
        for (const char* c = doc; *c; ++c)
        {
            if (isblank(*c))
                ++indent;
            else if (*c == '\n' || *c == '\r')
            {
                // strip empty lines
                doc    = c;
                indent = 0;
            }
            else
            {
                doc_indent = indent;
                break;
            }
        }
    }

    // Function name and signature
    res += name;
    res += '(';
    res += signature;
    res += ')';
    if (returns)
    {
        res += " -> ";
        res += returns;
    }
    res += "\n\n";

    // Indented summary
    if (summary)
    {
        for (unsigned i = 0; i < doc_indent; ++i)
            res += ' ';
        res += summary;
    }

    // Docstring
    if (doc)
    {
        res += "\n\n";
        res += doc;
    }

    // Return a C string with a copy of res
    return res;
}

} // namespace dballe::python
