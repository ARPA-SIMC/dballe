#ifndef DBA_CORE_OSTREAM_H
#define DBA_CORE_OSTREAM_H

/** @file
 * Ostream output for dballe/core objects.
 *
 * This is intended to only be used for testing and debugging.
 */

#include <iosfwd>

namespace dballe {
struct Date;
struct Time;
struct Datetime;
struct DatetimeRange;
struct LatRange;
struct LonRange;
struct Level;
struct Trange;

std::ostream& operator<<(std::ostream& out, const Date& dt);
std::ostream& operator<<(std::ostream& out, const Time& t);
std::ostream& operator<<(std::ostream& out, const Datetime& dt);
std::ostream& operator<<(std::ostream& out, const DatetimeRange& dtr);
std::ostream& operator<<(std::ostream& out, const LatRange& lr);
std::ostream& operator<<(std::ostream& out, const LonRange& lr);
std::ostream& operator<<(std::ostream& out, const Level& l);
std::ostream& operator<<(std::ostream& out, const Trange& l);

}
#endif
