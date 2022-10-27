%global releaseno 1
# Note: define _srcarchivename in CI build only.
%{!?srcarchivename: %global srcarchivename %{name}-%{version}-%{releaseno}}

Summary: DB-ALLe is a database for point-based metereological data  (Command line tools)
Name: dballe
Version: 9.3
Release: %{releaseno}%{dist}
License: GPL
Group: Applications/Meteo
URL: https://github.com/ARPA-SIMC/dballe
Source0: https://github.com/arpa-simc/%{name}/archive/v%{version}-%{releaseno}.tar.gz#/%{srcarchivename}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
%if 0%{?rhel} == 7
%define python3_vers python36
# to have python 3.6 interpreter
BuildRequires: python3-rpm-macros >= 3-23
%else
%define python3_vers python3
%endif
BuildRequires: libtool
BuildRequires: gcc-c++
BuildRequires: gperf
BuildRequires: which
BuildRequires: doxygen
BuildRequires: pkgconfig(lua) > 5.1.1
BuildRequires: pkgconfig(libwreport) >= 3.29
BuildRequires: %{python3_vers}-devel
%if 0%{?rhel} == 7
BuildRequires: popt-devel
BuildRequires: postgresql-devel
BuildRequires: mariadb-devel
%else
BuildRequires: pkgconfig(popt)
BuildRequires: pkgconfig(libpq)
BuildRequires: pkgconfig(mariadb)
%endif
BuildRequires: pkgconfig(sqlite3)
BuildRequires: help2man
BuildRequires: libwreport-doc
BuildRequires: %{python3_vers}-wreport3
BuildRequires: gcc-gfortran
BuildRequires: %{python3_vers}-numpy
%if ! 0%{?el7}
BuildRequires: %{python3_vers}-sphinx
BuildRequires: %{python3_vers}-breathe
%endif
%{!?el7:BuildRequires: xapian-core-devel}

Requires: %{python3_vers}-dballe = %{?epoch:%epoch:}%{version}-%{release}
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description
 Database for point-based meteorological data (Command line tools)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This framework allows to manage large amounts of data using its simple
 Application Program Interface, and provides tools to visualise, import
 and export in the standard formats BUFR, AOF and CREX.
 .
 The main characteristics of DB-ALL.e are:
 .
  * Fortran, C, C++ and Python APIs are provided.
  * To make computation easier, data is stored as physical quantities,
    that is, as measures of a variable in a specific point of space and
    time, rather than as a sequence of report.
  * Internal representation is similar to BUFR and CREX WMO standard
    (table code driven) and utility for import and export are included
    (generic and ECMWF template).
  * Representation is in 7 dimensions: latitude and longitude geographic
    coordinates, table driven vertical coordinate, reference time,
    table driven observation and forecast specification, table driven
    data type.
  * It allows to store extra information linked to the data, such as
    confidence intervals for quality control.
  * It allows to store extra information linked to the stations.
  * Variables can be represented as real, integer and characters, with
    appropriate precision for the type of measured value.
  * It is based on physical principles, that is, the data it contains are
    defined in terms of homogeneous and consistent physical data. For
    example, it is impossible for two incompatible values to exist in the
    same point in space and time.
  * It can manage fixed stations and moving stations such as airplanes or
    ships.
  * It can manage both observational and forecast data.
  * It can manage data along all three dimensions in space, such as data
    from soundings and airplanes.
  * Report information is preserved. It can work based on physical
    parameters or on report types.

%package  -n libdballe-devel
Summary:  DB-ALL.e core C development library
Group:    Applications/Meteo
Requires: libdballe9 = %{?epoch:%epoch:}%{version}-%{release}
Requires: popt-devel
Requires: postgresql-devel
Requires: mariadb-devel
%{!?el7:Requires: xapian-core-devel}

%description -n libdballe-devel
 DB-ALL.e core C development library
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.

