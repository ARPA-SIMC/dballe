#include <dballe/core/tests.h>
#include <dballe/message.h>
#include <dballe/importer.h>
#include <dballe/exporter.h>
#include <dballe/msg/msg.h>
#include <vector>

namespace wreport {
struct Vartable;
}

namespace dballe {
namespace tests {

impl::Messages read_msgs(const char* filename, Encoding type, const dballe::ImporterOptions& opts=dballe::ImporterOptions::defaults);
impl::Messages read_msgs(const char* filename, Encoding type, const std::string& opts);
impl::Messages read_msgs_csv(const char* filename);

struct ActualMessage : public Actual<const Message&>
{
    using Actual::Actual;

    void is_undef(const impl::Shortcut& shortcut) const;
};

inline ActualMessage actual(const Message& message) { return ActualMessage(message); }

std::unique_ptr<wreport::Bulletin> export_msgs(Encoding enctype, const impl::Messages& in, const std::string& tag, const dballe::ExporterOptions& opts=dballe::ExporterOptions::defaults);
#define test_export_msgs(...) wcallchecked(export_msgs(__VA_ARGS__))

void track_different_msgs(const Message& msg1, const Message& msg2, const std::string& prefix);
void track_different_msgs(const impl::Messages& msgs1, const impl::Messages& msgs2, const std::string& prefix);

extern const char* bufr_files[];
extern const char* crex_files[];

const wreport::Var& want_var(const Message& msg, const impl::Shortcut& shortcut);
const wreport::Var& want_var(const Message& msg, wreport::Varcode code, const dballe::Level& lev, const dballe::Trange& tr);

inline ActualVar actual(const Message& message, const impl::Shortcut& shortcut) { return ActualVar(want_var(message, shortcut)); }
inline ActualVar actual(const Message& message, wreport::Varcode code, const dballe::Level& lev, const dballe::Trange& tr) { return ActualVar(want_var(message, code, lev, tr)); }

void dump(const std::string& tag, const Message& msg, const std::string& desc="message");
void dump(const std::string& tag, const impl::Messages& msgs, const std::string& desc="message");
void dump(const std::string& tag, const wreport::Bulletin& bul, const std::string& desc="message");
void dump(const std::string& tag, const BinaryMessage& msg, const std::string& desc="message");
void dump(const std::string& tag, const std::string& str, const std::string& desc="message");

struct MessageTweaker
{
    virtual ~MessageTweaker() {}
    virtual void tweak(impl::Messages&) {}
    virtual std::string desc() const = 0;
};

struct MessageTweakers
{
    std::vector<MessageTweaker*> tweaks;

    ~MessageTweakers();
    // Takes ownership of memory management
    void add(MessageTweaker* tweak);
    void apply(impl::Messages& msgs);
};

namespace tweaks {

// Strip attributes from all variables in a impl::Messages
struct StripAttrs : public MessageTweaker
{
    std::vector<wreport::Varcode> codes;

    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "StripAttrs"; }
};

// Strip attributes from all variables in a impl::Messages
struct StripQCAttrs : public StripAttrs
{
    StripQCAttrs();
    virtual std::string desc() const { return "StripQCAttrs"; }
};

// Strip attributes with substituted values
struct StripSubstituteAttrs : public MessageTweaker
{
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "StripSubstituteAttrs"; }
};

// Strip context attributes from all variables in a impl::Messages
struct StripContextAttrs : public StripAttrs
{
    StripContextAttrs();
    virtual std::string desc() const { return "StripContextAttrs"; }
};

// Strip a user-defined list of vars from all levels
struct StripVars : public MessageTweaker
{
    std::vector<wreport::Varcode> codes;

    StripVars() {}
    StripVars(std::initializer_list<wreport::Varcode> codes) : codes(codes) {}
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "StripVars"; }
};

// Round variables to account for a passage through legacy vars
struct RoundLegacyVars : public MessageTweaker
{
    const wreport::Vartable* table;
    RoundLegacyVars();
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "RoundLegacyVars"; }
};

// Remove synop vars present in WMO templates but not in ECMWF templates
struct RemoveSynopWMOOnlyVars : public MessageTweaker
{
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "RemoveSynopWMOOnlyVars"; }
};

// Remove temp vars present in WMO templates but not in ECMWF templates
struct RemoveTempWMOOnlyVars : public MessageTweaker
{
    void tweak(impl::Messages& msgs);
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
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "RemoveSynopWMOOddprec"; }
};

// Truncate station name to its canonical length
struct TruncStName : public MessageTweaker
{
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "TruncStName"; }
};

// Round geopotential with a B10003->B10008->B10009->B10008->B10003 round trip
struct RoundGeopotential : public MessageTweaker
{
    const wreport::Vartable* table;
    RoundGeopotential();
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "RoundGeopotential"; }
};

// Add B10008 GEOPOTENTIAL to all height levels, with its value taken from the height
struct HeightToGeopotential : public MessageTweaker
{
    const wreport::Vartable* table;
    HeightToGeopotential();
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "HeightToGeopotential"; }
};

// Round vertical sounding significance with a B08042->B08001->B08042 round trip
struct RoundVSS : public MessageTweaker
{
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "RoundVSS"; }
};

// Remove a context given its level and time range
struct RemoveContext : public MessageTweaker
{
    Level lev;
    Trange tr;
    RemoveContext(const Level& lev, const Trange& tr);
    void tweak(impl::Messages& msgs);
    virtual std::string desc() const { return "RemoveContext"; }
};

}

struct TestMessage
{
    std::string name;
    Encoding type;
    BinaryMessage raw;
    wreport::Bulletin* bulletin = 0;
    impl::Messages msgs;

    TestMessage(Encoding type, const std::string& name);
    ~TestMessage();

    void read_from_file(const std::string& fname, const ImporterOptions& input_opts);
    void read_from_raw(const BinaryMessage& msg, const ImporterOptions& input_opts);
    void read_from_msgs(const impl::Messages& msgs, const ExporterOptions& export_opts);
    void dump() const;
};

struct TestCodec
{
    std::string fname;
    Encoding type;
    bool verbose = false;
    impl::ImporterOptions input_opts;
    impl::ExporterOptions output_opts;
    std::string expected_template;
    int expected_subsets = 1;
    int expected_min_vars = 1;
    int expected_data_category = MISSING_INT;
    int expected_data_subcategory = MISSING_INT;
    int expected_data_subcategory_local = MISSING_INT;
    MessageTweakers after_reimport_import;
    MessageTweakers after_reimport_reimport;
    MessageTweakers after_convert_import;
    MessageTweakers after_convert_reimport;

    void do_compare(const TestMessage& msg1, const TestMessage& msg2);

    TestCodec(const std::string& fname, Encoding type=Encoding::BUFR);

    void configure_ecmwf_to_wmo_tweaks();

    // "import, export, import again, compare" test
    void run_reimport();

    // "import, export as different template, import again, compare" test
    void run_convert(const std::string& tplname);
};


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
