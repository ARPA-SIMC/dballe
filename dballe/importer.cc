#include "importer.h"
#include "file.h"
#include "dballe/msg/wr_codec.h"
#include "dballe/msg/json_codec.h"
#include <wreport/error.h>
#include <wreport/bulletin.h>

#include "config.h"

using namespace wreport;
using namespace std;

namespace dballe {

const ImporterOptions ImporterOptions::defaults;

ImporterOptions::ImporterOptions(const std::string& s)
    : simplified(s != "accurate")
{
}

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

std::unique_ptr<ImporterOptions> ImporterOptions::create()
{
    return std::unique_ptr<ImporterOptions>(new ImporterOptions);
}

std::unique_ptr<ImporterOptions> ImporterOptions::create(const std::string& s)
{
    return std::unique_ptr<ImporterOptions>(new ImporterOptions(s));
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

std::unique_ptr<Importer> Importer::create(Encoding type, const ImporterOptions& opts)
{
    switch (type)
    {
        case Encoding::BUFR:
            return unique_ptr<Importer>(new impl::msg::BufrImporter(opts));
        case Encoding::CREX:
            return unique_ptr<Importer>(new impl::msg::CrexImporter(opts));
        case Encoding::JSON:
            return unique_ptr<Importer>(new impl::msg::JsonImporter(opts));
        default:
            error_unimplemented::throwf("%s importer is not implemented yet", File::encoding_name(type));
    }
}

std::unique_ptr<Importer> Importer::create(Encoding type, const std::string& opts)
{
    return Importer::create(type, ImporterOptions(opts));
}

}
