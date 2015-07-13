#include "conversion.h"
#include "processor.h"
#include "dballe/file.h"
#include "dballe/message.h"
#include "dballe/msg/msg.h"
#include "dballe/msg/context.h"
#include "dballe/msg/codec.h"

#include <wreport/bulletin.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace cmdline {

Converter::~Converter()
{
    if (file) delete file;
    if (exporter) delete exporter;
}

void Converter::process_bufrex_msg(const BinaryMessage& orig, const Bulletin& msg)
{
    string raw;
    try {
        raw = msg.encode();
    } catch (std::exception& e) {
        throw ProcessingException(orig.pathname, orig.index, e);
    }
    file->write(raw);
}

void Converter::process_dba_msg(const BinaryMessage& orig, const Messages& msgs)
{
    string raw;
    try {
        raw = exporter->to_binary(msgs);
    } catch (std::exception& e) {
        throw ProcessingException(orig.pathname, orig.index, e);
    }
    file->write(raw);
}

// Recompute data_category and data_subcategory according to WMO international values
static void compute_wmo_categories(Bulletin& b, const Bulletin& orig, const Messages& msgs)
{
    b.data_category = orig.data_category;
    b.data_subcategory_local = 255;
    switch (orig.data_category)
    {
        case 0:
        {
            // BC01-SYNOP
            // Get the hour from the first message
            // Default to 1 to simulate an odd observation time
            int hour = msgs[0].get_datetime().is_missing() ? 1 : msgs[0].get_datetime().hour;

            if ((hour % 6) == 0)
                // 002 at main synoptic times 00, 06, 12, 18 UTC,
                b.data_subcategory = 2;
            else if ((hour % 3 == 0))
                // 001 at intermediate synoptic times 03, 09, 15, 21 UTC,
                b.data_subcategory = 1;
            else
                // 000 at observation times 01, 02, 04, 05, 07, 08, 10, 11, 13, 14, 16, 17, 19, 20, 22 and 23 UTC.
                b.data_subcategory = 0;
            break;
        }
        case 1:
            // BC10-SHIP
            // If required, the international data sub-category shall be included for SHIP data as 000 at all
            // observation times 00, 01, 02, ..., 23 UTC.
            b.data_subcategory = 0;
            break;
        case 2:
            // BC20-PILOT
            // BC25-TEMP
            switch (Msg::downcast(msgs[0]).type)
            {
                // 001 for PILOT data,
                case MSG_PILOT:
                    b.data_subcategory = 1;
                    // ncdf_pilot     =  4 ,& ! indicator for proc. NetCDF PILOT (z-levels)   input
                    // ncdf_pilot_p   =  5 ,& ! indicator for proc. NetCDF PILOT (p-levels)   input
                    break;
                // 002 for PILOT SHIP data, (TODO)
                // 003 for PILOT MOBIL data. (TODO)
                // 004 for TEMP data,
                case MSG_TEMP: b.data_subcategory = 4; break;
                // 005 for TEMP SHIP data,
                case MSG_TEMP_SHIP: b.data_subcategory = 5; break;
                // 006 for TEMP MOBIL data (TODO)
                // Default to TEMP
                default: b.data_subcategory = 4; break;
                // TODO-items are not supported since I have never seen one
            }
            break;
        // Missing data from this onwards
        case 3: b.data_subcategory = 0; break;
        case 4:
            switch (Msg::downcast(msgs[0]).type)
            {
                case MSG_AIREP: b.data_subcategory = 1; break;
                default: b.data_subcategory = 0; break;
            }
            break;
        case 5: b.data_subcategory = 0; break;
        case 6: b.data_subcategory = 0; break;
        case 7: b.data_subcategory = 0; break;
        case 8: b.data_subcategory = 0; break;
        case 9: b.data_subcategory = 0; break;
        case 10: b.data_subcategory = 1; break;
        case 12: b.data_subcategory = 0; break;
        case 21: b.data_subcategory = 5; break;
        case 31: b.data_subcategory = 0; break;
        case 101: b.data_subcategory = 7; break;
        default: b.data_subcategory = 255; break;
    }
}

