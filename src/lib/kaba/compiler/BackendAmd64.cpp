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
			} else if (p.kind == NodeKind::CONSTANT) {
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

	Array<SerialNodeParam> func_params;

	for (int i=0; i<cmd.num; i++) {
		auto &c = cmd[i];
		if (c.inst == Asm::INST_MOV) {

		}
		if ((c.inst == Asm::INST_IMUL) or (c.inst == Asm::INST_ADD) or (c.inst == Asm::INST_SUB)) {
			auto inst = c.inst;
			auto r = c.p[0];
			auto p1 = c.p[1];
			auto p2 = c.p[2];
			auto type = p1.type;
			int reg = serializer->find_unused_reg(i, i, type->size);

			auto t = serializer->param_vreg(type, reg);
			serializer->remove_cmd(i);
			serializer->next_cmd_target(i);
			serializer->add_cmd(Asm::INST_MOV, t, p1);
			serializer->next_cmd_target(i+1);
			serializer->add_cmd(inst, t, p2);
			serializer->next_cmd_target(i+2);
			serializer->add_cmd(Asm::INST_MOV, r, t);
			serializer->set_virtual_reg(reg, i, i + 2);

			i += 2;
		}
		if (c.inst == Asm::INST_PUSH) {
			func_params.add(c.p[0]);
		}
		if (c.inst == Asm::INST_CALL) {
			//func_params.add(c.p[0]);
			func_params.clear();
		}
	}
	serializer->cmd_list_out("x:b", "post paramtrafo");
}



}
