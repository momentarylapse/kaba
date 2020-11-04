/*
 * BackendAmd64.cpp
 *
 *  Created on: Nov 4, 2020
 *      Author: michi
 */

#include "BackendAmd64.h"
#include "serializer.h"

namespace kaba {

BackendAmd64::BackendAmd64(Serializer *s) : cmd(s->cmd) {
	serializer = s;
	script = s->script;
	list = s->list;
}

BackendAmd64::~BackendAmd64() {
}

void BackendAmd64::correct() {
	for (auto &c: cmd) {
		for (auto &p: c.p) {
			if (p.kind == NodeKind::VAR_LOCAL) {
				p.p = ((Variable*)p.p)->_offset;
				p.kind = NodeKind::LOCAL_MEMORY;
			}else if (p.kind == NodeKind::CONSTANT){
				p.p = (int_p)((Constant*)p.p)->address; // FIXME ....need a cleaner approach for compiling os...
				if (config.compile_os)
					p.kind = NodeKind::MEMORY;
				else
					p.kind = NodeKind::CONSTANT_BY_ADDRESS;
				if (script->syntax->flag_function_pointer_as_code and (p.type == TypeFunctionP)) {
					auto *fp = (Function*)(int_p)((Constant*)p.p)->as_int64();
					p.kind = NodeKind::MARKER;
					p.p = fp->_label;
				}
			}
		}
	}

	serializer->cmd_list_out("x:a", "paramtrafo");

	for (int i=0; i<cmd.num; i++) {
		auto &c = cmd[i];
		if (c.inst == Asm::INST_MOV) {

		}
	}
}



}
