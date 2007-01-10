Summary: DB-ALLe is a database for punctual metereological data  (Command line tools)
Name: dballe
Version: 3.0.1
Release: 1
License: GPL
Group: Applications/Meteo
URL: http://www.arpa.emr.it/sim
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: unixODBC-devel, gperf, starmet, tetex, tetex-latex, doxygen
Requires: MyODBC, mysql >= 4.1.1 , sqlite sqliteodbc, %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

 
%description
 Database for punctual meteorological data (Command line tools)
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


%package  -n libdballe-core-dev
Summary:  DB-ALL.e core C development library
Group:    Applications/Meteo
Requires: lib%{name}-core3 = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballe-core-dev
 DB-ALL.e core C development library
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the core DB_All.e development the library.  It includes:
 .
  * Error handling infrastructure
  * Unit conversion
  * Handling of physical variables

%package -n libdballe-core-doc
Summary:   DB-ALL.e core C development library (documentation)
Group: Applications/Meteo
%description  -n libdballe-core-doc
 DB-ALL.e core C development library (documentation)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the documentation for the core DB_All.e development the library.


%package  -n libdballe-core3
Summary:   DB-ALL.e core shared library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballe-core3
DB-ALL.e core shared library
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the core shared library for C programs.



%package -n libdballe-bufrex-dev
Summary:   Read and write functions for BUFR and CREX weather data
Group:    Applications/Meteo
Requires: lib%{name}-core-dev = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-core3 = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-bufrex3 = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballe-bufrex-dev
 Read and write functions for BUFR and CREX weather data
 Functions to read and write weather data in WMO BUFR and CREX formats.
 .
 It is being used and tested with these kinds of BUFR and CREX messages:
  * Fixed land and sea stations, like synops and buoys
  * Mobile stations: ships, airplanes
  * Soundings: temp, pilot
  * METAR reports
  * Satellite strides (decode only)



%package -n libdballe-bufrex-doc
Summary:   Read and write functions for BUFR and CREX weather data (documentation)
Group:    Applications/Meteo

%description  -n libdballe-bufrex-doc 
 Read and write functions for BUFR and CREX weather data (documentation)
 Documentation for the functions to read and write weather data in WMO BUFR and
 CREX formats.



%package  -n libdballe-bufrex3
Summary:   Read and write functions for BUFR and CREX weather data (shared library)
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballe-bufrex3
 Read and write functions for BUFR and CREX weather data (shared library)
 Shared library with functions to read and write weather data in WMO BUFR and
 CREX formats.


%package   -n libdballe-msg-dev
Summary:    Interpret weather reports into physycal data, and vice-versa
Group:    Applications/Meteo
Requires: lib%{name}-core-dev = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-core3  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-bufrex-dev = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-bufrex3  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-msg3 = %{?epoch:%epoch:}%{version}-%{release} 
%description  -n libdballe-msg-dev
Interpret weather reports into physycal data, and vice-versa
 The library implements the dba_msg infrastructure for handling physical data
 precisely located in space and time.  The structure can be:
 .
  * Filled in with data from weather reports encoded BUFR, CREX or AOF formats
  * Used to create BUFR or CREX weather reports
  * Read from disk in BUFR, CREX or AOF files
  * Written to disk in BUFR or CREX files
  * Stored into a DB-All.e database via libdballe-db
  * Queried from a DB-All.e database via libdballe-db
 .
 The representation is in 7 dimensions: latitude and longitude geographic
 coordinates, table driven vertical coordinate, reference time, table driven
 observation and forecast specification, table driven data type.
 .
 dba_msg is a very convenient way of accessing weather data already digested as
 properly located physical quantities, as well as as a common middle ground for
 performing conversions between the various supported message formats.



%package   -n libdballe-msg-doc
Summary:  Interpret weather reports into physycal data, and vice-versa (documentation)
Group:    Applications/Meteo

%description  -n libdballe-msg-doc
Interpret weather reports into physycal data, and vice-versa (documentation)
 Documentation for the C API of the dba_msg infrastructure for handling
 physical data precisely located in space and time.


%package   -n libdballe-msg3
Summary:   Interpret weather reports into physycal data, and vice-versa (shared library)
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballe-msg3
Interpret weather reports into physycal data, and vice-versa (shared library)
 Shared library for the dba_msg infrastructure for handling physical data
 precisely located in space and time.





%package   -n libdballe-db-dev

Summary:   Smart on-disk database for weather data
Group:    Applications/Meteo
Requires: lib%{name}-core-dev = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-core3  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-bufrex-dev = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-bufrex3  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-msg-dev  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-msg3 = %{?epoch:%epoch:}%{version}-%{release}, libdballe-db3 = %{?epoch:%epoch:}%{version}-%{release}

