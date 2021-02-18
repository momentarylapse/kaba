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
#include "../asm/asm.h"
#include "../../file/msg.h"


namespace Asm{
	extern int ARM_DATA_INSTRUCTIONS[16]; // -> asm.cpp
};

namespace kaba {

#define reg_s0 param_preg(TypeFloat32, Asm::REG_S0)
#define reg_s1 param_preg(TypeFloat32, Asm::REG_S1)

bool is_typed_function_pointer(const Class *c);

BackendARM::BackendARM(Serializer *s) : Backend(s) {
	map_reg_root = {Asm::RegRoot[Asm::REG_R0], Asm::RegRoot[Asm::REG_R1]};
}

BackendARM::~BackendARM() {
}

void BackendARM::process(Function *f, int index) {
	cur_func = f;
	cur_func_index = index;
	//call_used = false;
	stack_offset = f->_var_size;
	stack_max_size = f->_var_size;

	do_mapping();
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

void BackendARM::_immediate_to_register(int val, int r) {
	insert_cmd(Asm::INST_MOV, param_vreg(TypeInt, r), param_imm(TypeInt, val&0x000000ff));
	if ((val & 0x0000ff00) != 0)
		insert_cmd(Asm::INST_ADD, param_vreg(TypeInt, r), param_vreg(TypeInt, r), param_imm(TypeInt, val&0x0000ff00));
	if ((val & 0x00ff0000) != 0)
		insert_cmd(Asm::INST_ADD, param_vreg(TypeInt, r), param_vreg(TypeInt, r), param_imm(TypeInt, val&0x00ff0000));
	if ((val & 0xff000000) != 0)
		insert_cmd(Asm::INST_ADD, param_vreg(TypeInt, r), param_vreg(TypeInt, r), param_imm(TypeInt, val&0xff000000));

}
void BackendARM::_register_to_local(int r, int offset) {
	insert_cmd(Asm::INST_STR, param_vreg(TypeInt, r), param_local(TypeInt, offset));
}
void BackendARM::_local_to_register(int offset, int r) {
	insert_cmd(Asm::INST_LDR, param_vreg(TypeInt, r), param_local(TypeInt, offset));
}

int BackendARM::_to_register(const SerialNodeParam &p, int offset) {
	//if (p.kind == NodeKind::REGISTER)
	//	return p.p;

	int reg = find_unused_reg(cmd.next_cmd_index, cmd.next_cmd_index, 4);//Asm::REG_R0;
	if (p.kind == NodeKind::CONSTANT) {
		auto cc = (Constant*)p.p;
		_immediate_to_register(*((int*)((char*)cc->p() + offset)), reg);
	} else if (p.kind == NodeKind::VAR_LOCAL) {
		auto var2 = (Variable*)p.p;
		_local_to_register(var2->_offset + offset, reg);
	} else if (p.kind == NodeKind::LOCAL_MEMORY) {
		_local_to_register(p.p + offset, reg);
	} else {
		do_error("evil mov target..." + kind2str(p.kind));
	}
	return reg;
}

void BackendARM::_from_register(int reg, const SerialNodeParam &p, int offset) {
	if (p.kind == NodeKind::VAR_LOCAL) {
		auto var = (Variable*)p.p;
		_register_to_local(reg, var->_offset + offset);
	} else if (p.kind == NodeKind::LOCAL_MEMORY) {
		_register_to_local(reg, p.p + offset);
	} else {
		do_error("evil mov target..." + kind2str(p.kind));
	}
}

void BackendARM::correct_implement_commands() {

	Array<SerialNodeParam> func_params;

	for (int i=0; i<cmd.cmd.num; i++) {
		auto &c = cmd.cmd[i];
		if (c.inst == INST_MARKER)
			continue;
		if (c.inst == Asm::INST_MOV) {
			int size = c.p[0].type->size;
			auto p0 = c.p[0];
			auto p1 = c.p[1];
			cmd.remove_cmd(i);

			for (int k=0; k<size/4; k++) {
				int reg = _to_register(p1, k*4);
				_from_register(reg, p0, k*4);
			}
			if ((size%4) != 0)
				do_error("byte move...");
			i = cmd.next_cmd_index - 1;
		} else if (c.inst == Asm::INST_MOVSX or c.inst == Asm::INST_MOVZX) {
			do_error("no movsx yet");

		} else if (c.inst == Asm::INST_ADD) {
			auto p0 = c.p[0];
			auto p1 = c.p[1];
			auto p2 = c.p[2];
			cmd.remove_cmd(i);

			int reg1 = _to_register(p1, 0);
			cmd.set_virtual_reg(reg1, i, cmd.next_cmd_index);
			int reg2 = _to_register(p2, 0);

			insert_cmd(Asm::INST_ADD, param_vreg(TypeInt, reg1), param_vreg(TypeInt, reg1), param_vreg(TypeInt, reg2));
			_from_register(reg1, p0, 0);

			i = cmd.next_cmd_index - 1;
		} else if (c.inst == Asm::INST_PUSH) {
			func_params.add(c.p[0]);
			cmd.remove_cmd(i);
			i --;
		} else if (c.inst == Asm::INST_CALL) {

			if (c.p[1].type == TypeFunctionCodeP) {
				//do_error("indirect call...");
				auto fp = c.p[1];
				auto ret = c.p[0];
				cmd.remove_cmd(i);
				cmd.next_cmd_target(i);
				add_pointer_call(fp, func_params, ret);
			} else if (is_typed_function_pointer(c.p[1].type)) {
				do_error("BACKEND: POINTER CALL");
			} else {
				//func_params.add(c.p[0]);
				auto *f = ((Function*)c.p[1].p);
				auto ret = c.p[0];
				cmd.remove_cmd(i);
				cmd.next_cmd_target(i);
				add_function_call(f, func_params, ret);
			}
			func_params.clear();
			i = cmd.next_cmd_index - 1;
		} else if (c.inst == Asm::INST_RET) {
			cmd.remove_cmd(i);
			cmd.next_cmd_target(i);
			if (stack_max_size > 0) {
				cmd.next_cmd_target(i ++);
				insert_cmd(Asm::INST_ADD, param_preg(TypePointer, Asm::REG_R13), param_preg(TypePointer, Asm::REG_R13), param_imm(TypeInt, stack_max_size));
			}
			cmd.next_cmd_target(i);
			insert_cmd(Asm::INST_LDMIA, param_preg(TypePointer, Asm::REG_R13), param_imm(TypeInt, 0xaff0)); // {r4,r5,r6,r7,r8,r9,r10,r11,r13,r15}
		} else {
			do_error("unhandled:  " + c.str(serializer));
		}
	}
}

SerialNodeParam BackendARM::insert_reference(const SerialNodeParam &param, const Class *type) {
	SerialNodeParam ret;
	if (!type)
		type = param.type->get_pointer();
	ret.type = type;
	ret.shift = 0;
	if (param.kind == NodeKind::CONSTANT_BY_ADDRESS) {
		ret.kind = NodeKind::IMMEDIATE;
		ret.p = (int_p)((char*)param.p + param.shift);
	} else if ((param.kind == NodeKind::IMMEDIATE) or (param.kind == NodeKind::MEMORY)) {
		ret.kind = NodeKind::IMMEDIATE;
		if (param.shift > 0)
			msg_error("Serializer: immediade/mem + shift?!?!?");
		ret.p = param.p;
	} else if (param.kind == NodeKind::DEREF_VAR_TEMP) {
		ret = param;
		ret.kind = NodeKind::VAR_TEMP; // FIXME why was it param.kind ?!?!?
	} else {
		ret = cmd._add_temp(type);
		insert_cmd(Asm::INST_LEA, ret, param);
	}
	return ret;
}

int BackendARM::fc_begin(const Array<SerialNodeParam> &_params, const SerialNodeParam &ret, bool is_static) {
	const Class *type = ret.type;
	if (!type)
		type = TypeVoid;

	// grow stack (down) for local variables of the calling function
//	insert_cmd(- cur_func->_VarSize - LocalOffset - 8);
	int64 push_size = 0;

	Array<SerialNodeParam> params = _params;

	// instance as first parameter
	//if (instance.type)
	//	params.insert(instance, 0);

	// return as _very_ first parameter
	if (type->uses_return_by_memory()) {
		SerialNodeParam ret_ref = insert_reference(ret);
		params.insert(ret_ref, 0);
	}

	// map params...
	Array<SerialNodeParam> reg_param;
	Array<SerialNodeParam> stack_param;
	Array<SerialNodeParam> float_param;
	for (SerialNodeParam &p: params){
		if ((p.type == TypeInt) /*or (p.type == TypeInt64)*/ or (p.type == TypeChar) or (p.type == TypeBool) or p.type->is_some_pointer()){
			if (reg_param.num < 4){
				reg_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else if ((p.type == TypeFloat32) /*or (p.type == TypeFloat64)*/){
			if (float_param.num < 8){
				float_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else
			do_error("parameter type currently not supported: " + p.type->name);
	}

	// push parameters onto stack
/*	push_size = 4 * stack_param.num;
	if (push_size > 127)
		insert_cmd(Asm::inst_add, param_reg(TypePointer, Asm::REG_RSP), param_const(TypeInt, (void*)push_size));
	else if (push_size > 0)
		insert_cmd(Asm::inst_add, param_reg(TypePointer, Asm::REG_RSP), param_const(TypeChar, (void*)push_size));
	foreachb(SerialCommandParam &p, stack_param)
		insert_cmd(Asm::inst_push, p);
	max_push_size = max(max_push_size, push_size);*/

	// s0-7
	foreachib(auto &p, float_param, i){
		int reg = Asm::REG_S0 + i;
		/*if (p.type == TypeFloat64)
			insert_cmd(Asm::inst_movsd, param_reg(TypeReg128, reg), p);
		else*/
			insert_cmd(Asm::INST_FLDS, param_preg(TypeFloat32, reg), p);
	}

	// r0, r1, r2, r3
	foreachib(auto &p, reg_param, i){
		int v = cmd.add_virtual_reg(Asm::REG_R0 + i);
		insert_cmd(Asm::INST_MOV, param_vreg(p.type, v), p);
		cmd.set_virtual_reg(v, cmd.next_cmd_index - 1, -100); // -> call
	}

	// extend reg channels to call
	for (VirtualRegister &r: cmd.virtual_reg)
		if (r.last == -100)
			r.last = cmd.next_cmd_index;

	return push_size;
}

void BackendARM::fc_end(int push_size, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	const Class *type = ret.type;
	if (!type)
		return;

	// return > 4b already got copied to [ret] by the function!
	if ((type != TypeVoid) and (!type->uses_return_by_memory())) {
		if (type == TypeFloat32) {
			insert_cmd(Asm::INST_FSTS, ret, reg_s0);
		//else if (type == TypeFloat64)
			//insert_cmd(Asm::INST_MOVSD, ret, param_preg(TypeReg128, Asm::REG_XMM0));
		} else if ((type->size == 1) or (type->size == 4)) {
			int v = cmd.add_virtual_reg(Asm::REG_R0);
			insert_cmd(Asm::INST_MOV, ret, param_vreg(TypeReg32, v));
			cmd.set_virtual_reg(v, cmd.next_cmd_index - 2, cmd.next_cmd_index - 1);
		} else {
			do_error("unhandled function value receiving... " + type->long_name());
			int v = cmd.add_virtual_reg(Asm::REG_R0);
			insert_cmd(Asm::INST_MOV, ret, param_vreg(TypeReg32, v));
			cmd.set_virtual_reg(v, cmd.next_cmd_index - 2, cmd.next_cmd_index - 1);
		}
	}
}

void BackendARM::add_function_call(Function *f, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	serializer->call_used = true;
	int push_size = fc_begin(params, ret, f->is_static());

	if ((f->owner() == script->syntax) and (!f->is_extern())) {
		insert_cmd(Asm::INST_CALL, param_marker(TypePointer, f->_label));
	} else {
		if (!f->address)
			script->do_error_link("could not link function " + f->long_name());
		if (abs((int_p)f->address - (int_p)this->script->opcode) < 30000000) {
			insert_cmd(Asm::INST_CALL, param_imm(TypePointer, (int_p)f->address)); // the actual call
			// function pointer will be shifted later...
		} else {

			// TODO FIXME
			// really find a usable register...

			int v = cmd.add_virtual_reg(Asm::REG_R4);//find_unused_reg(cmd.next_cmd_index-1, cmd.next_cmd_index-1, 4);
			insert_cmd(Asm::INST_MOV, param_vreg(TypePointer, v), param_lookup(TypePointer, add_global_ref(f->address)));
			insert_cmd(Asm::INST_CALL, param_vreg(TypePointer, v));
			cmd.set_virtual_reg(v, cmd.next_cmd_index-2, cmd.next_cmd_index-1);
		}
	}

	fc_end(push_size, params, ret);
}


void BackendARM::add_pointer_call(const SerialNodeParam &fp, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {

}

void BackendARM::add_function_intro_params(Function *f) {
}

int BackendARM::add_global_ref(void *p) {
	foreachi(GlobalRef &g, global_refs, i)
		if (g.p == p)
			return i;
	GlobalRef g;
	g.p = p;
	g.label = -1;
	global_refs.add(g);
	return global_refs.num - 1;
}

void BackendARM::do_mapping() {

	map_remaining_temp_vars_to_stack();

	serializer->cmd_list_out("map:e", "post var reg");
}


inline void try_map_param_to_stack(SerialNodeParam &p, int v, SerialNodeParam &stackvar) {
	if ((p.kind == NodeKind::VAR_TEMP) and (p.p == v)) {
		p.kind = NodeKind::LOCAL_MEMORY;//stackvar.kind;
		p.p = stackvar.p;
	} else if ((p.kind == NodeKind::DEREF_VAR_TEMP) and (p.p == v)) {
		p.kind = NodeKind::DEREF_LOCAL_MEMORY;
		p.p = stackvar.p;
	}
}

void BackendARM::map_remaining_temp_vars_to_stack() {
	for (int i=cmd.temp_var.num-1;i>=0;i--) {
		SerialNodeParam stackvar;
		serializer->add_stack_var(cmd.temp_var[i], stackvar);
		for (int j=0;j<cmd.cmd.num;j++) {
			for (int k=0; k<SERIAL_NODE_NUM_PARAMS; k++)
				try_map_param_to_stack(cmd.cmd[j].p[k], i, stackvar);
		}
		cmd.remove_temp_var(i);
	}
}

Asm::InstructionParam BackendARM::prepare_param(int inst, SerialNodeParam &p) {
	if (p.kind == NodeKind::NONE) {
		return Asm::param_none;
	} else if (p.kind == NodeKind::MARKER) {
		return Asm::param_label(p.p, p.type->size);
	} else if (p.kind == NodeKind::DEREF_MARKER) {
		return Asm::param_deref_label(p.p, p.type->size);
	} else if (p.kind == NodeKind::REGISTER) {
		if (p.shift > 0)
			do_error("prepare_param: reg + shift");
		return Asm::param_reg(p.p);
		//param_size = p.type->size;
	} else if (p.kind == NodeKind::DEREF_REGISTER) {
		if (p.shift != 0)
			return Asm::param_deref_reg_shift(p.p, p.shift, p.type->size);
		else
			return Asm::param_deref_reg(p.p, p.type->size);
	} else if (p.kind == NodeKind::MEMORY) {
		int size = p.type->size;
		// compiler self-test
		if ((size != 1) and (size != 2) and (size != 4) and (size != 8))
			do_error("prepare_param: evil global of type " + p.type->name);
		return Asm::param_deref_imm(p.p + p.shift, size);
	} else if (p.kind == NodeKind::LOCAL_MEMORY) {
		if (config.instruction_set == Asm::InstructionSet::ARM) {
			return Asm::param_deref_reg_shift(Asm::REG_R13, p.p + p.shift, p.type->size);
		} else {
			return Asm::param_deref_reg_shift(Asm::REG_EBP, p.p + p.shift, p.type->size);
		}
		//if ((param_size != 1) and (param_size != 2) and (param_size != 4) and (param_size != 8))
		//	param_size = -1; // lea doesn't need size...
			//s->DoErrorInternal("get_param: evil local of type " + p.type->name);
	} else if (p.kind == NodeKind::CONSTANT_BY_ADDRESS) {
		bool imm_allowed = Asm::get_instruction_allow_const(inst);
		if ((imm_allowed) and (p.type->is_pointer())) {
			return Asm::param_imm(*(int_p*)(p.p + p.shift), p.type->size);
		} else if ((p.type->size <= 4) and (imm_allowed)) {
			return Asm::param_imm(*(int*)(p.p + p.shift), p.type->size);
		} else {
			return Asm::param_deref_imm(p.p + p.shift, p.type->size);
		}
	} else if (p.kind == NodeKind::IMMEDIATE) {
		if (p.shift > 0)
			do_error("get_param: immediate + shift");
		return Asm::param_imm(p.p, p.type->size);
	} else {
		do_error("prepare_param: unexpected param..." + p.str(serializer));
	}
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



