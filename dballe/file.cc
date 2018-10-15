#include "file.h"
#include "core/file.h"
#include <wreport/error.h>
#include <wreport/bulletin.h>
#include <wreport/utils/string.h>
#include <cstring>
#include <cstdio>
#include <algorithm>

using namespace std;
using namespace wreport;

namespace dballe {

BinaryMessage::operator bool() const { return !data.empty(); }

File::~File()
{
}

const char* File::encoding_name(File::Encoding enc)
{
    switch (enc)
    {
        case BUFR: return "BUFR";
        case CREX: return "CREX";
        default: error_notfound::throwf("unsupported encoding value %d", (int)enc);
    }
}

File::Encoding File::parse_encoding(const char* s)
{
    std::string str(s);
    return File::parse_encoding(str);
}

File::Encoding File::parse_encoding(const std::string& s)
{
    std::string str = wreport::str::upper(s);
    if (str == "BUFR") return BUFR;
    if (str == "CREX") return CREX;
    error_notfound::throwf("unsupported encoding '%s'", s.c_str());
}

unique_ptr<File> File::create(const std::string& pathname, const char* mode)
{
    FILE* fp = fopen(pathname.c_str(), mode);
    if (fp == NULL)
        error_system::throwf("opening %s with mode '%s'", pathname.c_str(), mode);
    return File::create(fp, true, pathname);
}

unique_ptr<File> File::create(File::Encoding type, const std::string& pathname, const char* mode)
{
    FILE* fp = fopen(pathname.c_str(), mode);
    if (fp == NULL)
        error_system::throwf("opening %s with mode '%s'", pathname.c_str(), mode);
    return File::create(type, fp, true, pathname);
}

namespace {

struct stream_tracker
{
    FILE* stream;
    bool close_on_exit;

    stream_tracker(FILE* stream, bool close_on_exit)
        : stream(stream), close_on_exit(close_on_exit) {}
    ~stream_tracker() { if (stream && close_on_exit) fclose(stream); }

    FILE* release()
    {
        FILE* res = stream;
        stream = nullptr;
        return res;
    }
};

}

unique_ptr<File> File::create(FILE* stream, bool close_on_exit, const std::string& name)
{
    stream_tracker st(stream, close_on_exit);

    // Auto-detect from the first character in the stream
    int c = getc(stream);

    // In case of EOF, pick any type that will handle EOF gracefully.
    if (c == EOF)
        return create(BUFR, st.release(), close_on_exit, name);

    if (ungetc(c, stream) == EOF)
        error_system::throwf("cannot put the first byte of %s back into the input stream", name.c_str());

    switch (c)
    {
        case 'B': return create(BUFR, st.release(), close_on_exit, name);
        case 'C': return create(CREX, st.release(), close_on_exit, name);
        default: throw error_notfound("could not detect the encoding of " + name);
    }
}

unique_ptr<File> File::create(File::Encoding type, FILE* stream, bool close_on_exit, const std::string& name)
{
    switch (type)
    {
        case BUFR: return unique_ptr<File>(new core::BufrFile(name, stream, close_on_exit));
        case CREX: return unique_ptr<File>(new core::CrexFile(name, stream, close_on_exit));
        default: error_consistency::throwf("cannot handle unknown file type %d", (int)type);
    }
}

}
