#include "lib/base/base.h"
#include "lib/file/file.h"
#include "lib/hui/hui.h"
#include "lib/nix/nix.h"
#include "lib/net/net.h"
#include "lib/kaba/kaba.h"

string AppName = "Kaba";
string AppVersion = "0.18.5.1";


typedef void main_arg_func(const Array<string>&);
typedef void main_void_func();



namespace Kaba {
	extern int64 s2i2(const string &str);
};

static void extern_func_int_out(int a) {
	msg_write("out: " + i2s(a));
}

static void extern_func_float_out(float a) {
	msg_write("float out..." + string(&a, 4).hex());
	msg_write("out: " + f2s(a, 6));
}

static float extern_func_float_ret() {
	return 13.0f;
}

static void _x_call_float() {
	extern_func_float_out(13);
}


int extern_function2() {
	return 2001;
}

static int extern_variable1 = 13;


class CLIParser {
public:
	struct Option {
		string name;
		string parameter;
		hui::Callback callback;
		std::function<void(const string&)> callback_param;
	};
	Array<Option> options;
	void option(const string &name, const hui::Callback &cb) {
		options.add({name, "", cb, nullptr});
	}
	void option(const string &name, const string &p, const std::function<void(const string&)> &cb) {
		options.add({name, p, nullptr, cb});
	}
	string _info;
	void info(const string &i) {
		_info = i;
	}
	void show() {
		msg_write(_info);
		msg_write("");
		msg_write("options:");
		for (auto &o: options)
			if (o.parameter.num > 0)
				msg_write("  " + o.name + " " + o.parameter);
			else
				msg_write("  " + o.name);
	}
	Array<string> arg;
	void parse(const Array<string> &_arg) {
		for (int i=1; i<_arg.num; i++) {
			if (_arg[i].head(1) == "-") {
				bool found = false;
				for (auto &o: options) {
					if (sa_contains(o.name.explode("/"), _arg[i])) {
						if (o.parameter.num > 0) {
							if (_arg.num <= i + 1) {
								msg_error("parameter '" + o.parameter + "' expected after " + o.name);
								exit(1);
							}
							o.callback_param(_arg[i+1]);
							i ++;
						} else {
							o.callback();
						}
						found = true;
					}
				}
				if (!found) {
					msg_error("unknown option " + _arg[i]);
					exit(1);
				}
			} else {
				// rest
				arg = _arg.sub(i, -1);
				break;
			}
		}

	}
};


class KabaApp : public hui::Application {
public:
	KabaApp() :
		hui::Application("kaba", "", 0)//hui::FLAG_LOAD_RESOURCE)
	{
		set_property("name", AppName);
		set_property("version", AppVersion);
		//hui::EndKeepMsgAlive = true;
	}

