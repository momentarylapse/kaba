#include "lib/base/base.h"
#include "lib/file/file.h"
#include "lib/hui/hui.h"
#include "lib/nix/nix.h"
#include "lib/net/net.h"
#include "lib/script/script.h"

string AppName = "Kaba";
string AppVersion = "0.14.2.0";


typedef void main_arg_func(const Array<string>&);
typedef void main_void_func();

namespace Script{
extern long long s2i2(const string &str);
};

void execute(Script::Script *s, Array<string> &arg)
{
	// set working directory -> script file
	//msg_write(HuiInitialWorkingDirectory);
	HuiSetDirectory(HuiInitialWorkingDirectory);
	HuiSetDirectory(s->filename.dirname());

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

void dump_to_file(Script::Script *s, const string &out_file)
{
	CFile *f = FileCreate(out_file);
	if (!f)
		exit(1);
	f->SetBinaryMode(true);
	f->WriteBuffer(s->opcode, s->opcode_size);
	delete(f);
}

void export_symbols(Script::Script *s, const string &symbols_out_file)
{
	CFile *f = FileCreate(symbols_out_file);
	if (!f)
		exit(1);
	foreachi(Script::Function *fn, s->syntax->functions, i){
		f->WriteStr(fn->name);
		f->WriteInt((long)s->func[i]);
	}
	foreachi(Script::Variable &v, s->syntax->root_of_all_evil.var, i){
		f->WriteStr(v.name);
		f->WriteInt((long)s->g_var[i]);
	}
	f->WriteStr("#");
	delete(f);
}

void import_symbols(const string &symbols_in_file)
{
	CFile *f = FileOpen(symbols_in_file);
	if (!f)
		exit(1);
	while (!f->Eof){
		string name = f->ReadStr();
		if (name == "#")
			break;
		int pos = f->ReadInt();
		Script::LinkExternal(name, (void*)(long)pos);
	}
	delete(f);
}


void Test1(int a)
{
	msg_write("out: " + i2s(a));
}

int Test2()
{
	return 2001;
}

int hui_main(const Array<string> &arg0)
{
	// hui
	HuiInit("kaba", false, "");
	HuiSetProperty("name", AppName);
	HuiSetProperty("version", AppVersion);
	HuiEndKeepMsgAlive = true;

	bool use_gui = false;
	int instruction_set = -1;
	int abi = -1;
	string out_file, symbols_out_file, symbols_in_file;
	bool flag_allow_std_lib = true;
	bool flag_disassemble = false;
	bool flag_verbose = false;
	bool flag_compile_os = false;
	bool flag_overwrite_variable_offset = false;
	long long variable_offset = 0;
	bool flag_overwrite_code_origin = false;
	long long code_origin = 0;
	bool flag_no_function_frames = false;
	bool flag_add_entry_point = false;

	bool error = false;

	// parameters
	Array<string> arg = arg0;
	for (int i=1;i<arg.num;i++){
		if ((arg[i] == "--version") or (arg[i] == "-v")){
			// tell versions
			msg_right();
			msg_write(AppName + " " + AppVersion);
			msg_write("Script-Version: " + Script::Version);
			msg_write("Bibliothek-Version: " + Script::DataVersion);
			msg_write("Hui-Version: " + HuiVersion);
			msg_left();
			return 0;
		}else if ((arg[i] == "--gui") || (arg[i] == "-g")){
			use_gui = true;
			arg.erase(i --);
		}else if (arg[i] == "--arm"){
			instruction_set = Asm::INSTRUCTION_SET_ARM;
			arg.erase(i --);
		}else if (arg[i] == "--amd64"){
			instruction_set = Asm::INSTRUCTION_SET_AMD64;
			arg.erase(i --);
		}else if (arg[i] == "--x86"){
			instruction_set = Asm::INSTRUCTION_SET_X86;
			arg.erase(i --);
		}else if (arg[i] == "--no-std-lib"){
			flag_allow_std_lib = false;
			arg.erase(i --);
		}else if (arg[i] == "--os"){
			flag_compile_os = true;
			arg.erase(i --);
		}else if (arg[i] == "--verbose"){
			flag_verbose = true;
			flag_disassemble = true;
			arg.erase(i --);
		}else if (arg[i] == "--disasm"){
			flag_disassemble = true;
			arg.erase(i --);
		}else if (arg[i] == "--no-function-frames"){
			flag_no_function_frames = true;
			arg.erase(i --);
		}else if (arg[i] == "--add-entry-point"){
			flag_add_entry_point = true;
			arg.erase(i --);
		}else if (arg[i] == "--code-origin"){
			flag_overwrite_code_origin = true;
			arg.erase(i);
			if (i >= arg.num){
				msg_error("offset nach --code-origin erwartet");
				return -1;
			}
			code_origin = Script::s2i2(arg[i]);
			arg.erase(i --);
		}else if (arg[i] == "--variable-offset"){
			flag_overwrite_variable_offset = true;
			arg.erase(i);
			if (i >= arg.num){
				msg_error("offset nach --variable-offset erwartet");
				return -1;
			}
			variable_offset = Script::s2i2(arg[i]);
			arg.erase(i --);
		}else if (arg[i] == "-o"){
			arg.erase(i);
			if (i >= arg.num){
				msg_error("Dateiname nach -o erwartet");
				return -1;
			}
			out_file = arg[i];
			arg.erase(i --);
		}else if (arg[i] == "--export-symbols"){
			arg.erase(i);
			if (i >= arg.num){
				msg_error("Dateiname nach --export-symbols erwartet");
				return -1;
			}
			symbols_out_file = arg[i];
			arg.erase(i --);
		}else if (arg[i] == "--import-symbols"){
			arg.erase(i);
			if (i >= arg.num){
				msg_error("Dateiname nach --import-symbols erwartet");
				return -1;
			}
			symbols_in_file = arg[i];
			arg.erase(i --);
		}else if (arg[i][0] == '-'){
			msg_error("unbekannte Option: " + arg[i]);
			return -1;
		}
	}

	// init
	msg_db_r("main", 1);
	HuiRegisterFileType("kaba", "MichiSoft Script Datei", "", HuiAppFilename, "execute", false);
	NetInit();
	Script::Init(instruction_set, abi, flag_allow_std_lib);
	//Script::LinkDynamicExternalData();
	Script::config.stack_size = 10485760; // 10 mb (mib)


	Script::LinkExternal("Test1", (void*)&Test1);
	Script::LinkExternal("Test2", (void*)&Test2);

	if (symbols_in_file.num > 0)
		import_symbols(symbols_in_file);

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
	Script::config.compile_silently = !flag_verbose;
	Script::config.verbose = flag_verbose;
	Script::config.compile_os = flag_compile_os;
	Script::config.add_entry_point = flag_add_entry_point;
	Script::config.no_function_frame = flag_no_function_frames;
	Script::config.overwrite_variables_offset = flag_overwrite_variable_offset;
	Script::config.variables_offset = variable_offset;
	Script::config.overwrite_code_origin = flag_overwrite_code_origin;
	Script::config.code_origin = code_origin;
	SilentFiles = true;

	try{
		Script::Script *s = Script::Load(filename);
		if (symbols_out_file.num > 0)
			export_symbols(s, symbols_out_file);
		if (out_file.num > 0){
			dump_to_file(s, out_file);

			if (flag_disassemble)
				msg_write(Asm::Disassemble(s->opcode, s->opcode_size, true));
		}else{
			if (flag_disassemble)
				msg_write(Asm::Disassemble(s->opcode, s->opcode_size, true));

			if (Script::config.instruction_set == Asm::QueryLocalInstructionSet())
				execute(s, arg);
		}
	}catch(Script::Exception &e){
		if (use_gui)
			HuiErrorBox(NULL, _("Fehler in Script"), e.message);
		else
			msg_error(e.message);
		error = true;
	}

	// end
	msg_db_l(1);
	Script::End();
	msg_end();
	return error ? -1 : 0;
}

