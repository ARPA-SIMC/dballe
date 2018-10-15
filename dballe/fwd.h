#ifndef DBALLE_FWD_H
#define DBALLE_FWD_H

namespace dballe {

/// Supported encodings
enum class Encoding {
    BUFR = 0,
    CREX = 1,
};

struct Datetime;
struct DatetimeRange;
struct Level;
struct Trange;
struct Record;
struct Coords;
struct Query;
struct Message;
struct File;
struct BinaryMessage;
struct ImporterOptions;
struct Importer;
struct ExporterOptions;
struct Exporter;
}

#endif

