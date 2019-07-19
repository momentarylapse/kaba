#include "lib/base/base.h"
#include "lib/file/file.h"
#include "lib/hui/hui.h"
#include "lib/nix/nix.h"
#include "lib/net/net.h"
#include "lib/kaba/kaba.h"

string AppName = "Kaba";
string AppVersion = "0.17.2.1";


typedef void main_arg_func(const Array<string>&);
typedef void main_void_func();



namespace Kaba{
extern long long s2i2(const string &str);
};


void Test1(int a)
{
	msg_write("out: " + i2s(a));
}

int Test2()
{
	return 2001;
}

void print_class(Kaba::Class *c)
{
	msg_write("==  " + c->name + "  ==");
	for (auto &f: c->functions)
		msg_write(f.signature(true));
}


class KabaApp : public hui::Application
{
public:
	KabaApp() :
		hui::Application("kaba", "", 0)//hui::FLAG_LOAD_RESOURCE)
	{
		set_property("name", AppName);
		set_property("version", AppVersion);
		//hui::EndKeepMsgAlive = true;

	}

	virtual bool on_startup(const Array<string> &arg0)
	{

		bool use_gui = false;
		int instruction_set = -1;
		int abi = -1;
		string out_file, symbols_out_file, symbols_in_file;
		bool flag_allow_std_lib = true;
		bool flag_disassemble = false;
		bool flag_verbose = false;
		string debug_func_filter = "*";
		string debug_stage_filter = "*";
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
				msg_write("Hui-Version: " + hui::Version);
				msg_left();
				return false;
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
			}else if (arg[i] == "--vfunc"){
				arg.erase(i);
				debug_func_filter = arg[i];
				arg.erase(i --);
			}else if (arg[i] == "--vstage"){
				arg.erase(i);
				debug_stage_filter = arg[i];
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
					exit(1);
				}
				code_origin = Kaba::s2i2(arg[i]);
				arg.erase(i --);
			}else if (arg[i] == "--variable-offset"){
				flag_override_variable_offset = true;
				arg.erase(i);
				if (i >= arg.num){
					msg_error("offset nach --variable-offset erwartet");
					exit(1);
				}
				variable_offset = Kaba::s2i2(arg[i]);
				arg.erase(i --);
			}else if (arg[i] == "-o"){
				arg.erase(i);
				if (i >= arg.num){
					msg_error("Dateiname nach -o erwartet");
					exit(1);
				}
				out_file = arg[i];
				arg.erase(i --);
			}else if (arg[i] == "--output-format"){
				arg.erase(i);
				if (i >= arg.num){
					msg_error("format 'raw' or 'elf' after --output-format expected");
					exit(1);
				}
				output_format = arg[i];
				if ((output_format != "raw") and (output_format != "elf")){
					msg_error("output format has to be 'raw' or 'elf', not: " + output_format);
					exit(1);
				}
				arg.erase(i --);
			}else if (arg[i] == "--export-symbols"){
				arg.erase(i);
				if (i >= arg.num){
					msg_error("Dateiname nach --export-symbols erwartet");
					exit(1);
				}
				symbols_out_file = arg[i];
				arg.erase(i --);
			}else if (arg[i] == "--import-symbols"){
				arg.erase(i);
				if (i >= arg.num){
					msg_error("Dateiname nach --import-symbols erwartet");
					exit(1);
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
				exit(1);
			}else{
				break;
			}
		}

		// init
		//hui::RegisterFileType("kaba", "MichiSoft Script Datei", "", hui::AppFilename, "execute", false);
		NetInit();
		Kaba::Init(instruction_set, abi, flag_allow_std_lib);
		//Kaba::LinkDynamicExternalData();
		Kaba::config.stack_size = 10485760; // 10 mb (mib)


		// for huibui.kaba...
		Kaba::LinkExternalClassFunc("Resource.str", Kaba::mf(&hui::Resource::to_string));
		Kaba::LinkExternalClassFunc("Resource.show", Kaba::mf(&hui::Resource::show));
		Kaba::LinkExternal("ParseResource", (void*)&hui::ParseResource);


		// for experiments
		Kaba::LinkExternal("Test1", (void*)&Test1);
		Kaba::LinkExternal("Test2", (void*)&Test2);
		Kaba::LinkExternal("print_class", (void*)&print_class);

		if (symbols_in_file.num > 0)
			import_symbols(symbols_in_file);

		// script file as parameter?
		string filename;
		if (arg.num > 1){
			filename = arg[1];
			if (installed and filename.tail(5) != ".kaba"){
				string dd = directory_static + "apps/" + arg[1] + "/" + arg[1] + ".kaba";
				if (arg[1].find("/") >= 0)
					dd = directory_static + "apps/" + arg[1] + ".kaba";
				if (file_test_existence(dd))
					filename = dd;
			}
		}else{
			msg_end();
			return false;
		}

		// compile
		Kaba::config.compile_silently = !flag_verbose;
		Kaba::config.verbose = flag_verbose;
		Kaba::config.verbose_func_filter = debug_func_filter;
		Kaba::config.verbose_stage_filter = debug_stage_filter;
		Kaba::config.compile_os = flag_compile_os;
		Kaba::config.add_entry_point = flag_add_entry_point;
		Kaba::config.no_function_frame = flag_no_function_frames;
		Kaba::config.override_variables_offset = flag_override_variable_offset;
		Kaba::config.variables_offset = variable_offset;
		Kaba::config.override_code_origin = flag_override_code_origin;
		Kaba::config.code_origin = code_origin;

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
				hui::ErrorBox(NULL, _("Fehler in Script"), e.message());
			else
				msg_error(e.message());
			error = true;
		}

		// end
		Kaba::End();
		msg_end();
		if (error)
			exit(1);
		return false;

	}