// Compute local data_subcategory to tell bufr2netcdf output files apart using
// lokal-specific categorisation
static void compute_bufr2netcdf_categories(Bulletin& b, const Bulletin& orig, const Messages& msgs)
{
    switch (orig.data_category)
    {
        case 0:
            // Force data_subcategory to 0, as bufr2netcdf processing doesn't need the
            // hour distinction
            b.data_subcategory = 0;
            // 13 for fixed stations
            // 14 for mobile stations
            b.data_subcategory_local = 13;
            if (const wreport::Var* v = Msg::downcast(msgs[0]).get_ident_var())
                if (v->isset())
                    b.data_subcategory_local = 14;
            break;
        case 2:
            if (b.data_subcategory == 1)
            {
                // 4 for z-level pilots
                // 5 for p-level pilots
                // Arbitrary default to z-level pilots
                const Msg& msg = Msg::downcast(msgs[0]);
                b.data_subcategory_local = 4;
                for (std::vector<msg::Context*>::const_iterator i = msg.data.begin();
                        i != msg.data.end(); ++i)
                {
                    switch ((*i)->level.ltype1)
                    {
                        case 100: // Isobaric Surface
                            b.data_subcategory_local = 5;
                            break;
                        case 102: // Specific Altitude Above Mean Sea Level
                            b.data_subcategory_local = 4;
                            break;
                    }
                }
            }
            break;
        case 4:
            switch (Msg::downcast(msgs[0]).type)
            {
                case MSG_AMDAR: b.data_subcategory_local = 8; break;
                case MSG_ACARS: b.data_subcategory_local = 9; break;
                default: break;
            }
            break;
    }
}


void Converter::process_dba_msg_from_bulletin(const BinaryMessage& orig, const Bulletin& bulletin, const Messages& msgs)
{
    string raw;
    try {
        unique_ptr<Bulletin> b1 = exporter->to_bulletin(msgs);
        if (bufr2netcdf_categories)
        {
            compute_wmo_categories(*b1, bulletin, msgs);
            compute_bufr2netcdf_categories(*b1, bulletin, msgs);
        } else {
            b1->data_category = bulletin.data_category;
            b1->data_subcategory = bulletin.data_subcategory;
            b1->data_subcategory_local = bulletin.data_subcategory_local;
        }

        raw = b1->encode();
    } catch (std::exception& e) {
        throw ProcessingException(orig.pathname, orig.index, e);
    }
    file->write(raw);
}

bool Converter::operator()(const cmdline::Item& item)
{
    if (item.msgs == NULL || item.msgs->size() == 0)
    {
        fprintf(stderr, "No interpreted information available: is a recoding enough?\n");
        // See if we can just recode the raw data

        // We want bufrex raw data
        if (item.bulletin == NULL)
        {
            fprintf(stderr, "No BUFREX raw data to attempt low-level bufrex recoding\n");
            return false;
        }

        // No report override
        if (dest_rep_memo != NULL)
        {
            fprintf(stderr, "report override not allowed for low-level bufrex recoding\n");
            return false;
        }

        // No template change
        if (dest_template != NULL)
        {
            fprintf(stderr, "template change not supported for low-level bufrex recoding\n");
            return false;
        }

        // Same encoding
        if ((file->encoding() == File::BUFR && string(item.bulletin->encoding_name()) == "CREX")
                || (file->encoding() == File::CREX && string(item.bulletin->encoding_name()) == "BUFR"))
        {
            fprintf(stderr, "encoding change not yet supported for low-level bufrex recoding\n");
            return false;
        }

        // We can just recode the raw braw
        fprintf(stderr, "we can do a low-level bufrex recoding\n");
        process_bufrex_msg(*item.rmsg, *item.bulletin);
        return true;
    }

    if (dest_rep_memo != NULL)
    {
        // Force message type (will also influence choice of template later)
        MsgType type = Msg::type_from_repmemo(dest_rep_memo);
        for (size_t i = 0; i < item.msgs->size(); ++i)
            Msg::downcast((*item.msgs)[i]).type = type;
    }

    if (item.bulletin and dest_rep_memo == NULL)
        process_dba_msg_from_bulletin(*item.rmsg, *item.bulletin, *item.msgs);
    else
        process_dba_msg(*item.rmsg, *item.msgs);

    return true;
}

}
}

/* vim:set ts=4 sw=4: */
