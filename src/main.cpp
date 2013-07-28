#include "lib/base/base.h"
#include "lib/file/file.h"
#include "lib/hui/hui.h"
#include "lib/nix/nix.h"
#include "lib/net/net.h"
#include "lib/script/script.h"

string AppName = "Kaba";
string AppVersion = "0.2.8.0";


typedef void main_arg_func(const Array<string>&);
typedef void main_void_func();

void execute(Script::Script *s, Array<string> &arg)
{
	// set working directory -> script file
	//msg_write(HuiInitialWorkingDirectory);
	HuiSetDirectory(HuiInitialWorkingDirectory);
	HuiSetDirectory(s->Filename.dirname());

	main_arg_func *f_arg = (main_arg_func*)s->MatchFunction("main", "void", 1, "string[]");
	main_void_func *f_void = (main_void_func*)s->MatchFunction("main", "void", 0);

	if (f_arg){
		// special execution...
		msg_db_f("Execute main(arg)", 1);
		f_arg(arg.sub(2, -1));
	}else if (f_void){
		// default execution
		msg_db_f("Execute main()", 1);
		f_void();
	}
	Script::Remove(s);
}

int hui_main(Array<string> arg)
{
	// hui
	HuiInit("kaba", false, "");
	HuiSetProperty("name", AppName);
	HuiSetProperty("version", AppVersion);
	HuiEndKeepMsgAlive = true;

	bool use_gui = false;

	if (arg.num > 1){
		// tell versions
		if (arg[1] == "-v"){
			msg_right();
			msg_write(AppName + " " + AppVersion);
			msg_write("Script-Version: " + Script::Version);
			msg_write("Bibliothek-Version: " + Script::DataVersion);
			msg_write("Hui-Version: " + HuiVersion);
			msg_left();
			return 0;
		}else if ((arg[1] == "--gui") || (arg[1] == "-g")){
			use_gui = true;
			arg.erase(1);
		}
	}

	// init
	msg_db_r("main", 1);
	HuiRegisterFileType("kaba", "MichiSoft Script Datei", "", HuiAppFilename, "execute", false);
	NetInit();
	Script::Init();
	//Script::LinkDynamicExternalData();
	Script::config.StackSize = 10485760; // 10 mb (mib)

	// script file as parameter?
	string filename;
	if (arg.num > 1){
		filename = arg[1];
	}else{
		if (!HuiFileDialogOpen(NULL, _("Script &offnen"), "", "*.kaba", "*.kaba")){
			msg_end();
			return 0;
		}
		filename = HuiFilename;
		//HuiInfoBox(NULL, "Fehler", "Als Parameter muss eine Script-Datei mitgegeben werden!");
	}

	// compile
	Script::config.CompileSilently = true;
	SilentFiles = true;

	try{
		Script::Script *s = Script::Load(filename);
		execute(s, arg);
	}catch(Script::Exception &e){
		if (use_gui)
			HuiErrorBox(NULL, _("Fehler in Script"), e.message);
		else
			msg_error(e.message);
	}

	// end
	msg_db_l(1);
	Script::End();
	msg_end();
	return 0;
}

