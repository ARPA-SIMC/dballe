#ifndef DBALLE_FILE_H
#define DBALLE_FILE_H

#include <dballe/fwd.h>
#include <functional>
#include <iosfwd>
#include <memory>
#include <string>

namespace dballe {

/**
 * File object for doing I/O on binary message streams.
 *
 * It provides a unified interface to read and write messages to files.
 */
class File
{
public:
    virtual ~File();

    /// Get the file pathname
    virtual std::string pathname() const = 0;

    /// Get the file encoding
    virtual Encoding encoding() const = 0;

    /**
     * Close the underlying file
     */
    virtual void close() = 0;

    /**
     * Read a message from the file.
     *
     * @return
     *   the BinaryMessage with the binary data that have been read, or nullptr
     *   when the end of file has been reached.
     */
    virtual BinaryMessage read() = 0;

    /**
     * Read all the messages from the file, calling the function on each of
     * them.
     *
     * If @a dest returns false, reading will stop.
     *
     * @return
     *   true if all file was read, false if reading was stopped because
     *   @a dest returned false.
     */
    virtual bool foreach (std::function<bool(const BinaryMessage&)> dest) = 0;

    /// Append the binary message to the file
    virtual void write(const std::string& msg) = 0;

    /**
     * Open a file from the filesystem, autodetecting the encoding type.
     *
     * @param pathname
     *   The pathname of the file to access.
     * @param mode
     *   The opening mode of the file, as used by fopen.
     * @returns
     *   The new File object.
     */
    static std::unique_ptr<File> create(const std::string& pathname,
                                        const char* mode);

    /**
     * Open a file from the filesystem.
     *
     * @param type
     *   The type of data contained in the file.
     * @param pathname
     *   The pathname of the file to access.
     * @param mode
     *   The opening mode of the file, as used by fopen.
     * @returns
     *   The new File object.
     */
    static std::unique_ptr<File>
    create(Encoding type, const std::string& pathname, const char* mode);

    /**
     * Create a File from an existing FILE* stream, autodetecting the encoding
     * type.
     *
     * @param file
     *   The FILE* stream for the file to access.
     * @param close_on_exit
     *   If true, fclose() will be called on the stream when the File object is
     *   destroyed.
     * @param name
     *   Pathname or description of the stream, used when generating error
     *   messages
     * @returns
     *   The new File object.
     */
    static std::unique_ptr<File> create(FILE* file, bool close_on_exit,
                                        const std::string& name = "(fp)");

    /**
     * Create a File from an existing FILE* stream.
     *
     * @param type
     *   The type of data contained in the file.
     * @param file
     *   The FILE* stream for the file to access.
     * @param close_on_exit
     *   If true, fclose() will be called on the stream when the File object is
     *   destroyed.
     * @param name
     *   Pathname or description of the stream, used when generating error
     *   messages
     * @returns
     *   The new File object.
     */
    static std::unique_ptr<File> create(Encoding type, FILE* file,
                                        bool close_on_exit,
                                        const std::string& name = "(fp)");

    /// Return a string with the name of this encoding
    static const char* encoding_name(Encoding enc);

    /// Return the Encoding corresponding to the given name
    static Encoding parse_encoding(const char* s);

    /// Return the Encoding corresponding to the given name
    static Encoding parse_encoding(const std::string& s);
};

/// Binary message
class BinaryMessage
{
public:
    /// Format of the binary data
    Encoding encoding;

    /// Binary message data
    std::string data;

    /**
     * Pathname of the file from where the BinaryMessage has been read.  It can
     * be empty when not applicable, such as when the message is created from
     * scratch and not yet written
     */
    std::string pathname;

    /// Start offset of this message inside the file
    off_t offset = (off_t)-1;

    /// Index of the message from the beginning of the file
    int index = MISSING_INT;

    BinaryMessage(Encoding encoding) : encoding(encoding) {}
    BinaryMessage(const BinaryMessage&)            = default;
    BinaryMessage(BinaryMessage&&)                 = default;
    BinaryMessage& operator=(const BinaryMessage&) = default;
    BinaryMessage& operator=(BinaryMessage&&)      = default;

    /// Return true if the message is not empty
    operator bool() const;
};

/// Serialize Encoding
std::ostream& operator<<(std::ostream&, const dballe::Encoding&);

} // namespace dballe

#endif
