#ifndef DBALLE_IMPORTER_H
#define DBALLE_IMPORTER_H

#include <dballe/fwd.h>
#include <dballe/file.h>
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
 * Options to control message import
 */
struct ImporterOptions
{
    bool simplified = true;

    /// Create new Options initialised with default values
    ImporterOptions();
    ImporterOptions(const ImporterOptions&) = default;
    ImporterOptions(ImporterOptions&&) = default;
    ImporterOptions& operator=(const ImporterOptions&) = default;
    ImporterOptions& operator=(ImporterOptions&&) = default;

    bool operator==(const ImporterOptions&) const;
    bool operator!=(const ImporterOptions&) const;

    /// Print a summary of the options to \a out
    void print(FILE* out);

    /// Generate a string summary of import options
    std::string to_string() const;

    /// Opposite of to_string: create an Options from a string
    static ImporterOptions from_string(const std::string& s);
};


/**
 * Message importer interface
 */
class Importer
{
protected:
    ImporterOptions opts;

public:
    Importer(const ImporterOptions& opts);
    Importer(const Importer&) = delete;
    Importer(Importer&&) = delete;
    virtual ~Importer();

    Importer& operator=(const Importer&) = delete;
    Importer& operator=(Importer&&) = delete;

    /**
     * Decode a message from its raw encoded representation
     *
     * @param msg
     *   Encoded message
     * @retval msgs
     *   The resulting Messages
     */
    std::vector<std::shared_ptr<Message>> from_binary(const BinaryMessage& msg) const;

    /**
     * Import a decoded BUFR/CREX message
     */
    virtual std::vector<std::shared_ptr<Message>> from_bulletin(const wreport::Bulletin& msg) const = 0;

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
    static std::unique_ptr<Importer> create(File::Encoding type, const ImporterOptions& opts=ImporterOptions());
};

}

#endif
