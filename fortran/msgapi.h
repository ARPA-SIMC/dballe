#ifndef FDBA_MSGAPI_H
#define FDBA_MSGAPI_H

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

#include "commonapi.h"

namespace wreport {
struct Var;
}

namespace dballe {
struct File;
struct Msgs;
struct Msg;

namespace msg {
struct Importer;
struct Exporter;
}

namespace fortran {

class MsgAPI : public CommonAPIImplementation
{
protected:
	enum {
		STATE_QUANTESONO = 1,
		STATE_VOGLIOQUESTO = 2,
		STATE_EOF = 4,
	};
	File* file;
	/**
	 * State flag to track what actions have been performed in order to decide
	 * what to do next
	 */
	unsigned int state;
	/// Importer (NULL if we export)
	msg::Importer* importer;
	/// Exporter (NULL if we import)
	msg::Exporter* exporter;
	/// Template selected for exporter (empty if auto detect)
	std::string exporter_template;
	Msgs* msgs;
	/// Message being written
	Msg* wmsg;
	/// Pointer to the last variable written, to set attributes
	wreport::Var* wvar;
	size_t curmsgidx;
	int iter_ctx;
	int iter_var;
	/// Category set for the message that we are writing
	int cached_cat;
	/// Subcategory set for the message that we are writing
	int cached_subcat;
	/// Local category set for the message that we are writing
	int cached_lcat;


	/**
	 * Read the next message
	 * @returns
	 *   true if there was a next message, false if we reached end of file
	 */
	bool readNextMessage();

	/**
	 * Increment message iterators
	 * @returns
	 *   true if it could move on, false if we are at the end
	 */
	bool incrementMsgIters();

	/**
	 * Get a pointer to the current message being read or written
	 */
	Msg* curmsg();

	void flushSubset();
	void flushMessage();

public:
	/**
	 * @param fname 
	 *   the name of the file to open
	 * @param mode
	 *   the fopen-style mode to use when opening the file
	 * @param type
	 *   the encoding to use for the file.  It can be "BUFR", "CREX", "AOF"
	 *   (read only) or "AUTO" (read only).
	 */
	MsgAPI(const char* fname, const char* mode, const char* type);
	virtual ~MsgAPI();

	virtual void scopa(const char* repinfofile = 0);

	virtual int quantesono();
	virtual void elencamele();

	virtual int voglioquesto();
	virtual const char* dammelo();

	virtual void prendilo();
	virtual void dimenticami();

	virtual int voglioancora();

	virtual void critica();
	virtual void scusa();
};

}
}

/* vim:set ts=4 sw=4: */
#endif
