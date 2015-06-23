#ifndef DBA_MSG_CODEC_H
#define DBA_MSG_CODEC_H

#include <dballe/file.h>
#include <dballe/message.h>
#include <memory>
#include <string>
#include <cstdio>

/** @file
 * @ingroup msg
 * General codec options
 */

namespace wreport {
struct Bulletin;
}

namespace dballe {
struct Messages;
struct Message;

namespace msg {

/**
 * Message importer
 *
 * This class is designed like a configurable virtual functor.
 *
 * Importers of various kinds can provide their implementations.
 */
class Importer
{
public:
    struct Options
    {
        bool simplified;

        /// Create new Options initialised with default values
        Options()
            : simplified(true) {}

        bool operator==(const Options& o) const { return simplified == o.simplified; }
        bool operator!=(const Options& o) const { return simplified != o.simplified; }

        /// Print a summary of the options to \a out
        void print(FILE* out);

        /// Generate a string summary of import options
        std::string to_string() const;

        /// Opposite of to_string: create an Options from a string
        static Options from_string(const std::string& s);
    };

protected:
    Options opts;

public:
    Importer(const Options& opts);
    virtual ~Importer();

    /**
     * Decode a message from its raw encoded representation
     *
     * @param rmsg
     *   Encoded message
     * @retval msgs
     *   The resulting ::dba_msg
     */
    Messages from_binary(const BinaryMessage& msg) const;

    /**
     * Decode a message from its raw encoded representation, calling \a dest on
     * each resulting Message.
     *
     * Return false from \a dest to stop decoding.
     *
     * @param rmsg
     *   Encoded message.
     * @retval dest
     *   The function that consumes the decoded messages.
     * @returns true if it got to the end of decoding, false if dest returned false.
     */
    virtual bool foreach_decoded(const BinaryMessage& msg, std::function<bool(std::unique_ptr<Message>&&)> dest) const = 0;

    /**
     * Import a decoded BUFR/CREX message
     */
    virtual Messages from_bulletin(const wreport::Bulletin& msg) const = 0;


    /// Instantiate the right importer for the given type
    static std::unique_ptr<Importer> create(File::Encoding type, const Options& opts=Options());
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
public:
    struct Options
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
        Options()
            : centre(MISSING_INT), subcentre(MISSING_INT), application(MISSING_INT) {}

        /// Print a summary of the options to \a out
        void print(FILE* out);

        /// Generate a string summary of export options
        std::string to_string() const;
    };

protected:
    Options opts;

public:
    Exporter(const Options& opts);
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
    virtual void to_bulletin(const Messages& msgs, wreport::Bulletin& msg) const = 0;

    /**
     * Create a bulletin that works with this exporter.
     *
     * @returns the bulletin, or NULL of this is an exporter for a format not
     * covered by Bulletin
     */
    virtual std::unique_ptr<wreport::Bulletin> make_bulletin() const;


    /// Instantiate the right importer for the given type
    static std::unique_ptr<Exporter> create(File::Encoding type, const Options& opts=Options());
};

}
}

#endif
