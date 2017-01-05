#include "lib/base/base.h"
#include "lib/file/file.h"
#include "lib/hui/hui.h"
#include "lib/nix/nix.h"
#include "lib/net/net.h"
#include "lib/kaba/kaba.h"

string AppName = "Kaba";
string AppVersion = "0.14.2.0";


typedef void main_arg_func(const Array<string>&);
typedef void main_void_func();

namespace Kaba{
extern long long s2i2(const string &str);
};

void execute(Kaba::Script *s, Array<string> &arg)
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
	Kaba::Remove(s);
}

void output_to_file_raw(Kaba::Script *s, const string &out_file)
{
	File *f = FileCreate(out_file);
	if (!f)
		exit(1);
	f->SetBinaryMode(true);
	f->WriteBuffer(s->opcode, s->opcode_size);
	delete(f);
}

void output_to_file_elf(Kaba::Script *s, const string &out_file)
{
	File *f = FileCreate(out_file);
	if (!f)
		exit(1);
	f->SetBinaryMode(true);
	f->WriteChar(0x7f);
	f->WriteChar('E');
	f->WriteChar('L');
	f->WriteChar('F');
	f->WriteChar(0x02); // 64 bit
	f->WriteChar(0x01); // little-endian
	f->WriteChar(0x01); // version
	for (int i=0; i<9; i++)
		f->WriteChar(0x00);
	f->WriteWord(0x0003); // 3=shared... 2=exec
	f->WriteWord(0x003e); // machine
	f->WriteInt(1); // version

	f->WriteInt(0);	f->WriteInt(0);// entry point
	f->WriteInt(0x40);	f->WriteInt(0x00); // program header table offset
	f->WriteInt(0x00);	f->WriteInt(0x00); // section header table
	f->WriteInt(0); // flags
	f->WriteWord(64); // header size
	f->WriteBuffer(s->opcode, s->opcode_size);
	delete(f);
}

void export_symbols(Kaba::Script *s, const string &symbols_out_file)
{
	File *f = FileCreate(symbols_out_file);
	if (!f)
		exit(1);
	foreachi(Kaba::Function *fn, s->syntax->functions, i){
		f->WriteStr(fn->name + ":" + i2s(fn->num_params));
		f->WriteInt((long)s->func[i]);
	}
	foreachi(Kaba::Variable &v, s->syntax->root_of_all_evil.var, i){
		f->WriteStr(v.name);
		f->WriteInt((long)s->g_var[i]);
	}
	f->WriteStr("#");
	delete(f);
}

