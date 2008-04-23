// dballe.i - SWIG interface
%module dballe

%include "stl.i"
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
#include <dballe++/init.h>
#include <dballe++/bufrex.h>
#include <dballe++/msg.h>
#include <dballe/core/aliases.h>
#include <iostream>

using namespace dballe;

DballeInit dballeInit;

%}

#ifdef SWIGPYTHON

%pythoncode %{
import datetime

class Level(tuple):
	"""
	Represents a level value as a 4-tuple
	"""
	def __new__(self, leveltype1=0, l1=0, leveltype2=0, l2=0):
		return tuple.__new__(self, (leveltype1, l1, leveltype2, l2))
	def type1(self):
		"Return the type of the first level"
		return self[0]
	def l1(self):
		"Return l1"
		return self[1]
	def type2(self):
		"Return the type of the second level"
		return self[2]
	def l2(self):
		"Return l2"
		return self[3]
        def __str__(self):
                return describeLevel(*self)
        def __repr__(self):
                return "Level"+tuple.__repr__(self)

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
        def __str__(self):
                return describeTrange(*self)
        def __repr__(self):
                return "TimeRange"+tuple.__repr__(self)
%}

%extend dballe::Var {
        %ignore operator==;
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
                def __eq__(self, var):
                        if var is None:
                                return False
                        elif not issubclass(var.__class__, Var):
                                return self.enq() == var
                        else:
                                return self.equals(var)
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
                        return "Varinfo(%s,%s,%s,scale %d,len %d,string %s,irange %d..%d,frange: %f..%f)" % \
                            (self.var(), self.unit(), self.desc(), self.scale(), self.len(), \
                             str(self.is_string()), self.imin(), self.imax(), self.dmin(), self.dmax())
        %}
}

%extend dballe::Cursor {
        %rename attributes attributes_orig;
        %pythoncode %{
                def __iter__(self):
                        record = Record()
                        while self.next(record):
                                yield record
                def attributes(self, *args):
                        """
                        Read the attributes for the variable pointed by this record.

                        If a rec argument is provided, it will write the
                        attributes in that record and return the number of
                        attributes read.  If rec is None, it will return a
                        tuple (Record, count) with a newly created Record.
                        """
                        if len(args) == 0:
                                # attributes()
                                rec = Record()
                                count = self.attributes_orig(rec)
                                return rec, count
                        elif len(args) == 1:
                                if isinstance(args[0], Record):
                                        # attributes(rec)
                                        return self.attributes_orig(args[0])
                                else:
                                        # attributes(seq)
                                        rec = Record()
                                        count = self.attributes_orig(args[0], rec)
                                        return rec, count
                        elif len(args) == 2:
                                # attributes(seq, rec)
                                return self.attributes_orig(args[0], args[1])

        %}
}

