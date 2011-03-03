/*
 * dballe/aof_codec - AOF import
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#include <config.h>

#include "aof_codec.h"
#include "aof_importers/common.h"
#include "msg.h"
#include <dballe/core/file.h>
#include <dballe/msg/msgs.h>
#include <wreport/conv.h>
//#include <dballe/core/file_internals.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <byteswap.h>
#include <time.h>
#include <errno.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {

AOFImporter::AOFImporter(const Options& opts)
    : Importer(opts) {}
AOFImporter::~AOFImporter() {}

void AOFImporter::from_rawmsg(const Rawmsg& msg, Msgs& msgs) const
{
    /* char id[10]; */
    TRACE("aof_message_decode\n");

    /* Access the raw data in a more comfortable form */
    const uint32_t* obs = (const uint32_t*)msg.data();
    int obs_len = msg.size() / sizeof(uint32_t);

    TRACE("05 grid box number: %d\n", OBS(5));
    TRACE("obs type: %d, %d\n", OBS(6), OBS(7));

    auto_ptr<Msg> out(new Msg);

#if 0
    /* 13 Station ID (1:4) */
    /* 14 Station ID (5:8) */
    /* B01011 [CHARACTER] SHIP OR MOBILE LAND STATION IDENTIFIER */
    parse_station_id(msg, id);
    TRACE("ID: %s\n", id);
    for (i = 0; i < 8 && isspace(id[i]); i++)
        /* Skip leading spaces */ ;
    DBA_RUN_OR_RETURN(aof_message_store_variable_c(msg, WR_VAR(0,  1,  11), id + i));
#endif

    /* 06 Observation type */
    /* 07 Code type */
    switch (OBS(6))
    {
        case 1: read_synop(obs, obs_len, *out); break;
        case 2: read_flight(obs, obs_len, *out); break;
        case 3: read_satob(obs, obs_len, *out); break;
        case 4: read_dribu(obs, obs_len, *out); break;
        case 5: read_temp(obs, obs_len, *out); break;
        case 6: read_pilot(obs, obs_len, *out); break;
        case 7: read_satem(obs, obs_len, *out); break;
        default:
            error_parse::throwf(msg.file.c_str(), msg.offset,
                    "cannot handle AOF observation type %d subtype %d",
                    OBS(5), OBS(6));
    }

    msgs.acquire(out);
}

void AOFImporter::from_bulletin(const wreport::Bulletin&, Msgs&) const
{
    throw error_unimplemented("AOF importer cannot import from wreport::Bulletin");
}

void AOFImporter::get_category(const Rawmsg& msg, int* category, int* subcategory)
{
    /* Access the raw data in a more comfortable form */
    const uint32_t* obs = (const uint32_t*)msg.data();
    int obs_len = msg.size() / sizeof(uint32_t);

    if (obs_len < 7)
        throw error_parse(msg.file.c_str(), msg.offset,
                "the buffer is too short to contain an AOF message");

    *category = obs[5];
    *subcategory = obs[6];
}

void AOFImporter::dump(const Rawmsg& msg, FILE* out)
{
    /* Access the raw data in a more comfortable form */
    const uint32_t* obs = (const uint32_t*)msg.data();
    int obs_len = msg.size() / sizeof(uint32_t);

    for (int i = 0; i < obs_len; i++)
        if (obs[i] == 0x7fffffff)
            fprintf(out, "%2d %10s\n", i+1, "missing");
        else
        {
            int j;
            uint32_t x = obs[i];
            fprintf(out, "%2d %10u %8x ", i+1, obs[i], obs[i]);
            for (j = 0; j < 32; j++)
            {
                fputc((x & 0x80000000) != 0 ? '1' : '0', out);
                x <<= 1;
                if ((j+1) % 8 == 0)
                    fputc(' ', out);
            }
            fputc('\n', out);
        }
}


void AOFImporter::read_satob(const uint32_t*, int, Msg&)
{
    throw error_unimplemented("parsing AOF SATOB observations");
}

void AOFImporter::read_satem(const uint32_t*, int, Msg&)
{
    throw error_unimplemented("parsing AOF SATEM observations");
}

