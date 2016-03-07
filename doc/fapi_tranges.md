# Time range values"

Definition of the main concepts related to the description of time
range and statistical processing for observed and forecast data:

* Validity time is defined as the time at which the data are measured or at which forecast is valid; for statistically processed data, the validity time is the end of the time interval.
* Reference time is defined as the nominal time of an observation for observed values, or as the time at which a model forecast starts for forecast values.
* The date and time in DB-All.e are always the validity date and time of a value, regardless of the value being an observation or a forecast.
* P1 is defined as the difference in seconds between validity time and reference time. For forecasts it is the positive forecast time. For observed values, the reference time is usually the same as the validity time, therefore P1 is zero. However P1 < 0 is a valid case for reports containing data in the past with respect to the nominal report time.
* P2 is defined as the duration of the period over which statistical processing is performed, and is always nonnegative. Note that, for instantaneous values, P2 is always zero.

The following table lists the possible values for pindicator and the
interpretation of the corresponding values of P1 and P2 specifying a
time range:

* **0** Average
* **1** Accumulation
* **2** Maximum
* **3** Minimum
* **4** Difference (value at the end of the time range minus value at the beginning)
* **5** Root Mean Square
* **6** Standard Deviation
* **7** Covariance (temporal variance)
* **8** Difference (value at the beginning of the time range minus value at the end)
* **9** Ratio
* **51** Climatological Mean Value
* **10-191** Reserved
* **192-254** Reserved for Local Use
* **200** Vectorial mean
* **201** Mode
* **202** Standard deviation vectorial mean
* **203** Vectorial maximum
* **204** Vectorial minimum
* **205** Product with a valid time ranging inside the given period
* **254** Istantaneous value
