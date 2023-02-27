/*
 * symbols.cpp
 *
 *  Created on: 21 Jul 2022
 *      Author: michi
 */

#include "symbols.h"
#include "../lib/os/file.h"
#include "../lib/os/formatter.h"
#include "../lib/kaba/lib/extern.h"


namespace kaba {
	extern string function_link_name(Function *f);
	extern string decode_symbol_name(const string&);
};


void export_symbols(shared<kaba::Module> s, const Path &symbols_out_file) {
	auto f = new BinaryFormatter(os::fs::open(symbols_out_file, "wb"));
	for (auto *fn: s->tree->functions) {
		f->write_str(kaba::function_link_name(fn));
		f->write_int(fn->address);
	}
	for (auto *v: weak(s->tree->base_class->static_variables)) {
		f->write_str(kaba::decode_symbol_name(v->name));
		f->write_int((int_p)v->memory);
	}
	f->write_str("#");
	delete f;
}

void import_symbols(const Path &symbols_in_file) {
	auto f = new BinaryFormatter(os::fs::open(symbols_in_file, "rb"));
	while (!f->stream->is_end()) {
		string name = f->read_str();
		if (name == "#")
			break;
		int pos = f->read_int();
		kaba::default_context->external->link(name, (void*)(int_p)pos);
	}
	delete f;
}