static inline const char* stationID(const uint32_t* obs)
{
    static char id[9];
    id[0] = (OBS(13) >> 21) & 0x7f;
    id[1] = (OBS(13) >> 14) & 0x7f;
    id[2] = (OBS(13) >> 7) & 0x7f;
    id[3] = OBS(13) & 0x7f;
    id[4] = (OBS(14) >> 21) & 0x7f;
    id[5] = (OBS(14) >> 14) & 0x7f;
    id[6] = (OBS(14) >> 7) & 0x7f;
    id[7] = OBS(14) & 0x7f;
    id[8] = 0;
    return id;
}

void AOFImporter::parse_st_block_station(const uint32_t* obs, Msg& msg)
{
    const char* id = stationID(obs);
    char block[4];

    strncpy(block, id + 3, 2);
    block[2] = 0;

    msg.set_block(strtol(block, 0, 10), -1);
    msg.set_station(strtol(id + 5, 0, 10), -1);
}

void AOFImporter::parse_altitude(const uint32_t* obs, Msg& msg)
{
    /* 16 station altitude */
    if (OBS(16) != AOF_UNDEF)
        msg.set_height_station((int)OBS(16) - 1000, get_conf6((OBS(19) >> 24) & 0x3f));
}

void AOFImporter::parse_st_ident(const uint32_t* obs, Msg& msg)
{
    /* 13,14 station ID */
    const char* id = stationID(obs);
    /* Trim leading spaces */
    while (*id && isspace(*id))
        id++;
    msg.set_ident(id, -1);
}

static inline void parse_date(uint32_t val, struct tm* tm)
{
    tm->tm_mday = val % 100;
    tm->tm_mon = ((val / 100) % 100) - 1;
    tm->tm_year = (val / 10000) - 1900;
}

static inline void parse_obs_datetime(const uint32_t* obs, struct tm* t)
{
    bzero(t, sizeof (struct tm));
    /* 10 Observation date */
    parse_date(obs[9], t);
    /* 12 Exact time of observation */
    t->tm_hour = obs[11] / 100;
    t->tm_min = obs[11] % 100;
}

void AOFImporter::parse_lat_lon_datetime(const uint32_t* obs, Msg& msg)
{
    struct tm datetime;

    /* 08 Latitude */
    msg.set_latitude(((double)OBS(8) - 9000.0)/100.0, get_conf6(OBS(19) & 0x3f));
    /* 09 Longitude */
    msg.set_longitude(((double)OBS(9) - 18000.0)/100.0, get_conf6((OBS(19) >> 6) & 0x3f));

    TRACE("11 Synoptic time: %d\n", OBS(11));
    /* 10 Observation date */
    /* 12 Exact time of observation */
    parse_obs_datetime(obs, &datetime);

    msg.set_year(datetime.tm_year + 1900, get_conf6((OBS(19) >> 12) & 0x3f));
    msg.set_month(datetime.tm_mon + 1, get_conf6((OBS(19) >> 12) & 0x3f));
    msg.set_day(datetime.tm_mday, get_conf6((OBS(19) >> 12) & 0x3f));
    msg.set_hour(datetime.tm_hour, get_conf6((OBS(19) >> 18) & 0x3f));
    msg.set_minute(datetime.tm_min, get_conf6((OBS(19) >> 18) & 0x3f));
}

/* 27 Weather group word */
void AOFImporter::parse_weather_group(const uint32_t* obs, Msg& msg)
{
    int _pastw = OBS(27) & 0x7f;
    int _presw = (OBS(27) >> 7) & 0x7f;
    int _visib = OBS(27) >> 14;

    /* dump_word("Weather: ", OBS(27)); fputc('\n', stderr); */

    /* B20001 HORIZONTAL VISIBILITY: 5000.000000 M */
    if (_visib != 0x1ffff)
        msg.set_visibility((double)_visib, get_conf2(OBS(31) >> 16));

    /* B20003 PRESENT WEATHER (SEE NOTE 1): 10.000000 CODE TABLE 20003 */
    if (_presw != 0x7f)
    {
        int val = convert_WMO4677_to_BUFR20003(_presw);
        msg.set_pres_wtr(val, get_conf2(OBS(31) >> 14));
    }

    /* B20004 PAST WEATHER (1) (SEE NOTE 2): 2.000000 CODE TABLE 20004 */
    if (_pastw != 0x7f)
    {
        int val = convert_WMO4561_to_BUFR20004(_pastw);
        msg.set_past_wtr1(val, get_conf2(OBS(31) >> 12));
    }
}

