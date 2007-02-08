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
	def __new__(self, leveltype, l1=0, l2=0):
		return tuple.__new__(self, (leveltype, l1, l2))
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
	def __new__(self, pindicator, p1=0, p2=0):
		return tuple.__new__(self, (pindicator, p1, p2))
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
                def __str__(self):
                        return self.format("None")
                def __repr__(self):
                        return "Var(%s, %s)" % (self.code(), self.format("None"))
                def enq(self):
                        if self.info().is_string():
                                return self.enqc()
                        elif self.info().scale() == 0:
                                return self.enqi()
                        else:
                                return self.enqd()
        %}
}

%extend dballe::Varinfo {
        %pythoncode %{
                def __str__(self):
                        return "%s (%s,%s)" % (self.var(), self.unit(), self.desc())
                def __repr__(self):
                        return "Varinfo(%s)" % (self.var(),)
        %}
}

%extend dballe::Cursor {
        %rename attributes attributes_orig;
        %pythoncode %{
                def __iter__(self):
                        record = Record()
                        while self.next(record):
                                yield record
                def attributes(self, rec = None):
                        """
                        Read the attributes for the variable pointed by this record.

                        If a rec argument is provided, it will write the
                        attributes in that record and return the number of
                        attributes read.  If rec is None, it will return a
                        tuple (Record, count) with a newly created Record.
                        """
                        if rec == None:
                                rec = Record()
                                count = self.attributes_orig(rec)
                                return rec, count
                        else:
                                count = self.attributes_orig(rec)
                                return count
        %}
}

// Rewrite Record methods to make use of the None value, and add convenience
// methods and iteration
%extend dballe::Record {
        %rename enq enqvar;
        %rename enqi enqi_orig;
        %rename enqd enqd_orig;
        %rename enqs enqs_orig;
        %rename enqc enqc_orig;
        %rename seti seti_orig;
        %rename setd setd_orig;
        %rename sets sets_orig;
        %rename setc setc_orig;
        %rename set set_orig;
        %pythoncode %{
                def enq(self, name):
                       v = self.enqvar(name)
                       return v.enq()
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

                def set(self, *args):
                       if len(args) == 2:
                                self.set_orig(*args)
                       elif len(args) == 1:
                                if 'iteritems' in args[0].__class__.__dict__:
                                        for key, val in args[0].iteritems():
                                                self.set_orig(key, val)
                                else:
                                        self.set_orig(args[0])
                       else:
                                raise ValueError, "Set wants 1 or 2 values ("+str(len(args))+" provided)"

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

                def __getitem__(self, key):
                        if key == "date":
                                return self.enqdate()
                        elif key == "level":
                                return self.enqlevel()
                        elif key == "timerange":
                                return self.enqtimerange()
                        else:
                                return self.enq(key)
                def __setitem__(self, key, val):
                        if key == "date":
                                self.setdate(val)
                        elif key == "level":
                                self.setlevel(val)
                        elif key == "timerange":
                                self.settimerange(val)
                        else:
                                self.set(key, val)
                def __delitem__(self, key):
                        self.unset(key, val)
                def __iter__(self):
                        "Iterate all the contents of the record"
                        i = self.begin()
                        while i.valid():
                                yield i.var()
                                i.next()
                def iterkeys(self):
                        "Iterate all the keyword and variable names in the record"
                        i = self.begin()
                        while i.valid():
                                if i.isKeyword():
                                        yield i.keywordName()
                                else:
                                        yield i.var().code()
                                i.next()
                def itervalues(self):
                        "Iterate all the values in the record"
                        return self.__iter__()
                def iteritems(self):
                        """
                        Iterate all the keyword and variable names in the
                        record, generating (name, value) tuples
                        """
                        i = self.begin()
                        while i.valid():
                                v = i.var()
                                if i.isKeyword():
                                        yield (i.keywordName(), v)
                                else:
                                        yield (v.code(), v)
                                i.next()
                def itervars(self):
                        "Iterate all the variables in the record"
                        i = self.varbegin()
                        while i.valid():
                                yield i.var()
                                i.next()
                def __str__(self):
                        return "Record{"+",".join([str(key)+": "+str(val) for key, val in self.iteritems()])+"}"
                def __repr__(self):
                        return self.__str__();
        %}
}

%typemap(in) dba_keyword {
	$1 = dba_record_keyword_byname(PyString_AsString($input));
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_STRING) dba_keyword {
        $1 = PyString_Check($input) ? 1 : 0;
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

%typemap(typecheck,precedence=SWIG_TYPECHECK_STRING) dba_varcode {
        $1 = PyString_Check($input) ? 1 : 0;
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
