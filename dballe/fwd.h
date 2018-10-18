#ifndef DBALLE_FWD_H
#define DBALLE_FWD_H

#include <limits.h>

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

/**
 * Value to use for missing integer values
 */
static constexpr int MISSING_INT = INT_MAX;

// Types
struct Date;
struct Time;
struct Datetime;
struct DatetimeRange;
struct Coords;
struct LatRange;
struct LonRange;
struct Level;
struct Trange;
struct Ident;

struct Record;
struct Query;
struct Message;

// File
struct File;
struct BinaryMessage;

// Importer
struct ImporterOptions;
struct Importer;

// Exporter
struct ExporterOptions;
struct Exporter;

}

#endif

