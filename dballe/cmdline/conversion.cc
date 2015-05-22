/*
 * Copyright (C) 2005--2015  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include "conversion.h"
#include "processor.h"

#include <dballe/core/file.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/context.h>
#include <dballe/msg/codec.h>

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

void Converter::process_bufrex_msg(const Rawmsg& orig, const Bulletin& msg)
{
    Rawmsg raw;
    try {
        msg.encode(raw);
    } catch (std::exception& e) {
        throw ProcessingException(orig.file, orig.index, e);
    }
    file->write(raw);
}

void Converter::process_dba_msg(const Rawmsg& orig, const Msgs& msgs)
{
    Rawmsg raw;
    try {
        exporter->to_rawmsg(msgs, raw);
    } catch (std::exception& e) {
        throw ProcessingException(orig.file, orig.index, e);
    }
    file->write(raw);
}

// Recompute type and subtype according to WMO international values
static void compute_wmo_categories(Bulletin& b, const Bulletin& orig, const Msgs& msgs)
{
    b.type = orig.type;
    b.localsubtype = 255;
    switch (orig.type)
    {
        case 0:
        {
            // BC01-SYNOP
            // Get the hour from the first message
            // Default to 1 to simulate an odd observation time
            int hour = msgs[0]->datetime().is_missing() ? 1 : msgs[0]->datetime().time.hour;

            if ((hour % 6) == 0)
                // 002 at main synoptic times 00, 06, 12, 18 UTC,
                b.subtype = 2;
            else if ((hour % 3 == 0))
                // 001 at intermediate synoptic times 03, 09, 15, 21 UTC,
                b.subtype = 1;
            else
                // 000 at observation times 01, 02, 04, 05, 07, 08, 10, 11, 13, 14, 16, 17, 19, 20, 22 and 23 UTC.
                b.subtype = 0;
            break;
        }
        case 1:
            // BC10-SHIP
            // If required, the international data sub-category shall be included for SHIP data as 000 at all
            // observation times 00, 01, 02, ..., 23 UTC.
            b.subtype = 0;
            break;
        case 2:
            // BC20-PILOT
            // BC25-TEMP
            switch (msgs[0]->type)
            {
                // 001 for PILOT data,
                case MSG_PILOT:
                    b.subtype = 1;
                    // ncdf_pilot     =  4 ,& ! indicator for proc. NetCDF PILOT (z-levels)   input
                    // ncdf_pilot_p   =  5 ,& ! indicator for proc. NetCDF PILOT (p-levels)   input
                    break;
                // 002 for PILOT SHIP data, (TODO)
                // 003 for PILOT MOBIL data. (TODO)
                // 004 for TEMP data,
                case MSG_TEMP: b.subtype = 4; break;
                // 005 for TEMP SHIP data,
                case MSG_TEMP_SHIP: b.subtype = 5; break;
                // 006 for TEMP MOBIL data (TODO)
                // Default to TEMP
                default: b.subtype = 4; break;
                // TODO-items are not supported since I have never seen one
            }
            break;
        // Missing data from this onwards
        case 3: b.subtype = 0; break;
        case 4:
            switch (msgs[0]->type)
            {
                case MSG_AIREP: b.subtype = 1; break;
                default: b.subtype = 0; break;
            }
            break;
        case 5: b.subtype = 0; break;
        case 6: b.subtype = 0; break;
        case 7: b.subtype = 0; break;
        case 8: b.subtype = 0; break;
        case 9: b.subtype = 0; break;
        case 10: b.subtype = 1; break;
        case 12: b.subtype = 0; break;
        case 21: b.subtype = 5; break;
        case 31: b.subtype = 0; break;
        case 101: b.subtype = 7; break;
        default: b.subtype = 255; break;
    }
}

// Compute local subtype to tell bufr2netcdf output files apart using
// lokal-specific categorisation
static void compute_bufr2netcdf_categories(Bulletin& b, const Bulletin& orig, const Msgs& msgs)
{
    switch (orig.type)
    {
        case 0:
            // Force subtype to 0, as bufr2netcdf processing doesn't need the
            // hour distinction
            b.subtype = 0;
            // 13 for fixed stations
            // 14 for mobile stations
            b.localsubtype = 13;
            if (const wreport::Var* v = msgs[0]->get_ident_var())
                if (v->isset())
                    b.localsubtype = 14;
            break;
        case 2:
            if (b.subtype == 1)
            {
                // 4 for z-level pilots
                // 5 for p-level pilots
                // Arbitrary default to z-level pilots
                b.localsubtype = 4;
                for (std::vector<msg::Context*>::const_iterator i = msgs[0]->data.begin();
                        i != msgs[0]->data.end(); ++i)
                {
                    switch ((*i)->level.ltype1)
                    {
                        case 100: // Isobaric Surface
                            b.localsubtype = 5;
                            break;
                        case 102: // Specific Altitude Above Mean Sea Level
                            b.localsubtype = 4;
                            break;
                    }
                }
            }
            break;
        case 4:
            switch (msgs[0]->type)
            {
                case MSG_AMDAR: b.localsubtype = 8; break;
                case MSG_ACARS: b.localsubtype = 9; break;
                default: break;
            }
            break;
    }
}


void Converter::process_dba_msg_from_bulletin(const Rawmsg& orig, const Bulletin& bulletin, const Msgs& msgs)
{
    Rawmsg raw;
    try {
        unique_ptr<Bulletin> b1(exporter->make_bulletin());
        exporter->to_bulletin(msgs, *b1);
        if (bufr2netcdf_categories)
        {
            compute_wmo_categories(*b1, bulletin, msgs);
            compute_bufr2netcdf_categories(*b1, bulletin, msgs);
        } else {
            b1->type = bulletin.type;
            b1->subtype = bulletin.subtype;
            b1->localsubtype = bulletin.localsubtype;
        }

        b1->encode(raw);
    } catch (std::exception& e) {
        throw ProcessingException(orig.file, orig.index, e);
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
        if ((file->type() == BUFR && string(item.bulletin->encoding_name()) == "CREX")
                || (file->type() == CREX && string(item.bulletin->encoding_name()) == "BUFR"))
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
            (*item.msgs)[i]->type = type;
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