The Fedora packaging of DB-All.e includes all the features of the libraries,
 but any subset can be used without interference from other subsets.  It is
 also possible to rebuild the library to include only those features that are
 needed.
 .
 Features provided:
 .
  * Unit conversion
  * Handling of physical variables
  * Encoding and decoding of BUFR and CREX reports from:
     * fixed land and sea stations, like synops and buoys
     * mobile stations: ships, airplanes
     * soundings: temp, pilot
     * METAR reports
     * Satellite strides (decode only)
  * Decoding of AOF reports
  * Interpretation of weather reports as physical data precisely located in
    space and time, and encoding of physical data into weather reports.
  * Smart on-disk database for observed and forecast weather data based on
    physical principles, built to support operations such as quality control,
    data thinning, correlation of data from mixed sources

%package -n libdballe-doc
Summary:   DB-ALL.e core C development library (documentation)
Group: Applications/Meteo
%description  -n libdballe-doc
 DB-ALL.e core C development library (documentation)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the documentation for the core DB_All.e development library.


%package  -n libdballe9
Summary:   DB-ALL.e core shared library
Group:    Applications/Meteo
Requires: %{name}-common >= %{?epoch:%epoch:}%{version}-%{release}
Requires: pkgconfig(libwreport) >= 3.29
Obsoletes: libdballe6 < 8.21

%description -n libdballe9
DB-ALL.e C shared library
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 
 This is the shared library for C programs.


%package -n libdballef-devel

Summary:  DB-All.e Fortran development library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}
Requires: libdballe-devel = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballef-devel
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the DB-All.e Fortran API, designed to make it easy to use the DB-All.e
 database as a smart working area for meteorological software.


%package -n libdballef5

Summary:  DB-ALL.e Fortran shared library
Group:    Applications/Meteo
Requires: %{name}-common >= %{?epoch:%epoch:}%{version}-%{release}
Requires: libdballe9 = %{?epoch:%epoch:}%{version}-%{release}
Provides: lidballef4 = %{?epoch:%epoch:}%{version}-%{release}
Obsoletes: libdballef4 < 8.21

%description -n libdballef5
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the shared library for Fortran programs.


%package common

Summary:  Common data files for all DB-All.e modules
Group:    Applications/Meteo

%description common
Common data files for all DB-All.e modules
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This package contains common DB-All.e data files, including variable metadata,
 BUFR and CREX decoding tables, report metadata, level and time range
 descriptions.

%package -n %{python3_vers}-dballe
Summary:  DB-ALL.e Python library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}
Requires: %{python3_vers}-numpy
Requires: %{python3_vers}-wreport3
Obsoletes: python-dballe < 8.0

%description -n %{python3_vers}-dballe
 DB-ALL.e Python library for weather research
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.

 These are the python bindings.

%prep
%setup -q -n %{srcarchivename}

%build

autoreconf -ifv
%if 0%{?rhel} == 7
# CentOS7 doesn't support [[deprecated]] attribute
CPPFLAGS="-Wno-error=attributes"
%else
CPPFLAGS=""
%endif
%configure FC=gfortran F90=gfortan F77=gfortran --enable-dballef --enable-dballe-python --enable-docs --disable-static CPPFLAGS="$CPPFLAGS -Wno-error=cpp"
make
make check

%install
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"

