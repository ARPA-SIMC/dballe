#ifndef DBALLE_CMDLINE_CONVERSION_H
#define DBALLE_CMDLINE_CONVERSION_H

#include <dballe/cmdline/processor.h>

namespace wreport {
struct Bulletin;
}

namespace dballe {
struct File;

namespace msg {
struct Importer;
struct Exporter;
}

namespace cmdline {

struct Converter : public Action
{
    File* file;
    const char* dest_rep_memo;
    const char* dest_template;
    bool bufr2netcdf_categories;

    msg::Exporter* exporter;

    Converter() : file(0), dest_rep_memo(0), dest_template(0), bufr2netcdf_categories(false), exporter(0) {}
    ~Converter();

    /**
     * Convert the item as configured in the Converter, and write it to the
     * output file
     */
    virtual bool operator()(const cmdline::Item& item);

protected:
    /**
     * Perform conversion at the encoding level only (e.g. BUFR->CREX)
     *
     * @param orig
     *   Original BinaryMessage used for its source information, to report errors
     */
    void process_bufrex_msg(const BinaryMessage& orig, const wreport::Bulletin& msg);

    /**
     * Perform conversion of decoded data, auto-inferring
     * type/subtype/localsubtype from the Msgs contents
     */
    void process_dba_msg(const BinaryMessage& orig, const Msgs& msgs);

    /**
     * Perform conversion of decded data, using the original bulletin for
     * type/subtype/localsubtype information
     */
    void process_dba_msg_from_bulletin(const BinaryMessage& orig, const wreport::Bulletin& bulletin, const Msgs& msgs);
};

}
}

#endif
