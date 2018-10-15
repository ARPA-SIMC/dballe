#ifndef DBALLE_CMDLINE_PROCESSOR_H
#define DBALLE_CMDLINE_PROCESSOR_H

#include <dballe/importer.h>
#include <dballe/msg/codec.h>
#include <stdexcept>
#include <list>
#include <string>

#define DBALLE_JSON_VERSION "0.1"

namespace wreport {
struct Bulletin;
}

namespace dballe {
struct Query;
struct BinaryMessage;
struct Matcher;

namespace cmdline {

/**
 * Exception used to embed processing issues that mean that processing of the
 * current element can safely be skipped.
 *
 * When this exception is caught we know, for example, that no output has been
 * produced for the item currently being processed.
 */
struct ProcessingException : public std::exception
{
    std::string filename;
    unsigned index;
    std::string msg;

    /**
     * Create a new exception
     *
     * @param filename Input file being processed
     * @param index Index of the data being processed in the input file
     * @param msg Error message
     */
    ProcessingException(
            const std::string& filename,
            unsigned index,
            const std::string& msg)
        : filename(filename), index(index)
    {
        initmsg(filename, index, msg.c_str());
    }

    /**
     * Create a new exception
     *
     * @param filename Input file being processed
     * @param index Index of the data being processed in the input file
     * @param original (optional) original exception that was caught from the
     *        underlying subsystem
     */
    ProcessingException(
            const std::string& filename,
            unsigned index,
            const std::exception& original)
        : filename(filename), index(index)
    {
        initmsg(filename, index, original.what());
    }

    /**
     * Create a new exception
     *
     * @param filename Input file being processed
     * @param index Index of the data being processed in the input file
     * @param msg Error message
     * @param original (optional) original exception that was caught from the
     *        underlying subsystem
     */
    ProcessingException(
            const std::string& filename,
            unsigned index,
            const std::string& msg,
            const std::exception& original)
        : filename(filename), index(index)
    {
        initmsg(filename, index, msg.c_str());
        this->msg += ": ";
        this->msg += original.what();
    }

    virtual ~ProcessingException() throw() {}

    virtual const char* what() const throw ()
    {
        return msg.c_str();
    }

protected:
    void initmsg(const std::string& fname, unsigned index, const char* msg);
};

struct Item
{
    unsigned idx;
    BinaryMessage* rmsg;
    wreport::Bulletin* bulletin;
    std::vector<std::shared_ptr<Message>>* msgs;

    Item();
    ~Item();

    /// Decode all that can be decoded
    void decode(Importer& imp, bool print_errors=false);

    /// Set the value of msgs, possibly replacing the previous one
    void set_msgs(std::vector<std::shared_ptr<Message>>* new_msgs);

    /// Throw a ProcessingException based on e
    void processing_failed(std::exception& e) const __attribute__ ((noreturn));
};

struct Action
{
    virtual ~Action() {}
    virtual bool operator()(const Item& item) = 0;
};

struct IndexMatcher
{
    std::vector<std::pair<int, int>> ranges;

    void parse(const std::string& str);

    bool match(int val) const;
};

struct ReaderOptions
{
    int category = -1;
    int subcategory = -1;
    int checkdigit = -1;
    int unparsable = 0;
    int parsable = 0;
    const char* index_filter = nullptr;
    const char* input_type = "auto";
    const char* fail_file_name = nullptr;
};

struct Filter
{
    msg::ExporterOptions export_opts;
    int category = -1;
    int subcategory = -1;
    int checkdigit = -1;
    int unparsable = 0;
    int parsable = 0;
    IndexMatcher imatcher;
    Matcher* matcher = nullptr;

    Filter();
    Filter(const ReaderOptions& opts);
    ~Filter();

    void set_index_filter(const std::string& val);

    /// Reset to the empty matcher
    void matcher_reset();

    /// Initialise the matcher from a record
    void matcher_from_record(const Query& query);

    bool match_index(int idx) const;
    bool match_common(const BinaryMessage& rmsg, const Messages* msgs) const;
    bool match_msgs(const Messages& msgs) const;
    bool match_bufrex(const BinaryMessage& rmsg, const wreport::Bulletin* rm, const Messages* msgs) const;
    bool match_bufr(const BinaryMessage& rmsg, const wreport::Bulletin* rm, const Messages* msgs) const;
    bool match_crex(const BinaryMessage& rmsg, const wreport::Bulletin* rm, const Messages* msgs) const;
    bool match_item(const Item& item) const;
};

class Reader
{
protected:
    std::string input_type;
    const char* fail_file_name;

    void read_csv(const std::list<std::string>& fnames, Action& action);
    void read_json(const std::list<std::string>& fnames, Action& action);
    void read_file(const std::list<std::string>& fnames, Action& action);

public:
    ImporterOptions import_opts;
    Filter filter;
    bool verbose = false;
    unsigned count_successes = 0;
    unsigned count_failures = 0;

    Reader(const ReaderOptions& opts);

    bool has_fail_file() const;

    void read(const std::list<std::string>& fnames, Action& action);
};

}
}
#endif
