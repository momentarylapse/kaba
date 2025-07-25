#include "lib/base/base.h"
#include "lib/os/app.h"
#include "lib/os/file.h"
#include "lib/os/filesystem.h"
#include "lib/os/msg.h"
#include "lib/os/date.h"
#include "lib/os/CommandLineParser.h"
#include "lib/base/future.h"
//#include "lib/hui/hui.h"
#include "lib/net/net.h"
#include "lib/kaba/kaba.h"
#include "lib/kaba/Interpreter.h"
#include "lib/kaba/lib/lib.h"
#include "helper/elf.h"
#include "helper/symbols.h"
#include "helper/experiments.h"
#include "helper/ErrorHandler.h"


string AppName = "kaba";
string AppVersion = kaba::Version;

base::future<string> xxx_fff() {
	return base::failed<string>();
}

namespace kaba {
	extern int64 s2i2(const string &str);
};

KABA_LINK_GROUP_BEGIN

class KabaApp {
public:
	KabaApp() {
		//set_property("name", AppName);
		//set_property("version", AppVersion);
		//hui::EndKeepMsgAlive = true;
	}

	bool use_gui = false;
	kaba::Abi abi = kaba::Abi::NATIVE;
	Path out_file, symbols_out_file, symbols_in_file;
	bool flag_allow_std_lib = true;
	bool flag_disassemble = false;
	bool flag_verbose = false;
	bool flag_just_check_syntax = false;
	string debug_func_filter = "*";
	string debug_stage_filter = "*";
	bool flag_show_consts = false;
	string output_format = "raw";
	bool flag_interpret = false;
	bool flag_clean_up = false;

