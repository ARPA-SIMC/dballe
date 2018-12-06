#ifndef FDBA_MSGAPI_H
#define FDBA_MSGAPI_H

#include "commonapi.h"
#include <dballe/fwd.h>
#include <dballe/msg/fwd.h>
#include <dballe/core/defs.h>

namespace wreport {
struct Var;
}

namespace dballe {
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
    /// Name of the last exporter template set
    std::string exporter_template;
    /// Exporter (NULL if we import)
    Exporter* exporter = nullptr;
	size_t curmsgidx;
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


public:
    /// Message subset being written
    impl::Message* wmsg = nullptr;
    /// Message being written
    std::vector<std::shared_ptr<dballe::Message>>* msgs = nullptr;

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

    /**
     * Get a pointer to the current message being read or written
     */
    const impl::Message* curmsg() const;
    impl::Message* curmsg();
    void flushSubset();
    void flushMessage();
    void set_exporter(const char* template_name);

    void reinit_db(const char* repinfofile=0) override;
    int query_stations() override;
    int voglioquesto() override;
    void prendilo() override;
    void dimenticami() override;
    void remove_all() override;
    void messages_open_input(const char* filename, const char* mode, Encoding format, bool) override;
    void messages_open_output(const char* filename, const char* mode, Encoding format) override;
    bool messages_read_next() override;
    void messages_write_next(const char*) override;
};

}
}
#endif
