#include "string.h"

using namespace std;

namespace dballe {

namespace {

struct QSParser
{
    std::string& url;
    size_t qs_start = 0;
    size_t pos = 0;
    size_t value_start = 0;

    QSParser(std::string& url)
        : url(url)
    {
    }

    bool seek_qs()
    {
        pos = url.find('?');
        if (pos == string::npos)
            return false;
        ++pos;
        qs_start = pos;
        return true;
    }

    bool next_arg()
    {
        // Move to the next query string argument
        pos = url.find('&', pos);
        if (pos == string::npos)
            return false;
        ++pos;
        return true;
    }

    bool value(const std::string& key)
    {
        if (url.substr(pos, key.size()) != key)
            return false;

        if (pos + key.size() == url.size())
        {
            value_start = url.size();
            return true;
        }

        if (url[pos + key.size()] == '=')
        {
            value_start = pos + key.size() + 1;
            return true;
        }

        if (url[pos + key.size()] == '&')
        {
            value_start = pos + key.size();
            return true;
        }

        return false;
    }

    std::string pop_value()
    {
        std::string res;
        size_t value_end = url.find('&', value_start);
        if (value_end == string::npos)
        {
            res = url.substr(value_start);
            url.erase(pos);
        }
        else
        {
            res = url.substr(value_start, value_end - value_start);
            url.erase(pos, value_end - pos + 1);
        }
        // Remove trailing ? if there was only this value in the query string
        if (pos == url.size())
            url.resize(url.size() - 1);
        return res;
    }
};

}

bool url_pop_query_string(std::string& url, const std::string& name, std::string& val)
{
    QSParser parser(url);

    // Look for the beginning of the query string
    if (!parser.seek_qs())
        return false;

    // Look for the beginning of the argument in the query string
    while (true)
    {
        if (!parser.value(name))
        {
            // Move to the next query string argument
            if (!parser.next_arg())
                return false;
            continue;
        }

        val = parser.pop_value();
        return true;
    }
}

}
