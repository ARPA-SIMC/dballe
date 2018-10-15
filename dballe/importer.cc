#include "importer.h"
#include "dballe/msg/aof_codec.h"
#include "dballe/msg/wr_codec.h"
#include <wreport/error.h>
#include <wreport/bulletin.h>

#include "config.h"

using namespace wreport;
using namespace std;

namespace dballe {

ImporterOptions::ImporterOptions() {}

bool ImporterOptions::operator==(const ImporterOptions& o) const
{
    return simplified == o.simplified;
}

bool ImporterOptions::operator!=(const ImporterOptions& o) const
{
    return simplified != o.simplified;
}

void ImporterOptions::print(FILE* out)
{
    string str = to_string();
    fputs(str.c_str(), out);
}

std::string ImporterOptions::to_string() const
{
    string res;
    res += simplified ? "simplified" : "accurate";
    return res;
}

ImporterOptions ImporterOptions::from_string(const std::string& s)
{
    ImporterOptions res;
    if (s.empty()) return res;
    if (s == "simplified") return res;
    if (s == "accurate")
        res.simplified = false;
    return res;
}


Importer::Importer(const ImporterOptions& opts)
    : opts(opts)
{
}

Importer::~Importer()
{
}

std::vector<std::shared_ptr<Message>> Importer::from_binary(const BinaryMessage& msg) const
{
    std::vector<std::shared_ptr<Message>> res;
    foreach_decoded(msg, [&](unique_ptr<Message> m) { res.emplace_back(move(m)); return true; });
    return res;
}

std::unique_ptr<Importer> Importer::create(File::Encoding type, const ImporterOptions& opts)
{
    switch (type)
    {
        case File::BUFR:
            return unique_ptr<Importer>(new msg::BufrImporter(opts));
        case File::CREX:
            return unique_ptr<Importer>(new msg::CrexImporter(opts));
        case File::AOF:
            return unique_ptr<Importer>(new msg::AOFImporter(opts));
        default:
            error_unimplemented::throwf("%s importer is not implemented yet", File::encoding_name(type));
    }
}


}
