#ifndef FDBA_MSGAPI_H
#define FDBA_MSGAPI_H

#include "commonapi.h"
#include <dballe/core/defs.h>

namespace wreport {
struct Var;
}

namespace dballe {
struct File;
struct Message;
struct Msg;

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
    Importer* importer;
    /// Exporter (NULL if we import)
    Exporter* exporter;
    /// Template selected for exporter (empty if auto detect)
    std::string exporter_template;
    /// Message being written
    std::vector<std::shared_ptr<dballe::Message>>* msgs;
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
	 *   the encoding to use for the file.  It can be "BUFR" or "CREX"
	 *   (read only) or "AUTO" (read only).
	 */
	MsgAPI(const char* fname, const char* mode, const char* type);
	virtual ~MsgAPI();

    void scopa(const char* repinfofile=0) override;
    int quantesono() override;
    void elencamele() override;
    int voglioquesto() override;
    wreport::Varcode dammelo() override;
    void prendilo() override;
    void dimenticami() override;
    int voglioancora() override;
    void critica() override;
    void scusa() override;
    void remove_all() override;
    void messages_open_input(const char* filename, const char* mode, Encoding format, bool) override;
    void messages_open_output(const char* filename, const char* mode, Encoding format) override;
    bool messages_read_next() override;
    void messages_write_next(const char*) override;
};

}
}
#endif