/* 28 General cloud group */
void AOFImporter::parse_general_cloud_group(const uint32_t* obs, Msg& msg)
{
    /* uint32_t v = OBS(28); */
    int ch = OBS(28) & 0xf;
    int cm = (OBS(28) >>  4) & 0xf;
    int h  = (OBS(28) >>  8) & 0x7ff;
    int cl = (OBS(28) >> 19) & 0xf;
    int nh = (OBS(28) >> 23) & 0xf;
    int n  = (OBS(28) >> 27) & 0xf;
    /*{
        int x = v, i;
        fprintf(stderr, "Clouds %x: ", v);
        for (i = 0; i < 32; i++)
        {
            fprintf(stderr, "%c", (x & 0x80000000) != 0 ? '1' : '0');
            x <<= 1;
        }
        fprintf(stderr, "\n");
    }*/

    /* B20012 CLOUD TYPE: 35.000000 CODE TABLE 20012 */
    if (ch != 0xf)
    {
        /* DBA_RUN_OR_RETURN(dba_convert_WMO0513_to_BUFR20012(ch, &val)); */
        msg.set_cloud_ch(ch + 10, get_conf2(OBS(31) >> 18));
        msg.seti(WR_VAR(0, 8, 2), 3, -1, Level::cloud(259, 3), Trange::instant());
    }

    /* B20012 CLOUD TYPE: 35.000000 CODE TABLE 20012 */
    if (cm != 0xf)
    {
        /* DBA_RUN_OR_RETURN(dba_convert_WMO0513_to_BUFR20012(cm, &val)) */;
        msg.set_cloud_cm(cm + 20, get_conf2(OBS(31) >> 20));
        msg.seti(WR_VAR(0, 8, 2), 2, -1, Level::cloud(259, 2), Trange::instant());
    }

    /* B20013 HEIGHT OF BASE OF CLOUD in  M */
    if (h != 0x7ff)
    {
        msg.set_cloud_hh(h * 10.0, get_conf2(OBS(31) >> 22));
        msg.seti(WR_VAR(0, 8, 2), 1, -1, Level::cloud(258, 0), Trange::instant());
    }

    /* B20012 CLOUD TYPE: 35.000000 CODE TABLE 20012 */
    if (cl != 0xf)
    {
        /* DBA_RUN_OR_RETURN(dba_convert_WMO0513_to_BUFR20012(cl, &val)) */;
        msg.set_cloud_cl(cl + 30, get_conf2(OBS(31) >> 24));
        msg.seti(WR_VAR(0, 8, 2), 1, -1, Level::cloud(259, 1), Trange::instant());
    }

    /* B20011 CLOUD AMOUNT in CODE TABLE 20011 */
    if (nh != 0xf)
    {
        msg.seti(WR_VAR(0, 8, 2), 1, -1, Level::cloud(258, 0), Trange::instant());
        msg.set_cloud_nh(nh, get_conf2(OBS(31) >> 26));
    }

    /* B20010 CLOUD COVER (TOTAL) in % */
    if (n != 0xf)
    {
        msg.seti(WR_VAR(0, 8, 2), 1, -1, Level::cloud(258, 0), Trange::instant());
        msg.set_cloud_n(n * 100 / 8, get_conf2(OBS(31) >> 28));
    }
}

/* Decode a bit-packed cloud group */
void AOFImporter::parse_cloud_group(uint32_t val, int* ns, int* c, int* h)
{
    *ns = (val >> 19) & 0xf;
    if (*ns == 0xf) *ns = AOF_UNDEF;

    *c = (val >> 15) & 0xf;
    if (*c == 0xf)
        *c = AOF_UNDEF;
    else
        *c = convert_WMO0500_to_BUFR20012(*c);

    *h = val & 0x7fff;
    if (*h == 0x7fff) *h = AOF_UNDEF;
}

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
