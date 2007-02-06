/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005,2006  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#define _GNU_SOURCE
/* _GNU_SOURCE is defined to have asprintf */

#include "formatter.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>     /* strtoul, getenv */

dba_err dba_formatter_describe_level(int ltype, int l1, int l2, char** res)
{
	switch (ltype)
	{
		case 1:		*res = strdup("Ground or water surface"); break;
		case 2:		*res = strdup("Cloud base level"); break;
		case 3:		*res = strdup("Level of cloud tops"); break;
		case 4:		*res = strdup("Level of 0°C isotherm"); break;
		case 5:		*res = strdup("Level of adiabatic condensation lifted from the surface"); break;
		case 6:		*res = strdup("Maximum wind level"); break;
		case 7:		*res = strdup("Tropopause"); break;
		case 8:		*res = strdup("Nominal top of atmosphere"); break;
		case 9:		*res = strdup("Sea bottom"); break;
		case 20:	asprintf(res, "Isothermal level, %.2f K", (double)l1/100); break;
		case 100:	asprintf(res, "Isobaric surface, %d hPa", l1); break;
		case 101:	asprintf(res, "Layer between the two isobaric surfaces %d kPa and %d kPa", l1, l2); break;
		case 102:	*res = strdup("Mean sea level"); break;
		case 103:	asprintf(res, "Specified altitude above mean sea level, %dm", l1); break;
		case 104:	asprintf(res, "Layer between %d00m and %d00m above mean sea level", l1, l2); break;
		case 105:	asprintf(res, "Specified height above ground, %dm", l1); break;
		case 106:	asprintf(res, "Layer between %dhm and %dhm above ground", l1, l2); break;
		case 107:	asprintf(res, "Sigma level %d (1/10000)", l1); break;
		case 108:	asprintf(res, "Layer between the two sigma levels %d and %d (1/100)", l1, l2); break;
		case 109:	asprintf(res, "Hybrid level %d", l1); break;
		case 110:	asprintf(res, "Layer between the two hybrid levels %d and %d", l1, l2); break;
		case 111:	asprintf(res, "%dcm below land surface", l1); break;
		case 112:	asprintf(res, "Between %dcm and %dcm below land surface", l1, l2); break;
		case 113:	asprintf(res, "Isentropic (theta) level, potential temperature %dK", l1); break;
/*		case 114:	asprintf(res, "Layer between two isentropic levels & 475 K minus theta of top in K & 475 K minus theta of bottom in K */
		case 115:	asprintf(res, "Pressure difference %dhPa from ground to level", l1); break;
		case 116:	asprintf(res, "Between pressure difference %dhPa and %dhPa from ground to level", l1, l2); break;
/* 		case 117:	asprintf(res, "Potential vorticity surface & 10-9 K m2 kg-1 s-1 */
/*		case 119:	asprintf(res, "ETA* level & ETA value in 1/10000 (2 octets) */
/*		case 120:	asprintf(res, "Layer between two ETA* levels & ETA value at top of layer in 1/100 & ETA value at bottom of layer in 1/100 */
/*		case 121:	asprintf(res, "Layer between two isobaric surfaces (high precision) & 1100 hPa minus pressure of top in hPa & 1100 hPa minus pressure of bottom in hPa */
		case 125:	asprintf(res, "%dcm above ground", l1); break;
/* 		case 128:	asprintf(res, "Layer between two sigma levels (high precision) & 1.1 minus sigma of top in 1/1000 of sigma & 1.1 minus sigma of bottom in 1/1000 of sigma */
/*		case #141:	asprintf(res, "Layer between two isobaric surfaces (mixed precision) & Pressure of top in kPa & 1100 hPa minus pressure of bottom in hPa */
		case 160:	asprintf(res, "%dm below sea level", l1); break;
		case 200:	*res = strdup("Entire atmosphere (considered as a single layer)"); break;
		case 201:	*res = strdup("Entire ocean (considered as a single layer)"); break;
		case 256:
			switch (l1) {
				case 0:
					switch (l2) {
						case 0: *res = strdup("General cloud group"); break;
						case 1: *res = strdup("CL"); break;
						case 2: *res = strdup("CM"); break;
						case 3: *res = strdup("CH"); break;
						default: asprintf(res, "%d %d %d", ltype, l1, l2); break;
					}
				case 1: *res = strdup("First synop cloud group"); break;
				case 2: *res = strdup("Second synop cloud group"); break;
				case 3: *res = strdup("Third synop cloud group"); break;
				case 4: *res = strdup("Fourth synop cloud group"); break;
				default: asprintf(res, "%d %d %d", ltype, l1, l2); break;
			}
			break;
		case 257:	*res = strdup("Extra anagraphical information"); break;
		default:	asprintf(res, "%d %d %d", ltype, l1, l2); break;
	}

	return dba_error_ok();
}