	virtual bool on_startup(const Array<string> &arg0) {

		bool use_gui = false;
		Asm::InstructionSet instruction_set = Asm::InstructionSet::NATIVE;
		Kaba::Abi abi = Kaba::Abi::NATIVE;
		string out_file, symbols_out_file, symbols_in_file;
		bool flag_allow_std_lib = true;
		bool flag_disassemble = false;
		bool flag_verbose = false;
		string debug_func_filter = "*";
		string debug_stage_filter = "*";
		bool flag_show_consts = false;
		bool flag_compile_os = false;
		bool flag_override_variable_offset = false;
		int64 variable_offset = 0;
		bool flag_override_code_origin = false;
		int64 code_origin = 0;
		bool flag_no_function_frames = false;
		bool flag_add_entry_point = false;
		string output_format = "raw";
		string command;

		bool error = false;

		CLIParser p;
		p.info(AppName + " " + AppVersion);
		p.option("--version/-v", [=]{
			msg_write("--- " + AppName + " " + AppVersion + " ---");
			msg_write("kaba: " + Kaba::Version);
			msg_write("kaba-lib: " + Kaba::LibVersion);
			msg_write("hui: " + hui::Version);
		});
		p.option("--gui/-g", [&]{ use_gui = true; });
		p.option("--arm", [&]{ instruction_set = Asm::InstructionSet::ARM; });
		p.option("--amd64", [&]{ instruction_set = Asm::InstructionSet::AMD64; });
		p.option("--x86", [&]{ instruction_set = Asm::InstructionSet::X86; });
		p.option("--no-std-lib", [&]{ flag_allow_std_lib = false; });
		p.option("--os", [&]{ flag_compile_os = true; });
		p.option("--verbose", [&]{ flag_verbose = true; flag_disassemble = true; });
		p.option("--vfunc", "FILTER", [&](const string &a){ debug_func_filter = a; });
		p.option("--vstage", "FILTER", [&](const string &a){ debug_stage_filter = a; });
		p.option("--disasm", [&]{ flag_disassemble = true; });
		p.option("--show-tree", [&]{ flag_verbose = true; debug_stage_filter = "parse:a"; });
		p.option("--show-consts", [&]{ flag_show_consts = true; });
		p.option("--no-function-frames", [&]{ flag_no_function_frames = true; });
		p.option("--add-entry-point", [&]{ flag_add_entry_point = true; });
		p.option("--code-origin", "ORIGIN", [&](const string &a){ flag_override_code_origin = true; code_origin = Kaba::s2i2(a); });
		p.option("--variable-offset", "OFFSET", [&](const string &a){ flag_override_variable_offset = true; variable_offset = Kaba::s2i2(a); });
		p.option("--output/-o", "OUTFILE", [&](const string &a){ out_file = a; });
		p.option("--output-format", "raw/elf", [&](const string &a){
			output_format = a;
			if ((output_format != "raw") and (output_format != "elf")){
				msg_error("output format has to be 'raw' or 'elf', not: " + output_format);
				exit(1);
			}
		});
		p.option("--export-symbols", "FILE", [&](const string &a){ symbols_out_file = a; });
		p.option("--import-symbols", "FILE", [&](const string &a){ symbols_in_file = a; });
		p.option("--command/-c", "CODE", [&](const string &a){ command = a; });
		p.option("--just-disasm", "FILE", [&](const string &a){
			string s = FileRead(a);
			Kaba::init(instruction_set, abi, flag_allow_std_lib);
			int data_size = 0;
			if (flag_compile_os) {
				for (int i=0; i<s.num-1; i++)
					if (s[i] == 0x55 and s[i+1] == 0x89) {
						data_size = i;
						break;
					}
			}
			for (int i=0; i<data_size; i+= 4)
				msg_write(format("   data %03x:  ", i) + s.substr(i, 4).hex());
			msg_write(Asm::disassemble(&s[data_size], s.num-data_size, true));
			exit(0);
		});
		p.option("--help/-h", [&p]{ p.show(); });
		p.parse(arg0);


		// init
		srand(Date::now().time*73 + Date::now().milli_second);
		NetInit();
		Kaba::init(instruction_set, abi, flag_allow_std_lib);
		Kaba::config.stack_size = 10485760; // 10 mb (mib)


		// for huibui.kaba...
		Kaba::link_external_class_func("Resource.str", &hui::Resource::to_string);
		Kaba::link_external_class_func("Resource.show", &hui::Resource::show);
		Kaba::link_external("ParseResource", (void*)&hui::ParseResource);


		// for experiments
		Kaba::link_external("__int_out", (void*)&extern_func_int_out);
		Kaba::link_external("__float_out", (void*)&extern_func_float_out);
		Kaba::link_external("__float_ret", (void*)&extern_func_float_ret);
		Kaba::link_external("__xxx", (void*)&_x_call_float);
		Kaba::link_external("Test2", (void*)&extern_function2);
		Kaba::link_external("extern_variable1", (void*)&extern_variable1);

		if (symbols_in_file.num > 0)
			import_symbols(symbols_in_file);
			
			
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

		// script file as parameter?
		string filename;
		if (p.arg.num > 0) {
			filename = p.arg[0];
			if (installed and filename.tail(5) != ".kaba") {
				string dd = directory_static + "apps/" + filename + "/" + filename + ".kaba";
				if (filename.find("/") >= 0)
					dd = directory_static + "apps/" + filename + ".kaba";
				if (file_test_existence(dd))
					filename = dd;
			}
		} else if (command.num > 0) {
			Kaba::ExecuteSingleScriptCommand(command);
			msg_end();
			return false;
		} else {
			msg_end();
			return false;
		}

		// compile

		try {
			Kaba::Script *s = Kaba::Load(filename);
			if (symbols_out_file.num > 0)
				export_symbols(s, symbols_out_file);
			if (flag_show_consts) {
				msg_write("---- constants ----");
				for (auto *c: s->syntax->base_class->constants) {
					msg_write(c->type->name + " " + c->str() + "  " + c->value.hex());
				}
			}
			if (out_file.num > 0) {
				if (output_format == "raw")
					output_to_file_raw(s, out_file);
				else if (output_format == "elf")
					output_to_file_elf(s, out_file);

				if (flag_disassemble)
					msg_write(Asm::disassemble(s->opcode, s->opcode_size, true));
			} else {
				if (flag_disassemble)
					msg_write(Asm::disassemble(s->opcode, s->opcode_size, true));

				if (Kaba::config.instruction_set == Asm::QueryLocalInstructionSet())
					execute(s, p.arg.sub(1, -1));
			}
		} catch(Kaba::Exception &e) {
			if (use_gui)
				hui::ErrorBox(NULL, _("Fehler in Script"), e.message());
			else
				msg_error(e.message());
			error = true;
		}

		// end
		Kaba::clean_up();
		msg_end();
		if (error)
			exit(1);
		return false;

	}

#pragma GCC push_options
#pragma GCC optimize("no-omit-frame-pointer")
#pragma GCC optimize("no-inline")
#pragma GCC optimize("0")

