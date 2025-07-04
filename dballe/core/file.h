#ifndef DBA_CORE_FILE_H
#define DBA_CORE_FILE_H

#include <cstdio>
#include <dballe/core/defs.h>
#include <dballe/file.h>
#include <functional>
#include <memory>
#include <string>

namespace dballe {
namespace core {

/// Base for dballe::File implementations
class File : public dballe::File
{
protected:
    /// Name of the file
    std::string m_name;
    /// FILE structure used to read or write to the file
    FILE* fd;
    /// True if fd should be closed on destruction
    bool close_on_exit;
    /// Index of the last message read from the file or written to the file
    int idx;

public:
    File(const std::string& name, FILE* fd, bool close_on_exit = true);
    virtual ~File();

    std::string pathname() const override { return m_name; }
    void close() override;
    bool foreach (std::function<bool(const BinaryMessage&)> dest) override;

    /**
     * Resolve the location of a test data file
     *
     * This should only be used during dballe unit tests.
     */
    static std::string resolve_test_data_file(const std::string& name);

    /**
     * Open a test data file.
     *
     * This should only be used during dballe unit tests.
     */
    static std::unique_ptr<dballe::File>
    open_test_data_file(Encoding type, const std::string& name);
};

class BufrFile : public dballe::core::File
{
public:
    BufrFile(const std::string& name, FILE* fd, bool close_on_exit = true)
        : File(name, fd, close_on_exit)
    {
    }

    Encoding encoding() const override { return Encoding::BUFR; }
    BinaryMessage read() override;
    void write(const std::string& msg) override;
};

class CrexFile : public dballe::core::File
{
public:
    CrexFile(const std::string& name, FILE* fd, bool close_on_exit = true)
        : File(name, fd, close_on_exit)
    {
    }

    Encoding encoding() const override { return Encoding::CREX; }
    BinaryMessage read() override;
    void write(const std::string& msg) override;
};

class JsonFile : public dballe::core::File
{
public:
    JsonFile(const std::string& name, FILE* fd, bool close_on_exit = true)
        : File(name, fd, close_on_exit)
    {
    }

    Encoding encoding() const override { return Encoding::JSON; }
    BinaryMessage read() override;
    void write(const std::string& msg) override;
};

} // namespace core
} // namespace dballe
#endif
