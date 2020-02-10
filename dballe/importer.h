#ifndef DBALLE_IMPORTER_H
#define DBALLE_IMPORTER_H

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
 * Options to control message import.
 *
 * To maintain ABI stability and allow to add options to this class, code using
 * the stable ABI cannot create objects, but need to use the create() static
 * methods.
 */
class ImporterOptions
{
public:
    bool simplified = true;

    bool operator==(const ImporterOptions&) const;
    bool operator!=(const ImporterOptions&) const;

    /// Print a summary of the options to \a out
    void print(FILE* out);

    /// Generate a string summary of import options
    std::string to_string() const;

    /// Create with default values
    static std::unique_ptr<ImporterOptions> create();

    /// Opposite of to_string: create an Options from a string
    static std::unique_ptr<ImporterOptions> create(const std::string& s);

    /// Default importer options
    static const ImporterOptions defaults;

    friend class Importer;

protected:
    ImporterOptions() = default;
    ImporterOptions(const std::string& s);
    ImporterOptions(const ImporterOptions&) = default;
    ImporterOptions(ImporterOptions&&) = default;
    ImporterOptions& operator=(const ImporterOptions&) = default;
    ImporterOptions& operator=(ImporterOptions&&) = default;
};


/**
 * Message importer interface
 */
class Importer
{
protected:
    ImporterOptions opts;

    Importer(const ImporterOptions& opts);

public:
    Importer(const Importer&) = delete;
    Importer(Importer&&) = delete;
    virtual ~Importer();

    Importer& operator=(const Importer&) = delete;
    Importer& operator=(Importer&&) = delete;

    /**
     * Return the encoding for this importer
     */
    virtual Encoding encoding() const = 0;

    /**
     * Decode a message from its raw encoded representation
     *
     * @param msg
     *   Encoded message
     * @retval msgs
     *   The resulting messages
     */
    std::vector<std::shared_ptr<Message>> from_binary(const BinaryMessage& msg) const;

    /**
     * Import a decoded BUFR/CREX message
     */
    virtual std::vector<std::shared_ptr<Message>> from_bulletin(const wreport::Bulletin& msg) const;

    /**
     * Decode a message from its raw encoded representation, calling \a dest on
     * each resulting Message.
     *
     * Return false from \a dest to stop decoding.
     *
     * @param msg
     *   Encoded message.
     * @param dest
     *   The function that consumes the decoded messages.
     * @returns true if it got to the end of decoding, false if dest returned false.
     */
    virtual bool foreach_decoded(const BinaryMessage& msg, std::function<bool(std::unique_ptr<Message>)> dest) const = 0;

    /**
     * Instantiate an importer
     *
     * @param type
     *   The input file type
     * @param opts
     *   Options controlling import behaviour
     */
    static std::unique_ptr<Importer> create(Encoding type, const ImporterOptions& opts=ImporterOptions::defaults);

    /**
     * Instantiate an importer
     *
     * @param type
     *   The input file type
     * @param opts
     *   Options controlling import behaviour
     */
    static std::unique_ptr<Importer> create(Encoding type, const std::string& opts);
};


class BulletinImporter : public Importer
{
public:
    using Importer::Importer;

    /**
     * Import a decoded BUFR/CREX message
     */
    std::vector<std::shared_ptr<Message>> from_bulletin(const wreport::Bulletin& msg) const override = 0;
};

}

#endif
