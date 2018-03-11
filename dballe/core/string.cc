#include "string.h"

using namespace std;

namespace dballe {

std::string url_pop_query_string(std::string& url, const std::string& name)
{
    // Look for the beginning of the query string
    size_t qs_begin = url.find('?');
    if (qs_begin == string::npos)
        return std::string();

    // Look for the beginning of the argument in the query string
    size_t arg_begin = qs_begin + 1;
    std::string arg = name + "=";
    while (true)
    {
        if (url.substr(arg_begin, arg.size()) != arg)
        {
            // Move to the next query string argument
            arg_begin = url.find('&', arg_begin);
            if (arg_begin == string::npos)
                return std::string();
            ++arg_begin;
            continue;
        }

        // We are on the right query string argument
        size_t arg_end = url.find('&', arg_begin);
        std::string res = url.substr(arg_begin + arg.size(), arg_end - (arg_begin + arg.size()));
        url.erase(arg_begin - 1, arg_end - arg_begin + 1);
        if (qs_begin < url.size() && url[qs_begin] == '&')
            url[qs_begin] = '?';
        return res;
    }
}

}
