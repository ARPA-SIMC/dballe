This is a summary of changes from DB-All.e 4.x that might require porting of
Fortran code, or scripts using the command line tools, even if the Fortran API
and the command line interfaces did not change.

Changes in tables:

 * Tables are now in /usr/share/wreport
 * Table location can be customized with two environment variables:

   WREPORT_TABLES=(colon separated list of directories)
     The primary location(s) of tables. It has a compiled in default if it is
     not set.

   WREPORT_EXTRA_TABLES=(colon separated list of directories)
     Extra location(s) of tables. It is ignored if not set.

   The reason for having two variables is in their use: if you want to override
   the compiled in default, set WREPORT_TABLES. If you want to add tables to
   the compiled-in default, set WREPORT_EXTRA_TABLES.

 * Generic BUFR messages now use table B000000002001400.txt instead of
   B000000002001200.txt

Local B table changes to follow the new WMO templates:

 * B12001 is replaced by B12101
 * B12003 is replaced by B12103
 * B10061 is replaced by B10060
 * B10003 is replaced by B10008

Added to local B table:

 * B02003 TYPE OF MEASURING EQUIPMENT USED
 * B02004 TYPE OF INSTRUMENTATION FOR EVAPORATION MEASUREMENT OR TYPE OF C
 * B02013 SOLAR AND INFRARED RADIATION CORRECTION
 * B02014 TRACKING TECHNIQUE/STATUS OF SYSTEM USED
 * B04086 LONG TIME PERIOD OR DISPLACEMENT
 * B05015 LATITUDE DISPLACEMENT (HIGH ACCURACY)
 * B06015 LONGITUDE DISPLACEMENT (HIGH ACCURACY)
 * B07007 HEIGHT
 * B08002 VERTICAL SIGNIFICANCE (SURFACE OBSERVATIONS)
 * B08042 EXTENDED VERTICAL SOUNDING SIGNIFICANCE
 * B11043 MAXIMUM WIND GUST DIRECTION
 * B11061 ABSOLUTE WIND SHEAR IN 1 KM LAYER BELOW
 * B11062 ABSOLUTE WIND SHEAR IN 1 KM LAYER ABOVE
 * B12121 GROUND MINIMUM TEMPERATURE
 * B13033 EVAPORATION/EVAPOTRANSPIRATION
 * B14031 TOTAL SUNSHINE
 * B20017 CLOUD TOP DESCRIPTION

Changes in levels and time ranges:

 * Meaningless level or timerange components now have a proper missing value
   instead of 0. The station level, for example, is now level 257,-,-,- and
   time range -,-,-

 * All cloud levels have been reorganised. For clouds levels, leveltype1 is
   always 256 and l1 is always missing. The cloud level is identified by
   leveltype2 and l2, as follows:

    Leveltype2  Description
    258         Cloud Data group, L2 = 1 low clouds, 2 middle clouds, 3 high clouds, 0 others
    259         Individual cloud groups, L2 = group number
    260         Cloud drift, L2 = group number
    261         Cloud elevation, L2 = group number
    262         Direction and elevation of clouds, L2 is ignored

Command line tools changes:

 * dbamsg dump does not show attributes trailing at the end of the message
   dump, but it only shows them attached to their variables.
 * dbamsg dump now also shows data present bitmaps as a Cxxyyy variable with a
   string value of '-' when the bitmap has 'missing' and '+' when the bitmap
   has a value.

Miscellaneous changes:

 * bound checking of variable values now uses the most restrictive range
   between BUFR and CREX (it used to use the least restrictive)
 * The minimal MySQL versione should be 5.1, or one that supports subqueries,
   to adapt to newer MySQLs dropping the old custom semanthics of GROUP BY …
   HAVING …
 * vertical significance of synop clouds is now imported as a measured value
 * importing messages with a sensor, barometer or anemometer height indicators
   will now by default import the values in the appropriate vertical level. An
   option will be soon provided for 'simplified' imports, whereby data are
   imported in standard levels with the correct height added as an attribute.


