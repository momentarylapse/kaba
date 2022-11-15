/*----------------------------------------------------------------------------*\
| Kaba                                                                         |
| -> C-like scripting system                                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2009.10.04 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(KABA_H__INCLUDED_)
#define KABA_H__INCLUDED_

namespace kaba {
	class Module;
}

#include "../base/base.h"
#include "../base/pointer.h"
#include "../os/path.h"
#include "Context.h"
#include "Module.h"
#include "lib/lib.h"
#include "syntax/SyntaxTree.h"

namespace kaba {

extern string Version;


shared<Module> load(const Path &filename, bool just_analyse = false);
shared<Module> create_for_source(const string &source, bool just_analyse = false);
void execute_single_command(const string &cmd);

const Class *get_dynamic_type(const VirtualBase *p);
string var2str(const void *p, const Class *type);

};

#endif
