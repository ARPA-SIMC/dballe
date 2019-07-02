# Level type values

This table lists the possible values for leveltype1 or
leveltype2 and the interpretation of the corresponding numerical
value l1 or l2.  Leveltype values in the range 0-255 can
be used for defining either a single level (leveltype1) or a surface
delimiting a layer (leveltype1 and leveltype2) with any meaningful
combination of leveltypes; values of leveltype >255 have a special use
for encoding cloud values in SYNOP reports and they do not strictly
define physical surfaces.

The idea is borrowed from the GRIB edition 2 fixed surface
concept and the values for leveltype coincide with the GRIB standard
where possible.

| Level Type  | Meaning                                | Unit/contents of l1/l2      |
| ----------- | -------------------------------------- | --------------------------- |
| 0           | Reserved                               |                             |
| 1           | Ground or Water Surface                |                             |
| 2           | Cloud Base Level                       |                             |
| 3           | Level of Cloud Tops                    |                             |
| 4           | Level of 0C Isotherm                   |                             |
| 5           | Level of Adiabatic Condensation Lifted from the Surface |                             |
| 6           | Maximum Wind Level                     |                             |
| 7           | Tropopause                             |                             |
| 8           | Nominal Top of the Atmosphere          | DB-All.e encodes the channel number of polar satellites in L1 |
| 9           | Sea Bottom                             |                             |
| 10-19       | Reserved                               |                             |
| 20          | Isothermal Level                       | K/10                        |
| 21-99       | Reserved                               |                             |
| 100         | Isobaric Surface                       | Pa                          |
| 101         | Mean Sea Level                         |                             |
| 102         | Specific Altitude Above Mean Sea Level | mm                          |
| 103         | Specified Height Level Above Ground    | mm                          |
| 104         | Sigma Level                            | 1/10000                     |
| 105         | Hybrid Level                           |                             |
| 106         | Depth Below Land Surface               | mm                          |
| 107         | Isentropic (theta) Level               | K/10                        |
| 108         | Level at Specified Pressure Difference from Ground to Level | Pa                          |
| 109         | Potential Vorticity Surface            | 10-9 K m2 kg-1 s-1          |
| 110         | Reserved                               |                             |
| 111         | Eta (NAM) Level (see note below)       | 1/10000                     |
| 112         | 116 Reserved                           |                             |
| 117         | Mixed Layer Depth                      | mm                          |
| 118-159 Reserved |                                        |                             |
| 160         | Depth Below Mean Sea Level             | mm                          |
| 161         | Depth Below water surface              | mm                          |
| 162-191 Reserved |                                        |                             |
| 200         | Entire atmosphere (considered as a single layer) |                             |
| 201         | Entire ocean (considered as a single layer) |                             |
| 204         | Highest tropospheric freezing level    |                             |
| 206         | Grid scale cloud bottom level          |                             |
| 207         | Grid scale cloud top level             |                             |
| 209         | Boundary layer cloud bottom level      |                             |
| 210         | Boundary layer cloud top level         |                             |
| 211         | Boundary layer cloud layer             |                             |
| 212         | Low cloud bottom level                 |                             |
| 213         | Low cloud top level                    |                             |
| 214         | Low cloud layer                        |                             |
| 215         | Cloud ceiling                          |                             |
| 220         | Planetary Boundary Layer               |                             |
| 222         | Middle cloud bottom level              |                             |
| 223         | Middle cloud top level                 |                             |
| 224         | Middle cloud layer                     |                             |
| 232         | High cloud bottom level                |                             |
| 233         | High cloud top level                   |                             |
| 234         | High cloud layer                       |                             |
| 235         | Ocean Isotherm Level                   | K/10                        |
| 240         | Ocean Mixed Layer                      |                             |
| 241         | Ordered Sequence of Data               |                             |
| 242         | Convective cloud bottom level          |                             |
| 243         | Convective cloud top level             |                             |
| 244         | Convective cloud layer                 |                             |
| 245         | Lowest level of the wet bulb zero      |                             |
| 246         | Maximum equivalent potential temperature level |                             |
| 247         | Equilibrium level                      |                             |
| 248         | Shallow convective cloud bottom level  |                             |
| 249         | Shallow convective cloud top level     |                             |
| 251         | Deep convective cloud bottom level     |                             |
| 252         | Deep convective cloud top level        |                             |
| 253         | Lowest bottom level of supercooled liquid water layer |                             |
| 254         | Highest top level of supercooled liquid water layer |                             |
| 256         | Clouds                                 |                             |
| 257         | Information about the station that generated the data |                             |
| 258         | (use when ltype1=256) Cloud Data group, L2 = 1 low clouds, 2 middle clouds, 3 high clouds, 0 others |                             |
| 259         | (use when ltype1=256) Individual cloud groups, L2 = group number |                             |
| 260         | (use when ltype1=256) Cloud drift, L2 = group number |                             |
| 261         | (use when ltype1=256) Cloud elevation, L2 = group number; (use when ltype1=264) L2 = swell wave group number |                             |
| 262         | (use when ltype1=256) Direction and elevation of clouds, L2 is ignored |                             |
| 263         | (use when ltype1=256) Cloud groups with bases below station level, L2 = group number |                             |
| 264         | Waves                                  |                             |
| 265     Non-physical data level |                                        |                             |
