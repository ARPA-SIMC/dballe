/*
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

#ifndef CONVERSION_H
#define CONVERSION_H

#if 0
#include <dballe/core/file.h>
#include <dballe/core/rawmsg.h>
#include <dballe/msg/msgs.h>
#include <dballe/bufrex/msg.h>
#endif
#include <extra/processor.h>

namespace wreport {
struct Bulletin;
}

namespace dballe {
namespace msg {
struct Importer;
struct Exporter;
}

namespace proc {

struct Converter : public Action
{
	File* file;
	const char* dest_rep_memo;
	const char* dest_template;

	msg::Importer* importer;
	msg::Exporter* exporter;
#if 0
	dba_encoding outType;
	void* outAction;
	void* outActionData;
#endif
	Converter() : file(0), dest_rep_memo(0), dest_template(0), importer(0), exporter(0) {}
	~Converter();

	virtual void operator()(const Rawmsg& rmsg, const wreport::Bulletin* braw, const Msgs* msgs);

	void process_bufrex_msg(const wreport::Bulletin& msg);
	void process_dba_msg(const Msgs& msgs);
};

}
}

#endif
