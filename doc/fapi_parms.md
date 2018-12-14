# Input/output/query parameters

## Parameters used when setting up a query

| Name            | Type      | Description                    | Comment                                     |
| --------------- | --------- | ------------------------------ | ------------------------------------------- |
| `priority`      | Integer   | Priority of the type of report | Every type of report has an associated priority that controls which data are first returned when there is more than one in the same physical space.  It can be changed by editing /etc/dballe/repinfo.csv |
| `priomax`       | Integer   | Maximum priority of reports queried |                                        |
| `priomin`       | Integer   | Minimum priority of reports queried |                                        |
| `report`        | String    | Type of report                 | Alias for `rep_memo`                        |
| `rep_memo`      | String    | Type of report                 | Alias for `repoert`                         |
| `ana_id`        | Integer   | Station database ID            | internal DB-ALLe ID referring to an existing station, usable as a shortcut reference instead of specifying the full data |
| `mobile`        | Integer   | Station is mobile              | Set to 1 to query mobile station, such as a ship or a flight; to 0 to query only fixed stations, such as synop or metar |
| `ident`         | String    | Station identifier             |                                             |
| `lat`           | Float     | Latitude                       | Setting as integer requires the value * 10^5; equivalent to setting latmin and latmax     |
| `lon`           | Float     | Longitude                      | Setting as integer requires the value * 10^5; equivalent to setting lonmin and lonmax     |
| `latmax`        | Float     | Maximum latitude queried       | Setting as integer requires the value * 10^5 |
| `latmin`        | Float     | Minimum latitude queried       | Setting as integer requires the value * 10^5 |
| `lonmax`        | Float     | Maximum longitude queried      | Setting as integer requires the value * 10^5 |
| `lonmin`        | Float     | Minimum longitude queried      | Setting as integer requires the value * 10^5 |
| `year`          | Integer   | Year                           | Equivalent to setting yearmin and yearmax   |
| `month`         | Integer   | Month                          | Equivalent to setting monthmin and monthmax |
| `day`           | Integer   | Day                            | Equivalent to setting daymin and daymax     |
| `hour`          | Integer   | Hour                           | Equivalent to setting hourmin and hourmax   |
| `min`           | Integer   | Minutes                        | Equivalent to setting minumin and minumax   |
| `sec`           | Integer   | Seconds                        | Equivalent to setting secmin and secmax     |
| `yearmax`       | Integer   | Maximum year queried           |                                             |
| `yearmin`       | Integer   | Year or minimum year queried   |                                             |
| `monthmax`      | Integer   | Maximum month queried          |                                             |
| `monthmin`      | Integer   | Minimum month queried          |                                             |
| `daymax`        | Integer   | Maximum day queried            |                                             |
| `daymin`        | Integer   | Minimum day queried            |                                             |
| `hourmax`       | Integer   | Maximum hour queried           |                                             |
| `hourmin`       | Integer   | Minumum hour queried           |                                             |
| `minumax`       | Integer   | Maxminum minutes queried       |                                             |
| `minumin`       | Integer   | Minimum minutes queried        |                                             |
| `secmax`        | Integer   | Maxminum seconds queried       |                                             |
| `secmin`        | Integer   | Minimum seconds queried        |                                             |
| `leveltype1`    | Integer   | Type of first level            |                                             |
| `l1`            | Integer   | Level layer L1                 |                                             |
| `leveltype2`    | Integer   | Type of second level           |                                             |
| `l2`            | Integer   | Level layer L2                 |                                             |
| `pindicator`    | Integer   | P indicator for time range     |                                             |
| `p1`            | Integer   | Time range P1                  |                                             |
| `p2`            | Integer   | Time range P2                  |                                             |
| `var`           | String    | Variable queried               | When setting var, varlist is cleared        |
| `varlist`       | String    | List of variables to query     | Comma-separated list of variable codes to query; when setting varlist, var is cleared |
| `query`         | String    | Query behaviour modifier       | Comma-separated list of query modifiers.  Can have one of: 'best', 'bigana', 'nosort', 'stream'.  Examples: 'best', 'nosort,stream' |
| `ana_filter`    | String    | Filter on anagraphical data    | Restricts the results to only those stations which have a pseudoana value that matches the filter. Examples: 'height>=1000', 'B02001=1', '1000<=height<=2000 |
| `data_filter`   | String    | Filter on data                 | Restricts the results to only the variables of the given type, which have a value that matches the filter. Examples: 't<260', 'B22021>2', '10<=B22021<=20' |
| `attr_filter`   | String    | Filter on data attributes      | Restricts the results to only those data which have an attribute that matches the filter. Examples: 'conf>70', 'B33197=0', '25<=conf<=50' |
| `limit`         | Integer   | Maximum number of results to return |                                        |
| `block`         | Integer   | WMO block number of the station |                                            |
| `station`       | Integer   | WMO station number of the station |                                          |


