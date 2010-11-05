// dballe.i - SWIG interface
%module dballe

%include "stl.i"
%include "typemaps.i"
%include "exception.i"
%include "std_string.i"
%include "std_vector.i"
%include "../doc/dballe-doc.i"

%exception {
        try { $action }
        catch (std::exception& e)
        {
                SWIG_exception(SWIG_RuntimeError, e.what());
        }
}

// %apply std::string {dballe::Rawmsg};

%{
#include <dballe/core/aliases.h>
#include <wreport/vartable.h>
#include <wreport/var.h>
#include <wreport/subset.h>
#include <wreport/bulletin.h>
#include <dballe/core/var.h>
#include <dballe/core/record.h>
#include <dballe/core/rawmsg.h>
#include <dballe/core/defs.h>
#include <dballe/core/file.h>
#include <dballe/db/db.h>
#include <dballe/db/cursor.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/codec.h>

using namespace wreport;
using namespace dballe;
%}

/*
%typemap(in) int {
        if ($input == Py_None)
                $1 = MISSING_INT;
        else
                $1 = PyInt_AsLong($input);
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_INTEGER) int {
           $1 = ($input == Py_None || PyInt_Check($input)) ? 1 : 0;
}
*/

%typemap(in) wreport::Varcode {
        const char* tmp = PyString_AsString($input);
        if (($1 = varcode_alias_resolve(tmp)) == 0)
                $1 = descriptor_code(tmp);
}

%typemap(out) wreport::Varcode {
        char buf[10];
        snprintf(buf, 10, "B%02d%03d", WR_VAR_X($1), WR_VAR_Y($1));
        $result = PyString_FromString(buf);
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_STRING) wreport::Varcode {
        $1 = PyString_Check($input) ? 1 : 0;
}

%ignore dballe::newvar;

namespace dballe {
        wreport::Var var(wreport::Varcode code) { return wreport::Var(varinfo(code)); }
        wreport::Var var(wreport::Varcode code, int val) { return wreport::Var(varinfo(code), val); }
        wreport::Var var(wreport::Varcode code, double val) { return wreport::Var(varinfo(code), val); }
        wreport::Var var(wreport::Varcode code, const char* val) { return wreport::Var(varinfo(code), val); }
};

%typemap(in) DBALLE_SQL_C_SINT_TYPE {
        $1 = PyInt_AsLong($input);
}

%typemap(out) DBALLE_SQL_C_SINT_TYPE {
        $result = PyInt_FromLong($1);
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_INTEGER) wreport::Varcode {
        $1 = PyInt_Check($input) ? 1 : 0;
}

%typemap(in) dba_keyword {
        $1 = record_keyword_byname(PyString_AsString($input));
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_STRING) dba_keyword {
        $1 = PyString_Check($input) ? 1 : 0;
}

%typemap(in) const std::vector<wreport::Varcode>& (std::vector<wreport::Varcode> vec) {
        if (!PySequence_Check($input))
                PyErr_SetString(PyExc_NotImplementedError,"A sequence is needed for the varcode list");
        int len = PyObject_Length($input);
        for (int i = 0; i < len; ++i)
        {
                PyObject* o = PySequence_GetItem($input, i);
                if (o == NULL) break;
                const char* str = PyString_AsString(o);
                wreport::Varcode vc = 0;
                if ((vc = varcode_alias_resolve(str)) == 0)
                        vc = descriptor_code(str);
                vec.push_back(vc);
        }
        $1 = &vec;
}
%typemap(typecheck) const std::vector<wreport::Varcode>& {
        $1 = PySequence_Check($input) ? 1 : 0;
}

