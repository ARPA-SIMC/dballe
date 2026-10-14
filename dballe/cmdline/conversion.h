#ifndef DBALLE_CMDLINE_CONVERSION_H
#define DBALLE_CMDLINE_CONVERSION_H

#include <dballe/cmdline/processor.h>

namespace wreport {
struct Bulletin;
}

namespace dballe {
struct File;

namespace cmdline {

struct Converter : public Action
{
    File* file                  = nullptr;
    const char* dest_rep_memo   = nullptr;
    const char* dest_template   = nullptr;
    bool bufr2netcdf_categories = false;

    Converter() {}
    ~Converter();

    void set_exporter(dballe::Encoding encoding,
                      const impl::ExporterOptions& opts);

    /**
     * Convert the item as configured in the Converter, and write it to the
     * output file
     */
    bool operator()(const cmdline::Item& item) override;

protected:
    Exporter* exporter                = nullptr;
    const BulletinExporter* bexporter = nullptr;

    /**
     * Perform conversion at the encoding level only (e.g. BUFR->CREX)
     *
     * @param orig
     *   Original BinaryMessage used for its source information, to report
     * errors
     * @param msg
     *   Decoded wreport::Bulletin to to convert
     */
    void process_bufrex_msg(const BinaryMessage& orig,
                            const wreport::Bulletin& msg);

    /**
     * Perform conversion of decoded data, auto-inferring
     * type/subtype/localsubtype from the Messages contents
     */
    void
    process_dba_msg(const BinaryMessage& orig,
                    const std::vector<std::shared_ptr<dballe::Message>>& msgs);

    /**
     * Perform conversion of decded data, using the original bulletin for
     * type/subtype/localsubtype information
     */
    void process_dba_msg_from_bulletin(
        const BinaryMessage& orig, const wreport::Bulletin& bulletin,
        const std::vector<std::shared_ptr<dballe::Message>>& msgs);
};

} // namespace cmdline
} // namespace dballe

#endif