void import_symbols(const string &symbols_in_file)
{
	File *f = FileOpen(symbols_in_file);
	if (!f)
		exit(1);
	while (!f->Eof){
		string name = f->ReadStr();
		if (name == "#")
			break;
		int pos = f->ReadInt();
		Kaba::LinkExternal(name, (void*)(long)pos);
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
	bool flag_override_variable_offset = false;
	long long variable_offset = 0;
	bool flag_override_code_origin = false;
	long long code_origin = 0;
	bool flag_no_function_frames = false;
	bool flag_add_entry_point = false;
	string output_format = "raw";

	bool error = false;

	// parameters
	Array<string> arg = arg0;
	for (int i=1;i<arg.num;i++){
		if ((arg[i] == "--version") or (arg[i] == "-v")){
			// tell versions
			msg_right();
			msg_write(AppName + " " + AppVersion);
			msg_write("Script-Version: " + Kaba::Version);
			msg_write("Bibliothek-Version: " + Kaba::LibVersion);
			msg_write("Hui-Version: " + HuiVersion);
			msg_left();
			return 0;
		}else if ((arg[i] == "--gui") or (arg[i] == "-g")){
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
			flag_override_code_origin = true;
			arg.erase(i);
			if (i >= arg.num){
				msg_error("offset nach --code-origin erwartet");
				return -1;
			}
			code_origin = Kaba::s2i2(arg[i]);
			arg.erase(i --);
		}else if (arg[i] == "--variable-offset"){
			flag_override_variable_offset = true;
			arg.erase(i);
			if (i >= arg.num){
				msg_error("offset nach --variable-offset erwartet");
				return -1;
			}
			variable_offset = Kaba::s2i2(arg[i]);
			arg.erase(i --);
		}else if (arg[i] == "-o"){
			arg.erase(i);
			if (i >= arg.num){
				msg_error("Dateiname nach -o erwartet");
				return -1;
			}
			out_file = arg[i];
			arg.erase(i --);
		}else if (arg[i] == "--output-format"){
			arg.erase(i);
			if (i >= arg.num){
				msg_error("format 'raw' or 'elf' after --output-format expected");
				return -1;
			}
			output_format = arg[i];
			if ((output_format != "raw") and (output_format != "elf")){
				msg_error("output format has to be 'raw' or 'elf', not: " + output_format);
				return -1;
			}
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
		}else if ((arg[i] == "--help") or (arg[i] == "-h")){
			msg_write("options:");
			msg_write("--version, -v");
			msg_write("--verbose");
			msg_write("--disasm");
			msg_write("--os");
			msg_write("--gui, -g");
			msg_write("--no-std-lib");
			msg_write("--no-function-frames");
			msg_write("--add-entry-point");
			msg_write("--code-origin <OFFSET>");
			msg_write("--variable-offset <OFFSET>");
			msg_write("--export-symbols <FILE>");
			msg_write("--import-symbols <FILE>");
			msg_write("-o <FILE>");
			msg_write("--output-format {raw,elf}");
		}else if (arg[i][0] == '-'){
			msg_error("unknown option: " + arg[i]);
			return -1;
		}else{
			break;
		}
	}

	// init
	msg_db_r("main", 1);
	HuiRegisterFileType("kaba", "MichiSoft Script Datei", "", HuiAppFilename, "execute", false);
	NetInit();
	Kaba::Init(instruction_set, abi, flag_allow_std_lib);
	//Kaba::LinkDynamicExternalData();
	Kaba::config.stack_size = 10485760; // 10 mb (mib)


	Kaba::LinkExternal("Test1", (void*)&Test1);
	Kaba::LinkExternal("Test2", (void*)&Test2);

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
	Kaba::config.compile_silently = !flag_verbose;
	Kaba::config.verbose = flag_verbose;
	Kaba::config.compile_os = flag_compile_os;
	Kaba::config.add_entry_point = flag_add_entry_point;
	Kaba::config.no_function_frame = flag_no_function_frames;
	Kaba::config.override_variables_offset = flag_override_variable_offset;
	Kaba::config.variables_offset = variable_offset;
	Kaba::config.override_code_origin = flag_override_code_origin;
	Kaba::config.code_origin = code_origin;
	SilentFiles = true;

	try{
		Kaba::Script *s = Kaba::Load(filename);
		if (symbols_out_file.num > 0)
			export_symbols(s, symbols_out_file);
		if (out_file.num > 0){
			if (output_format == "raw")
				output_to_file_raw(s, out_file);
			else if (output_format == "elf")
				output_to_file_elf(s, out_file);

			if (flag_disassemble)
				msg_write(Asm::Disassemble(s->opcode, s->opcode_size, true));
		}else{
			if (flag_disassemble)
				msg_write(Asm::Disassemble(s->opcode, s->opcode_size, true));

			if (Kaba::config.instruction_set == Asm::QueryLocalInstructionSet())
				execute(s, arg);
		}
	}catch(Kaba::Exception &e){
		if (use_gui)
			HuiErrorBox(NULL, _("Fehler in Script"), e.message);
		else
			msg_error(e.message);
		error = true;
	}

	// end
	msg_db_l(1);
	Kaba::End();
	msg_end();
	return error ? -1 : 0;
}