#pragma GCC push_options
#pragma GCC optimize("no-omit-frame-pointer")
#pragma GCC optimize("no-inline")
#pragma GCC optimize("0")

	void execute(Kaba::Script *s, Array<string> &arg)
	{
		// set working directory -> script file
		//msg_write(initial_working_directory);
		//hui::setDirectory(initial_working_directory);
		//setDirectory(s->filename.dirname());

		main_arg_func *f_arg = (main_arg_func*)s->match_function("main", "void", {"string[]"});
		main_void_func *f_void = (main_void_func*)s->match_function("main", "void", {});

		if (f_arg){
			// special execution...
			Array<string> _arg = arg.sub(2, -1);
			f_arg(_arg);
		}else if (f_void){
			// default execution
			f_void();
		}
		Kaba::Remove(s);
	}
#pragma GCC pop_options

	void output_to_file_raw(Kaba::Script *s, const string &out_file)
	{
		File *f = FileCreate(out_file);
		f->write_buffer(s->opcode, s->opcode_size);
		delete(f);
	}

	void output_to_file_elf(Kaba::Script *s, const string &out_file)
	{
		File *f = FileCreate(out_file);

		bool is64bit = (Kaba::config.pointer_size == 8);

		// 16b header
		f->write_char(0x7f);
		f->write_char('E');
		f->write_char('L');
		f->write_char('F');
		f->write_char(0x02); // 64 bit
		f->write_char(0x01); // little-endian
		f->write_char(0x01); // version
		for (int i=0; i<9; i++)
			f->write_char(0x00);

		f->write_word(0x0003); // 3=shared... 2=exec
		if (Kaba::config.instruction_set == Asm::INSTRUCTION_SET_AMD64){
			f->write_word(0x003e); // machine
		}else if (Kaba::config.instruction_set == Asm::INSTRUCTION_SET_X86){
			f->write_word(0x0003); // machine
		}else if (Kaba::config.instruction_set == Asm::INSTRUCTION_SET_ARM){
			f->write_word(0x0028); // machine
		}
		f->write_int(1); // version

		if (is64bit){
			f->write_int(0);	f->write_int(0);// entry point
			f->write_int(0x40);	f->write_int(0x00); // program header table offset
			f->write_int(0x00);	f->write_int(0x00); // section header table
		}else{
			f->write_int(0);// entry point
			f->write_int(0x00); // program header table offset
			f->write_int(0x00); // section header table
		}
		f->write_int(0); // flags
		f->write_word(is64bit ? 64 : 52); // header size
		f->write_word(0); // prog header size
		f->write_word(0); // # prog header table entries
		f->write_word(0); // size of section header entry table
		f->write_word(0); // # section headers
		f->write_word(0); // names entry section header index
		//f->WriteBuffer(s->opcode, s->opcode_size);
		delete(f);

		system(("chmod a+x " + out_file).c_str());
	}

	void export_symbols(Kaba::Script *s, const string &symbols_out_file)
	{
		File *f = FileCreate(symbols_out_file);
		for (auto *fn: s->syntax->functions){
			f->write_str(fn->long_name + ":" + i2s(fn->num_params));
			f->write_int((long)fn->address);
		}
		for (auto *v: s->syntax->root_of_all_evil.var){
			f->write_str(v->name);
			f->write_int((long)v->memory);
		}
		f->write_str("#");
		delete(f);
	}

	void import_symbols(const string &symbols_in_file)
	{
		File *f = FileOpen(symbols_in_file);
		while (!f->eof()){
			string name = f->read_str();
			if (name == "#")
				break;
			int pos = f->read_int();
			Kaba::LinkExternal(name, (void*)(long)pos);
		}
		delete(f);
	}
};


HUI_EXECUTE(KabaApp)
