/*
 * import.cpp
 *
 *  Created on: 29 Mar 2022
 *      Author: michi
 */

#include "../kaba.h"
#include "Parser.h"
#include "../../file/file_op.h"
#include "../../file/msg.h"


#include "../../config.h"

#ifdef _X_USE_HUI_
#include "../../hui/Application.h"
#elif defined(_X_USE_HUI_MINIMAL_)
#include "../../hui_minimal/Application.h"
#endif


const int MAX_IMPORT_DIRECTORY_PARENTS = 5;

namespace kaba {

extern ExpressionBuffer *cur_exp_buf;
extern Array<shared<Module>> loading_module_stack;
void SetImmortal(SyntaxTree *ps);

string canonical_import_name(const string &s) {
	return s.lower().replace(" ", "").replace("_", "");
}

string dir_has(const Path &dir, const string &name) {
	auto list = dir_search(dir, "*", "fd");
	for (auto &e: list)
		if (canonical_import_name(e.str()) == name)
			return e.str();
	return "";
}

Path import_dir_match(const Path &dir0, const string &name) {
	auto xx = name.explode("/");
	Path filename = dir0;

	for (int i=0; i<xx.num; i++) {
		string e = dir_has(filename, canonical_import_name(xx[i]));
		if (e == "")
			return Path::EMPTY;
		filename <<= e;
	}
	return filename;

	if (file_exists(dir0 << name))
		return dir0 << name;
	return Path::EMPTY;
}

Path find_installed_lib_import(const string &name) {
	Path kaba_dir = hui::Application::directory.parent() << "kaba";
	if (hui::Application::directory.basename()[0] == '.')
		kaba_dir = hui::Application::directory.parent() << ".kaba";
	Path kaba_dir_static = hui::Application::directory_static.parent() << "kaba";
	for (auto &dir: Array<Path>({kaba_dir, kaba_dir_static})) {
		auto path = (dir << "lib" << name).canonical();
		if (file_exists(path))
			return path;
	}
	return Path::EMPTY;
}

Path find_import(Module *s, const string &_name) {
	string name = _name.replace(".kaba", "");
	name = name.replace(".", "/") + ".kaba";

	if (name.head(2) == "@/")
		return find_installed_lib_import(name.sub(2));

	for (int i=0; i<MAX_IMPORT_DIRECTORY_PARENTS; i++) {
		Path filename = import_dir_match((s->filename.parent() << string("../").repeat(i)).canonical(), name);
		if (!filename.is_empty())
			return filename;
	}

	return find_installed_lib_import(name);

	return Path::EMPTY;
}

shared<Module> get_import(Parser *parser, const string &name) {

	// internal packages?
	for (auto p: packages)
		if (p->filename.str() == name)
			return p;

	Path filename = find_import(parser->tree->module, name);
	if (filename.is_empty())
		parser->do_error_exp(format("can not find import '%s'", name));

	for (auto ss: weak(loading_module_stack))
		if (ss->filename == filename)
			parser->do_error_exp("recursive import");

	msg_right();
	shared<Module> include;
	try {
		include = load(filename, parser->tree->module->just_analyse or config.compile_os);
		// os-includes will be appended to syntax_tree... so don't compile yet
	} catch (Exception &e) {
		msg_left();

		int token_id = parser->Exp.cur_token();
		string expr = parser->Exp.get_token(token_id);
		e.line = parser->Exp.token_physical_line_no(token_id);
		e.column = parser->Exp.token_line_offset(token_id);
		e.text += format("\n...imported from:\nline %d, %s", e.line+1, parser->tree->module->filename);
		throw e;
		//msg_write(e.message);
		//msg_write("...");
		string msg = e.message() + "\nimported file:";
		//string msg = "in imported file:\n\"" + e.message + "\"";
		parser->do_error_exp(msg);
	}
	cur_exp_buf = &parser->Exp;

	msg_left();
	return include;
}


static bool _class_contains(const Class *c, const string &name) {
	for (auto *cc: weak(c->classes))
		if (cc->name == name)
			return true;
	for (auto *f: weak(c->functions))
		if (f->name == name)
			return true;
	for (auto *cc: weak(c->constants))
		if (cc->name == name)
			return true;
	return false;
}

// import data from an included module file
void SyntaxTree::add_include_data(shared<Module> s, bool indirect) {
	for (auto i: weak(includes))
		if (i == s)
			return;

	SyntaxTree *ps = s->syntax;
	if (flag_immortal)
		SetImmortal(ps);

	flag_string_const_as_cstring |= ps->flag_string_const_as_cstring;


	/*if (FlagCompileOS) {
		import_deep(this, ps);
	} else {*/
	if (indirect) {
		imported_symbols->classes.add(ps->base_class);
	} else {
		for (auto *c: weak(ps->base_class->classes))
			imported_symbols->classes.add(c);
		for (auto *f: weak(ps->base_class->functions))
			imported_symbols->functions.add(f);
		for (auto *v: weak(ps->base_class->static_variables))
			imported_symbols->static_variables.add(v);
		for (auto *c: weak(ps->base_class->constants))
			imported_symbols->constants.add(c);
		if (s->filename.basename().find(".kaba") < 0)
			if (!_class_contains(imported_symbols.get(), ps->base_class->name)) {
				imported_symbols->classes.add(ps->base_class);
			}
	}
	includes.add(s);
	//}

	for (Operator *op: ps->operators)
		if (op->owner == ps)
			operators.add(op);
}


}
