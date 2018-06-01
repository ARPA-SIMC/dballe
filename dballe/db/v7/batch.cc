#include "batch.h"

namespace dballe {
namespace db {
namespace v7 {

std::shared_ptr<batch::Station> Batch::get_station(const std::string& report, const Coords& coords)
{
    throw std::runtime_error("not implemented yet");
}

std::shared_ptr<batch::Station> Batch::get_station(const std::string& report, const Coords& coords, const std::string& ident)
{
    throw std::runtime_error("not implemented yet");
}

void Batch::commit()
{
    throw std::runtime_error("not implemented yet");
}

namespace batch {

void Data::add(const wreport::Var& var, bool overwrite, bool with_attrs)
{
    throw std::runtime_error("not implemented yet");
}

MeasuredData& Station::get_measured_data(int id_levtr, const Datetime& datetime)
{
    throw std::runtime_error("not implemented yet");
}

}

}
}
}
