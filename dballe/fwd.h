#ifndef DBALLE_FWD_H
#define DBALLE_FWD_H

namespace dballe {

/// Supported encodings
enum class Encoding {
    BUFR = 0,
    CREX = 1,
};

/// Type of the data in a message
enum class MessageType {
    GENERIC   =  1,  /**< Data from unspecified source */
    SYNOP     =  2,  /**< Synop measured data */
    PILOT     =  3,  /**< Pilot sounding data */
    TEMP      =  4,  /**< Temp sounding data */
    TEMP_SHIP =  5,  /**< Temp ship sounding data */
    AIREP     =  6,  /**< Airep airplane data */
    AMDAR     =  7,  /**< Amdar airplane data */
    ACARS     =  8,  /**< Acars airplane data */
    SHIP      =  9,  /**< Ship measured data */
    BUOY      = 10,  /**< Buoy measured data */
    METAR     = 11,  /**< Metar data */
    SAT       = 12,  /**< Satellite data */
    POLLUTION = 13,  /**< Pollution data */
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

