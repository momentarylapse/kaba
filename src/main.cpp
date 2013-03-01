#include "lib/base/base.h"
#include "lib/file/file.h"
#include "lib/hui/hui.h"
#include "lib/nix/nix.h"
#include "lib/net/net.h"
#include "lib/x/x.h"
#include "lib/script/script.h"

string AppName = "Kaba";
string AppVersion = "0.2.7.0";

#if 0

#if 0
void fff(int a, int b)
{
	int sdf = a +b;
}

/*void f(int a)
{
	int bb = 3;
	ff(a, 4);
}*/

vector ff(int u)
{
	return e_x;
}

void f()
{
	vector a = ff(5);
}
#endif

class C
{
	public:
	int i;
	void f(int a)
	{
		i+=a;
		//return e_x;
	}
};

void ff()
{
	C c;
	c.f(3);
	c.f(3);
}

int hui_main()
{
	// hui
//	HuiInitExtended("kaba", AppName, NULL, (void*)&NixNetSendBugReport, false, "");
	HuiEndKeepMsgAlive = true;
	msg_init();

	AsmInit();

	msg_write(Opcode2Asm((void*)&C::f, -1));
	msg_write(Opcode2Asm((void*)&ff, -1));
	//msg_write(Opcode2Asm((void*)&fff, -1));

	return 0;
}
#else

typedef void main_arg_func(const Array<string>&);
typedef void main_void_func();

int hui_main(Array<string> arg)
{
	// hui
	HuiInitExtended("kaba", AppVersion, NULL, false, "");
	HuiEndKeepMsgAlive = true;


	// tell versions
	if (arg.num > 1)
		if (arg[1] == "-v"){
			msg_right();
			msg_write(AppName + " " + AppVersion);
			msg_write("Script-Version: " + Script::Version);
			msg_write("Bibliothek-Version: " + Script::DataVersion);
			msg_write("Hui-Version: " + HuiVersion);
			msg_left();
			return 0;
		}
	//msg_write("");

	// init
	msg_db_r("main", 1);
	HuiRegisterFileType("kaba", "MichiSoft Script Datei", "", HuiAppFilename, "execute", false);
	NetInit();
	MetaInit();
	Script::Init();
	//Script::LinkDynamicExternalData();
	Script::StackSize = 10485760; // 10 mb (mib)

	// script file as parameter?
	string filename;
	if (arg.num > 1)
		filename = arg[1];
	else{
		if (!HuiFileDialogOpen(NULL, _("Script &offnen"), "", "*.kaba", "*.kaba")){
			msg_end();
			return 0;
		}
		filename = HuiFilename;
		//HuiInfoBox(NULL, "Fehler", "Als Parameter muss eine Script-Datei mitgegeben werden!");
	}

	// compile
	Script::CompileSilently = true;
	SilentFiles = true;
	Script::Script *s = Script::Load(filename, true);

	// set working directory -> script file
	//msg_write(HuiInitialWorkingDirectory);
	HuiSetDirectory(HuiInitialWorkingDirectory);
	HuiSetDirectory(filename.dirname());

	if (s->Error || s->LinkerError){
		HuiErrorBox(NULL, _("Fehler in Script"), s->ErrorMsgExt[0] + "\n" + s->ErrorMsgExt[1]);
	}else{

		main_arg_func *f_arg = (main_arg_func*)s->MatchFunction("main", "void", 1, "string[]");
		main_void_func *f_void = (main_void_func*)s->MatchFunction("main", "void", 0);

		if (f_arg){
			// special execution...
			msg_db_r("Execute main(arg)", 1);
			f_arg(arg.sub(2, -1));
			msg_db_l(1);
		}else if (f_void){
			// default execution
			msg_db_r("Execute main()", 1);
			f_void();
			msg_db_l(1);
		}
	}
	Script::Remove(s);

	// end
	msg_db_l(1);
	Script::End();
	MetaEnd();
	msg_end();
	return 0;
}
#endif

