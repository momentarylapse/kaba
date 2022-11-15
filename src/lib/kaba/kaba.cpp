/*----------------------------------------------------------------------------*\
| Kaba                                                                         |
| -> C-like scripting system                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2010.07.07 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "../os/file.h"
#include "kaba.h"

namespace kaba {

string Version = "0.19.22.11";

//#define ScriptDebug





shared<Module> load(const Path &filename, bool just_analyse) {
	return default_context.load(filename, just_analyse);
}

shared<Module> create_for_source(const string &buffer, bool just_analyse) {
	return default_context.create_for_source(buffer, just_analyse);
}

VirtualTable* get_vtable(const VirtualBase *p) {
	return *(VirtualTable**)p;
}

const Class *get_dynamic_type(const VirtualBase *p) {
	return default_context.get_dynamic_type(p);
}


void execute_single_command(const string &cmd) {
	default_context.execute_single_command(cmd);
}

};