make install DESTDIR="%{buildroot}" STRIP=/bin/true
mkdir -p $RPM_BUILD_ROOT%{_fmoddir}
mv $RPM_BUILD_ROOT%{_includedir}/*.mod $RPM_BUILD_ROOT%{_fmoddir}


%clean
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"



%files
%defattr(-,root,root,-)
%{_bindir}/dbadb
%{_bindir}/dbamsg
%{_bindir}/dbatbl
%{_bindir}/dbaexport
%doc %{_mandir}/man1/dbadb*
%doc %{_mandir}/man1/dbamsg*
%doc %{_mandir}/man1/dbatbl*
%doc %{_mandir}/man1/dbaexport*

%files common
%defattr(-,root,root,-)
%{_datadir}/wreport/[BD]*
%{_datadir}/wreport/dballe.txt
%{_datadir}/wreport/repinfo.csv

%files -n libdballe9
%defattr(-,root,root,-)
%{_libdir}/libdballe.so.*

%files -n libdballe-devel
%defattr(-,root,root,-)
%{_includedir}/dballe/*.h
%{_includedir}/dballe/core/*
%{_includedir}/dballe/msg/*
%{_includedir}/dballe/sql/*
%{_includedir}/dballe/db/*
%{_includedir}/dballe/cmdline/*
%{_includedir}/dballe/fortran/*

%exclude %{_libdir}/libdballe.la
%{_libdir}/libdballe.so
%{_libdir}/pkgconfig/libdballe.pc
%{_datadir}/aclocal/libdballe.m4


%files -n libdballef-devel
%defattr(-,root,root,-)
%{_includedir}/dballe/dballef.h
%{_includedir}/dballe/dballeff.h
%{_libdir}/pkgconfig/libdballef*
%exclude %{_libdir}/libdballef*.la
%{_libdir}/libdballef*.so
%{_datadir}/aclocal/libdballef*.m4
%{_fmoddir}/*.mod


%files -n libdballef5
%defattr(-,root,root,-)
%{_libdir}/libdballef*.so.*


%files -n libdballe-doc
%defattr(-,root,root,-)
%if ! 0%{?el7}
%doc %{_docdir}/%{name}/*
%endif

%files -n %{python3_vers}-dballe
%defattr(-,root,root,-)
%dir %{python3_sitelib}/dballe
%{python3_sitelib}/dballe/*
%dir %{python3_sitearch}
%exclude %{python3_sitearch}/*.la
%{python3_sitearch}/*.so*

%post
/sbin/ldconfig

%postun
/sbin/ldconfig


%changelog
* Thu Oct 27 2022 Daniele Branchini <dbranchini@arpae.it> - 9.3-1
- Implemented debian packaging (#273, #274)

* Wed May 25 2022 Emanuele Di Giacomo <edigiacomo@arpae.it> - 9.2-1
- Added entries for mobile telephony links
- Fix bug in `dballe_txt_add_entry` when precision is not a power of 10
- `dballe_txt_add_entry` finds max code among the selected category (#271)
- Added B13207 "Water skin velocity module"

* Tue Feb 22 2022 Daniele Branchini <dbranchini@arpae.it> - 9.1-1
- Implemented new query modifier `query=last` (#80)
- Fixed a DeprecationWarning when querying latitude/longitude with Decimal values (#264)
- Open/create a Xapian explorer only when needed, fixing locking issues on
  concurrent access (#262)
- Added `ilat` e `ilon` (integer lat/lon) for `Station` objects (#247)
- Added examples for messages import/export (#255)
- Removed dependency on nose (#268)
- Added Turbidity and ORP variables (#267)

* Mon May 17 2021 Daniele Branchini <dbranchini@arpae.it> - 9.0-3
- Resolving dballe-common conflicts by obsoleting lidballe6

* Mon May 17 2021 Daniele Branchini <dbranchini@arpae.it> - 9.0-2
- fixed dependency error in libdballe-devel

* Thu May 13 2021 Daniele Branchini <dbranchini@arpae.it> - 9.0-1
- C++ API changes (#171)

* Tue May 11 2021 Daniele Branchini <dbranchini@arpae.it> - 8.20-2
- Handling cpp warnings for F34

* Mon May 10 2021 Daniele Branchini <dbranchini@arpae.it> - 8.20-1
- Added UV index variable
- Fixed -Werror=range-loop-construct in F34

* Mon Apr 12 2021 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.19-1
- Fixed B13211 length (#239)

* Tue Mar  2 2021 Daniele Branchini <dbranchini@arpae.it> - 8.18-1
- Added variables 025194 011211 011212 011213 011214 011215 011216

* Mon Jan 25 2021 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.17-2
- Explicit requires wreport >= 3.29-1

* Mon Jan 25 2021 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.17-1
- `dbadb import --domain-errors=tag` clamps the value instead of
  unsetting it (#241)

* Mon Jan 25 2021 Daniele Branchini <dbranchini@arpae.it> - 8.16-1
- Added `dbadb import --domain-errors={unset|clamp|tag}`. `tag` is only
  available when compiling with wreport from version 3.29. (#241)

* Wed Jan 13 2021 Daniele Branchini <dbranchini@arpae.it> - 8.15-1
- Added Lifted, virtual T and Skin Temperature variables
- Added support for python 3.9 in nose2 tests
- Dropped support for nose1

* Tue Dec 15 2020 Daniele Branchini <dbranchini@arpae.it> - 8.14-1
- Added `dbadb import --domain-errors={unset|clamp}`. `clamp` is only available
  when compiling with wreport from version 3.28. (#241)
- Fixed querying by datetime extremes in explorer (#242)
- Added alternative meson build system

* Wed Sep 23 2020 Daniele Branchini <dbranchini@arpae.it> - 8.13-1
- Always ignore stations without contexts (#235)

* Mon Aug 10 2020 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.12-1
- Improved documentation
- Use BUFR unit in documentation (#222)
- dballe.Message.query_data() error when the BUFR has only station data (#213)
- Fixed bugs in Explorer (#217, #218, #228)
- attr_filter supports != operator (#224)
- Fixed JSON datetime parser (#230)
- Fixed segmentation fault querying min and max datetimes in
  dballe.CursorSummarySummary (#232)

* Thu Apr 30 2020 Daniele Branchini <dbranchini@arpae.it> - 8.11-2
- Fixed compilation error for gcc 10 (#211)

* Fri Mar 13 2020 Daniele Branchini <dbranchini@arpae.it> - 8.11-1
- Fix errors after failed starting of transactions and raise clear errors if
  using a db connection in a forked process. (#210)

* Thu Feb 13 2020 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.10-1
- Fixed explorer query (#209)

* Mon Feb 10 2020 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.9-2
- Fixed typo in BuildRequires

* Mon Feb 10 2020 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.9-1
- Worked around an internal compiler error on Centos7
- Made Xapian support really optional

* Mon Feb 10 2020 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.8-1
- Added `dballe.Explorer` examples to python HOWTO (#181)
- Creating an Explorer with a pathname makes it load/save from that file, which
  can be a JSON file if it ends with `.json` or no Xapian support is compiled
  in, otherwise Explorer will persist using an indexed Xapian database.
- Undo breaking changes to stable API on importers/exporters

* Tue Feb  4 2020 Daniele Branchini <dbranchini@arpae.it> - 8.7-1
- Fixed the command line documentation of possible input types (#202)
- Restructured and tested documentation (#204, #205, #206)
- JSON is now supported for encoding/decoding wherever BUFR and CREX are (#202)

* Mon Dec  9 2019 Daniele Branchini <dbranchini@arpae.it> - 8.6-1
- Turned a segfault into a proper exception (#197)
- Parse again '-' as missing (#200)

* Thu Nov 21 2019 Daniele Branchini <dbranchini@arpae.it> - 8.5-1
- Fixed passing strings as datetime values from python (#174)

* Mon Oct 28 2019 Daniele Branchini <dbranchini@arpae.it> - 8.4-1
- Redesigned and unified all the documentation
- B table updates
- Allow querying from python without specifying all the datetime min/max values
- Fixed querying longitude ranges from python
- Fixed reading unset query results from python

* Mon Aug  5 2019 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.3-1
- ported python bindings to wobblepy
- cleaned use of wreport python bindings

* Mon Jul  8 2019 Emanuele Di Giacomo <edigiacomo@arpae.it> - 8.2-1
- Python API: fixed `Message.query_station_data` method (#160)
- Add level 161 Depth below water surface (#161)

* Wed Jun 12 2019 Daniele Branchini <dbranchini@arpae.it> - 8.1-1
- Python API: bindings for core::Data (#158)
- Python API: add doc for keywords in cursor classes (#155)
- Python API: enabled cursor-based iteration (#154)
- Implemented dbadb --varlist (#149)

* Tue Apr 16 2019 Daniele Branchini <dbranchini@arpae.it> - 8.0-3
- moving to python 3.6 on Centos7

* Tue Mar 12 2019 Daniele Branchini <dbranchini@arpae.it> - 8.0-2
- Documented ExplorerUpdate and DBExplorerUpdate
- Updated fapi_btable.md
- Fixed rawhide builds

* Wed Feb 27 2019 Daniele Branchini <dbranchini@arpae.it> - 8.0-1
- Added more functionality to the stable C++ API
- Added more functionality to the Python API
- New english descriptive names for Fortran API (the old names still work for compatibility)
- Dropped support for Python 2
- Miscellaneous improvements (see NEWS.md)

* Thu Feb 14 2019 Daniele Branchini <dbranchini@arpae.it> - 7.37-1
- Added variables
- Fixed make check error (#135)
- Fixed attribute import on json (#134)
- Added Explorer python bindings

* Tue Oct 23 2018 Daniele Branchini <dbranchini@arpae.it> - 7.36-1
- Added variables
- Removed static library and .la from rpm (#130)
- Fix libdballe.pc dependencies (#126)
- Explicit dependency between fortran and c++ packages

* Wed Jul 25 2018 Emanuele Di Giacomo <edigiacomo@arpae.it> - 7.35-1
- Export summary stats in python explorer (python3 only)

* Mon Jul 16 2018 Emanuele Di Giacomo <edigiacomo@arpae.it> - 7.34-1
- Serialization/deserialization and merge of db::Summary

* Wed Jun 27 2018 Emanuele Di Giacomo <edigiacomo@arpae.it> - 7.33-3
- optimizations addressing #117

* Tue Jun 19 2018 Daniele Branchini <dbranchini@arpae.it> - 7.33-2
- other optimizations addressing #116

* Thu Jun 14 2018 Daniele Branchini <dbranchini@arpae.it> - 7.33-1
- added B22195 local variable
- optimizations addressing #116

* Thu Jun 7 2018 Daniele Branchini <dbranchini@arpae.it> - 7.32-1
- accept file objects for export in python API (#104)
- removed V6 database code
- refactored import to reduce the number of queries to the db (#112)

* Mon Jun 4 2018 Daniele Branchini <dbranchini@arpae.it> - 7.31-2
- addressing #112

* Tue May 29 2018 Daniele Branchini <dbranchini@arpae.it> - 7.31-1
- added support for readonly transactions
- updated pollen variables

* Wed Mar 28 2018 Daniele Branchini <dbranchini@arpae.it> - 7.30-2
- dependency on newer sqlite is now conditional

* Wed Mar 28 2018 Daniele Branchini <dbranchini@arpae.it> - 7.30-1
- fixed #86, #67

* Wed Feb 28 2018 Daniele Branchini <dbranchini@arpae.it> - 7.29-1
- fixed PostgreSQL support when pkg-config is missing

* Wed Feb 21 2018 Daniele Branchini <dbranchini@arpae.it> - 7.28-1
- fixed #103, #105, #107

* Wed Nov 22 2017 Daniele Branchini <dbranchini@arpae.it> - 7.27-1
- improved handling of rejected messages and consistency errors

* Thu Nov 16 2017 Daniele Branchini <dbranchini@arpae.it> - 7.26-1
- added variables to dballe.txt
- various fixes for copr and travis automation
- fixed bug on default dballe format (#97)

* Fri Sep 1 2017 Daniele Branchini <dbranchini@arpae.it> - 7.24-1
- closed #82, #87, #91, #93, #97, #98
- added new variable B13211

* Fri Apr 21 2017 Daniele Branchini <dbranchini@arpae.it> - 7.23-1
- closed #85
- added new pollen variables

* Mon Dec 19 2016 Daniele Branchini <dbranchini@arpae.it> - 7.22-1
- closed #76, #77

* Thu Nov 10 2016 Daniele Branchini <dbranchini@arpae.it> - 7.21-1
- closed #30

* Thu Oct 27 2016 Daniele Branchini <dbranchini@arpae.it> - 7.20-1
- closed #73

* Wed Oct 12 2016 Daniele Branchini <dbranchini@arpae.it> - 7.19-1
- closed #41, #47, #48, #49, #50, #51, #58, #65, #72

* Mon Jul 18 2016 Daniele Branchini <dbranchini@arpae.it> - 7.16-1
- closed #63 (removed memdb)
- closed #62 (dbadb export missing date)
- removed warnings for Fedora 24 compiling

* Mon Jul 11 2016 Daniele Branchini <dbranchini@arpae.it> - 7.15-1
- closed #61 (removed cnf)

* Tue May 3 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.14-1
- removed ODBC support
- make the fortran trace file line-buffered (#53)

* Thu Apr 21 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.13-1
- closed #52

* Thu Apr 21 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.12-1
- packaged missing headers
- closed #45

* Tue Apr 19 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.11-1
- Implemented experimental V7 database format for MySQL
- Implemented dbadb info
- Added documentation (see #18, #19)
- Adapted tests to fit the #44 attribute behaviour in V7 databases

* Thu Apr 14 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.10-1
- Added new experimental V7 database format
- closed #45, #46

* Tue Mar 15 2016 Daniele Branchini <dbranchini@arpa.emr.it> - 7.9-1
- dbadb import tuning
- closed #35, #37

* Thu Feb 25 2016 Davide Cesari <dcesari@arpa.emr.it> - 7.8-1
- closed #33

* Tue Nov 24 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.7-2
- managing provami messy legacy

* Tue Nov 24 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.7-1
- virtualenv/pip support
- closed #13, #22

* Thu Nov 12 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.6-2
- Fix dballe and dballe-python dependencies, excluded old provami files

* Wed Sep 16 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.6-1
- Fix JSON import from stdin (issue #11)

* Wed Sep 16 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.5-1
- Fixed tests

* Tue Sep 15 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.4-1
- JSON import/export (issue #5)
- Fix empty report in import (issue #8)
- Fix opening stdin and stdout from Fortran bindings (issue #3)
- Stable CSV header (issue #1)
- Ported to wreport-3.2
- Removed wibble dependency

* Fri Sep  4 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.3-2
- Fixed test

* Fri Sep  4 2015 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 7.3-1
- Encoding parser ignore case

* Mon Aug  3 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.2-1
- Requires libwreport v3.0
- Switching to git upstream

* Wed Apr 29 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 7.1-4715%{dist}
- using spostgresql-devel and mariadb-devel

* Wed Feb  4 2015 Daniele Branchini <dbranchini@arpa.emr.it> - 6.8-4479%{dist}
- using sqlite-devel instead of unixodbc

* Wed Feb  5 2014 Daniele Branchini <dbranchini@arpa.emr.it> - 6.6-4233%{dist}
- fixed conversion B07007 (M) <-> B07193 (mm).

* Wed Nov 13 2013 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 6.6-4105%{dist}
- Requires libwreport v2.10

* Wed Sep  4 2013 Paolo Patruno <ppatruno@pigna.metarpa> - 6.3-3991%{dist}
-  * New upstream release
     - refactored cursor+querybuilder for v6 databases
     - do not leave SQL_TIMESTAMP_STRUCT.fraction uninitialised, which caused
       duplicate imports in some cases
     - fortran message api: deal gracefully with uninterpretable messaes
     - fixed data overwrite on V6 databases
     - provami: documented the possibility of passing initial key=val filters
       on command line


* Fri Aug 23 2013 Emanuele Di Giacomo <edigiacomo@arpa.emr.it> - 6.2-3967%{dist}
- Record::set_from_string: hyphen ("-") as MISSING_INT
- Fixes for PostgreSQL (trim char(n) and sequence support)
- B22049 - Sea-surface temperature
- B15236 - [SIM] C6H6 Concentration

* Tue May 28 2013 Paolo Patruno <ppatruno@pigna.metarpa> - 6.1-3884%{dist}
- dballe 6.1
