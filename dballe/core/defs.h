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
 * Supported encodings
 */
typedef enum {
	BUFR = 0,
	CREX = 1,
	AOF = 2,
} Encoding;

const char* encoding_name(Encoding enc);
Encoding parse_encoding(const std::string& s);

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

std::ostream& operator<<(std::ostream& out, const Coords& c);
std::ostream& operator<<(std::ostream& out, const Date& dt);
std::ostream& operator<<(std::ostream& out, const Time& t);
std::ostream& operator<<(std::ostream& out, const Datetime& dt);
std::ostream& operator<<(std::ostream& out, const DatetimeRange& dtr);
std::ostream& operator<<(std::ostream& out, const LatRange& lr);
std::ostream& operator<<(std::ostream& out, const LonRange& lr);
std::ostream& operator<<(std::ostream& out, const Level& l);
std::ostream& operator<<(std::ostream& out, const Trange& l);
std::ostream& operator<<(std::ostream& out, const Ident& i);

}

#endif
