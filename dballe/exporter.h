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
 * Options to control message export.
 *
 * To maintain ABI stability and allow to add options to this class, code using
 * the stable ABI cannot create objects, but need to use the create() static
 * methods.
 */
class ExporterOptions
{
public:
    /// Name of template to use for output (leave empty to autodetect)
    std::string template_name;
    /// Originating centre
    int centre = MISSING_INT;
    /// Originating subcentre
    int subcentre = MISSING_INT;
    /// Originating application ID
    int application = MISSING_INT;


    bool operator==(const ExporterOptions&) const;
    bool operator!=(const ExporterOptions&) const;

    /// Print a summary of the options to \a out
    void print(FILE* out);

    /// Generate a string summary of export options
    std::string to_string() const;

    /// Create with default values
    static std::unique_ptr<ExporterOptions> create();

    static const ExporterOptions defaults;

    friend class Exporter;

protected:
    /// Create new Options initialised with default values
    ExporterOptions() = default;
    ExporterOptions(const ExporterOptions&) = default;
    ExporterOptions(ExporterOptions&&) = default;
    ExporterOptions& operator=(const ExporterOptions&) = default;
    ExporterOptions& operator=(ExporterOptions&&) = default;
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
     * @param messages
     *   Message to encode
     * @returns
     *   The resulting encoded data
     */
    virtual std::string to_binary(const std::vector<std::shared_ptr<Message>>& messages) const = 0;

    /**
     * Export to a Bulletin
     */
    virtual std::unique_ptr<wreport::Bulletin> to_bulletin(const std::vector<std::shared_ptr<Message>>& msgs) const;

    /**
     * Create a bulletin that works with this exporter.
     *
     * @returns the bulletin, or NULL of this is an exporter for a format not
     * covered by Bulletin
     */
    virtual std::unique_ptr<wreport::Bulletin> make_bulletin() const;


    /// Instantiate the right importer for the given type
    static std::unique_ptr<Exporter> create(Encoding type, const ExporterOptions& opts=ExporterOptions::defaults);
};

class BulletinExporter : public Exporter
{
public:
    using Exporter::Exporter;

    /**
     * Export to a Bulletin
     */
    std::unique_ptr<wreport::Bulletin> to_bulletin(const std::vector<std::shared_ptr<Message>>& msgs) const override = 0;
};

}

#endif
