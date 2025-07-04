#ifndef DBALLE_FORTRAN_API_H
#define DBALLE_FORTRAN_API_H

#include <dballe/fwd.h>
#include <wreport/varinfo.h>

namespace dballe {
namespace fortran {

/**
 * C++ implementation for the Fortran API.
 *
 * See the Fortran API documentation for details on the members of this class.
 */
struct API
{
    static const signed char missing_byte;
    static const int missing_int;
    static const float missing_float;
    static const double missing_double;

    virtual ~API() {}

    virtual void reinit_db(const char* repinfofile = nullptr)         = 0;
    virtual void remove_all()                                         = 0;
    virtual int enqi(const char* param)                               = 0;
    virtual signed char enqb(const char* param)                       = 0;
    virtual float enqr(const char* param)                             = 0;
    virtual double enqd(const char* param)                            = 0;
    virtual bool enqc(const char* param, char* res, unsigned res_len) = 0;
    virtual void seti(const char* param, int value)                   = 0;
    virtual void setb(const char* param, signed char value)           = 0;
    virtual void setr(const char* param, float value)                 = 0;
    virtual void setd(const char* param, double value)                = 0;
    virtual void setc(const char* param, const char* value)           = 0;
    virtual void set_station_context()                                = 0;
    virtual void enqlevel(int& ltype1, int& l1, int& ltype2, int& l2) = 0;
    virtual void setlevel(int ltype1, int l1, int ltype2, int l2)     = 0;
    virtual void enqtimerange(int& ptype, int& p1, int& p2)           = 0;
    virtual void settimerange(int ptype, int p1, int p2)              = 0;
    virtual void enqdate(int& year, int& month, int& day, int& hour, int& min,
                         int& sec)                                    = 0;
    virtual void setdate(int year, int month, int day, int hour, int min,
                         int sec)                                     = 0;
    virtual void setdatemin(int year, int month, int day, int hour, int min,
                            int sec)                                  = 0;
    virtual void setdatemax(int year, int month, int day, int hour, int min,
                            int sec)                                  = 0;
    virtual void unset(const char* param)                             = 0;
    virtual void unsetall()                                           = 0;
    virtual void unsetb()                                             = 0;
    virtual int query_stations()                                      = 0;
    virtual void next_station()                                       = 0;
    virtual int query_data()                                          = 0;
    virtual wreport::Varcode next_data()                              = 0;
    virtual void insert_data()                                        = 0;
    virtual void remove_data()                                        = 0;
    virtual int query_attributes()                                    = 0;
    virtual const char* next_attribute()                              = 0;
    virtual void insert_attributes()                                  = 0;
    virtual void remove_attributes()                                  = 0;
    virtual void messages_open_input(const char* filename, const char* mode,
                                     Encoding format,
                                     bool simplified = true)          = 0;
    virtual void messages_open_output(const char* filename, const char* mode,
                                      Encoding format)                = 0;
    virtual bool messages_read_next()                                 = 0;
    virtual void messages_write_next(const char* template_name = 0)   = 0;
    virtual const char* describe_level(int ltype1, int l1, int ltype2,
                                       int l2)                        = 0;
    virtual const char* describe_timerange(int ptype, int p1, int p2) = 0;
    virtual const char* describe_var(const char* varcode,
                                     const char* value)               = 0;
    virtual void commit()                                             = 0;

    /**
     * Wrap enqc in a way that makes it easier to use in unit tests.
     *
     * Returns nullptr if the variable is unset, or a C string otherwise. The
     * result is in a statically allocated buffer that is invalidated at the
     * next test_enqc
     */
    const char* test_enqc(const char* param, unsigned len);

    static void to_fortran(int32_t val, char* buf, unsigned buf_len);
    static void to_fortran(const char* str, char* buf, unsigned buf_len);
    static void to_fortran(const std::string& str, char* buf, unsigned buf_len);
};

} // namespace fortran
} // namespace dballe
#endif
