Summary: DB-ALLe is a database for punctual metereological data  (Command line tools)
Name: dballe
Version: 4.0.0
Release: 4%{?dist}
License: GPL
Group: Applications/Meteo
URL: http://www.arpa.emr.it/dettaglio_documento.asp?id=514&idlivello=64
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot
BuildRequires: unixODBC-devel, gperf, cnf-devel, tetex, tetex-latex, doxygen, latex2html, python-docutils
#Requires:  mysql >= 4.1.1 ,mysql-connector-odbc, sqlite sqliteodbc, %{name}-common = %{?epoch:%epoch:}%{version}-%{release}
Requires:  %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%{!?python_sitelib: %define python_sitelib %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib()")}
%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
%{!?python_siteinc: %define python_siteinc %(%{__python} -c "from distutils.sysconfig import get_python_inc; print get_python_inc()")}

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


%package  -n provami
Summary: Graphical interface to DB-All.e databases
Group: Applications/Meteo
requires: wxPython, dballe >= 3.0-1

%description -n provami
 provami is a GUI application to visualise and navigate DB-All.e databases.
 It also allows to perform simple editing tasks, and to graphically select and
 export data subsets.


%package  -n libdballe-devel
Summary:  DB-ALL.e core C development library
Group:    Applications/Meteo
Requires: lib%{name}4 = %{?epoch:%epoch:}%{version}-%{release}

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


%package  -n libdballe4
Summary:   DB-ALL.e core shared library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballe4
DB-ALL.e C shared library
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the shared library for C programs.


%package    -n  libdballef-devel

Summary:  DB-All.e Fortran development library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release},lib%{name}f-devel = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballef-devel
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the DB-All.e Fortran API, designed to make it easy to use the DB-All.e
 database as a smart working area for meteorological software.


%package    -n libdballef4

Summary:  DB-ALL.e Fortran shared library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description -n libdballef4
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




%package -n libdballepp4

Summary:  DB-ALL.e C++ shared library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}
%description -n libdballepp4
 Database for punctual meteorological data (C++ shared library)
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the shared library for C++ programs.


%package -n  libdballepp-devel
Summary:  DB-All.e C++ development library
Group:    Applications/Meteo
Requires: lib%{name}-devel = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}4  = %{?epoch:%epoch:}%{version}-%{release}, lib%{name}pp4 = %{?epoch:%epoch:}%{version}-%{release} 

%description -n libdballepp-devel
 Database for punctual meteorological data (C++ development library) 
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.
 .
 This is the DB-All.e C++ API, a thin wrapper around the C API that takes
 advantage of C++ features.


%package -n python-dballe
Summary:  DB-ALL.e Python library
Group:    Applications/Meteo
Requires: %{name}-common = %{?epoch:%epoch:}%{version}-%{release}

%description -n python-dballe
 DB-ALL.e Python library for weather research
 DB-All.e is a fast on-disk database where meteorological observed and
 forecast data can be stored, searched, retrieved and updated.

 These are the python bindings.


%prep
%setup -q 

%build
### configure  pyexecdir=%{python_sitearch}/dballe pkgpythondir=%{python_sitelib}/dballe
%configure --disable-rpath FC=gfortran F90=gfortan F77=gfortran

# do not work smp
#make %{?_smp_mflags}

make

#make SITELIB=%{python_sitelib}
#make  pyexecdir=%{python_sitearch}/dballe pkgpythondir=%{python_sitelib}/dballe

#make check

%install
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"

#make install DESTDIR=$RPM_BUILD_ROOT pyexecdir=%{python_sitearch}/dballe  pkgpythondir=%{python_sitelib}/dballe
#make install DESTDIR=$RPM_BUILD_ROOT SITELIB=%{python_sitelib}

make install DESTDIR=$RPM_BUILD_ROOT

%clean
[ "%{buildroot}" != / ] && rm -rf "%{buildroot}"
#rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%dir %{_bindir}
%dir %{_mandir}/man1
%{_bindir}/dbadb
%{_bindir}/dbamsg
%{_bindir}/dbatbl
%doc %{_mandir}/man1/dbadb*
%doc %{_mandir}/man1/dbamsg*
%doc %{_mandir}/man1/dbatbl*
%doc %{_docdir}/dballe/guide.ps
%doc %{_docdir}/dballe/guide_html/*


%files -n provami
%defattr(-,root,root,-)
%dir /usr/bin
%{_bindir}/provami

%dir %{python_sitearch}/
%{python_sitearch}//provami
%dir %{_mandir}/man1
%doc %{_mandir}/man1/provami*
/usr/share/dballe/icon*.png
/usr/share/dballe/world.dat

%files common
%defattr(-,root,root,-)
/usr/share/dballe/[DB]*
/usr/share/dballe/dballe.txt
/usr/share/dballe/repinfo.csv

%files -n libdballe4
%defattr(-,root,root,-)
%{_libdir}/libdballe.so.*
#%{_libdir}/libdballe[0-9]*.so.*

%files -n libdballe-devel
%defattr(-,root,root,-)
%doc %{_docdir}/dballe/libdballe.doxytags
%{_includedir}/dballe/core/*
%{_includedir}/dballe/bufrex/*
%{_includedir}/dballe/msg/*
%{_includedir}/dballe/db/*
%{_includedir}/dballe/init.h

%{_libdir}/libdballe.a
%{_libdir}/libdballe.la
%{_libdir}/libdballe.so
#%{_libdir}/libdballe[0-9]*.so
%{_libdir}/pkgconfig/libdballe.pc
/usr/share/aclocal/libdballe.m4


%files -n libdballef-devel
%defattr(-,root,root,-)

%doc %{_docdir}/dballe/fapi_html
%doc %{_docdir}/dballe/fapi.ps

%{_includedir}/dballe/dballef.h
%{_libdir}/libdballef*.a
%{_libdir}/pkgconfig/libdballef*
%{_libdir}/libdballef*.la
%{_libdir}/libdballef*.so
/usr/share/aclocal/libdballef*.m4




%files -n libdballef4
%defattr(-,root,root,-)
%{_libdir}/libdballef*.so.*


%files -n python-dballe
%defattr(-,root,root,-)
%dir %{python_sitearch}/
%{python_sitearch}//*

#%dir %{python_sitelib}/dballe
#%{python_sitelib}/dballe/*

%doc %{_docdir}/dballe/python-dballe*


%files -n libdballepp4
%defattr(-,root,root,-)
%{_libdir}/libdballepp*.so.*

%files -n libdballepp-devel
%defattr(-,root,root,-)

%doc %{_docdir}/dballe/libdballepp.doxytags
%doc %{_docdir}/dballe/c++_api

%{_includedir}/dballe++
%{_libdir}/libdballepp*.a
%{_libdir}/pkgconfig/libdballepp*
%{_libdir}/libdballepp*.la
%{_libdir}/libdballepp*.so
/usr/share/aclocal/libdballepp.m4


%files -n libdballe-doc
%defattr(-,root,root,-)
%doc %{_docdir}/dballe/c_api


%post
/sbin/ldconfig

%postun
/sbin/ldconfig


%changelog
* Tue Mar 18 2008 root <root@spinacio> - %epoch:}%{version}-%{release}:4.0.0-4
- new pachage (less pachages)

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


