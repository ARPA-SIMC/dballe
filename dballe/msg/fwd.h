#ifndef DBALLE_MSG_FWD_H
#define DBALLE_MSG_FWD_H

namespace dballe {
class Msg;

/**
 * Source of the data
 */
enum MsgType {
    MSG_GENERIC,    /**< Data from unspecified source */
    MSG_SYNOP,      /**< Synop measured data */
    MSG_PILOT,      /**< Pilot sounding data */
    MSG_TEMP,       /**< Temp sounding data */
    MSG_TEMP_SHIP,  /**< Temp ship sounding data */
    MSG_AIREP,      /**< Airep airplane data */
    MSG_AMDAR,      /**< Amdar airplane data */
    MSG_ACARS,      /**< Acars airplane data */
    MSG_SHIP,       /**< Ship measured data */
    MSG_BUOY,       /**< Buoy measured data */
    MSG_METAR,      /**< Metar data */
    MSG_SAT,        /**< Satellite data */
    MSG_POLLUTION   /**< Pollution data */
};

namespace msg {
class Importer;
class ImporterOptions;
class Exporter;
class ExporterOptions;

}
}

#endif