%description  -n libdballe-db-dev

Smart on-disk database for weather data
 Fast on-disk database where meteorological observed and forecast data can be
 stored, searched, retrieved and updated.
 .
 The main characteristics of the DB-ALL.e database are:
 .
  * To make computation easier, data is stored as physical quantities,
    that is, as measures of a variable in a specific point of space and
    time, rather than as a sequence of report.
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
  * Can concurrently access multiple MySQL and SQLite databases via ODBC




%package   -n libdballe-db-doc

Summary:   Smart on-disk database for weather data (documentation)
Group:    Applications/Meteo

%description -n libdballe-db-doc
 Smart on-disk database for weather data (documentation)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the C API documentation for the database functions of DB-All.e.



%package    -n libdballe-db3
Summary:  Smart on-disk database for weather data (shared library)
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description  -n libdballe-db3
 Smart on-disk database for weather data (shared library)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the shared library for the database functions of DB-All.e.



%package    -n  libdballef-dev

Summary:  Database for punctual meteorological data (Fortran development library)
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release},lib%{name}f-dev = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballef-dev
 Database for punctual meteorological data (Fortran development library)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the DB-All.e Fortran API, designed to make it easy to use the DB-All.e
 database as a smart working area for meteorological software.




%package    -n libdballef3

Summary:  Database for punctual meteorological data (Fortran shared library)
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballef3
 Database for punctual meteorological data (Fortran shared library)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the shared library for Fortran programs.




%package    common

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




%package -n libdballepp3

Summary:  Database for punctual meteorological data (C++ shared library)
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}
%description -n libdballepp3
 Database for punctual meteorological data (C++ shared library)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the shared library for C++ programs.


%package -n  libdballepp-dev
Summary:  Database for punctual meteorological data (C++ development library)
Group:    Applications/Meteo
Requires: lib%{name}-core-dev = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-core3  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-bufrex-dev = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-bufrex3  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-msg-dev  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-msg3 = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-db-dev  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}-db3 = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}pp3 = %{?epoch:%epoch:}%{version}-%{release} 

%description -n libdballepp-dev
 Database for punctual meteorological data (C++ development library) 
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the DB-All.e C++ API, a thin wrapper around the C API that takes
 advantage of C++ features.


%package -n python-dballe
Summary:  Database for punctual meteorological data (Python bindings)
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description -n python-dballe
 Database for punctual meteorological data (Python bindings)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 These are the python bindings.


%prep
%setup -q

%build
%configure
make
#make check

%install
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"
#rm -rf $RPM_BUILD_ROOT
%makeinstall

%clean
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"
#rm -rf $RPM_BUILD_ROOT




