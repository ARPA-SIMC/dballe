/*
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

#if 0
        // Above ground is ltype = 105, l1 = metri (se undef, cosa metto? Chiedo a
        // Stortini per un default?)

                switch (dba_var_code(var))
                {


cleanup:
        if (finalvar != NULL)
                dba_var_delete(finalvar);
        return err == DBA_OK ? dba_error_ok() : err;
}
#endif

#include "base.h"
#include <wreport/bulletin.h>
#include <wreport/subset.h>
#include <cmath>

using namespace wreport;
using namespace std;

namespace dballe {
namespace msg {
namespace wr {

#define MISSING_PRESS -1.0
static inline int to_h(double val)
{
        return lround(val / 9.80665);
}

class PollutionImporter : public WMOImporter
{
protected:
    Level lev;
    Trange tr;
    int valtype;
    int decscale;
    double value;
    const Var* attr_conf;
    const Var* attr_cas;
    const Var* attr_pmc;
    const Var* finalvar;

    void import_var(const Var& var);

public:
    PollutionImporter(const msg::Importer::Options& opts) : WMOImporter(opts) {}
    virtual ~PollutionImporter() {}

    virtual void init()
    {
        WMOImporter::init();
        lev = Level(103);
        tr = Trange(0);
        valtype = 0;
        decscale = MISSING_INT;
        value = 0;
        attr_conf = NULL;
        attr_cas = NULL;
        attr_pmc = NULL;
        finalvar = NULL;
    }

    virtual void run()
    {
        // Scan the input variables
        for (pos = 0; pos < subset->size(); ++pos)
        {
                const Var& var = (*subset)[pos];
                if (WR_VAR_F(var.code()) != 0) continue;
                if (var.value() != NULL)
                        import_var(var);
        }

        // Create the final pollutant variables by putting all the pieces
        // together

        // Use default level and time range if the message did not report it
        if (lev.l1 == MISSING_INT) lev.l1 = 3000;
        if (tr.p1 == MISSING_INT)
        {
                tr.p1 = -3600;
                tr.p2 = 3600;
        }

        // Create the final variable
        auto_ptr<Var> finalvar = newvar(valtype);

        // Scale the value and set it
        value = value * exp10(decscale);
        finalvar->setd(value);

        // Add the attributes
        if (attr_conf) finalvar->seta(*attr_conf);
        if (attr_cas) finalvar->seta(*attr_cas);
        if (attr_pmc) finalvar->seta(*attr_pmc);

        // Store it into the dba_msg
        msg->set(finalvar, lev, tr);
    }

    MsgType scanType(const Bulletin& bulletin) const { return MSG_POLLUTION; }
};

std::auto_ptr<Importer> Importer::createPollution(const msg::Importer::Options& opts)
{
    return auto_ptr<Importer>(new PollutionImporter(opts));
}

void PollutionImporter::import_var(const Var& var)
{
    switch (var.code())
    {
        /* For this parameter you can give up to 32 characters as a station
         * name. */
        case WR_VAR(0,  1,  19): msg->set_st_name_var(var); break;
        /* Airbase local code -- Up to 7 characters reflecting the local
         * station code supplied with the observations. If not given then
         * leave blank. */
        case WR_VAR(0,  1, 212): msg->set_poll_lcode_var(var); break;
        /* Airbase station code -- 7 character code supplied with AirBase
         * observations (see Ref 1, II.1.4, page 23). If not supplied then
         * leave blank.*/
        case WR_VAR(0,  1, 213): msg->set_poll_scode_var(var); break;
        /* GEMS code -- 6 character code suggested at RAQ Paris meeting,
         * December 2006. First 2 characters to be country code (using
         * ISO 3166-1-alpha-2 code), next 4 characters to be unique station
         * number within national boundary for each station (numbering to
         * be defined and maintained by each GEMS RAQ partner responsible
         * for collecting observations within each national boundar
         * invovled)*/
        case WR_VAR(0,  1, 214): msg->set_poll_gemscode_var(var); break;
        /*
         * Dominant emission source influencing the air pollution
         * concentrations at the station (based on Ref 1, II.2.2,  page 28)
         * Possible values are:
         * 0     traffic
         * 1     industrial
         * 2     background
         * 3-6   reserved (do not use)
         * 7     missing (or unknown)
         */
        case WR_VAR(0,  1, 215): msg->set_poll_source_var(var); break;
        /*
         * Type of area in which station is located (based on Ref 1, II.2.1, page 27)
         * Possible values are:
         * 0     urban
         * 1     suburban
         * 2     rural
         * 3-6   reserved (do not use)
         * 7     missing (or unknown)
         */
        case WR_VAR(0,  1, 216): msg->set_poll_atype_var(var); break;
        /*
         * Type of terrain in which the station is located (based on table in Ref 1, II.1.12, page 26)
         * Possible values are:
         * 0     mountain
         * 1     valley
         * 2     seaside
         * 3     lakeside
         * 4     plain
         * 5     hilly terrain
         * 6-14  reserved (do not use)
         * 15    missing (or unknown)
         */
        case WR_VAR(0,  1, 217): msg->set_poll_ttype_var(var); break;
        /*
         * Height of station above mean sea level and height of sensing
         * instrument above local ground. Both in metres. If not known than
         * can be coded as missing value.
         */
        case WR_VAR(0,  7,  30): msg->set_height_var(var); break;
        case WR_VAR(0,  7,  31): lev.l1 = var.enqi(); break;
        /* Signifies that observation is an average over a certain time period. Value set to 2. */
        case WR_VAR(0,  8,  21):
                if (var.enqi() != 2)
                    error_consistency::throwf("time significance is %d instead of 2", var.enqi());
                break;
        /*
         * Time period over which the average has been taken (in minutes),
         * e.g. -60 for average over the previous hour.  The period is
         * relative to the date/time of the observation.
         */
        case WR_VAR(0,  4,  25):
                // Convert from minutes to seconds
                tr.p1 = var.enqi() * 60;
                tr.p2 = -tr.p1;
                break;
        /*
         * VAL stands for validation and signifies that this parameter has
         * not yet reached operational status at WMO.
         * Identifier for species observed.
         * Possible values are:
         * Code    Constituent                        CAS Registry Number (if applicable)
         * 0       Ozone (O3)                         10028-15-6
         * 1       Water vapour (H2O)                 7732-18-5
         * 2       Methane (CH4)                      74-82-8
         * 3       Carbon dioxide (CO2)               37210-16-5
         * 4       Carbon monoxide (CO)               630-08-0
         * 5       Nitrogen dioxide (NO2)             10102-44-0
         * 6       Nitrous oxide (N2O)                10024-97-2
         * 7       Formaldehyde (HCHO)                50-00-0
         * 8       Sulphur dioxide (SO2)              7446-09-5
         * 9-24    reserved
         * 25      Particulate matter < 1.0 microns
         * 26      Particulate matter < 2.5 microns
         * 27      Particulate matter < 10 microns
         * 28      Aerosols (generic)
         * 29      Smoke (generic)
         * 30      Crustal material (generic)
         * 31      Volcanic ash
         * 32-200  reserved
         * 201-254 reserved for local use
         * 255     missing
         * We may have to propose some new entries to WMO if this does not
         * cover the range of constituents of the air quality observations.
         * N.B. Do not code this as missing. This is a key piece of
         * information in identifying the observed quantity.
         *
         * Our target BUFR entries are:
         *  015192 [SIM] NO Concentration     Does not fit in above table: not exported
         *  015193 [SIM] NO2 Concentration    5
         *  015194 [SIM] O3 Concentration     0
         *  015195 [SIM] PM10 Concentration   27
         */
        case WR_VAR(0,  8,  43):
            switch (var.enqi())
            {
                case  0: valtype = WR_VAR(0, 15, 194); break;
                case  4: valtype = WR_VAR(0, 15, 196); break;
                case  5: valtype = WR_VAR(0, 15, 193); break;
                case  8: valtype = WR_VAR(0, 15, 197); break;
                case 26: valtype = WR_VAR(0, 15, 198); break;
                case 27: valtype = WR_VAR(0, 15, 195); break;
                default:
                     error_consistency::throwf("cannot import constituent %d as there is no mapping for it", var.enqi());
            }
            break;
        /*
         * Chemical Abstracts Service (CAS) Registry number of constituent,
         * if applicable. This parameter is optional and can be coded as a
         * blank character string.
         */
        case WR_VAR(0,  8,  44): attr_cas = &var; break;
        /*
         * If parameter 008043 is coded as 25, 26 or 27, then this
         * parameter can be used to further categorise the nature of the
         * particulate matter.
         *
         * Possible values are:
         * 0        Particulate matter (all types)
         * 1        NO3(-)
         * 2        NH4(+)
         * 3        Na(+)
         * 4        Cl(-)
         * 5        Ca(2+)
         * 6        Mg(2+)
         * 7        K(+)
         * 8        SO4(2-)
         * 9-200    reserved
         * 201-254  reserved for local use
         * 255      missing
         */
        case WR_VAR(0,  8,  45): attr_pmc = &var; break;
        /*
         * A recent feature (still pre-operational at WMO) is the
         * introduction of scaled quanities specifically to deal with
         * quantities which may exhibit a large dynamic range. In order to
         * be able to cover a large dynamic range whilst conserving
         * precision some specific scale quantities have been introduced
         * which have an associated decimal scaling factor. The
         * example_airbase2bufr.f90 code gives an example of a method to
         * calculate the decimal scaling factor.
         */
        case WR_VAR(0,  8,  90):
            /* Someone seems to have thought that C fields in BUFR data
             * section were not crazy enough, and went on reimplementing
             * them using B fields.  So we have to reimplement the same
             * logic here.
             *
             * I'll however ignore resetting the decimal scale because in
             * this template it is only used for the measured pollutant
             * value.
             */
            if (decscale == MISSING_INT)
            {
                decscale = var.enqi();

                /*
                 * Sadly, someone seems to have decided that "all 8 bits to
                 * 1" missing data is the same as encoding -127, so they
                 * encode "all 8 bits to 0 with a reference value of -127"
                 */
                if (decscale == -127)
                        decscale = 0;
            }
            break;
        /*
         * This is the most suitable parameter (that I can find) in BUFR to
         * represent the concentration of pollutants.  The units are
         * kg/m**3. The example_airbase2bufr.f90 code gives an example of a
         * method to calculate the decimal scaling factor and scaled mass
         * density from the observed concentration. To do the backwards
         * calculation, concentration (kg/m**3)  = scaled mass density *
         * 10**(decimal scaling factor)
         */
        case WR_VAR(0, 15,  23): value = var.enqd(); break;
        /*
         * Parameter to give a qualitative measure of the quality of the
         * observation. Set at the discretion of the encoder given any
         * information they have either directly from the observation data
         * set or otherwise.
         * Possible values are:
         * 0    Data not suspect
         * 1    Data slightly suspect
         * 2    Data highly suspect
         * 3    Data considered unfit for use
         * 4-6  Reserved
         * 7    Quality information not given
         */
        case WR_VAR(0, 33,   3): attr_conf = &var; break;
        default:
                WMOImporter::import_var(var);
                break;
    }
}

} // namespace wbimporter
} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
