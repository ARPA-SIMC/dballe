#ifndef DBALLE_EXPORTER_H
#define DBALLE_EXPORTER_H

#include <dballe/fwd.h>
#include <vector>
#include <memory>
#include <string>
#include <cstdio>
#include <functional>

namespace wreport {
struct Bulletin;
}

namespace dballe {

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
    ExporterOptions();

    bool operator==(const ExporterOptions&) const;
    bool operator!=(const ExporterOptions&) const;

    /// Print a summary of the options to \a out
    void print(FILE* out);

    /// Generate a string summary of export options
    std::string to_string() const;
};


/**
 * Message exporter interface
 */
class Exporter
{
protected:
    ExporterOptions opts;

    Exporter(const ExporterOptions& opts);

public:
    Exporter(const Exporter&) = delete;
    Exporter(Exporter&&) = delete;
    virtual ~Exporter();

    Exporter& operator=(const Exporter&) = delete;
    Exporter& operator=(Exporter&&) = delete;

    /**
     * Encode a message
     *
     * @param msgs
     *   Message to encode
     * @retval rmsg
     *   The resulting encoded data
     */
    virtual std::string to_binary(const std::vector<std::shared_ptr<Message>>& messages) const = 0;

    /**
     * Export to a Bulletin
     */
    virtual std::unique_ptr<wreport::Bulletin> to_bulletin(const std::vector<std::shared_ptr<Message>>& msgs) const = 0;

    /**
     * Create a bulletin that works with this exporter.
     *
     * @returns the bulletin, or NULL of this is an exporter for a format not
     * covered by Bulletin
     */
    virtual std::unique_ptr<wreport::Bulletin> make_bulletin() const;


    /// Instantiate the right importer for the given type
    static std::unique_ptr<Exporter> create(Encoding type, const ExporterOptions& opts=ExporterOptions());
};

}

#endif
