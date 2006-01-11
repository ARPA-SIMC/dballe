// dballe.i - SWIG interface
%module Dballe
//%include "typemaps.i"

%include std_string.i


%exception {
	try { $action }
	catch (DBAException& e)
	{
		std::string desc = e.fulldesc();

		fprintf(stderr, "Error %.*s", desc.size(), desc.data());
		SWIG_fail;
	}
	catch (...) { SWIG_fail; }
}


%{
#include "DBAlle.h"
%}

%include "DBAlle.h"
%include "DBARecord.h"
%include "DBAError.h"


/*
%ignore SmartPointer;
%ignore SmartPointerItem;
*/

//#include <dballe/dballe.h>

/*
// Parse the original header file
%include "SmartPointer.h"
%include "Consumer.h"
%include "Config.h"
%include "MailProgram.h"

// Instantiate the base class needed for MailFolder, but dont wrap it
%template() SmartPointer<MailFolderImpl>;
%template(MailFolderVector) std::vector<MailFolder>;
%template(MailProgramVector) std::vector<MailProgram>;
%template(StringVector) std::vector<std::string>;

%ignore MailFolderImpl;

class MailFolder : public SmartPointer<MailFolderImpl>
{
public:
	const std::string& name() const throw ();
	const std::string& path() const throw ();

	int getMsgTotal() const throw ();
	int getMsgUnread() const throw ();
	int getMsgNew() const throw ();
	int getMsgFlagged() const throw ();

	/// Return true if the folder has been changed since the last updateStatistics
	bool changed();

	/// Rescan the folder to update its statistics
	void updateStatistics();

	static std::vector<MailFolder> enumerateFolders(const std::string& path);
};



// TODO:
// Rename the static method to be called as MailFolder.enumerateFolders
// (or even as Buffy.renameFolders?)
*/