dba_err dba_formatter_describe_trange(int ptype, int p1, int p2, char** res)
{
	switch (ptype)
	{
		default:	asprintf(res, "%d %d %d", ptype, p1, p2); break;
	}
	/*
#\begin{description}
#\item [0]
#  Forecast product valid for reference time + P1 (P1 > 0), or Uninitialized
#  analysis product for reference time (P1 = 0), or Image product for reference
#  time (P1 = 0).
#\item [1]
#  Initialized analysis product for reference time (P1 = 0).
#\item [2]
#  Product with a valid time ranging between reference time + P1 and reference time + P2.
  3 Average between reference time+%ds to reference time+%ds.
#\item [4]
#  Accumulation (reference time + P1 to reference time + P2) product
#  considered valid at reference time + P2.
#\item [5]
#  Difference (reference time + P2 minus reference time + P1) product
#  considered valid at reference time + P2.
#\item [6-9] Reserved.
#\item [10]
#  P2 = 0; product valid at reference time + P1.
#\item [11-50] Reserved.
#\item [51]
#  Climatological Mean Value: multiple year averages of quantities which are
#  themselves means over some period of time (P2) less than a year. The
#  reference time (R) indicates the date and time of the start of a period of
#  time, given by R to R + P2 , over which a mean is formed; N indicates the
#  number of such period-means that are averaged together to form the
#  climatological value, assuming that the N period-mean fields are separated by
#  one year. The reference time indicates the start of the N-year climatology. N
#  is given in octets 22 and 23 of the PDS.
#
#  If P1 = 0, then the data averaged in the basic interval P2 are assumed to be
#  continuous (i.e., all available data are simply averaged together).
#
#  If P1 = 1 (the units of time - octet 18,Code table 4 - are not relevant
#  here), then the data averaged together in the basic interval P2 are valid
#  only at the time (hour, minute) given in the reference time, for all the days
#  included in the P2 period. The units of P2  are given by the contents of
#  octet 18 and Code table 4.
#\item [52-112] Reserved.
#\item [113]
#  Average of N forecasts (or initialized analyses); each product has forecast
#  period of P1 (P1 = 0 for initialized analyses); products have reference
#  times at intervals of P2, beginning at the given reference time.
#\item [114]
#  Accumulation of N forecasts (or initialized analyses); each product has
#  forecast period of P1 (P1 = 0 for initialized analyses); products have
#  reference times at intervals of P2, beginning at the given reference time.
#\item [115]
#  Average of N forecasts, all with the same reference time; the first has a
#  forecast period of P1, the remaining forecasts follow at intervals of P2.
#\item [116]
#  Accumulation of N forecasts, all with the same reference time; the first has
#  a forecast period of P1, the remaining forecasts follow at intervals of P2.
#\item [117]
#  Average of N forecasts; the first has a forecast period of P1, the
#  subsequent ones have forecast periods reduced from the previous one by an
#  interval of P2; the reference time for the first is given in octets 13 to 17,
#  the subsequent ones have reference times increased from the previous one by
#  an interval of P2. Thus all the forecasts have the same valid time, given by
#  the reference time + P1.
#\item [118]
#  Temporal variance, or covariance, of N initialized analyses; each product has
#  forecast period of P1  = 0; products have reference times at intervals of P2,
#  beginning at the given reference time.
#\item [119]
#  Standard deviation of N forecasts, all with the same reference time with
#  respect to the time average of forecasts; the first forecast has a forecast
#  period of P1, the remaining forecasts follow at intervals of P2.
#\item [120-122] Reserved.
#\item [123]
#  Average of N uninitialized analyses, starting at the reference time, at
#  intervals of P2.
#\item [124]
#  Accumulation of N uninitialized analyses, starting at the reference time, at
#  intervals of P2.
#\item [125-254] Reserved.
#\item [256]
#  When P1=0 and P2=0: General cloud group \\
#  When P1=0 and P2=1: CL \\
#  When P1=0 and P2=2: CM \\
#  When P1=0 and P2=3: CH \\
#  When P1=1 and P2=0: Vertical significance 1 \\
#  When P1=2 and P2=0: Vertical significance 2 \\
#  When P1=3 and P2=0: Vertical significance 3 \\
#  When P1=4 and P2=0: Vertical significance 4 \\
#  When P1=0 and P2=0: Extra station information
#\end{description}
#
#Notes about the time range values:
#
#\begin{itemize}
#\item For analysis products, or the first of a series of analysis products, the reference time (octets 13 to 17) indicates the valid time.
#\item For forecast products, or the first of a series of forecast products, the reference time indicates the valid time of the analysis upon which the (first) forecast is based.
#\item Initialized analysis products are allocated code figures distinct from those allocated to uninitialized analysis products.
#\item Code figure 10 allows the period of a forecast to be extended over two octets; this is to assist with extended range forecasts.
#\item Where products or a series of products are averaged or accumulated, the number involved is to be represented in octets 22 and 23 of Section 1, while any number missing is to be represented in octet 24.
#\item Forecasts of the accumulation or difference of some quantity (e.g. quantitative precipitation forecasts), indicated by values of 4 or 5 in octet 21, have a product valid time given by the reference time + P2; the period of accumulation, or difference, can be calculated as $P2 - P1$.
#\item A few examples may help to clarify the use of Code table 5:
#  \begin{itemize}
#  \item For analysis products, P1 will be zero and the time range indicator
#	(octet 21) will also be zero; for initialized products (sometimes
#	called "zero hour forecasts"), P1 will be zero, but octet 21 will be
#	set to 1.
#  \item For forecasts, typically, P1  will contain the number of hours of the
#	forecast (the unit indicator given in octet 18 would be 1) and octet 21
#	would contain a zero.
#  \end{itemize}
#\item Code value 115 would be used, typically, for multiple day mean forecasts,
#      all derived from the same initial conditions.
#\item Code value 117 would be used, typically, for Monte Carlo type
#      calculations; many forecasts valid at the same time from different
#      initial (reference) times.
#\item Averages, accumulations, and differences get a somewhat specialized
#      treatment. If octet 21 has a value between 2 and 5 (inclusive), then the
#      reference time + P1  is the initial date/time and the reference time + P2
#      is the final date/time of the period over which averaging or accumulation
#      takes place. If, however, octet 21 has a value of 113, 114, 115, 116,
#      117, 123, or 124, the P2 specifies the time interval between each of the
#      fields (or the forecast initial times) that have been averaged or
#      accumulated. These latter values of octet 21 require the quantities
#      averaged to be equally separated in time; the former values, 3 and 4 in
#      particular, allow for irregular or unspecified intervals of time between
#      the fields that are averaged or accumulated.
#\end{itemize}
#
	*/
	return dba_error_ok();
}


/* vim:set ts=4 sw=4: */
