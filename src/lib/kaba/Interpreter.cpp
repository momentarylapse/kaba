/*
 * Interpreter.cpp
 *
 *  Created on: Jan 22, 2021
 *      Author: michi
 */

#include "Interpreter.h"
#include "kaba.h"
#include "compiler/SerializerX.h"
#include "../file/msg.h"

namespace kaba {


bool call_function(Function *f, void *ff, void *ret, const Array<void*> &param);

Interpreter::Interpreter(Script *s) {
	script = s;
}

Interpreter::~Interpreter() {
}

void Interpreter::add_function(Function *f, SerializerX *ser) {
	//msg_write("INT: add func " + f->signature(TypeVoid));
	IFunction ff;
	ff.f = f;
	//ff.cmd = new CommandList;
	//*ff.cmd = ser->cmd;
	ff.ser = ser;
	functions.add(ff);
}

void Interpreter::run(const string &name) {
	for (auto &f: functions) {
		msg_write(f.f->name);
		if (f.f->name == name) {
			run_function(f.f, f.ser);
		}
	}
}

void Interpreter::run_function(Function *f, SerializerX *ser) {
	for (int i=0; i<ser->cmd.cmd.num; i++) {
		auto &c = ser->cmd.cmd[i];
		run_command(c, ser);
	}
}

void Interpreter::run_command(SerialNode &n,SerializerX *ser) {
	//msg_write(n.str(ser));
	if (n.inst == INST_MARKER) {
	} else if (n.inst == Asm::INST_PUSH) {
		//msg_write(n.str(ser));
		msg_write("PUSH " + n.p[0].type->name);
		int64 p = 0;
		n.p[0].type;
		if (n.p[0].kind == NodeKind::CONSTANT_BY_ADDRESS) {
			p = n.p[0].p;
		}
		call_params.add(p);
		//stack.resize(stack.num + n.p[0].type->size);
		//memcpy(&stack[stack.num - n.p[0].type->size], p, n.p[0].type->size);
	} else if (n.inst == Asm::INST_RET) {
		msg_write("RETURN");
	} else if (n.inst == Asm::INST_CALL) {
		auto *f = ((Function*)n.p[1].p);
		msg_write("CALL " + f->signature(TypeVoid));
		if (f->address) {
			//msg_write("addr...");
			char rrr[64];
			Array<void*> param;
			for (int64 &p: call_params) {
				param.add(*(void**)p);
			}
			call_function(f, f->address, &rrr, param);
		} else {
			msg_error("call non-addr");
		}
		auto ret = n.p[0];
		call_params.clear();
	} else {
		msg_error(n.str(ser));
	}
}


}
