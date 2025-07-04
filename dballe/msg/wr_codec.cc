#include "wr_codec.h"
#include "context.h"
#include "dballe/core/shortcuts.h"
#include "dballe/file.h"
#include "domain_errors.h"
#include "msg.h"
#include "wr_importers/base.h"
#include <wreport/bulletin.h>
#include <wreport/options.h>
#include <wreport/vartable.h>

using namespace wreport;
using namespace std;

namespace dballe {
namespace impl {
namespace msg {

WRImporter::WRImporter(const dballe::ImporterOptions& opts)
    : BulletinImporter(opts)
{
}

BufrImporter::BufrImporter(const dballe::ImporterOptions& opts)
    : WRImporter(opts)
{
}
BufrImporter::~BufrImporter() {}

bool BufrImporter::foreach_decoded(
    const BinaryMessage& msg,
    std::function<bool(std::shared_ptr<dballe::Message>)> dest) const
{
    unique_ptr<BufrBulletin> bulletin(BufrBulletin::decode(msg.data));
    return foreach_decoded_bulletin(*bulletin, dest);
}

CrexImporter::CrexImporter(const dballe::ImporterOptions& opts)
    : WRImporter(opts)
{
}
CrexImporter::~CrexImporter() {}

bool CrexImporter::foreach_decoded(
    const BinaryMessage& msg,
    std::function<bool(std::shared_ptr<dballe::Message>)> dest) const
{
    unique_ptr<CrexBulletin> bulletin(CrexBulletin::decode(msg.data));
    return foreach_decoded_bulletin(*bulletin, dest);
}

Messages WRImporter::from_bulletin(const wreport::Bulletin& msg) const
{
    Messages res;
    foreach_decoded_bulletin(msg, [&](std::shared_ptr<dballe::Message>&& m) {
        res.emplace_back(move(m));
        return true;
    });
    return res;
}

bool WRImporter::foreach_decoded_bulletin(
    const wreport::Bulletin& msg,
    std::function<bool(std::shared_ptr<dballe::Message>)> dest) const
{
    WreportVarOptionsForImport wreport_config(opts.domain_errors);

    // Infer the right importer. See Common Code Table C-13
    std::unique_ptr<wr::Importer> importer;
    switch (msg.data_category)
    {
        // Surface data - land
        case 0:
            switch (msg.data_subcategory)
            {
                // Routine aeronautical observations (METAR)
                case 10: importer = wr::Importer::createMetar(opts); break;
                default:
                    // Old ECMWF METAR type
                    if (msg.data_subcategory_local == 140)
                        importer = wr::Importer::createMetar(opts);
                    else
                        importer = wr::Importer::createSynop(opts);
                    break;
            }
            break;
        // Surface data - sea
        case 1: importer = wr::Importer::createShip(opts); break;
        // Vertical soundings (other than satellite)
        case 2: importer = wr::Importer::createTemp(opts); break;
        // Vertical soundings (satellite)
        case 3: importer = wr::Importer::createSat(opts); break;
        // Single level upper-air data (other than satellite)
        case 4: importer = wr::Importer::createFlight(opts); break;
        // Radar data
        case 6:
            if (msg.data_subcategory == 1)
                // Doppler wind profiles
                importer = wr::Importer::createTemp(opts);
            else
                importer = wr::Importer::createGeneric(opts);
            break;
        // Physical/chemical constituents
        case 8:  importer = wr::Importer::createPollution(opts); break;
        default: importer = wr::Importer::createGeneric(opts); break;
    }

    MessageType type = importer->scanType(msg);
    for (unsigned i = 0; i < msg.subsets.size(); ++i)
    {
        auto newmsg  = std::make_shared<Message>();
        newmsg->type = type;
        importer->import(msg.subsets[i], *newmsg);
        if (!dest(newmsg))
            return false;
    }
    return true;
}

WRExporter::WRExporter(const dballe::ExporterOptions& opts)
    : BulletinExporter(opts)
{
}

BufrExporter::BufrExporter(const dballe::ExporterOptions& opts)
    : WRExporter(opts)
{
}

BufrExporter::~BufrExporter() {}

std::unique_ptr<wreport::Bulletin> BufrExporter::make_bulletin() const
{
    return std::unique_ptr<wreport::Bulletin>(BufrBulletin::create().release());
}

std::string BufrExporter::to_binary(const Messages& msgs) const
{
    return to_bulletin(msgs)->encode();
}

CrexExporter::CrexExporter(const dballe::ExporterOptions& opts)
    : WRExporter(opts)
{
}
CrexExporter::~CrexExporter() {}

std::unique_ptr<wreport::Bulletin> CrexExporter::make_bulletin() const
{
    return std::unique_ptr<wreport::Bulletin>(CrexBulletin::create().release());
}

std::string CrexExporter::to_binary(const Messages& msgs) const
{
    return to_bulletin(msgs)->encode();
}

namespace {

const char* infer_from_message(const Message& msg)
{
    switch (msg.type)
    {
        case MessageType::TEMP_SHIP: return "temp-ship";
        default:                     break;
    }
    return format_message_type(msg.type);
}

} // namespace

unique_ptr<wr::Template> WRExporter::infer_template(const Messages& msgs) const
{
    // Select initial template name
    string tpl = opts.template_name;
    if (tpl.empty())
        tpl = infer_from_message(Message::downcast(*msgs[0]));

    // Get template factory
    const wr::TemplateFactory& fac = wr::TemplateRegistry::get(tpl);
    return fac.factory(opts, msgs);
}

unique_ptr<Bulletin> WRExporter::to_bulletin(const Messages& msgs) const
{
    std::unique_ptr<wr::Template> encoder = infer_template(msgs);
    // fprintf(stderr, "Encoding with template %s\n", encoder->name());
    auto res                              = make_bulletin();
    encoder->to_bulletin(*res);
    return res;
}

namespace wr {

extern void register_synop(TemplateRegistry&);
extern void register_ship(TemplateRegistry&);
extern void register_buoy(TemplateRegistry&);
extern void register_metar(TemplateRegistry&);
extern void register_temp(TemplateRegistry&);
extern void register_flight(TemplateRegistry&);
extern void register_generic(TemplateRegistry&);
extern void register_pollution(TemplateRegistry&);

static TemplateRegistry* registry = NULL;
const TemplateRegistry& TemplateRegistry::get()
{
    if (!registry)
    {
        registry = new TemplateRegistry;

        registry->register_factory(
            MISSING_INT, "wmo", "WMO style templates (autodetect)",
            [](const dballe::ExporterOptions& opts, const Messages& msgs) {
                auto msg = Message::downcast(msgs[0]);
                string tpl;
                switch (msg->type)
                {
                    case MessageType::TEMP_SHIP: tpl = "temp-wmo"; break;
                    default:
                        tpl = format_message_type(msg->type);
                        tpl += "-wmo";
                        break;
                }
                const wr::TemplateFactory& fac = wr::TemplateRegistry::get(tpl);
                return fac.factory(opts, msgs);
            });

        // Populate it
        register_synop(*registry);
        register_ship(*registry);
        register_buoy(*registry);
        register_metar(*registry);
        register_temp(*registry);
        register_flight(*registry);
        register_generic(*registry);
        register_pollution(*registry);

        // registry->insert("synop", ...)
        // registry->insert("synop-high", ...)
        // registry->insert("wmo-synop", ...)
        // registry->insert("wmo-synop-high", ...)
        // registry->insert("ecmwf-synop", ...)
        // registry->insert("ecmwf-synop-high", ...)
    }
    return *registry;
}

const TemplateFactory& TemplateRegistry::get(const std::string& name)
{
    const TemplateRegistry& tr         = get();
    TemplateRegistry::const_iterator i = tr.find(name);
    if (i == tr.end())
        error_notfound::throwf(
            "requested export template %s which does not exist", name.c_str());
    return i->second;
}

void TemplateRegistry::register_factory(unsigned data_category,
                                        const std::string& name,
                                        const std::string& desc,
                                        TemplateFactory::factory_func fac)
{
    insert(make_pair(name, TemplateFactory(data_category, name, desc, fac)));
}

void Template::to_bulletin(wreport::Bulletin& bulletin)
{
    setupBulletin(bulletin);

    for (unsigned i = 0; i < msgs.size(); ++i)
    {
        Subset& s = bulletin.obtain_subset(i);
        to_subset(Message::downcast(*msgs[i]), s);
    }
}

void Template::setupBulletin(wreport::Bulletin& bulletin)
{
    // Get reference time from first msg in the set
    // If not found, use current time.
    Datetime dt                  = msgs[0]->get_datetime();
    bulletin.rep_year            = dt.year;
    bulletin.rep_month           = dt.month;
    bulletin.rep_day             = dt.day;
    bulletin.rep_hour            = dt.hour;
    bulletin.rep_minute          = dt.minute;
    bulletin.rep_second          = dt.second;
    bulletin.master_table_number = 0;
    bulletin.originating_centre =
        opts.centre != MISSING_INT ? opts.centre : 255;
    bulletin.originating_subcentre =
        opts.subcentre != MISSING_INT ? opts.subcentre : 255;
    bulletin.update_sequence_number = 0;

    if (BufrBulletin* b = dynamic_cast<BufrBulletin*>(&bulletin))
    {
        // Take from opts
        b->edition_number                    = 4;
        b->master_table_version_number       = 17;
        b->master_table_version_number_local = 0;
        b->compression                       = false;
    }
    if (CrexBulletin* b = dynamic_cast<CrexBulletin*>(&bulletin))
    {
        // TODO: change using BUFR tables, when the encoder can encode the full
        // CREX ed.2 header
        b->edition_number                    = 2;
        b->master_table_version_number       = 3;
        b->master_table_version_number_local = 0;
        b->master_table_version_number_bufr  = 0;
        b->has_check_digit                   = false;
    }
}

void Template::to_subset(const Message& msg, wreport::Subset& subset)
{
    this->msg           = &msg;
    this->subset        = &subset;
    this->c_gnd_instant = msg.find_context(Level(1), Trange::instant());
}

void Template::add(Varcode code, const msg::Context* ctx,
                   const Shortcut& shortcut) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->values.maybe_var(shortcut.code))
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void Template::add(Varcode code, const msg::Context* ctx, Varcode srccode) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else if (const Var* var = ctx->values.maybe_var(srccode))
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void Template::add(Varcode code, const msg::Context* ctx) const
{
    if (!ctx)
        subset->store_variable_undef(code);
    else
        add(code, ctx->values);
}

void Template::add(Varcode code, const Values& values) const
{
    if (const Var* var = values.maybe_var(code))
        subset->store_variable(*var);
    else
        subset->store_variable_undef(code);
}

void Template::add(Varcode code, const Values& values,
                   const Shortcut& shortcut) const
{
    if (const Var* var = values.maybe_var(shortcut.code))
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

void Template::add(Varcode code, const Shortcut& shortcut) const
{
    add(code, msg->get(shortcut));
}

void Template::add(Varcode code, Varcode srccode, const Level& level,
                   const Trange& trange) const
{
    add(code, msg->get(level, trange, srccode));
}

void Template::add(wreport::Varcode code, const wreport::Var* var) const
{
    if (var)
        subset->store_variable(code, *var);
    else
        subset->store_variable_undef(code);
}

const Var* Template::find_station_var(wreport::Varcode code) const
{
    return msg->station_data.maybe_var(code);
}

void Template::do_station_name(wreport::Varcode dstcode) const
{
    if (const wreport::Var* var = msg->station_data.maybe_var(sc::st_name.code))
    {
        Varinfo info = subset->tables->btable->query(dstcode);
        Var name(info);
        if (var->isset())
            name.setc_truncate(var->enqc());
        subset->store_variable(move(name));
    }
    else
        subset->store_variable_undef(dstcode);
}

void Template::do_ecmwf_past_wtr() const
{
    int hour = msg->get_datetime().hour == 0xff ? 0 : msg->get_datetime().hour;

    if (hour % 6 == 0)
    {
        add(WR_VAR(0, 20, 4), sc::past_wtr1_6h);
        add(WR_VAR(0, 20, 5), sc::past_wtr2_6h);
    }
    else
    {
        add(WR_VAR(0, 20, 4), sc::past_wtr1_3h);
        add(WR_VAR(0, 20, 5), sc::past_wtr2_3h);
    }
}

void Template::do_station_height() const
{
    add(WR_VAR(0, 7, 30), msg->station_data);
    add(WR_VAR(0, 7, 31), msg->station_data);
}

void Template::do_D01001() const
{
    add(WR_VAR(0, 1, 1), msg->station_data, sc::block);
    add(WR_VAR(0, 1, 2), msg->station_data, sc::station);
}

void Template::do_D01004() const
{
    do_D01001();
    do_station_name(WR_VAR(0, 1, 15));
    add(WR_VAR(0, 2, 1), msg->station_data, sc::st_type);
}

void Template::do_D01011() const
{
    // Year
    if (const Var* var = find_station_var(WR_VAR(0, 4, 1)))
        subset->store_variable(WR_VAR(0, 4, 1), *var);
    else if (!msg->get_datetime().is_missing())
        subset->store_variable_i(WR_VAR(0, 4, 1), msg->get_datetime().year);
    else
        subset->store_variable_undef(WR_VAR(0, 4, 1));

    // Month
    if (const Var* var = find_station_var(WR_VAR(0, 4, 2)))
        subset->store_variable(WR_VAR(0, 4, 2), *var);
    else if (!msg->get_datetime().is_missing())
        subset->store_variable_i(WR_VAR(0, 4, 2), msg->get_datetime().month);
    else
        subset->store_variable_undef(WR_VAR(0, 4, 2));

    // Day
    if (const Var* var = find_station_var(WR_VAR(0, 4, 3)))
        subset->store_variable(WR_VAR(0, 4, 3), *var);
    else if (!msg->get_datetime().is_missing())
        subset->store_variable_i(WR_VAR(0, 4, 3), msg->get_datetime().day);
    else
        subset->store_variable_undef(WR_VAR(0, 4, 3));
}

int Template::do_D01012() const
{
    int res = MISSING_INT;

    // Hour
    if (const Var* var = find_station_var(WR_VAR(0, 4, 4)))
    {
        subset->store_variable(WR_VAR(0, 4, 4), *var);
        res = var->enqi();
    }
    else if (!msg->get_datetime().is_missing())
    {
        subset->store_variable_i(WR_VAR(0, 4, 4), msg->get_datetime().hour);
        res = msg->get_datetime().hour;
    }
    else
        subset->store_variable_undef(WR_VAR(0, 4, 4));

    // Minute
    if (const Var* var = find_station_var(WR_VAR(0, 4, 5)))
        subset->store_variable(WR_VAR(0, 4, 5), *var);
    else if (!msg->get_datetime().is_missing())
        subset->store_variable_i(WR_VAR(0, 4, 5), msg->get_datetime().minute);
    else
        subset->store_variable_undef(WR_VAR(0, 4, 5));

    return res;
}

void Template::do_D01013() const
{
    do_D01012();

    // Second
    if (const Var* var = find_station_var(WR_VAR(0, 4, 6)))
        subset->store_variable(WR_VAR(0, 4, 6), *var);
    else if (!msg->get_datetime().is_missing())
        subset->store_variable_i(WR_VAR(0, 4, 6), msg->get_datetime().second);
    else
        subset->store_variable_i(WR_VAR(0, 4, 6), 0);
}

void Template::do_D01021() const
{
    add(WR_VAR(0, 5, 1), msg->station_data, sc::latitude);
    add(WR_VAR(0, 6, 1), msg->station_data, sc::longitude);
}

void Template::do_D01022() const
{
    do_D01021();
    add(WR_VAR(0, 7, 1), msg->station_data, sc::height_station);
}

void Template::do_D01023() const
{
    add(WR_VAR(0, 5, 2), msg->station_data, sc::latitude);
    add(WR_VAR(0, 6, 2), msg->station_data, sc::longitude);
}

} // namespace wr
} // namespace msg
} // namespace impl
} // namespace dballe