	void execute(Kaba::Script *s, const Array<string> &arg) {
		// set working directory -> script file
		//msg_write(initial_working_directory);
		//hui::setDirectory(initial_working_directory);
		//setDirectory(s->filename.dirname());

		main_arg_func *f_arg = (main_arg_func*)s->match_function("main", "void", {"string[]"});
		main_void_func *f_void = (main_void_func*)s->match_function("main", "void", {});

		if (f_arg) {
			// special execution...
			f_arg(arg);
		} else if (f_void) {
			// default execution
			f_void();
		}
		Kaba::Remove(s);
	}
#pragma GCC pop_options

	void output_to_file_raw(Kaba::Script *s, const string &out_file) {
		File *f = FileCreate(out_file);
		f->write_buffer(s->opcode, s->opcode_size);
		delete(f);
	}

	void output_to_file_elf(Kaba::Script *s, const string &out_file) {
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
		if (Kaba::config.instruction_set == Asm::InstructionSet::AMD64) {
			f->write_word(0x003e); // machine
		} else if (Kaba::config.instruction_set == Asm::InstructionSet::X86) {
			f->write_word(0x0003); // machine
		} else if (Kaba::config.instruction_set == Asm::InstructionSet::ARM) {
			f->write_word(0x0028); // machine
		}
		f->write_int(1); // version

		if (is64bit){
			f->write_int(0);	f->write_int(0);// entry point
			f->write_int(0x40);	f->write_int(0x00); // program header table offset
			f->write_int(0x00);	f->write_int(0x00); // section header table
		} else {
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

	string decode_symbol_name(const string &name) {
		return name.replace("lib__", "").replace("@list", "[]");
	}

	void export_symbols(Kaba::Script *s, const string &symbols_out_file) {
		File *f = FileCreate(symbols_out_file);
		for (auto *fn: s->syntax->functions) {
			int n = fn->num_params;
			if (!fn->is_static())
				n ++;
			f->write_str(decode_symbol_name(fn->long_name()) + ":" + i2s(n));
			f->write_int((int_p)fn->address);
		}
		for (auto *v: s->syntax->base_class->static_variables) {
			f->write_str(decode_symbol_name(v->name));
			f->write_int((int_p)v->memory);
		}
		f->write_str("#");
		delete(f);
	}

	void import_symbols(const string &symbols_in_file) {
		File *f = FileOpen(symbols_in_file);
		while (!f->eof()) {
			string name = f->read_str();
			if (name == "#")
				break;
			int pos = f->read_int();
			Kaba::link_external(name, (void*)(int_p)pos);
		}
		delete(f);
	}
};


HUI_EXECUTE(KabaApp)