%files
%defattr(-,root,root,-)
%dir /usr/bin
%dir /usr/share/man/man1
/usr/bin/*
%doc /usr/share/man/man1/*
%doc /usr/share/doc/dballe/guide.ps
%doc /usr/share/doc/dballe/guide_html
%doc doc


%files common
%defattr(-,root,root,-)
/usr/share/dballe


%files -n libdballe-bufrex3
%defattr(-,root,root,-)
/usr/lib/libdballe-bufrex*.so.*

%files -n libdballe-bufrex-dev
%defattr(-,root,root,-)
%doc /usr/share/doc/dballe/libdballe-bufrex.doxytags
/usr/include/dballe/bufrex/*
/usr/lib/libdballe-bufrex*.a
/usr/lib/pkgconfig/libdballe-bufrex*
/usr/lib/libdballe-bufrex*.la
/usr/lib/libdballe-bufrex*.so
/usr/share/aclocal/libdballe-bufrex*.m4

%files -n libdballe-core3
%defattr(-,root,root,-)
/usr/lib/libdballe-core*.so.*

%files -n libdballe-core-dev
%defattr(-,root,root,-)
%doc /usr/share/doc/dballe/libdballe-core.doxytags
/usr/include/dballe/core/*
/usr/lib/libdballe-core*.a
/usr/lib/pkgconfig/libdballe-core*
/usr/lib/libdballe-core*.la
/usr/lib/libdballe-core*.so
/usr/share/aclocal/libdballe-core*.m4

%files -n libdballe-db3
%defattr(-,root,root,-)
/usr/lib/libdballe-db*.so.*

%files -n libdballe-db-dev
%defattr(-,root,root,-)
%doc /usr/share/doc/dballe/libdballe-db.doxytags
/usr/include/dballe/db/*
/usr/lib/libdballe-db*.a
/usr/lib/pkgconfig/libdballe-db*
/usr/lib/libdballe-db*.la
/usr/lib/libdballe-db*.so
/usr/share/aclocal/libdballe-db*.m4

%files -n libdballef-dev
%defattr(-,root,root,-)

%doc /usr/share/doc/dballe/fapi_html
%doc /usr/share/doc/dballe/fapi.ps

/usr/include/dballe/dballef.h
/usr/lib/libdballef*.a
/usr/lib/pkgconfig/libdballef*
/usr/lib/libdballef*.la
/usr/lib/libdballef*.so
/usr/share/aclocal/libdballef*.m4


%files -n libdballef3
%defattr(-,root,root,-)
/usr/lib/libdballef*.so.*

%files -n libdballe-msg3
%defattr(-,root,root,-)
/usr/lib/libdballe-msg*.so.*

%files -n libdballe-msg-dev
%defattr(-,root,root,-)
%doc /usr/share/doc/dballe/libdballe-msg.doxytags
/usr/include/dballe/msg/*
/usr/lib/libdballe-msg*.a
/usr/lib/pkgconfig/libdballe-msg*
/usr/lib/libdballe-msg*.la
/usr/lib/libdballe-msg*.so
/usr/share/aclocal/libdballe-msg*.m4


%files -n python-dballe
%defattr(-,root,root,-)
/usr/lib/python*

%files -n libdballepp3
%defattr(-,root,root,-)
/usr/lib/libdballepp*.so.*



%files -n libdballe-core-doc
%defattr(-,root,root,-)
%doc /usr/share/doc/dballe/core_api

%files -n libdballe-bufrex-doc
%defattr(-,root,root,-)
%doc /usr/share/doc/dballe/bufrex_api

%files -n libdballe-msg-doc
%defattr(-,root,root,-)
%doc /usr/share/doc/dballe/msg_api

%files -n libdballe-db-doc
%defattr(-,root,root,-)
%doc /usr/share/doc/dballe/db_api

%files -n libdballepp-dev
%defattr(-,root,root,-)

%doc /usr/share/doc/dballe/libdballepp.doxytags
%doc /usr/share/doc/dballe/c++_api

/usr/include/dballe++
/usr/lib/libdballepp*.a
/usr/lib/pkgconfig/libdballepp*
/usr/lib/libdballepp*.la
/usr/lib/libdballepp*.so
/usr/share/aclocal/libdballepp.m4

%post
/sbin/ldconfig

%postun
/sbin/ldconfig


%changelog
* Tue Dec 19 2006 root <root@strip.metarpa> - 3.0-1
- spitted in more packages for version 3.0

* Wed Nov 29 2006 root <root@strip.metarpa> - 2.6-2
- aggiuntevar e rete per icecast

* Wed Nov 22 2006 root <root@strip.metarpa> - 2.6-1
- added support for sqlite

* Wed Aug  9 2006 root <root@strip.metarpa> - 2.3-3
- Aggiornato alla revisione 1086

* Wed Aug  2 2006 root <root@strip.metarpa> - 2.3-1
- some bugs solved

* Tue May  9 2006 root <root@strip.metarpa> - 2.0-1
- cambio delle api e della struttura db per permette la piu' versatile gestione dell'anagrafica

* Wed May  3 2006 root <root@strip.metarpa> - 1.1-1
- ottimizzazioni! eliminato querybest e introdotto parametro quesy con opzioni best e bigana

* Wed Apr 26 2006 root <root@strip.metarpa> - 1.0-5
- modificate query per ottimizzazione

* Tue Apr 11 2006 root <root@strip.metarpa> - 1.0-4
- inserita indicizzazione

* Tue Apr 11 2006 root <root@strip.metarpa> - 1.0-3
- corretti alcni bug

* Wed Mar 15 2006 root <root@strip.metarpa> - 1.0-2
- corrette conversioni mancanti e gestione generici

* Wed Mar  8 2006 root <root@strip.metarpa> - 1.0-1
- prima release ufficiale

* Wed Feb 15 2006 root <root@strip.metarpa> - 0.7-9
-  a lot of bug fixes

* Wed Feb  8 2006 root <root@strip.metarpa> - 0.7-8
- a lot of bug fixes

* Wed Feb  1 2006 root <root@strip.metarpa> - 0.7-7
- resolved performace iusses and metar implemented + aof fixes

* Wed Jan 25 2006 root <root@strip.metarpa> - 0.7-6
- corretti bug

* Wed Jan 18 2006 root <root@strip.metarpa> - 0.7-5
- about source and table bug

* Tue Jan 17 2006 root <root@strip.metarpa> - 0.7-4
- lot of bug corrected and documentation improvements

* Tue Jan 10 2006 root <patruno@strip.metarpa> - 0.7-2
- corretti vari bug di fine anno

* Tue Sep 13 2005 root <root@strip.metarpa> 
- Initial build.