## Parameters used when inserting values

| Name         | Unit    | Description                | Comment                         |
| ------------ | ------- | -------------------------- | ------------------------------- |
| `rep_memo`   | String  | Type of report             |                                 |
| `ana_id`     | Integer | Station database ID        | Can optionally be specified instead of `lat`, `lon`, `ident`, `rep_memo`: internal DB-ALLe ID referring to an existing station, usable as a shortcut |
| `ident`      | String  | Station identifier         | Optional: if missing, the station is fixed; if present, it is mobile |
| `lat`        | Float   | Latitude                   |                                 |
| `lon`        | Float   | Longitude                  |                                 |
| `year`       | Integer | Year                       |                                 |
| `month`      | Integer | Month                      | Optional: when missing, the minimum valid value is used |
| `day`        | Integer | Day                        | Optional: when missing, the minimum valid value is used |
| `hour`       | Integer | Hour                       | Optional: when missing, the minimum valid value is used |
| `min`        | Integer | Minutes                    | Optional: when missing, the minimum valid value is used |
| `sec`        | Integer | Seconds                    | Optional: when missing, the minimum valid value is used |
| `leveltype1` | Integer | Type of first level        |                                 |
| `l1`         | Integer | Level layer L1             |                                 |
| `leveltype2` | Integer | Type of second level       |                                 |
| `l2`         | Integer | Level layer L2             |                                 |
| `pindicator` | Integer | P indicator for time range |                                 |
| `p1`         | Integer | Time range P1              |                                 |
| `p2`         | Integer | Time range P2              |                                 |

Any variable code (as `Bxxyyy` or as a DB-All.e alias) can also be set to insert a variable.

In the Fortran API, after a data is inserted in the database, one can query
`ana_id` to get the database ID of the station used for that data.


## Parameters used when reading station query results

| Name         | Unit    | Description                     | Comment                         |
| ------------ | ------- | ------------------------------- | ------------------------------- |
| `priority`   | Integer | Priority of this type of report | Every type of report has an associated priority that controls which data are first returned when there is more than one in the same physical space.  It can be changed by editing /etc/dballe/repinfo.csv |
| `rep_memo`   | String  | Type of report                  |                                 |
| `ana_id`     | Integer | Station database ID             | Internal DB-ALLe ID referring to an existing station, usable as a shortcut |
| `mobile`     | Integer | Station is mobile               | Set to 1 if the station is mobile, such as a ship or a flight; else 0 |
| `ident`      | String  | Station identifier              | present if mobile=1             |
| `lat`        | Float   | Latitude                        |                                 |
| `lon`        | Float   | Longitude                       |                                 |

## Parameters used when reading query results for station values

| Name         | Unit    | Description                     | Comment                         |
| ------------ | ------- | ------------------------------- | ------------------------------- |
| `priority`   | Integer | Priority of this type of report | Every type of report has an associated priority that controls which data are first returned when there is more than one in the same physical space.  It can be changed by editing /etc/dballe/repinfo.csv |
| `rep_memo`   | String  | Type of report                  |                                 |
| `ana_id`     | Integer | Station database ID             | Internal DB-ALLe ID referring to an existing station, usable as a shortcut |
| `mobile`     | Integer | Station is mobile               | Set to 1 if the station is mobile, such as a ship or a flight; else 0 |
| `ident`      | String  | Station identifier              | present if mobile=1             |
| `lat`        | Float   | Latitude                        |                                 |
| `lon`        | Float   | Longitude                       |                                 |
| `context_id` | Integer | ID of the variable              | ID identifying a variable in the database, can be used as a shortcut to access its attributes |

The variable value can be queried using the code in `var`

## Parameters used when reading query results for data values

| Name         | Unit    | Description                     | Comment                         |
| ------------ | ------- | ------------------------------- | ------------------------------- |
| `priority`   | Integer | Priority of this type of report | Every type of report has an associated priority that controls which data are first returned when there is more than one in the same physical space.  It can be changed by editing /etc/dballe/repinfo.csv |
| `rep_memo`   | String  | Type of report                  |                                 |
| `ana_id`     | Integer | Station database ID             | Internal DB-ALLe ID referring to an existing station, usable as a shortcut |
| `mobile`     | Integer | Station is mobile               | Set to 1 if the station is mobile, such as a ship or a flight; else 0 |
| `ident`      | String  | Station identifier              | present if mobile=1             |
| `lat`        | Float   | Latitude                        |                                 |
| `lon`        | Float   | Longitude                       |                                 |
| `year`       | Integer | Year                            |                                 |
| `month`      | Integer | Month                           |                                 |
| `day`        | Integer | Day                             |                                 |
| `hour`       | Integer | Hour                            |                                 |
| `min`        | Integer | Minutes                         |                                 |
| `sec`        | Integer | Seconds                         |                                 |
| `leveltype1` | Integer | Type of first level             |                                 |
| `l1`         | Integer | Level layer L1                  |                                 |
| `leveltype2` | Integer | Type of second level            |                                 |
| `l2`         | Integer | Level layer L2                  |                                 |
| `pindicator` | Integer | P indicator for time range      |                                 |
| `p1`         | Integer | Time range P1                   |                                 |
| `p2`         | Integer | Time range P2                   |                                 |
| `var`        | String  | Variable queried                | Set to the current variable when iterating results |
| `context_id` | Integer | ID of the variable              | ID identifying a variable in the database, can be used as a shortcut to access its attributes |

The variable value can be queried using the code in `var`

## Parameters used when reading query results for summary values

| Name         | Unit    | Description                        | Comment                         |
| ------------ | ------- | ---------------------------------- | ------------------------------- |
| `priority`   | Integer | Priority of this type of report    | Every type of report has an associated priority that controls which data are first returned when there is more than one in the same physical space.  It can be changed by editing /etc/dballe/repinfo.csv |
| `rep_memo`   | String  | Type of report                     |                                 |
| `ana_id`     | Integer | Station database ID                | Internal DB-ALLe ID referring to an existing station, usable as a shortcut |
| `mobile`     | Integer | Station is mobile                  | Set to 1 if the station is mobile, such as a ship or a flight; else 0 |
| `ident`      | String  | Station identifier                 | present if mobile=1             |
| `lat`        | Float   | Latitude                           |                                 |
| `lon`        | Float   | Longitude                          |                                 |
| `leveltype1` | Integer | Type of first level                |                                 |
| `l1`         | Integer | Level layer L1                     |                                 |
| `leveltype2` | Integer | Type of second level               |                                 |
| `l2`         | Integer | Level layer L2                     |                                 |
| `pindicator` | Integer | P indicator for time range         |                                 |
| `p1`         | Integer | Time range P1                      |                                 |
| `p2`         | Integer | Time range P2                      |                                 |
| `var`        | String  | Variable code                      |                                 |
| `yearmax`    | Integer | Maximum year for this variable     |                                 |
| `yearmin`    | Integer | Minimum year for this variable     |                                 |
| `monthmax`   | Integer | Maximum month for this variable    |                                 |
| `monthmin`   | Integer | Minimum month for this variable    |                                 |
| `daymax`     | Integer | Maximum day for this variable      |                                 |
| `daymin`     | Integer | Minimum day for this variable      |                                 |
| `hourmax`    | Integer | Maximum hour for this variable     |                                 |
| `hourmin`    | Integer | Minumum hour for this variable     |                                 |
| `minumax`    | Integer | Maximum minutes for this variable  |                                 |
| `minumin`    | Integer | Minimum minutes for this variable  |                                 |
| `secmax`     | Integer | Maximum seconds for this variable  |                                 |
| `secmin`     | Integer | Minimum seconds for this variable  |                                 |
| `count`      | Integer | Number of values for this variable |                                 |
| `context_id` | Integer | Number of values for this variable | Deprecated: use `count`         |


## Input parameters for attribute-related action routines

| Name          | Unit      | Format     | Description                 | On insert | On query | On results | Comment                         |
| ------------- | --------- | ---------- | --------------------------- | --------- | -------- | ---------- | ------------------------------- |
| `*Bxxyyy`     | depends   | depends    | Value of the attribute      | required  | ignored  | present    |                                 |
| `*var`        | Character | 7 chars    | Acoderibute queried         | ignored   | optional | present, indicates the name of the last attribute returned |                                 |
| `*varlist`    | Character | 255 chars  | List of attributes to query | ignored   | optional | absent     | Comma-separated list of attribute B codes wanted on output |
| `*var_related` | Character | 6 chars    | Variable related to the attribute to query | required  | required | absent     | It is automatically set by `idba_next_data` and `idba_insert_data` (when `idba_insert_data` inserts a single variable) |
| `*context_id` | Numeric   | 10 digits  | Context ID of the variable related to the attribute to query | required  | required | absent     | It is automatically set by `idba_next_data` and `idba_insert_data` |
