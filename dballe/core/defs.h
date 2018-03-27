#ifndef DBA_MSG_DEFS_H
#define DBA_MSG_DEFS_H

/** @file
 * Common definitions
 */

#include <dballe/types.h>
#include <limits.h>
#include <string>
#include <iosfwd>

namespace dballe {

/**
 * A station identifier, that can be any string (including the empty string) or
 * a missing value.
 */
class Ident
{
protected:
    char* value = nullptr;

public:
    Ident() = default;
    Ident(const char* value);
    Ident(const Ident& o);
    Ident(Ident&& o);
    ~Ident();
    Ident& operator=(const Ident& o);
    Ident& operator=(Ident&& o);
    Ident& operator=(const char* o);
    Ident& operator=(const std::string& o);
    const char* get() const { return value; }
    void clear();
    int compare(const Ident& o) const;
    int compare(const char* o) const;
    int compare(const std::string& o) const;
    template<typename T> bool operator==(const T& o) const { return compare(o) == 0; }
    template<typename T> bool operator!=(const T& o) const { return compare(o) != 0; }
    template<typename T> bool operator<(const T& o) const  { return compare(o) < 0; }
    template<typename T> bool operator<=(const T& o) const { return compare(o) <= 0; }
    template<typename T> bool operator>(const T& o) const  { return compare(o) > 0; }
    template<typename T> bool operator>=(const T& o) const { return compare(o) >= 0; }

    bool is_missing() const { return value == nullptr; }

    operator const char*() const { return value; }
    operator std::string() const;
};

std::ostream& operator<<(std::ostream&, const Ident&);

}
#endif
