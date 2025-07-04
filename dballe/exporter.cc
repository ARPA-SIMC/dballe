#include "exporter.h"
#include "dballe/msg/json_codec.h"
#include "dballe/msg/wr_codec.h"
#include "file.h"
#include <wreport/bulletin.h>
#include <wreport/error.h>

#include "config.h"

using namespace wreport;
using namespace std;

namespace dballe {

const ExporterOptions ExporterOptions::defaults;

std::unique_ptr<ExporterOptions> ExporterOptions::create()
{
    return std::unique_ptr<ExporterOptions>(new ExporterOptions);
}

bool ExporterOptions::operator==(const ExporterOptions& o) const
{
    return std::tie(template_name, centre, subcentre, application) ==
           std::tie(o.template_name, o.centre, o.subcentre, o.application);
}

bool ExporterOptions::operator!=(const ExporterOptions& o) const
{
    return std::tie(template_name, centre, subcentre, application) !=
           std::tie(o.template_name, o.centre, o.subcentre, o.application);
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
        if (!res.empty())
            res += ", ";
        snprintf(buf, 100, "centre %d", centre);
        res += buf;
    }

    if (subcentre != MISSING_INT)
    {
        if (!res.empty())
            res += ", ";
        snprintf(buf, 100, "subcentre %d", subcentre);
        res += buf;
    }

    if (application != MISSING_INT)
    {
        if (!res.empty())
            res += ", ";
        snprintf(buf, 100, "application %d", application);
        res += buf;
    }

    return res;
}

Exporter::Exporter(const ExporterOptions& opts) : opts(opts) {}

Exporter::~Exporter() {}

std::unique_ptr<wreport::Bulletin>
Exporter::to_bulletin(const std::vector<std::shared_ptr<Message>>& msgs) const
{
    throw wreport::error_unimplemented(
        "this exporter cannot generate bulletins");
}

std::unique_ptr<wreport::Bulletin> Exporter::make_bulletin() const
{
    return std::unique_ptr<wreport::Bulletin>(nullptr);
}

std::unique_ptr<Exporter> Exporter::create(Encoding type,
                                           const ExporterOptions& opts)
{
    switch (type)
    {
        case Encoding::BUFR:
            return unique_ptr<Exporter>(new impl::msg::BufrExporter(opts));
        case Encoding::CREX:
            return unique_ptr<Exporter>(new impl::msg::CrexExporter(opts));
        case Encoding::JSON:
            return unique_ptr<Exporter>(new impl::msg::JsonExporter(opts));
        default:
            error_unimplemented::throwf("%s exporter is not implemented yet",
                                        File::encoding_name(type));
    }
}

} // namespace dballe
