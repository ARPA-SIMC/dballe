/*
 * Copyright (C) 2005--2012  ARPA-SIM <urpsim@smr.arpa.emr.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: Enrico Zini <enrico@enricozini.com>
 */

#include <dballe/core/test-utils-core.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/codec.h>
#if 0
#include <dballe/msg/bufrex_codec.h>
#include <dballe/msg/file.h>
#include <dballe/msg/marshal.h>

#include <string>
#include <iostream>
#endif
#include <vector>

namespace wreport {
struct Vartable;
}

namespace dballe {
namespace tests {

typedef wibble::tests::Location Location;

std::auto_ptr<Msgs> _read_msgs(const Location& loc, const char* filename, Encoding type, const dballe::msg::Importer::Options& opts=dballe::msg::Importer::Options());
#define read_msgs(filename, type) dballe::tests::_read_msgs(wibble::tests::Location(__FILE__, __LINE__, "load " #filename " " #type), (filename), (type))
#define inner_read_msgs(filename, type) dballe::tests::_read_msgs(wibble::tests::Location(loc, __FILE__, __LINE__, "load " #filename " " #type), (filename), (type))
#define read_msgs_opts(filename, type, opts) dballe::tests::_read_msgs(wibble::tests::Location(__FILE__, __LINE__, "load " #filename " " #type), (filename), (type), (opts))
#define inner_read_msgs_opts(filename, type, opts) dballe::tests::_read_msgs(wibble::tests::Location(loc, __FILE__, __LINE__, "load " #filename " " #type), (filename), (type), (opts))

std::auto_ptr<Msgs> _read_msgs_csv(const Location& loc, const char* filename);
#define read_msgs_csv(filename) dballe::tests::_read_msgs_csv(wibble::tests::Location(__FILE__, __LINE__, "load csv " #filename), (filename))
#define inner_read_msgs_csv(filename) dballe::tests::_read_msgs_csv(wibble::tests::Location(loc, __FILE__, __LINE__, "load csv " #filename), (filename))

void _export_msgs(const Location& loc, const Msgs& in, wreport::Bulletin& out, const std::string& tag, const dballe::msg::Exporter::Options& opts=dballe::msg::Exporter::Options());
#define test_export_msgs(...) dballe::tests::_export_msgs(wibble::tests::Location(__FILE__, __LINE__, "export"), __VA_ARGS__)
#define test_inner_export_msgs(...) dballe::tests::_export_msgs(wibble::tests::Location(loc, __FILE__, __LINE__, "export"), __VA_ARGS__)

void track_different_msgs(const Msg& msg1, const Msg& msg2, const std::string& prefix);
void track_different_msgs(const Msgs& msgs1, const Msgs& msgs2, const std::string& prefix);

extern const char* bufr_files[];
extern const char* crex_files[];
extern const char* aof_files[];

void _ensure_msg_undef(const Location& loc, const Msg& msg, int shortcut);
#define ensure_msg_undef(msg, id) dballe::tests::_ensure_msg_undef(wibble::tests::Location(__FILE__, __LINE__, #msg " has undefined " #id), (msg), (id))
#define inner_ensure_msg_undef(msg, id) dballe::tests::_ensure_msg_undef(wibble::tests::Location(loc, __FILE__, __LINE__, #msg " has undefined " #id), (msg), (id))

const wreport::Var& _want_var(const Location& loc, const Msg& msg, int shortcut);
const wreport::Var& _want_var(const Location& loc, const Msg& msg, wreport::Varcode code, const dballe::Level& lev, const dballe::Trange& tr);
#define want_var(msg, ...) dballe::tests::_want_var(wibble::tests::Location(__FILE__, __LINE__, #msg " needs to have var " #__VA_ARGS__), (msg), __VA_ARGS__)

void dump(const std::string& tag, const Msg& msg, const std::string& desc="message");
void dump(const std::string& tag, const Msgs& msgs, const std::string& desc="message");
void dump(const std::string& tag, const wreport::Bulletin& bul, const std::string& desc="message");
void dump(const std::string& tag, const Rawmsg& msg, const std::string& desc="message");
void dump(const std::string& tag, const std::string& str, const std::string& desc="message");

struct MessageTweaker
{
    virtual ~MessageTweaker() {}
    virtual void tweak(Msgs&) {}
    virtual std::string desc() const = 0;
};

struct MessageTweakers
{
    std::vector<MessageTweaker*> tweaks;

    ~MessageTweakers();
    // Takes ownership of memory management
    void add(MessageTweaker* tweak);
    void apply(Msgs& msgs);
};

namespace tweaks {

// Strip attributes from all variables in a Msgs
struct StripAttrs : public MessageTweaker
{
    std::vector<wreport::Varcode> codes;

    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "StripAttrs"; }
};

// Strip attributes from all variables in a Msgs
struct StripQCAttrs : public StripAttrs
{
    StripQCAttrs();
    virtual std::string desc() const { return "StripQCAttrs"; }
};

// Strip attributes with substituted values
struct StripSubstituteAttrs : public MessageTweaker
{
    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "StripSubstituteAttrs"; }
};

// Strip context attributes from all variables in a Msgs
struct StripContextAttrs : public StripAttrs
{
    StripContextAttrs();
    virtual std::string desc() const { return "StripContextAttrs"; }
};

// Strip a user-defined list of vars from all levels
struct StripVars : public MessageTweaker
{
    std::vector<wreport::Varcode> codes;

    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "StripVars"; }
};

