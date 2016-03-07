# Input/output/query parameters

| Name          | Unit      | Format     | Description                 | On insert | On query | On results | Comment                         |
| ------------- | --------- | ---------- | --------------------------- | --------- | -------- | ---------- | ------------------------------- |
| priority      | NUMBER    | 10 digits  | Priority of this type of report | ignored   | optional | present    | Every type of report has an associated priority that controls which data are first returned when there is more than one in the same physical space.  It can be changed by editing /etc/dballe/repinfo.csv |
| priomax       | NUMBER    | 10 digits  | Maximum priority of reports queried | ignored   | optional | absent     |                                 |
| priomin       | NUMBER    | 10 digits  | Minimum priority of reports queried | ignored   | optional | absent     |                                 |
| rep_memo      | CCITTIA5  | 20 digits  | Mnemonic alias for type of report | required  | optional | present    |                                 |
| ana_id        | NUMERIC   | 10 digits  | Station database ID         | optional  | optional | present    | Internal DB-ALLe ID referring to a pseudoana entry, used as a shortcut reference instead of specifying the full data |
| mobile        | NUMERIC   | 10 digits  | Station is mobile           | required  | optional | present    | Set to 1 if the station is mobile, such as a ship or a flight; else 0 |
| ident         | CCITTIA5  | 64 digits  | Identifier of flight or ship | required if mobile=1 | optional | present if mobile=1 |                                 |
| lat           | DEGREE    | ##.#####   | Latitude                    | required  | optional | present    | on insert, it has priority over ana_id |
| lon           | DEGREE    | ###.#####  | Longitude                   | required  | optional | present    | on insert, it has priority over ana_id |
| latmax        | DEGREE    | ##.#####   | Maximum latitude queried    | ignored   | optional | absent     |                                 |
| latmin        | DEGREE    | ##.#####   | Minimum latitude queried    | ignored   | optional | absent     |                                 |
| lonmax        | DEGREE    | ###.#####  | Maximum longitude queried   | ignored   | optional | absent     |                                 |
| lonmin        | DEGREE    | ###.#####  | Minimum longitude queried   | ignored   | optional | absent     |                                 |
| year          | YEAR      | 4 digits   | Year                        | required  | optional | present    |                                 |
| month         | MONTH     | 2 digits   | Month                       | required  | optional | present    |                                 |
| day           | DAY       | 2 digits   | Day                         | required  | optional | present    |                                 |
| hour          | HOUR      | 2 digits   | Hour                        | required  | optional | present    |                                 |
| min           | MINUTE    | 2 digits   | Minutes                     | required  | optional | present    |                                 |
| sec           | SECOND    | 2 digits   | Seconds                     | required  | optional | present    |                                 |
| yearmax       | YEAR      | 4 digits   | Maximum year queried        | ignored   | optional | absent     |                                 |
| yearmin       | YEAR      | 4 digits   | Year or minimum year queried | ignored   | optional | absent     |                                 |
| monthmax      | MONTH     | 2 digits   | Maximum month queried       | ignored   | optional | absent     |                                 |
| monthmin      | MONTH     | 2 digits   | Minimum month queried       | ignored   | optional | absent     |                                 |
| daymax        | DAY       | 2 digits   | Maximum day queried         | ignored   | optional | absent     |                                 |
| daymin        | DAY       | 2 digits   | Minimum day queried         | ignored   | optional | absent     |                                 |
| hourmax       | HOUR      | 2 digits   | Maximum hour queried        | ignored   | optional | absent     |                                 |
| hourmin       | HOUR      | 2 digits   | Minumum hour queried        | ignored   | optional | absent     |                                 |
| minumax       | MINUTE    | 2 digits   | Maxminum minutes queried    | ignored   | optional | absent     |                                 |
| minumin       | MINUTE    | 2 digits   | Minimum minutes queried     | ignored   | optional | absent     |                                 |
| secmax        | SECOND    | 2 digits   | Maxminum seconds queried    | ignored   | optional | absent     |                                 |
| secmin        | SECOND    | 2 digits   | Minimum seconds queried     | ignored   | optional | absent     |                                 |
| leveltype1    | NUMBER    | 10 digits  | Type of first level         |           |          |            |                                 |
| l1            | NUMBER    | 10 digits  | Level layer L1              | required  | optional | present    |                                 |
| leveltype2    | NUMBER    | 10 digits  | Type of second level        |           |          |            |                                 |
| l2            | NUMBER    | 10 digits  | Level layer L2              | required  | optional | present    |                                 |
| pindicator    | NUMBER    | 10 digits  | P indicator for time range  | required  | optional | present    |                                 |
| p1            | SECOND    | 10 digits  | Time range P1               | required  | optional | present    |                                 |
| p2            | SECOND    | 10 digits  | Time range P2               | required  | optional | present    |                                 |
| var           | CCITTIA5  | 7 digits   | Variable queried            | ignored   | optional | present, indicates the name of the last variable returned |                                 |
| varlist       | CCITTIA5  | 255 digits | List of variables to query  | ignored   | optional | absent     | Comma-separated list of variable codes wanted on output |
| context_id    | NUMERIC   | 10 digits  | Context ID of the variable  |           |          |            |                                 |
| query         | CCITTIA5  | 255 digits | Query behaviour modifier    | ignored   | optional | absent     | Comma-separated list of query modifiers.  Can have one of: 'best', 'bigana', 'nosort', 'stream'.  Examples: 'best', 'nosort,stream' |
| ana_filter    | CCITTIA5  | 255 digits | Filter on anagraphical data | ignored   | optional | absent     | Restricts the results to only those stations which have a pseudoana value that matches the filter. Examples: 'height>=1000', 'B02001=1', '1000<=height<=2000 |
| data_filter   | CCITTIA5  | 255 digits | Filter on data              | ignored   | optional | absent     | Restricts the results to only the variables of the given type, which have a value that matches the filter. Examples: 't<260', 'B22021>2', '10<=B22021<=20' |
| attr_filter   | CCITTIA5  | 255 digits | Filter on data attributes   | ignored   | optional | absent     | Restricts the results to only those data which have an attribute that matches the filter. Examples: 'conf>70', 'B33197=0', '25<=conf<=50' |
| limit         | NUMBER    | 10 digits  | Maximum number of results to return | ignored   | optional | absent     | Maximum number of results to return |
| var_related   | CCITTIA5  | 6 digits   | Variable related to attribute queried |           |          |            |                                 |

## Input parameters for attribute-related action routines

| Name          | Unit      | Format     | Description                 | On insert | On query | On results | Comment                         |
| ------------- | --------- | ---------- | --------------------------- | --------- | -------- | ---------- | ------------------------------- |
| `*Bxxyyy`     | depends   | depends    | Value of the attribute      | required  | ignored  | present    |                                 |
| `*var`        | Character | 7 chars    | Acoderibute queried         | ignored   | optional | present, indicates the name of the last attribute returned |                                 |
| `*varlist`    | Character | 255 chars  | List of attributes to query | ignored   | optional | absent     | Comma-separated list of attribute B codes wanted on output |
| `*var_related` | Character | 6 chars    | Variable related to the attribute to query | required  | required | absent     | It is automatically set by `idba_dammelo` and `idba_prendilo` (when `idba_prendilo` inserts a single variable) |
| `*context_id` | Numeric   | 10 digits  | Context ID of the variable related to the attribute to query | required  | required | absent     | It is automatically set by `idba_dammelo` and `idba_prendilo` |
