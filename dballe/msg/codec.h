#ifndef DBA_MSG_CODEC_H
#define DBA_MSG_CODEC_H

#include <dballe/file.h>
#include <dballe/message.h>
#include <dballe/fwd.h>
#include <dballe/msg/fwd.h>
#include <dballe/msg/msg.h>
#include <memory>
#include <string>
#include <cstdio>
#include <functional>

/** @file
 * @ingroup msg
 * General codec options
 */

namespace wreport {
struct Bulletin;
}

namespace dballe {
namespace msg {

/**
 * Options to control message export
 */
struct ExporterOptions
{
    /// Name of template to use for output (leave empty to autodetect)
    std::string template_name;
    /// Originating centre
    int centre;
    /// Originating subcentre
    int subcentre;
    /// Originating application ID
    int application;

    /// Create new Options initialised with default values
    ExporterOptions()
        : centre(MISSING_INT), subcentre(MISSING_INT), application(MISSING_INT) {}

    /// Print a summary of the options to \a out
    void print(FILE* out);

    /// Generate a string summary of export options
    std::string to_string() const;
};


/**
 * Message exporter
 *
 * This class is designed like a configurable virtual functor.
 *
 * Exporters of various kinds can provide their implementations.
 */
class Exporter
{
protected:
    ExporterOptions opts;

public:
    Exporter(const ExporterOptions& opts);
    virtual ~Exporter();

    /**
     * Encode a message
     *
     * @param msgs
     *   Message to encode
     * @retval rmsg
     *   The resulting BinaryMessage
     */
    virtual std::string to_binary(const Messages& msgs) const = 0;

    /**
     * Export to a Bulletin
     */
    virtual std::unique_ptr<wreport::Bulletin> to_bulletin(const Messages& msgs) const = 0;

    /**
     * Create a bulletin that works with this exporter.
     *
     * @returns the bulletin, or NULL of this is an exporter for a format not
     * covered by Bulletin
     */
    virtual std::unique_ptr<wreport::Bulletin> make_bulletin() const;


    /// Instantiate the right importer for the given type
    static std::unique_ptr<Exporter> create(File::Encoding type, const ExporterOptions& opts=ExporterOptions());
};

}
}

#endif