// Round variables to account for a passage through legacy vars
struct RoundLegacyVars : public MessageTweaker
{
    const wreport::Vartable* table;
    RoundLegacyVars();
    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "RoundLegacyVars"; }
};

// Remove synop vars present in WMO templates but not in ECMWF templates
struct RemoveSynopWMOOnlyVars : public MessageTweaker
{
    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "RemoveSynopWMOOnlyVars"; }
};

// Remove temp vars present in WMO templates but not in ECMWF templates
struct RemoveTempWMOOnlyVars : public MessageTweaker
{
    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "RemoveTempWMOOnlyVars"; }
};

// Remove temp vars present only in an odd temp template for which we have
// messages in the test suite
struct RemoveOddTempTemplateOnlyVars : public StripVars
{
    RemoveOddTempTemplateOnlyVars();
    virtual std::string desc() const { return "RemoveOddTempTemplateOnlyVars"; }
};

// Remove ground level with missing length of statistical processing, that
// cannot be encoded in ECMWF templates
struct RemoveSynopWMOOddprec : public MessageTweaker
{
    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "RemoveSynopWMOOddprec"; }
};

// Truncate station name to its canonical length
struct TruncStName : public MessageTweaker
{
    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "TruncStName"; }
};

// Round geopotential with a B10003->B10008->B10009->B10008->B10003 round trip
struct RoundGeopotential : public MessageTweaker
{
    const wreport::Vartable* table;
    RoundGeopotential();
    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "RoundGeopotential"; }
};

// Add B10008 GEOPOTENTIAL to all height levels, with its value taken from the height
struct HeightToGeopotential : public MessageTweaker
{
    const wreport::Vartable* table;
    HeightToGeopotential();
    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "HeightToGeopotential"; }
};

// Round vertical sounding significance with a B08042->B08001->B08042 round trip
struct RoundVSS : public MessageTweaker
{
    void tweak(Msgs& msgs);
    virtual std::string desc() const { return "RoundVSS"; }
};

}

struct TestMessage
{
    const std::string& name;
    Encoding type;
    Rawmsg raw;
    wreport::Bulletin* bulletin;
    Msgs msgs;

    TestMessage(Encoding type, const std::string& name);
    ~TestMessage();

    /// Create a bulletin for the current encoding
    std::auto_ptr<wreport::Bulletin> create_bulletin();

    void read_from_file(const std::string& fname, const msg::Importer::Options& input_opts);
    void read_from_raw(const Rawmsg& msg, const msg::Importer::Options& input_opts);
    void read_from_msgs(const Msgs& msgs, const msg::Exporter::Options& export_opts);
};

struct TestCodec
{
    std::string fname;
    Encoding type;
    bool verbose;
    msg::Importer::Options input_opts;
    msg::Exporter::Options output_opts;
    int expected_subsets;
    int expected_min_vars;
    MessageTweakers after_reimport_import;
    MessageTweakers after_reimport_reimport;
    MessageTweakers after_convert_import;
    MessageTweakers after_convert_reimport;

    void do_compare(const dballe::tests::Location& loc, const TestMessage& msg1, const TestMessage& msg2);

    TestCodec(const std::string& fname, Encoding type=BUFR);

    void configure_ecmwf_to_wmo_tweaks();

    // "import, export, import again, compare" test
    void run_reimport(const dballe::tests::Location& loc);

    // "import, export as different template, import again, compare" test
    void run_convert(const dballe::tests::Location& loc, const std::string& tplname);
};

#define TEST_reimport(obj) obj.run_reimport(wibble::tests::Location(__FILE__, __LINE__, #obj ".run_reimport"))
#define TEST_convert(obj, tpl) obj.run_convert(wibble::tests::Location(__FILE__, __LINE__, #obj ".run_convert " tpl), tpl)


#if 0

/* Random message generation functions */

class msg_generator : public generator
{
public:
	dba_err fill_message(dba_msg msg, bool mobile);
};


/* Message reading functions */

class msg_vector : public dba_raw_consumer, public std::vector<dba_msgs>
{
public:
	virtual ~msg_vector()
	{
		for (iterator i = begin(); i != end(); i++)
			dba_msgs_delete(*i);
	}
		
	virtual dba_err consume(dba_rawmsg raw)
	{
		dba_msgs msgs;

		DBA_RUN_OR_RETURN(dba_marshal_decode(raw, &msgs));
		push_back(msgs);

		return dba_error_ok();
	}
};

template <typename T>
void my_ensure_msg_equals(const char* file, int line, dba_msg msg, int id, const char* idname, const T& value)
{
	dba_var var = my_want_var(file, line, msg, id, idname);
	inner_ensure_var_equals(var, value);
}
#define gen_ensure_msg_equals(msg, id, value) my_ensure_msg_equals(__FILE__, __LINE__, (msg), (id), #id, (value))
#define inner_ensure_msg_equals(msg, id, value) my_ensure_msg_equals(file, line, (msg), (id), #id, (value))
#endif

}
}

// vim:set ts=4 sw=4:
