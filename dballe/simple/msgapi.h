#ifndef FDBA_MSGAPI_H
#define FDBA_MSGAPI_H

#include "commonapi.h"
#include <dballe/core/defs.h>

namespace wreport {
struct Var;
}

namespace dballe {
struct File;
struct Messages;
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
        STATE_BLANK = 1,
        STATE_QUANTESONO = 2,
        STATE_VOGLIOQUESTO = 4,
        STATE_EOF = 8,
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
    /// Message being written
    Messages* msgs;
	/// Message subset being written
	Msg* wmsg;
	/// Last variables written with prendilo
	std::vector<wreport::Var*> vars;
	/// Level for vars
	Level vars_level;
	/// Time range for vars
	Trange vars_trange;
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

	void flushVars();
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
    virtual void remove_all();
    virtual void messages_open_input(const char* filename, const char* mode, File::Encoding format, bool);
    virtual void messages_open_output(const char* filename, const char* mode, File::Encoding format);
    virtual bool messages_read_next();
    virtual void messages_write_next(const char*);
};

}
}
#endif