%typemap(in) dballe::Encoding {
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

namespace std {
        %template(VartableBase) vector<wreport::_Varinfo>;
        %template(VarpVector) vector<wreport::Var*>;
//        %template(VarVector) vector<wreport::Var>;
//        %template(VarcodeVector) vector<wreport::Varcode>;
//        %template(SubsetVector) vector<wreport::Subset>;
};

%extend wreport::Varinfo {
        %ignore _ref;

#ifdef SWIGPYTHON
        %typemap(out) char[64] {
                $result = PyString_FromString($1);
        }
        %typemap(out) char[24] {
                $result = PyString_FromString($1);
        }

        %pythoncode %{
                def __str__(self):
                        return "%s %s" % (self.var, self.desc)
                def __repr__(self):
                        return "<Varinfo %s,%s,%s,scale %d,len %d,string %s,irange %d..%d,frange: %f..%f>" % (
                                self.var, self.unit, self.desc, self.scale, self.len,
                                str(self.is_string()), self.imin, self.imax, self.dmin, self.dmax)
        %}
#endif
}

%extend wreport::Var {
#ifdef SWIGPYTHON
        %rename(equals) operator==;
        %ignore seta(std::auto_ptr<Var> attr);

        %pythoncode %{
                def __eq__(self, var):
                        if var is None:
                                return False
                        elif not isinstance(var, Var):
                                return self.enq() == var
                        else:
                                return self.equals(var)

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
                        return "<Var %s, %s>" % (self.code(), self.format("None"))
                def enq(self):
                        if self.info().is_string():
                                return self.enqc()
                        elif self.info().scale == 0:
                                return self.enqi()
                        else:
                                return self.enqd()
        %}
#endif
}

// Rewrite Record methods to make use of the None value, and add convenience
// methods and iteration
%extend dballe::Record {
        %rename(equals) operator==;
        %pythoncode %{
                def __eq__(self, rec):
                        if rec is None:
                                return False
                        elif not isinstance(rec, Record):
                                return False
                        else:
                                return self.equals(record)
        %}

        std::string _format_keys()
        {
                std::string res;
                for (int i = 0; i < DBA_KEY_COUNT; ++i)
                        if (const wreport::Var* var = $self->key_peek((dba_keyword)i))
                        {
                                if (!res.empty())
                                        res += ", ";
                                res += dballe::Record::keyword_name((dba_keyword)i);
                                res += ": ";
                                res += var->format();
                        }
                return res;
        }

        %ignore key;
        %ignore var;
        %ignore key_peek;
        %ignore var_peek;
        %ignore key_peek_value;
        %ignore var_peek_value;
        %ignore key_unset;
        %ignore var_unset;
        %ignore get(dba_keyword parameter) const;
        %ignore get(wreport::Varcode code) const;
        %ignore get(dba_keyword parameter);
        %ignore get(wreport::Varcode code);
        %ignore operator[];
        %ignore unset(dba_keyword parameter);
        %ignore unset(wreport::Varcode code);
        %ignore parse_date_extremes;
        %rename (getvar) get;

        %pythoncode %{
                def copy(self):
                        return Record(self)
        %}

        // Getters and setters
        %ignore set;
        %pythoncode %{
                def update(self, pairs):
                        if isinstance(pairs, dict):
                                for k, v in pairs.iteritems():
                                        self[k] = v
                        elif isinstance(pairs, Var):
                                self[pairs.code()] = pairs.enq()
                        else:
                                for k, v in pairs:
                                        self[k] = v
                def _get_iter(self, *args):
                        for x in args:
                                if x in self:
                                        yield self.getvar(x).enq()
                                else:
                                        yield None
                def _get_dt(self, *args):
                        res = []
                        for idx, x in enumerate(args):
                                if x in self:
                                        res.append(self.getvar(x).enq())
                                elif idx < 3:
                                        return None
                                else:
                                        res.append(0)
                        import datetime
                        return datetime.datetime(*res)

                KEYS_DATE = ("year", "month", "day", "hour", "min", "sec")
                KEYS_DATEMIN = ("yearmin", "monthmin", "daymin", "hourmin", "minumin", "secmin")
                KEYS_DATEMAX = ("yearmax", "monthmax", "daymax", "hourmax", "minumax", "secmax")
                KEYS_LEVEL = ("leveltype1", "l1", "leveltype2", "l2")
                KEYS_TRANGE = ("pindicator", "p1", "p2")

                def _macro_get_date(self):
                        return self._get_dt(*self.KEYS_DATE)
                def _macro_get_datemin(self):
                        return self._get_dt(*self.KEYS_DATEMIN)
                def _macro_get_datemax(self):
                        return self._get_dt(*self.KEYS_DATEMAX)
                def _macro_get_level(self):
                        return tuple(self._get_iter(*self.KEYS_LEVEL))
                def _macro_get_trange(self):
                        return tuple(self._get_iter(*self.KEYS_TRANGE))
                _macro_get_timerange = _macro_get_trange

                def __getitem__(self, key):
                        "Query one value by name"
                        macro = getattr(self, "_macro_get_" + key, None)
                        if macro:
                                return macro()
                        else:
                                return self.getvar(key).enq()

                def _macro_set_date(self, dt):
                        for kd, kr in zip(("year", "month", "day", "hour", "minute", "second"), self.KEYS_DATE):
                                self.getvar(kr).set(getattr(dt, kd))
                def _macro_set_datemin(self, dt):
                        for kd, kr in zip(("year", "month", "day", "hour", "minute", "second"), self.KEYS_DATEMIN):
                                self.getvar(kr).set(getattr(dt, kd))
                def _macro_set_datemax(self, dt):
                        for kd, kr in zip(("year", "month", "day", "hour", "minute", "second"), self.KEYS_DATEMAX):
                                self.getvar(kr).set(getattr(dt, kd))
                def _macro_set_level(self, tu):
                        for idx, key in enumerate(self.KEYS_LEVEL):
                                if idx >= len(tu) or tu[idx] is None:
                                        self.unset(key)
                                else:
                                        self.getvar(key).set(tu[idx])
                def _macro_set_trange(self, tu):
                        for idx, key in enumerate(self.KEYS_TRANGE):
                                if idx >= len(tu) or tu[idx] is None:
                                        self.unset(key)
                                else:
                                        self.getvar(key).set(tu[idx])
                _macro_set_timerange = _macro_set_trange

                def __setitem__(self, key, val):
                        "Set one value by name"
                        macro = getattr(self, "_macro_set_" + key, None)
                        if macro:
                                return macro(val)
                        else:
                                self.getvar(key).set(val)

                def _macro_del_date(self):
                        for k in self.KEYS_DATE:
                                self.unset(k)
                def _macro_del_datemin(self):
                        for k in self.KEYS_DATEMIN:
                                self.unset(k)
                def _macro_del_datemax(self):
                        for k in self.KEYS_DATEMAX:
                                self.unset(k)
                def _macro_del_level(self):
                        for k in self.KEYS_LEVEL:
                                self.unset(k)
                def _macro_del_trange(self):
                        for k in self.KEYS_TRANGE:
                                self.unset(k)
                _macro_del_timerange = _macro_del_trange

                def __delitem__(self, key):
                        "Unset one value by name"
                        macro = getattr(self, "_macro_del_" + key, None)
                        if macro:
                                return macro()
                        else:
                                self.unset(key)

                def _macro_has_date(self):
                        return all([self.peek_value(k) != None for k in self.KEYS_DATE[:3]])
                def _macro_has_datemin(self):
                        return all([self.peek_value(k) != None for k in self.KEYS_DATEMIN[:3]])
                def _macro_has_datemax(self):
                        return all([self.peek_value(k) != None for k in self.KEYS_DATEMAX[:3]])
                def _macro_has_level(self):
                        return True
                def _macro_has_trange(self):
                        return True
                _macro_has_timerange = _macro_has_trange

                def __contains__(self, key):
                        "Check if a value is set"
                        macro = getattr(self, "_macro_has_" + key, None)
                        if macro:
                                return macro()
                        else:
                                return self.peek_value(key) != None

                def __iter__(self):
                        "Iterate all the variables of the record"
                        for v in self.vars():
                                yield v
                def __len__(self):
                        "Number of variables in the record"
                        return len(self.vars())
                def keys(self):
                        "List of names of variables in the record"
                        return [x.code() for x in self.vars()]
                def iterkeys(self):
                        "List of names of variables in the record"
                        return (x.code() for x in self.vars())
                def values(self):
                        "List of names of variables in the record"
                        return [x.enq() for x in self.vars()]
                def itervalues(self):
                        "List of names of variables in the record"
                        return (x.enq() for x in self.vars())
                def items(self):
                        "List of names of variables in the record"
                        return [(x.code(), x.enq()) for x in self.vars()]
                def iteritems(self):
                        "List of names of variables in the record"
                        return ((x.code(), x.enq()) for x in self.vars())
                def itervars(self):
                        "Iterate all the variables in the record"
                        return (x for x in self.vars())
                def __str__(self):
                        keys = self._format_keys()
                        items = ", ".join(("%s: %s" % (a, b) for a, b in self.iteritems()))
                        return "{" + ", ".join((keys, items)) + "}"
                def __repr__(self):
                        return "<Record %s>" % self.__str__()
                def get(self, key, *args):
                        if key in self:
                                return self[key]
                        elif args:
                                return args[0]
                        else:
                                raise KeyError, key
                def pop(self, key, *args):
                        if key in self:
                                res = self[key]
                                del self[key]
                                return res
                        elif args:
                                return args[0]
                        else:
                                raise KeyError, key
        %}
}

%extend wreport::Bulletin {
        void datadesc_append(Varcode code)
        {
                $self->datadesc.push_back(code);
        }
        std::string encode()
        {
                std::string res;
                $self->encode(res);
                return res;
        }
        void subsets_clear() { $self->subsets.clear(); }
        size_t subsets_size() { return $self->subsets.size(); }
}
%extend wreport::BufrBulletin {
        std::string encode()
        {
                std::string res;
                $self->encode(res);
                return res;
        }
        void subsets_clear() { $self->subsets.clear(); }
        size_t subsets_size() { return $self->subsets.size(); }
}
%extend wreport::CrexBulletin {
        std::string encode()
        {
                std::string res;
                $self->encode(res);
                return res;
        }
        void subsets_clear() { $self->subsets.clear(); }
        size_t subsets_size() { return $self->subsets.size(); }
}

%extend dballe::DB {
        %ignore query;
        %ignore query_stations;
        %ignore query_data;
        dballe::db::Cursor* _query(const dballe::Record& rec, unsigned int wanted, unsigned int modifiers)
        {
                std::auto_ptr<db::Cursor> res = $self->query(rec, wanted, modifiers);
                return res.release();
        }
        dballe::db::Cursor* _query_stations(const dballe::Record& rec)
        {
                std::auto_ptr<db::Cursor> res = $self->query_stations(rec);
                return res.release();
        }
        dballe::db::Cursor* _query_data(const dballe::Record& rec)
        {
                std::auto_ptr<db::Cursor> res = $self->query_data(rec);
                return res.release();
        }
        %pythoncode %{
                def query(self, *args): return self._query(*args)
                def query_stations(self, *args): return self._query_stations(*args)
                def query_data(self, *args): return self._query_data(*args)
        %}
        dballe::db::Cursor* query_station_summary(const dballe::Record& rec)
        {
                return $self->query(rec, DBA_DB_WANT_ANA_ID | DBA_DB_WANT_COORDS | DBA_DB_WANT_IDENT,
                                DBA_DB_MODIFIER_DISTINCT).release();

        }
        dballe::db::Cursor* query_levels(const dballe::Record& rec)
        {
                return $self->query(rec, DBA_DB_WANT_LEVEL, DBA_DB_MODIFIER_DISTINCT).release();
        }
        dballe::db::Cursor* query_tranges(const dballe::Record& rec)
        {
                return $self->query(rec, DBA_DB_WANT_TIMERANGE, DBA_DB_MODIFIER_DISTINCT).release();
        }
        dballe::db::Cursor* query_levels_tranges(const dballe::Record& rec)
        {
                return $self->query(rec, DBA_DB_WANT_LEVEL | DBA_DB_WANT_TIMERANGE, DBA_DB_MODIFIER_DISTINCT).release();
        }
        dballe::db::Cursor* query_datetimes(const dballe::Record& rec)
        {
                return $self->query(rec, DBA_DB_WANT_DATETIME, DBA_DB_MODIFIER_DISTINCT).release();
        }
        dballe::db::Cursor* query_reports(const dballe::Record& rec)
        {
                return $self->query(rec, DBA_DB_WANT_REPCOD, DBA_DB_MODIFIER_DISTINCT).release();
        }
        dballe::db::Cursor* query_idents(const dballe::Record& rec)
        {
                return $self->query(rec, DBA_DB_WANT_IDENT, DBA_DB_MODIFIER_DISTINCT).release();
        }
        dballe::db::Cursor* query_variable_types(const dballe::Record& rec)
        {
                return $self->query(rec, DBA_DB_WANT_VAR_NAME, DBA_DB_MODIFIER_DISTINCT | DBA_DB_MODIFIER_NOANAEXTRA ).release();
        }
        void export_results(const dballe::Record& query, Encoding encoding, const std::string& file)
        {
                std::auto_ptr<dballe::File> out = dballe::File::create(encoding, file.c_str(), "wb");
                struct Consumer : public MsgConsumer {
                        dballe::File& out;
                        dballe::msg::Exporter* exporter;
                        Consumer(dballe::File& out)
                                : out(out), exporter(0)
                        {
                                exporter = msg::Exporter::create(out.type()).release();
                        }
                        ~Consumer()
                        {
                                if (exporter) delete exporter;
                        }
                        void operator()(std::auto_ptr<Msg> msg)
                        {
                                Rawmsg raw;
                                Msgs msgs;
                                msgs.acquire(msg);
                                exporter->to_rawmsg(msgs, raw);
                                out.write(raw);
                        }
                } msg_writer(*out);
                $self->export_msgs(query, msg_writer);
        }

        void export_results_as_generic(const dballe::Record& query, Encoding encoding, const std::string& file)
        {
                std::auto_ptr<dballe::File> out = dballe::File::create(encoding, file.c_str(), "wb");
                struct Consumer : public MsgConsumer {
                        dballe::File& out;
                        dballe::msg::Exporter* exporter;
                        Consumer(dballe::File& out)
                                : out(out), exporter(0)
                        {
                                msg::Exporter::Options opts;
                                opts.template_name = "generic";
                                exporter = msg::Exporter::create(out.type(), opts).release();
                        }
                        ~Consumer()
                        {
                                if (exporter) delete exporter;
                        }
                        void operator()(std::auto_ptr<Msg> msg)
                        {
                                Rawmsg raw;
                                Msgs msgs;
                                msgs.acquire(msg);
                                exporter->to_rawmsg(msgs, raw);
                                out.write(raw);
                        }
                } msg_writer(*out);
                $self->export_msgs(query, msg_writer);
        }

}

%extend dballe::db::Cursor {
//        %rename attributes attributes_orig;
        %pythoncode %{
                def __iter__(self):
                        record = Record()
                        while self.next():
                                self.to_record(record)
                                yield record
/*
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
*/
        %}
}


%include <wreport/varinfo.h>
%include <wreport/vartable.h>
%include <wreport/var.h>
%include <dballe/core/var.h>
%include <dballe/core/record.h>
%include <dballe/core/rawmsg.h>
%include <dballe/core/defs.h>
%include <wreport/subset.h>
%include <wreport/bulletin.h>
%include <dballe/db/db.h>
%include <dballe/db/cursor.h>

/*
Varinfo varinfo(Varcode code)
{
        return Vartable::get("dballe")->query(code);
}
*/