// Rewrite Record methods to make use of the None value, and add convenience
// methods and iteration
%extend dballe::Record {
        %ignore contains(dba_varcode) const;
        %ignore contains(dba_keyword) const;
        %ignore enq(dba_varcode) const;
        %ignore enq(dba_keyword) const;
        %rename enq enqvar;
        %ignore enqi;
        %ignore enqd;
        %ignore enqs;
        %ignore enqc;
        %ignore enqi_ifset(dba_varcode, bool&) const;
        %ignore enqi_ifset(dba_keyword, bool&) const;
        %ignore enqd_ifset(dba_varcode, bool&) const;
        %ignore enqd_ifset(dba_keyword, bool&) const;
        %ignore enqc_ifset(dba_varcode) const;
        %ignore enqc_ifset(dba_keyword) const;
        %ignore enqs_ifset(dba_varcode, bool&) const;
        %ignore enqs_ifset(dba_keyword, bool&) const;
        %ignore keySet;
        %ignore keySeti;
        %ignore keySetd;
        %ignore keySetc;
        %ignore keySets;
        %ignore varSet;
        %ignore varSeti;
        %ignore varSetd;
        %ignore varSetc;
        %ignore varSets;
        %ignore keyUnset;
        %ignore varUnset;
        %rename seti seti_orig;
        %rename setd setd_orig;
        %rename sets sets_orig;
        %rename setc setc_orig;
        %rename set set_orig;
        %rename unset unset_orig;
        %pythoncode %{
                _enqdate_parms = ("year", "month", "day", "hour", "min", "sec")
                _enqdatemin_parms = ("yearmin", "monthmin", "daymin", "hourmin", "minumin", "secmin")
                _enqdatemax_parms = ("yearmax", "monthmax", "daymax", "hourmax", "minumax", "secmax")
                _enqlevel_parms = ("leveltype1", "l1", "leveltype2", "l2")
                _enqtimerange_parms = ("pindicator", "p1", "p2")
                def _enqmany(self, names):
                        # If there is an unset value among ours, return None
                        parms = []
                        for p in names:
                                v = self.enqi(p)
                                if v == None:
                                        return None
                                parms.append(v)
                        return parms
                def enqdate(self):
                        parms = self._enqmany(self._enqdate_parms)
                        if parms == None: return None
                        return datetime.datetime(*parms)
                def enqdatemin(self):
                        parms = self._enqmany(self._enqdatemin_parms)
                        if parms == None: return None
                        return datetime.datetime(*parms)
                def enqdatemax(self):
                        parms = self._enqmany(self._enqdatemax_parms)
                        if parms == None: return None
                        return datetime.datetime(*parms)
                def enqlevel(self):
                        parms = self._enqmany(self._enqlevel_parms)
                        if parms == None: return None
                        return Level(*parms)
                def enqtimerange(self):
                        parms = self._enqmany(self._enqtimerange_parms)
                        if parms == None: return None
                        return TimeRange(*parms)
                def setdate(self, dt):
                        if dt == None:
                                self.seti("year", None)
                                self.seti("year", None)
                                self.seti("month", None)
                                self.seti("day", None)
                                self.seti("hour", None)
                                self.seti("min", None)
                                self.seti("sec", None)
                        else:
                                self.seti("year", dt.year)
                                self.seti("month", dt.month)
                                self.seti("day", dt.day)
                                self.seti("hour", dt.hour)
                                self.seti("min", dt.minute)
                                self.seti("sec", dt.second)
                def setdatemin(self, dt):
                        if dt == None:
                                self.seti("yearmin", None)
                                self.seti("monthmin", None)
                                self.seti("daymin", None)
                                self.seti("hourmin", None)
                                self.seti("minumin", None)
                                self.seti("secmin", None)
                        else:
                                self.seti("yearmin", dt.year)
                                self.seti("monthmin", dt.month)
                                self.seti("daymin", dt.day)
                                self.seti("hourmin", dt.hour)
                                self.seti("minumin", dt.minute)
                                self.seti("secmin", dt.second)
                def setdatemax(self, dt):
                        if dt == None:
                                self.seti("yearmax", None)
                                self.seti("monthmax", None)
                                self.seti("daymax", None)
                                self.seti("hourmax", None)
                                self.seti("minumax", None)
                                self.seti("secmax", None)
                        else:
                                self.seti("yearmax", dt.year)
                                self.seti("monthmax", dt.month)
                                self.seti("daymax", dt.day)
                                self.seti("hourmax", dt.hour)
                                self.seti("minumax", dt.minute)
                                self.seti("secmax", dt.second)
                def setlevel(self, level):
                        if level == None:
                                self.seti("leveltype1", None)
                                self.seti("l1", None)
                                self.seti("leveltype2", None)
                                self.seti("l2", None)
                        else:
                                self.seti("leveltype1", level[0])
                                self.seti("l1", level[1])
                                self.seti("leveltype2", level[2])
                                self.seti("l2", level[3])
                def settimerange(self, trange):
                        if trange == None:
                                self.seti("pindicator", None)
                                self.seti("p1", None)
                                self.seti("p2", None)
                        else:
                                self.seti("pindicator", trange[0])
                                self.seti("p1", trange[1])
                                self.seti("p2", trange[2])

                _specialenqs = {
                        'date': enqdate,
                        'datemin': enqdatemin,
                        'datemax': enqdatemax,
                        'level': enqlevel,
                        'timerange': enqtimerange
                }
                _specialsets = {
                        'date': setdate,
                        'datemin': setdatemin,
                        'datemax': setdatemax,
                        'level': setlevel,
                        'timerange': settimerange
                }
                _specialunsets = {
                        'date': _enqdate_parms,
                        'datemin': _enqdatemin_parms,
                        'datemax': _enqdatemax_parms,
                        'level': _enqlevel_parms,
                        'timerange': _enqtimerange_parms,
                }

                def enq(self, name):
                       if name in self._specialenqs:
                                return self._specialenqs[name](self)
                       else:
                                return self.enqvar(name).enq()
                def enqi(self, name):
                       val, found = self.enqi_ifset(name)
                       if found: return val
                       return None
                def enqd(self, name):
                       val, found = self.enqd_ifset(name)
                       if found: return val
                       return None
                def enqs(self, name):
                       val, found = self.enqs_ifset(name)
                       if found: return val
                       return None
                def enqc(self, name):
                       val, found = self.enqs_ifset(name)
                       if found: return val
                       return None

                def set(self, *args):
                       if len(args) == 2:
                                if args[0] in self._specialsets:
                                        return self._specialsets[args[0]](self, args[1])
                                else:
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

                def unset(self, name):
                        if name in self._specialunsets:
                                for i in self._specialunsets[name]:
                                        self.unset(i)
                        else:
                                self.unset_orig(name)

                def __getitem__(self, key):
                        return self.enq(key)
                def __setitem__(self, key, val):
                        self.set(key, val)
                def __delitem__(self, key):
                        return self.unset(key)
                def __contains__(self, key):
                        if key in self._specialunsets:
                                has = True
                                for i in self._specialunsets[key]:
                                        if not self.contains(i):
                                                has = False
                                                break
                                return has
                        else:
                                self.contains(key)
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

%typemap(in) const std::vector<dba_varcode>& (std::vector<dba_varcode> vec) {
        if (!PySequence_Check($input))
                PyErr_SetString(PyExc_NotImplementedError,"A sequence is needed for the varcode list");
        int len = PyObject_Length($input);
        for (int i = 0; i < len; ++i)
        {
                PyObject* o = PySequence_GetItem($input, i);
                if (o == NULL) break;
                const char* str = PyString_AsString(o);
                dba_varcode vc = 0;
                if ((vc = dba_varcode_alias_resolve(str)) == 0)
                        vc = dba_descriptor_code(str);
                vec.push_back(vc);
        }
        $1 = &vec;
}
%typemap(typecheck) const std::vector<dba_varcode>& {
        $1 = PySequence_Check($input) ? 1 : 0;
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
                $1 = dba_descriptor_code(tmp);
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
%apply bool& OUTPUT { bool& found };
%apply int *OUTPUT { int *count };
%apply int *OUTPUT { int *contextid };
%apply int *OUTPUT { int *anaid };

#endif

%include <dballe++/var.h>
%include <dballe++/record.h>
%include <dballe++/bufrex.h>
%include <dballe++/msg.h>
%include <dballe++/db.h>
%include <dballe++/format.h>
