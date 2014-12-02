/*
 * dballe/wr_codec - BUFR/CREX import and export
 *
 * Copyright (C) 2005--2010  ARPA-SIM <urpsim@smr.arpa.emr.it>
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

#ifndef DBALLE_MSG_WR_CODEC_H
#define DBALLE_MSG_WR_CODEC_H

#include <dballe/msg/codec.h>
#include <dballe/msg/msg.h>
#include <wreport/varinfo.h>
#include <stdint.h>
#include <map>
#include <string>

namespace wreport {
struct Bulletin;
struct Subset;
}

namespace dballe {
struct Msg;

namespace msg {

class WRImporter : public Importer
{
public:
    WRImporter(const Options& opts);

    /**
     * Import a decoded BUFR/CREX message
     */
    virtual void from_bulletin(const wreport::Bulletin& msg, Msgs& msgs) const;
};

class BufrImporter : public WRImporter
{
public:
    BufrImporter(const Options& opts=Options());
    virtual ~BufrImporter();

    virtual void from_rawmsg(const Rawmsg& msg, Msgs& msgs) const;
};

class CrexImporter : public WRImporter
{
public:
    CrexImporter(const Options& opts=Options());
    virtual ~CrexImporter();

    virtual void from_rawmsg(const Rawmsg& msg, Msgs& msgs) const;
};


class WRExporter : public Exporter
{
public:
    WRExporter(const Options& opts);

    /**
     * Import a decoded BUFR/CREX message
     */
    virtual void to_bulletin(const Msgs& msgs, wreport::Bulletin& msg) const;

    /**
     * Infer a template name from the message contents
     */
    std::string infer_template(const Msgs& msgs) const;
};

class BufrExporter : public WRExporter
{
public:
    BufrExporter(const Options& opts=Options());
    virtual ~BufrExporter();

    virtual void to_rawmsg(const Msgs& msgs, Rawmsg& msg) const;
    virtual std::unique_ptr<wreport::Bulletin> make_bulletin() const;
};

class CrexExporter : public WRExporter
{
public:
    CrexExporter(const Options& opts=Options());
    virtual ~CrexExporter();

    virtual void to_rawmsg(const Msgs& msgs, Rawmsg& msg) const;
    virtual std::unique_ptr<wreport::Bulletin> make_bulletin() const;
};

namespace wr {

struct TemplateRegistry;

class Template
{
protected:
    virtual void setupBulletin(wreport::Bulletin& bulletin);
    virtual void to_subset(const Msg& msg, wreport::Subset& subset);

    void add(wreport::Varcode code, const msg::Context* ctx, int shortcut) const;
    void add(wreport::Varcode code, const msg::Context* ctx, wreport::Varcode srccode) const;
    void add(wreport::Varcode code, const msg::Context* ctx) const;
    void add(wreport::Varcode code, int shortcut) const;
    void add(wreport::Varcode code, wreport::Varcode srccode, const Level& level, const Trange& trange) const;
    void add(wreport::Varcode code, const wreport::Var* var) const;
    // Set station name, truncating it if it's too long
    void add_st_name(wreport::Varcode dstcode, const msg::Context* ctx) const;

    void do_ecmwf_past_wtr() const;
    void do_D01001() const;
    void do_D01004() const;
    void do_D01011() const;
    int do_D01012() const;  // Return the number of hours
    void do_D01013() const;
    void do_D01021() const;
    void do_D01022() const;

public:
    const Exporter::Options& opts;
    const Msgs& msgs;
    const Msg* msg;     // Msg being read
    const msg::Context* c_station;
    const msg::Context* c_gnd_instant;
    wreport::Subset* subset; // Subset being written

    Template(const Exporter::Options& opts, const Msgs& msgs)
        : opts(opts), msgs(msgs), msg(0), subset(0) {}
    virtual ~Template() {}

    virtual const char* name() const = 0;
    virtual const char* description() const = 0;
    virtual void to_bulletin(wreport::Bulletin& bulletin);
};

struct TemplateFactory
{
    std::string name;
    std::string description;

    virtual ~TemplateFactory() {}
    virtual std::unique_ptr<Template> make(const Exporter::Options& opts, const Msgs& msgs) const = 0;
};

struct TemplateRegistry : public std::map<std::string, const TemplateFactory*>
{
    static const TemplateRegistry& get();
    static const TemplateFactory& get(const std::string& name);

    void register_factory(const TemplateFactory* fac);
};

} // namespace wr


#if 0

/** @file
 * @ingroup bufrex
 * High level encoding and decoding functions
 */

#include <dballe/core/rawmsg.h>
#include <dballe/msg/msgs.h>
#include <dballe/msg/codec.h>
#include <dballe/bufrex/msg.h>

/**
 * Encode a dba_msg into a BUFR message
 *
 * @param msgs
 *   The message to encode
 * @param type
 *   The type of the encoded message.  Use 0 to let the encoder make a good choice.
 * @param subtype
 *   The subtype of the encoded message.  Use 0 to let the encoder make a good choice.
 * @retval raw
 *   The raw encoded message
 * @return
 *   The error status (See @ref error.h)
 */
dba_err bufrex_encode_bufr(dba_msgs msgs, int type, int subtype, int localsubtype, dba_rawmsg* raw);

/**
 * Encode a dba_msg into a CREX message
 *
 * @param msgs
 *   The message to encode
 * @param type
 *   The type of the encoded message.  Use 0 to let the encoder make a good choice.
 * @param subtype
 *   The subtype of the encoded message.  Use 0 to let the encoder make a good choice.
 * @retval raw
 *   The raw encoded message
 * @return
 *   The error status (See @ref error.h)
 */
dba_err bufrex_encode_crex(dba_msgs msgs, int type, int subtype, dba_rawmsg* raw);


/**
 * Fill in the bufrex_msg with the contents of a dba_msg
 *
 * @param raw
 *   The bufrex message that will be filled with the data from msg
 * @param msg
 *   The dba_msg to take the data from
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err bufrex_msg_from_dba_msg(bufrex_msg raw, dba_msg msg);

/**
 * Fill in the bufrex_msg with the contents of a dba_msgs
 *
 * @param raw
 *   The bufrex message that will be filled with the data from msg
 * @param msgs
 *   The dba_msgs to take the data from
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err bufrex_msg_from_dba_msgs(bufrex_msg raw, dba_msgs msgs);

/**
 * Fill in a dba_msgs with the contents of the bufrex_msg
 *
 * @param raw
 *   The bufrex message with the data to interpret
 * @param opts
 *   Codec options
 * @retval msgs
 *   The dba_msgs with the interpreted data
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err bufrex_msg_to_dba_msgs(bufrex_msg raw, dba_msg_codec_options opts, dba_msgs* msgs);


/**
 * Infer good type and subtype from a dba_msg
 *
 * @param msg
 *   The message for which to infer the encoding template
 * @retval type
 *   The type of the encoding template to use
 * @retval subtype
 *   The subtype of the encoding template to use
 * @return
 *   The error indicator for the function (See @ref error.h)
 */
dba_err bufrex_infer_type_subtype(dba_msg msg, int* type, int* subtype, int* localsubtype);
	

#endif

} // namespace msg
} // namespace dballe

/* vim:set ts=4 sw=4: */
#endif