	int on_startup(const Array<string> &arg0) {

		CommandLineParser p;
		p.info(AppName, "compiler for the kaba language");
		p.option("-g/--gui", "show errors in dialog box", [this] {
			use_gui = true;
		});
		p.option("--arch", "CPU:SYSTEM", "override target architecture: x86/amd64/arm32/arm64:gnu/win", [this] (const string &a) {
			if (a == "amd64:gnu") {
				abi = kaba::Abi::AMD64_GNU;
			} else if (a == "amd64:win") {
				abi = kaba::Abi::AMD64_WINDOWS;
			} else if (a == "x86:gnu") {
				abi = kaba::Abi::X86_GNU;
			} else if (a == "x86:win") {
				abi = kaba::Abi::X86_WINDOWS;
			} else if (a == "arm32:gnu") {
				abi = kaba::Abi::ARM32_GNU;
			} else if (a == "arm64:gnu") {
				abi = kaba::Abi::ARM64_GNU;
			} else {
				throw Exception("unknown architecture");
			}
		});
		p.option("--no-std-lib", "(for --output)", [this] {
			flag_allow_std_lib = false;
		});
		p.option("--remove-unused", "code size optimization", [] {
			kaba::config.remove_unused = true;
		});
		p.option("--no-simplify-consts", "don't evaluate constant terms at compile time", [] {
			kaba::config.allow_simplify_consts = false;
		});
		p.option("--just-check", "check syntax, don't compile", [this] {
			flag_just_check_syntax = true;
		});
		p.option("--verbose", "lots of output", [this] {
			flag_verbose = true;
		});
		p.option("--vfunc", "FILTER", "restrict verbosity to functions", [this] (const string &a) {
			debug_func_filter = a;
		});
		p.option("--vstage", "FILTER", "restrict verbosity to compile stages", [this](const string &a) {
			debug_stage_filter = a;
		});
		p.option("--disasm", "show disassemble of opcode", [this] {
			flag_disassemble = true;
		});
		p.option("--show-tree", "show syntax tree", [this] {
			flag_verbose = true;
			debug_stage_filter = "parse:a";
		});
		p.option("--show-consts", "", [this] {
			flag_show_consts = true;
		});
		p.option("--add-entry-point", "(os)", [] {
			kaba::config.add_entry_point = true;
		});
		p.option("--code-origin", "ORIGIN", "(os) set a custom code location", [] (const string &a) {
			kaba::config.override_code_origin = true;
			kaba::config.code_origin = kaba::s2i2(a);
		});
		p.option("--variable-offset", "OFFSET", "(os) ", [] (const string &a) {
			kaba::config.override_variables_offset = true;
			kaba::config.variables_offset = kaba::s2i2(a);
		});
		p.option("-o/--output", "OUTFILE", "compile into file", [this] (const string &a) {
			out_file = a;
		});
		p.option("--output-format", "FORMAT", "format for --output: raw/elf", [this] (const string &a) {
			output_format = a;
			if ((output_format != "raw") and (output_format != "elf"))
				die("output format has to be 'raw' or 'elf', not: " + output_format);
		});
		p.option("--export-symbols", "FILE", "save link table to file", [this] (const string &a) {
			symbols_out_file = a;
		});
		p.option("--import-symbols", "FILE", "load link table from file", [this] (const string &a) {
			symbols_in_file = a;
		});
		p.option("--interpret", "run in interpreter instead of native", [this] {
			flag_interpret = true;
		});
		p.option("--clean", "clean up after execution", [this] {
			flag_clean_up = true;
		});
		p.cmd("--xxx", "", "some experiment", [this] (const Array<string>&) {
			kaba::init(abi, flag_allow_std_lib);
			do_experiments();
		});
		p.cmd("-v/--version", "", "print the version", [] (const Array<string>&) {
			msg_write("--- " + AppName + " " + AppVersion + " ---");
			msg_write("native arch: " + kaba::abi_name(kaba::guess_native_abi()));;
			msg_write("kaba: " + kaba::Version);
			//msg_write("hui: " + hui::Version);
		});
		p.cmd("-h/--help", "", "show this help page", [&p] (const Array<string>&) {
			p.show();
		});
		p.cmd("--just-disasm", "FILE", "disassemble opcode from a file", [this] (const Array<string> &a) {
			disassemble_file(a[0]);
		});
		p.cmd("-c/--command", "CODE", "compile and run a single command", [this] (const Array<string> &a) {
			init_environment();
			try {
				kaba::default_context->execute_single_command(a[0]);
			} catch (Exception &e) {
				die(e.message());
			}
			kaba::default_context->clean_up();
		});
		p.cmd("", "FILENAME ...", "compile and run a file", [this] (const Array<string> &a) {
			init_environment();

			auto s = compile_file(a[0]);
			if (!s or flag_just_check_syntax)
				return;

			if (out_file) {
				// output into file?
				if (output_format == "raw")
					output_to_file_raw(s, out_file);
				else if (output_format == "elf")
					output_to_file_elf(s, out_file);

				//if (flag_disassemble)
				//	msg_write(Asm::disassemble(s->opcode, s->opcode_size, true));
			} else if (kaba::config.target.is_native) {
				// direct execution
				execute(s, a.sub_ref(1));
			} else {
				die("can only execute files when using the native ABI");
			}
		});

		p.parse(arg0);

		// end
		if (flag_clean_up)
			kaba::clean_up();
		//msg_end();

		return 0;//hui::AppStatus::END;
	}
	/*hui::AppStatus on_startup(const Array<string> &arg0) override {
		return hui::AppStatus::END;
	}*/

	void die(const string &msg) {
		/*if (use_gui)
			hui::error_box(NULL, _("Error in script"), msg);
		else*/
			msg_error(msg);
		exit(1);
	}

	static void xxx_delete(VirtualBase* v) {
		msg_write("...xxx_delete");// +p2s(v));
		delete v;
		//v->__delete_external__();
		//v->__delete__();
	}

