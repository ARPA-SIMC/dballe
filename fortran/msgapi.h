#ifndef FDBA_MSGAPI_H
#define FDBA_MSGAPI_H

/*
 * DB-ALLe - Archive for punctual meteorological data
 *
 * Copyright (C) 2005--2008  ARPA-SIM <urpsim@smr.arpa.emr.it>
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
#include <dballe/core/file.h>
#include <dballe/msg/msgs.h>

namespace dballef
{

class MsgAPI : public CommonAPIImplementation
{
protected:
	enum {
		STATE_QUANTESONO = 1,
		STATE_VOGLIOQUESTO = 2,
		STATE_EOF = 4,
	};
	dba_file file;
	/**
	 * State flag to track what actions have been performed in order to decide
	 * what to do next
	 */
	unsigned int state;
	dba_msgs msgs;
	/// Message being written
	dba_msg wmsg;
	/// Pointer to the last variable written, to set attributes
	dba_var wvar;
	int curmsgidx;
	int iter_l;
	int iter_d;
	int forced_report;


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
	dba_msg curmsg();

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
	 * @param force_report
	 *   if 0, nothing happens; otherwise, choose the output message template
	 *   using this report type instead of the one in the message
	 */
	MsgAPI(const char* fname, const char* mode, const char* type, int force_report = 0);
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

/* vim:set ts=4 sw=4: */
#endif
