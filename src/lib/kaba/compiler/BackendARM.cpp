/*
 * BackendARM.cpp
 *
 *  Created on: Dec 1, 2020
 *      Author: michi
 */


#include "BackendARM.h"
#include "serializer.h"
#include "CommandList.h"
#include "SerialNode.h"
#include "../../file/msg.h"

namespace kaba {



bool is_typed_function_pointer(const Class *c);

BackendARM::BackendARM(Serializer *s) : Backend(s) {
}

BackendARM::~BackendARM() {
}

void BackendARM::process(Function *f, int index) {
	cur_func = f;
	cur_func_index = index;
	//call_used = false;
	stack_offset = f->_var_size;
	stack_max_size = f->_var_size;

	correct();
}

void BackendARM::correct() {
	cmd.next_cmd_target(0);
	add_function_intro_params(cur_func);

	correct_parameters();

	serializer->cmd_list_out("x:a", "paramtrafo");

	correct_implement_commands();
	serializer->cmd_list_out("x:b", "post paramtrafo");
}

void BackendARM::correct_parameters() {
}

void BackendARM::implement_mov_chunk(kaba::SerialNode &c, int i, int size) {
	auto p1 = c.p[0];
	auto p2 = c.p[1];
	cmd.remove_cmd(i);
	cmd.next_cmd_target(i);
	//msg_error("CORRECT MOV " + p1.type->name);

	for (int j=0; j<size/4; j++)
		insert_cmd(Asm::INST_MOV, param_shift(p1, j * 4, TypeInt), param_shift(p2, j * 4, TypeInt));
	for (int j=4*(size/4); j<size; j++)
		insert_cmd(Asm::INST_MOV, param_shift(p1, j, TypeChar), param_shift(p2, j, TypeChar));
}


void BackendARM::correct_implement_commands() {

	Array<SerialNodeParam> func_params;

	for (int i=0; i<cmd.cmd.num; i++) {
		auto &c = cmd.cmd[i];
		if (c.inst == Asm::INST_MOV) {
			int size = c.p[0].type->size;
			// mov can only copy these sizes (ignore 2...)
			if (size != 1 and size != 4) {
				implement_mov_chunk(c, i, size);
				i = cmd.next_cmd_index - 1;
			}
		} else if (c.inst == Asm::INST_MOVSX or c.inst == Asm::INST_MOVZX) {
			do_error("no movsx yet");
		}
	}

}

void BackendARM::add_function_intro_params(Function *f) {
}


void BackendARM::do_mapping() {
}

Asm::InstructionParam BackendARM::prepare_param(int inst, SerialNodeParam &p) {
	return Asm::param_none;
}


void BackendARM::assemble_cmd_arm(SerialNode &c) {
	// translate parameters
	auto p1 = prepare_param(c.inst, c.p[0]);
	auto p2 = prepare_param(c.inst, c.p[1]);
	auto p3 = prepare_param(c.inst, c.p[2]);

	// assemble instruction
	//list->current_line = c.
	list->add_arm(c.cond, c.inst, p1, p2, p3);
}


void BackendARM::assemble() {
//	do_error("new ARM assemble() not yet implemented");
	for (int i=0;i<cmd.cmd.num;i++) {

		if (cmd.cmd[i].inst == INST_MARKER) {
			list->insert_label(cmd.cmd[i].p[0].p);
		} else if (cmd.cmd[i].inst == INST_ASM) {
			do_error("asm block insert..."); //AddAsmBlock(list, script);
		} else {
			assemble_cmd_arm(cmd.cmd[i]);
		}
	}
	list->add2(Asm::INST_ALIGN_OPCODE);

}



}



