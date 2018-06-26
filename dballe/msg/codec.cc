#include "codec.h"
#include "aof_codec.h"
#include "wr_codec.h"
#include <wreport/error.h>
#include <wreport/bulletin.h>

#include "config.h"

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {

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

Messages Importer::from_binary(const BinaryMessage& msg) const
{
    Messages res;
    foreach_decoded(msg, [&](unique_ptr<Message>&& m) { res.append(move(m)); return true; });
    return res;
}

std::unique_ptr<Importer> Importer::create(File::Encoding type, const ImporterOptions& opts)
{
    switch (type)
    {
        case File::BUFR:
            return unique_ptr<Importer>(new BufrImporter(opts));
        case File::CREX:
            return unique_ptr<Importer>(new CrexImporter(opts));
        case File::AOF:
            return unique_ptr<Importer>(new AOFImporter(opts));
        default:
            error_unimplemented::throwf("%s importer is not implemented yet", File::encoding_name(type));
    }
}


void ExporterOptions::print(FILE* out)
{
    string str = to_string();
    fputs(str.c_str(), out);
}

std::string ExporterOptions::to_string() const
{
    string res;
    char buf[100];

    if (!template_name.empty())
        res += "tpl " + template_name;

    if (centre != MISSING_INT)
    {
        if (!res.empty()) res += ", ";
        snprintf(buf, 100, "centre %d", centre);
        res += buf;
    }

    if (subcentre != MISSING_INT)
    {
        if (!res.empty()) res += ", ";
        snprintf(buf, 100, "subcentre %d", subcentre);
        res += buf;
    }

    if (application != MISSING_INT)
    {
        if (!res.empty()) res += ", ";
        snprintf(buf, 100, "application %d", application);
        res += buf;
    }

    return res;
}


Exporter::Exporter(const ExporterOptions& opts)
    : opts(opts)
{
}

Exporter::~Exporter()
{
}

std::unique_ptr<wreport::Bulletin> Exporter::make_bulletin() const
{
    return std::unique_ptr<wreport::Bulletin>(nullptr);
}

std::unique_ptr<Exporter> Exporter::create(File::Encoding type, const ExporterOptions& opts)
{
    switch (type)
    {
        case File::BUFR:
            return unique_ptr<Exporter>(new BufrExporter(opts));
        case File::CREX:
            return unique_ptr<Exporter>(new CrexExporter(opts));
        case File::AOF:
            //return unique_ptr<Exporter>(new AOFExporter(opts));
        default:
            error_unimplemented::throwf("%s exporter is not implemented yet", File::encoding_name(type));
    }
}

}
}