	void init_environment() {
		srand(Date::now().time*73 + Date::now().milli_second);
		NetInit();
		kaba::init(abi, flag_allow_std_lib);
		ErrorHandler::init();


		// for huibui.kaba...
		auto e = kaba::default_context->external.get();
/*		e->link_class_func("Resource.str", &hui::Resource::to_string);
		e->link_class_func("Resource.show", &hui::Resource::show);
		e->link("ParseResource", (void*)&hui::parse_resource);*/

		e->link("xxx_delete", (void*)&xxx_delete);
		e->link("f", (void*)&xxx_fff);
		e->link("future[string].__delete__", (void*)&xxx_fff);



		if (symbols_in_file)
			import_symbols(symbols_in_file);

		if (flag_disassemble) {
			flag_verbose = true;
			if (debug_stage_filter == "*")
				debug_stage_filter = "dasm*";
			else
				debug_stage_filter += ",dasm*";
		}

		kaba::config.compile_silently = !flag_verbose;
		kaba::config.verbose = flag_verbose;
		kaba::config.verbose_func_filter = debug_func_filter;
		kaba::config.verbose_stage_filter = debug_stage_filter;
		kaba::config.fully_linear_output = !out_file.is_empty();
		kaba::config.target.interpreted = flag_interpret;
	}

	static Path try_get_installed_app_file(const Path &filename) {
		for (auto &dir: Array<Path>({os::app::directory_dynamic, os::app::directory_static})) {
			Path dd = dir | "packages" | filename.str() | (filename.str() + ".kaba");
			if (filename.str().find("/") >= 0)
				dd = dir | "packages" | (filename.str() + ".kaba");
			if (os::fs::exists(dd))
				return dd;
		}
		// nope, not found
		return filename;
	}

	static void show_constants(shared<kaba::Module> s) {
		msg_write("---- constants ----");
		for (auto *c: weak(s->tree->base_class->constants))
			msg_write(format("%12s %-20s %s", c->type->name, c->str(), c->value.hex()));
	}

	shared<kaba::Module> compile_file(const Path &_filename) {
		auto filename = _filename;
		if (os::app::installed and filename.extension() != "kaba")
			filename = try_get_installed_app_file(filename);

		try {
			auto s = kaba::default_context->load_module(filename, flag_just_check_syntax);
			if (symbols_out_file)
				export_symbols(s, symbols_out_file);
			if (flag_show_consts)
				show_constants(s);
			return s;
		} catch (kaba::Exception &e) {
			die(e.message());
		}
		return nullptr;
	}

	void execute(shared<kaba::Module> s, const Array<string> &args) {
		if (kaba::config.target.interpreted) {
			s->interpreter->run("main");
			return;
		}


		typedef void main_arg_func(const Array<string>&);
		typedef void main_void_func();
		typedef int xfunc(int, int);

		if (auto f = (main_arg_func*)s->match_function("main", "void", {"string[]"})) {
			f(args);
		} else if (auto f = (main_void_func*)s->match_function("main", "void", {})) {
			f();
		} else if (auto f = (xfunc*)s->match_function("xmain", "void", {})) {
			// EXPERIMENT!
			msg_write("EXECUTE:");
			int r = f(13, 4);
			msg_write(r);
		} else if (auto f = (xfunc*)s->match_function("main", "int", {"int", "int"})) {
			//EXPERIMENT
			msg_write("EXECUTE:");
			int r = f(13, 4);
			msg_write(r);
		} else {
			die("no 'func main()' or 'func main(string[])' found");
		}
	}

	static void output_to_file_raw(shared<kaba::Module> s, const Path &_out_file) {
		auto f = os::fs::open(_out_file, "wb");
		f->write(s->opcode, s->opcode_size);
		delete f;
	}

	void disassemble_file(const Path &filename) const {
		bytes s = os::fs::read_binary(filename);
		kaba::init(abi, flag_allow_std_lib);
		int data_size = 0;
		if (kaba::config.target.is_x86()) {
			for (int i=0; i<s.num-1; i++)
				if (s[i] == 0x55 and s[i+1] == 0x89) {
					data_size = i;
					break;
				}
		}
		for (int i=0; i<data_size; i+= 4)
			msg_write(format("   data %03x:  ", i) + s.sub(i, i+4).hex());
		msg_write(Asm::disassemble(&s[data_size], s.num-data_size, true));
	}
};
KABA_LINK_GROUP_END


namespace os::app {
	int main(const Array<string> &args) {
		detect(args, "kaba");
		auto app = new KabaApp();
		int r = app->on_startup(args);
		delete app;
		return r;
	}
}


