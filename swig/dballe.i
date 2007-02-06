// dballe.i - SWIG interface
%module dballe

%include "std_string.i"
%include "typemaps.i"
%include "exception.i"
%include "../doc/dballe-doc.i"

%exception {
	try { $action }
	catch (std::exception& e)
        {
                SWIG_exception(SWIG_RuntimeError, e.what());
        }
}


%{
#include <dballe++/db.h>
#include <dballe++/format.h>
#include <dballe/core/aliases.h>
#include <iostream>

using namespace dballe;

%}

#ifdef SWIGPYTHON

%pythoncode %{
import datetime

class Level(tuple):
	"""
	Represents a level value as a 3-tuple
	"""
	def __new__(self, *args):
		if len(args) != 3:
			raise ValueError, "Level wants exactly 3 values ("+str(len(args))+" provided)"
		return tuple.__new__(self, args)
	def type(self):
		"Return the level type"
		return self[0]
	def l1(self):
		"Return l1"
		return self[1]
	def l2(self):
		"Return l2"
		return self[2]

class TimeRange(tuple):
	"""
	Represents a time range value as a 3-tuple
	"""
	def __new__(self, *args):
		if len(args) != 3:
			raise ValueError, "TimeRange wants exactly 3 values ("+str(len(args))+" provided)"
		return tuple.__new__(self, args)
	def type(self):
		"Return the time range type"
		return self[0]
	def p1(self):
		"Return p1"
		return self[1]
	def p2(self):
		"Return p2"
		return self[2]

%}

%extend dballe::Var {
        %pythoncode %{
                def __cmp__(self, other):
                        if other == None:
                                return 1
                        codea = self.code()
                        codeb = self.code()
                        if codea != codeb:
                                return cmp(codea, codeb)
                        isstra = self.info().is_string()
                        isstrb = other.info().is_string()
                        if isstra and isstrb:
                                return cmp(self.enqc(), other.enqc())
                        elif isstra and not isstrb:
                                return 1
                        elif not isstra and isstrb:
                                return -1
                        else:
                                return cmp(self.enqi(), other.enqi())
        %}
}

%extend dballe::Record {
        %pythoncode %{
                def __iter__(self):
                        i = self.begin()
                        while i.valid():
                                yield i.var()
                                i.next()
                def enqdate(self):
                        return datetime.datetime(self.enqi("year"), self.enqi("month"), self.enqi("day"), self.enqi("hour"), self.enqi("min"), self.enqi("sec"))
                def enqlevel(self):
                        return Level(self.enqi("leveltype"), self.enqi("l1"), self.enqi("l2"))
                def enqtimerange(self):
                        return TimeRange(self.enqi("pindicator"), self.enqi("p1"), self.enqi("p2"))
                def setdate(self, dt):
                        self.seti("year", dt.year)
                        self.seti("month", dt.month)
                        self.seti("day", dt.day)
                        self.seti("hour", dt.hour)
                        self.seti("min", dt.minute)
                        self.seti("sec", dt.second)
                def setlevel(self, level):
                        self.seti("leveltype", level[0])
                        self.seti("l1", level[1])
                        self.seti("l2", level[2])
                def settimerange(self, trange):
                        self.seti("pindicator", trange[0])
                        self.seti("p1", trange[1])
                        self.seti("p2", trange[2])
        %}
}

%extend dballe::Cursor {
        %pythoncode %{
                def __iter__(self):
                        record = Record()
                        while self.next(record):
                                yield record
        %}
}

// Rewrite Record methods to make use of the None value
%extend dballe::Record {
        %rename enqi enqi_orig;
        %rename enqd enqd_orig;
        %rename enqs enqs_orig;
        %rename enqc enqc_orig;
        %rename seti seti_orig;
        %rename setd setd_orig;
        %rename sets sets_orig;
        %rename setc setc_orig;
        %pythoncode %{
                def enqi(self, name):
                       if self.contains(name):
                                return self.enqi_orig(name)
                       else:
                                return None
                def enqd(self, name):
                       if self.contains(name):
                                return self.enqd_orig(name)
                       else:
                                return None
                def enqs(self, name):
                       if self.contains(name):
                                return self.enqs_orig(name)
                       else:
                                return None
                def enqc(self, name):
                       return self.enqs(name)

                def seti(self, name, value):
                       if value == None:
                                self.unset(name)
                       else:
                                self.seti_orig(name, value)
                def setd(self, name, value):
                       if value == None:
                                self.unset(name)
                       else:
                                self.setd_orig(name, value)
                def sets(self, name, value):
                       if value == None:
                                self.unset(name)
                       else:
                                self.sets_orig(name, value)
                def setc(self, name, value):
                       return self.sets(name, value)
        %}
}

%typemap(in) dba_keyword {
	$1 = dba_record_keyword_byname(PyString_AsString($input));
}

%typemap(in) dba_encoding {
	const char* tmp = PyString_AsString($input);
        if (strcmp(tmp, "BUFR") == 0)
                $1 = BUFR;
        else if (strcmp(tmp, "CREX") == 0)
                $1 = CREX;
        else if (strcmp(tmp, "AOF") == 0)
                $1 = AOF;
        else
                throw std::runtime_error(std::string("Unknown encoding '") + tmp + "'");
}

%typemap(in) dba_varcode {
	const char* tmp = PyString_AsString($input);
        if (($1 = dba_varcode_alias_resolve(tmp)) == 0)
                $1 = DBA_STRING_TO_VAR(tmp + 1);
}

%typemap(out) dba_varcode {
	char buf[10];
	snprintf(buf, 10, "B%02d%03d", DBA_VAR_X($1), DBA_VAR_Y($1));
	$result = PyString_FromString(buf);
}

%typemap(in, numinputs=0) dba_varcode *varcode (dba_varcode temp) {
	$1 = &temp;
}
%typemap(argout) dba_varcode *varcode {
	char buf[10];
	snprintf(buf, 10, "B%02d%03d", DBA_VAR_X(*$1), DBA_VAR_Y(*$1));
	$result = SWIG_Python_AppendOutput($result, SWIG_FromCharPtr(buf));
}

%apply int *OUTPUT { int *count };
%apply int *OUTPUT { int *contextid };
%apply int *OUTPUT { int *anaid };

#endif

%include <dballe++/var.h>
%include <dballe++/record.h>
%include <dballe++/db.h>
%include <dballe++/format.h>
