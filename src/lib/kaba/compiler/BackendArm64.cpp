//
// Created by Michael Ankele on 2024-06-23.
//

#include "BackendArm64.h"
#include "Serializer.h"
#include "../asm/asm.h"
#include "../../os/msg.h"

namespace Asm{
	extern Asm::RegID r_reg(int i);
	extern Asm::RegID w_reg(int i);
	extern Asm::RegID s_reg(int i);
};

#define VREG_ROOT(r) cmd.virtual_reg[r].reg_root

namespace kaba {

BackendArm64::BackendArm64(Serializer* serializer) : BackendARM(serializer) {
}

void BackendArm64::process(Function *f, int index) {
	cur_func = f;
	cur_func_index = index;
	//call_used = false;
	stack_offset = f->_var_size;
	stack_max_size = f->_var_size;

	do_mapping();
	correct();
}

void BackendArm64::add_function_intro_params(Function *f) {
	// return, instance, params
	Array<Variable*> param;
	if (f->effective_return_type->uses_return_by_memory()) {
		for (Variable *v: weak(f->var))
			if (v->name == Identifier::RETURN_VAR) {
				param.add(v);
				break;
			}
	}
	if (!f->is_static()) {
		for (Variable *v: weak(f->var))
			if (v->name == Identifier::SELF) {
				param.add(v);
				break;
			}
	}
	for (int i=0;i<f->num_params;i++)
		param.add(f->var[i].get());

	// map params...
	Array<Variable*> reg_param;
	Array<Variable*> stack_param;
	Array<Variable*> float_param;
	for (Variable *p: param) {
		if ((p->type == TypeInt) or (p->type == TypeInt64) or (p->type == TypeInt8) or (p->type == TypeBool) or p->type->is_some_pointer()) {
			if (reg_param.num < 8) {
				reg_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else if (p->type == TypeFloat32) {
			if (float_param.num < 8) {
				float_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else {
			do_error("parameter type currently not supported: " + p->type->name);
		}
	}

	// s0-7
	foreachib(Variable *p, float_param, i) {
		int reg = cmd.add_virtual_reg(Asm::s_reg(i));
		_from_register_float(reg, param_local(p->type, p->_offset), 0);
	}

	// r0-7
	foreachib(Variable *p, reg_param, i) {
		if (p->type->size > 4) {
			int reg = cmd.add_virtual_reg(Asm::r_reg(i));
			_from_register_32(reg, param_local(p->type, p->_offset), 0);
			cmd.set_virtual_reg(reg, cmd.cmd.num - 1, cmd.cmd.num - 1);
		} else {
			int reg = cmd.add_virtual_reg(Asm::w_reg(i));
			_from_register_32(reg, param_local(p->type, p->_offset), 0);
			cmd.set_virtual_reg(reg, cmd.cmd.num - 1, cmd.cmd.num - 1);
		}
	}

	// get parameters from stack
	foreachb([[maybe_unused]] Variable *p, stack_param) {
		do_error("func with stack...");
		/*int s = 8;
		cmd.add_cmd(Asm::inst_push, p);
		push_size += s;*/
	}
}

void BackendArm64::correct() {
	// instead of in-place editing, let's create a backup and a new list from that
	pre_cmd.ser = cmd.ser;
	cmd.cmd.exchange(pre_cmd.cmd);
	// no vregs yet, but temp vars
	pre_cmd.temp_var = cmd.temp_var;
	for (auto &t: cmd.temp_var)
		t.first = t.last = -1;

	cmd.next_cmd_target(0);

	serializer->cmd_list_out("x:a", "paramtrafo");

	correct_implement_commands();

	cmd.next_cmd_target(0);
	add_function_intro_params(cur_func);
	serializer->cmd_list_out("x:b", "post paramtrafo");
}



static int first_bit(int i) {
	for (int b=0; b<32; b++)
		if ((i & (1 << b)) != 0)
			return b;
	return 0;
}

	// TODO better use int64?
void BackendArm64::_immediate_to_register_32(int val, int r) {
	bool first = true;
	while (true) {
		int b0 = first_bit(val) & 0xfe; // only even bit positions allowed!
		int mask = 0xff << b0;
		if (first)
			insert_cmd(Asm::InstID::MOV, param_vreg(TypeInt, r), param_imm(TypeInt, val&mask));
		else
			insert_cmd(Asm::InstID::ADD, param_vreg(TypeInt, r), param_vreg(TypeInt, r), param_imm(TypeInt, val&mask));
		val -= (val & mask);
		if (val == 0)
			break;
		first = false;
	}
}

void BackendArm64::_local_to_register_32(int offset, int r) {
	insert_cmd(Asm::InstID::LDR, param_vreg(TypeInt, r), param_local(TypeInt, offset));
}

void BackendArm64::_register_to_local_32(int r, int offset) {
	insert_cmd(Asm::InstID::STR, param_vreg(TypeInt, r), param_local(TypeInt, offset));
}

int BackendArm64::_to_register_32(const SerialNodeParam &p, int offset, int force_register) {
	//if (p.kind == NodeKind::REGISTER)
	//	return p.p;

	int reg = force_register;
	if (reg < 0)
		reg = find_unused_reg(cmd.next_cmd_index, cmd.next_cmd_index, 4);//Asm::RegID::R0;
	if (p.kind == NodeKind::CONSTANT) {
		auto cc = (Constant*)p.p;
		_immediate_to_register_32(*((int*)((char*)cc->p() + offset)), reg);
	} else if (p.kind == NodeKind::CONSTANT_BY_ADDRESS) {
		_immediate_to_register_32(*((int*)((char*)p.p + offset)), reg);
	} else if (p.kind == NodeKind::IMMEDIATE) {
		if (offset != 0)
			do_error("immediate + offset");
		_immediate_to_register_32(p.p, reg);
	} else if (p.kind == NodeKind::VAR_LOCAL) {
		auto var2 = (Variable*)p.p;
		_local_to_register_32(var2->_offset + offset, reg);
	} else if (p.kind == NodeKind::DEREF_LOCAL_MEMORY) {
		int reg2 = find_unused_reg(cmd.next_cmd_index, cmd.next_cmd_index, 4, VREG_ROOT(reg));
		_local_to_register_32(p.p + offset, reg2);
		insert_cmd(Asm::InstID::LDR, param_vreg(TypeInt, reg), param_deref_vreg(TypeInt, reg2));
	} else if (p.kind == NodeKind::LOCAL_MEMORY) {
		_local_to_register_32(p.p + offset, reg);
	} else if (p.kind == NodeKind::GLOBAL_LOOKUP) {
		// TODO really use global ref
		if (offset != 0)
			do_error("global lookup + offset");
		_immediate_to_register_32((int)(int_p)global_refs[p.p].p, reg);
	/*} else if (p.kind == NodeKind::VAR_GLOBAL) {
		auto var = (Variable*)p.p;
		_global_to_register_32((int_p)var->memory + offset, reg);*/
	} else {
		do_error("evil read source..." + kind2str(p.kind));
	}
	return reg;
}

void BackendArm64::_from_register_32(int reg, const SerialNodeParam &p, int offset) {
	if (p.kind == NodeKind::VAR_LOCAL) {
		auto var = (Variable*)p.p;
		_register_to_local_32(reg, var->_offset + offset);
	} else if (p.kind == NodeKind::LOCAL_MEMORY) {
		_register_to_local_32(reg, p.p + offset);
	/*} else if (p.kind == NodeKind::VAR_GLOBAL) {
		auto var = (Variable*)p.p;
		_register_to_global_32(reg, (int_p)var->memory + offset);*/
	} else if (p.kind == NodeKind::DEREF_LOCAL_MEMORY) {
		int reg2 = find_unused_reg(cmd.next_cmd_index, cmd.next_cmd_index, 4, VREG_ROOT(reg));
		// *mem = reg
		if (offset != 0)
			do_error("deref local + shift...");

		// reg2 = mem
		_local_to_register_32(p.p, reg2);
		// [reg2] = reg
		insert_cmd(Asm::InstID::STR, param_vreg(TypeInt, reg), param_deref_vreg(TypeInt, reg2));
	} else {
		do_error("evil write target..." + kind2str(p.kind));
	}
}


int BackendArm64::_to_register_8(const SerialNodeParam &p, int offset, int force_register) {
	//if (p.kind == NodeKind::REGISTER)
	//	return p.p;

	int reg = force_register;
	if (reg < 0)
		reg = find_unused_reg(cmd.next_cmd_index, cmd.next_cmd_index, 4);//Asm::RegID::R0;
	do_error("to reg 8");
#if 0
	if (p.kind == NodeKind::CONSTANT) {
		auto cc = (Constant*)p.p;
		_immediate_to_register_8(*((char*)cc->p() + offset), reg);
	} else if (p.kind == NodeKind::CONSTANT_BY_ADDRESS) {
		_immediate_to_register_8(*((char*)p.p + offset), reg);
	} else if (p.kind == NodeKind::IMMEDIATE) {
		if (offset != 0)
			do_error("immediate + offset");
		_immediate_to_register_8(p.p, reg);
	} else if (p.kind == NodeKind::VAR_LOCAL) {
		auto var2 = (Variable*)p.p;
		_local_to_register_8(var2->_offset + offset, reg);
	} else if (p.kind == NodeKind::DEREF_LOCAL_MEMORY) {
		int reg2 = find_unused_reg(cmd.next_cmd_index, cmd.next_cmd_index, 4, VREG_ROOT(reg));
		_local_to_register_32(p.p + offset, reg2);
		insert_cmd(Asm::InstID::LDR, param_vreg(TypeInt8, reg), param_deref_vreg(TypeInt8, reg2));
	} else if (p.kind == NodeKind::LOCAL_MEMORY) {
		_local_to_register_8(p.p + offset, reg);
	/*} else if (p.kind == NodeKind::GLOBAL_LOOKUP) {
		do_error("global loo")
		// TODO really use global ref
		if (offset != 0)
			do_error("global lookup + offset");
		_immediate_to_register_32((int)(int_p)global_refs[p.p].p, reg);*/
	} else if (p.kind == NodeKind::VAR_GLOBAL) {
		auto var = (Variable*)p.p;
		_global_to_register_8((int_p)var->memory + offset, reg);
	} else {
		do_error("evil read source..." + kind2str(p.kind));
	}
#endif
	return reg;
}

void BackendArm64::_from_register_8(int reg, const SerialNodeParam &p, int offset) {
	/*if (p.kind == NodeKind::VAR_LOCAL) {
		auto var = (Variable*)p.p;
		_register_to_local_8(reg, var->_offset + offset);
	} else if (p.kind == NodeKind::LOCAL_MEMORY) {
		_register_to_local_8(reg, p.p + offset);
	} else if (p.kind == NodeKind::VAR_GLOBAL) {
		auto var = (Variable*)p.p;
		_register_to_global_8(reg, (int_p)var->memory + offset);
	} else if (p.kind == NodeKind::DEREF_LOCAL_MEMORY) {
		int reg2 = find_unused_reg(cmd.next_cmd_index, cmd.next_cmd_index, 4, VREG_ROOT(reg));
		// *mem = reg
		if (offset != 0)
			do_error("deref local + shift...");

		// reg2 = mem
		_local_to_register_32(p.p, reg2);
		// [reg2] = reg
		insert_cmd(Asm::InstID::STRB, param_vreg(TypeInt8, reg), param_deref_vreg(TypeInt8, reg2));
	} else {
		do_error("evil write target..." + kind2str(p.kind));
	}*/
	do_error("from reg 8");
}


int BackendArm64::_to_register_float(const SerialNodeParam &p, int offset, int force_register) {
	if (force_register < 0)
		do_error("explicit register needed for float");
	int sreg = force_register;//cmd.add_virtual_reg(Asm::RegID::S1);
	int reg = _to_register_32(p, offset);
	insert_cmd(Asm::InstID::FMSR, param_vreg(TypeFloat32, sreg), param_vreg(TypeFloat32, reg));
	return sreg;
}

void BackendArm64::_from_register_float(int sreg, const SerialNodeParam &p, int offset) {
	if (p.kind == NodeKind::VAR_LOCAL) {
		auto var = (Variable*)p.p;
		insert_cmd(Asm::InstID::FSTS, param_local(TypeFloat32, var->_offset + offset), param_vreg(TypeFloat32, sreg));
	} else if (p.kind == NodeKind::LOCAL_MEMORY) {
		insert_cmd(Asm::InstID::FSTS, param_local(TypeFloat32, p.p + offset), param_vreg(TypeFloat32, sreg));
	} else {
		int reg = find_unused_reg(cmd.next_cmd_index, cmd.next_cmd_index, 4);
		insert_cmd(Asm::InstID::FMRS, param_vreg(TypeFloat32, reg), param_vreg(TypeFloat32, sreg));
		_from_register_32(reg, p, offset);
	}
}

void BackendArm64::correct_implement_commands() {

	Array<SerialNodeParam> func_params;

	for (int _i=0; _i<pre_cmd.cmd.num; _i++) {
		auto &c = pre_cmd.cmd[_i];
		cmd.next_cmd_index = cmd.cmd.num;

		//msg_write("CORRECT  " + c.str(serializer));
		if (c.inst == Asm::InstID::LABEL or c.inst == Asm::InstID::ASM)
			insert_cmd(c.inst, c.p[0], c.p[1], c.p[2]); // cmd.cmd.add(c);
		else if (c.inst == Asm::InstID::MOV) {
			int size = c.p[0].type->size;
			auto p0 = c.p[0];
			auto p1 = c.p[1];

			for (int k=0; k<size/4; k++) {
				int reg = _to_register_32(p1, k*4);
				_from_register_32(reg, p0, k*4);
			}
			int offset = (size / 4) * 4;
			for (int k=0; k<size%4; k++) {
				int reg = _to_register_8(p1, offset + k);
				_from_register_8(reg, p0, offset + k);
			}
#if 0
		} else if (c.inst == Asm::InstID::MOVSX or c.inst == Asm::InstID::MOVZX) {
			do_error("no movsx yet");
#endif
		} else if ((c.inst == Asm::InstID::ADD) or (c.inst == Asm::InstID::SUB) or (c.inst == Asm::InstID::IMUL) /*or (c.inst == Asm::InstID::IDIV)*/ or (c.inst == Asm::InstID::AND) or (c.inst == Asm::InstID::OR)) {
			auto inst = c.inst;
			/*if (inst ==  Asm::InstID::ADD)
				inst = Asm::InstID::ADDS;
			else if (inst ==  Asm::InstID::SUB)
				inst = Asm::InstID::SUBS;
			else*/ if (inst ==  Asm::InstID::IMUL)
				inst = Asm::InstID::MUL;
//			if (inst ==  Asm::InstID::IDIV)
//				inst = Asm::InstID::DIV;
			auto p0 = c.p[0];
			auto p1 = c.p[1];
			auto p2 = c.p[2];

			int reg1 = find_unused_reg(cmd.cmd.num-1, cmd.cmd.num-1, 4);
			int reg2 = find_unused_reg(cmd.cmd.num-1, cmd.cmd.num-1, 4, VREG_ROOT(reg1));

			if (p2.kind == NodeKind::NONE) {
				// a += b
				_to_register_32(p0, 0, reg1);
				_to_register_32(p1, 0, reg2);
			} else {
				// a = b + c
				_to_register_32(p1, 0, reg1);
				//cmd.set_virtual_reg(reg1, i, cmd.next_cmd_index);
				_to_register_32(p2, 0, reg2);
			}
			insert_cmd(inst, param_vreg(TypeInt, reg1), param_vreg(TypeInt, reg1), param_vreg(TypeInt, reg2));
			_from_register_32(reg1, p0, 0);

#if 0
		} else if ((c.inst == Asm::InstID::FADD) or (c.inst == Asm::InstID::FSUB) or (c.inst == Asm::InstID::FMUL) or (c.inst == Asm::InstID::FDIV)) {//or (c.inst == Asm::InstID::SUB) or (c.inst == Asm::InstID::IMUL) /*or (c.inst == Asm::InstID::IDIV)*/ or (c.inst == Asm::InstID::AND) or (c.inst == Asm::InstID::OR)) {
			auto inst = c.inst;
			if (inst ==  Asm::InstID::FADD)
				inst = Asm::InstID::FADDS;
			else if (inst ==  Asm::InstID::FSUB)
				inst = Asm::InstID::FSUBS;
			else if (inst ==  Asm::InstID::FMUL)
				inst = Asm::InstID::FMULS;
			else if (inst ==  Asm::InstID::FDIV)
				inst = Asm::InstID::FDIVS;
			auto p0 = c.p[0];
			auto p1 = c.p[1];
			auto p2 = c.p[2];
			cmd.remove_cmd(i);

			int sreg1 = cmd.add_virtual_reg(Asm::RegID::S0);
			int sreg2 = cmd.add_virtual_reg(Asm::RegID::S1);

			if (p2.kind == NodeKind::NONE) {
				// a += b
				_to_register_float(p0, 0, sreg1);
				_to_register_float(p1, 0, sreg2);
			} else {
				// a = b + c
				_to_register_float(p1, 0, sreg1);
				_to_register_float(p2, 0, sreg2);
			}

			insert_cmd(inst, param_vreg(TypeInt, sreg1), param_vreg(TypeInt, sreg1), param_vreg(TypeInt, sreg2));

			_from_register_float(sreg1, p0, 0);

			i = cmd.next_cmd_index - 1;


		} else if (c.inst == Asm::InstID::CMP) {
			auto p0 = c.p[0];
			auto p1 = c.p[1];
			cmd.remove_cmd(i);

			int reg1 = find_unused_reg(i, i, 4);
			int reg2 = find_unused_reg(i, i, 4, VREG_ROOT(reg1));

			if (p0.type->size == 1) {
				_to_register_8(p0, 0, reg1);
				_to_register_8(p1, 0, reg2);
			} else {
				_to_register_32(p0, 0, reg1);
				_to_register_32(p1, 0, reg2);
			}

			insert_cmd(Asm::InstID::CMP, param_vreg(p0.type, reg1), param_vreg(p1.type, reg2));
			i = cmd.next_cmd_index - 1;
		} else if ((c.inst == Asm::InstID::SETZ) or (c.inst == Asm::InstID::SETNZ) or (c.inst == Asm::InstID::SETNLE) or (c.inst == Asm::InstID::SETNL) or (c.inst == Asm::InstID::SETLE) or (c.inst == Asm::InstID::SETL)) {
			auto p0 = c.p[0];
			auto inst = c.inst;
			cmd.remove_cmd(i);
			int reg = cmd.add_virtual_reg(Asm::RegID::R0);
			insert_cmd(Asm::InstID::MOV, param_vreg(p0.type, reg), param_imm(TypeBool, 1), p_none);
			insert_cmd(Asm::InstID::MOV, param_vreg(p0.type, reg), param_imm(TypeBool, 0), p_none);
			if (inst == Asm::InstID::SETZ) { // ==
				cmd.cmd[cmd.next_cmd_index - 2].cond = Asm::ArmCond::EQUAL;
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::NOT_EQUAL;
			} else if (inst == Asm::InstID::SETNZ) { // !=
				cmd.cmd[cmd.next_cmd_index - 2].cond = Asm::ArmCond::NOT_EQUAL;
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::EQUAL;
			} else if (inst == Asm::InstID::SETNLE) { // >
				cmd.cmd[cmd.next_cmd_index - 2].cond = Asm::ArmCond::GREATER_THAN;
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::LESS_EQUAL;
			} else if (inst == Asm::InstID::SETNL) { // >=
				cmd.cmd[cmd.next_cmd_index - 2].cond = Asm::ArmCond::GREATER_EQUAL;
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::LESS_THAN;
			} else if (inst == Asm::InstID::SETL) { // <
				cmd.cmd[cmd.next_cmd_index - 2].cond = Asm::ArmCond::LESS_THAN;
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::GREATER_EQUAL;
			} else if (inst == Asm::InstID::SETLE) { // <=
				cmd.cmd[cmd.next_cmd_index - 2].cond = Asm::ArmCond::LESS_EQUAL;
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::GREATER_THAN;
			}
			_from_register_8(reg, p0, 0);
			i = cmd.next_cmd_index - 1;
		} else if ((c.inst == Asm::InstID::JMP) or (c.inst == Asm::InstID::JZ) or (c.inst == Asm::InstID::JNZ) or (c.inst == Asm::InstID::JNLE) or (c.inst == Asm::InstID::JNL) or (c.inst == Asm::InstID::JLE) or (c.inst == Asm::InstID::JL)) {
			auto p0 = c.p[0];
			auto inst = c.inst;
			cmd.remove_cmd(i);
			insert_cmd(Asm::InstID::B, p0);
			if (inst == Asm::InstID::JZ) { // ==
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::EQUAL;
			} else if (inst == Asm::InstID::JNZ) { // !=
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::NOT_EQUAL;
			} else if (inst == Asm::InstID::JNLE) { // >
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::GREATER_THAN;
			} else if (inst == Asm::InstID::JNL) { // >=
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::GREATER_EQUAL;
			} else if (inst == Asm::InstID::JL) { // <
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::LESS_THAN;
			} else if (inst == Asm::InstID::JLE) { // <=
				cmd.cmd[cmd.next_cmd_index - 1].cond = Asm::ArmCond::LESS_EQUAL;
			}
			i = cmd.next_cmd_index - 1;
		} else if (c.inst == Asm::InstID::LEA) {
			auto p0 = c.p[0];
			auto p1 = c.p[1];
			cmd.remove_cmd(i);

			int reg = _reference_to_register_32(p1);
			_from_register_32(reg, p0, 0);

			i = cmd.next_cmd_index - 1;
#endif
		} else if (c.inst == Asm::InstID::PUSH) {
			func_params.add(c.p[0]);
		} else if (c.inst == Asm::InstID::CALL) {
			if (c.p[1].type == TypeFunctionCodeRef) {
				//do_error("indirect call...");
				auto fp = c.p[1];
				auto ret = c.p[0];
				add_pointer_call(fp, func_params, ret);
//			} else if (is_typed_function_pointer(c.p[1].type)) {
//				do_error("BACKEND: POINTER CALL");
			} else {
				//func_params.add(c.p[0]);
				auto *f = ((Function*)c.p[1].p);
				auto ret = c.p[0];
				add_function_call(f, func_params, ret);
			}
			func_params.clear();
		} else if (c.inst == Asm::InstID::RET) {
			implement_return(c.p[0]);

#if 0
			i ++;
	//		cmd.next_cmd_target(i);
	//		insert_cmd(Asm::InstID::LDMIA, param_preg(TypePointer, Asm::RegID::R31), param_imm(TypeInt, 0xaff0)); // {r4,r5,r6,r7,r8,r9,r10,r11,r13,r15}
#endif
		} else {
			do_error("unhandled:  " + c.str(serializer));
		}
	}
}

void BackendArm64::implement_return(const SerialNodeParam &p) {
	if (p.kind != NodeKind::NONE) {
		if (cur_func->effective_return_type->_amd64_allow_pass_in_xmm()) {
/*			// if ((config.instruction_set == Asm::INSTRUCTION_SET_AMD64) or (config.compile_os)) ???
			//		cmd.add_cmd(Asm::InstID::FLD, t);
			if (cur_func->effective_return_type == TypeFloat32) {
				insert_cmd(Asm::InstID::MOVSS, p_xmm0, p);
			} else if (cur_func->effective_return_type == TypeFloat64) {
				insert_cmd(Asm::InstID::MOVSD, p_xmm0, p);
			} else if (cur_func->effective_return_type->size == 8) {
				// float[2]
				insert_cmd(Asm::InstID::MOVLPS, p_xmm0, p);
			} else if (cur_func->effective_return_type->size == 12) {
				// float[3]
				insert_cmd(Asm::InstID::MOVLPS, p_xmm0, param_shift(p, 0, TypeReg64));
				insert_cmd(Asm::InstID::MOVSS, p_xmm1, param_shift(p, 8, TypeFloat32));
			} else if (cur_func->effective_return_type->size == 16) {
				// float[4]
				insert_cmd(Asm::InstID::MOVLPS, p_xmm0, param_shift(p, 0, TypeReg64));
				insert_cmd(Asm::InstID::MOVLPS, p_xmm1, param_shift(p, 8, TypeReg64));
			} else {
				do_error("...ret xmm " + cur_func->effective_return_type->long_name());
			}*/
		} else {
			// store return directly in eax / fpu stack (4 byte)
			if (cur_func->effective_return_type->size == 1) {
	//			int v = cmd.add_virtual_reg(Asm::RegID::AL);
	//			insert_cmd(Asm::InstID::MOV, param_vreg(cur_func->effective_return_type, v), p);
			} else if (cur_func->effective_return_type->size == 8) {
	//			int v = cmd.add_virtual_reg(Asm::RegID::RAX);
	//			insert_cmd(Asm::InstID::MOV, param_vreg(cur_func->effective_return_type, v), p);
			} else {
				int reg = cmd.add_virtual_reg(Asm::RegID::W0);
				_to_register_32(p, 0, reg);
			}
		}
	}
	//if (cur_func->effective_return_type->uses_return_by_memory())
	//	insert_cmd(Asm::InstID::RET, param_imm(TypeReg16, 4));
	//else

	/*if (stack_max_size > 0) {
		insert_cmd(Asm::InstID::ADD, param_preg(TypePointer, Asm::RegID::R31), param_preg(TypePointer, Asm::RegID::R31), param_imm(TypeInt, stack_max_size + 8));
	}*/
	cmd.add_cmd(Asm::InstID::LDP, param_preg(TypePointer, Asm::RegID::R29), param_preg(TypePointer, Asm::RegID::R30), param_local(TypePointer, stack_max_size));
	insert_cmd(Asm::InstID::ADD, param_preg(TypePointer, Asm::RegID::R31), param_preg(TypePointer, Asm::RegID::R31), param_imm(TypeInt, stack_max_size + 16));

	insert_cmd(Asm::InstID::RET);
}

static bool arm_type_uses_int_register(const Class *t) {
	return (t == TypeInt) /*or (t == TypeInt64)*/ or (t == TypeInt8) or (t == TypeBool) or t->is_enum() or t->is_some_pointer();
}

int BackendArm64::fc_begin(const Array<SerialNodeParam> &_params, const SerialNodeParam &ret, bool is_static) {
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

	int max_reg_params = 8;
	int reg_param_offset = 0;
	if (type->uses_return_by_memory()) {
		max_reg_params --;
		reg_param_offset = 1;
	}


	// map params...
	Array<SerialNodeParam> reg_param;
	Array<SerialNodeParam> stack_param;
	Array<SerialNodeParam> float_param;
	for (SerialNodeParam &p: params) {
		if ((p.type == TypeInt) or (p.type == TypeInt64) or (p.type == TypeInt8) or (p.type == TypeBool) or p.type->is_some_pointer()) {
			if (reg_param.num < max_reg_params) {
				reg_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else if (p.type == TypeFloat32 /*or (p.type == TypeFloat64)*/) {
			if (float_param.num < 8) {
				float_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else
			do_error("parameter type currently not supported: " + p.type->name);
	}

	// push parameters onto stack
/*	push_size = 4 * stack_param.num;
	if (push_size > 127)
		insert_cmd(Asm::inst_add, param_reg(TypePointer, Asm::RegID::RSP), param_const(TypeInt, (void*)push_size));
	else if (push_size > 0)
		insert_cmd(Asm::inst_add, param_reg(TypePointer, Asm::RegID::RSP), param_const(TypeInt8, (void*)push_size));
	foreachb(SerialCommandParam &p, stack_param)
		insert_cmd(Asm::inst_push, p);
	max_push_size = max(max_push_size, push_size);*/

	// s0-7
	foreachib(auto &p, float_param, i) {
		auto reg = Asm::s_reg(i);
		/*if (p.type == TypeFloat64)
			insert_cmd(Asm::inst_movsd, param_reg(TypeReg128, reg), p);
		else*/
		_to_register_float(p, 0, cmd.add_virtual_reg(reg));
			//insert_cmd(Asm::InstID::FLDS, param_preg(TypeFloat32, reg), p);
	}

	// return as _very_ first parameter
	if (type->uses_return_by_memory()) {
		int reg = _reference_to_register_32(ret);
		cmd.set_virtual_reg(reg, cmd.next_cmd_index - 1, -100); // -> call
	}

	// r0, r1, r2, r3
	foreachib(auto &p, reg_param, i) {
		int v = cmd.add_virtual_reg(Asm::w_reg(i + reg_param_offset));
		_to_register_32(p, 0, v);
		//insert_cmd(Asm::InstID::MOV, param_vreg(p.type, v), p);
		cmd.set_virtual_reg(v, cmd.next_cmd_index - 1, -100); // -> call
	}

	// extend reg channels to call
	for (VirtualRegister &r: cmd.virtual_reg)
		if (r.last == -100)
			r.last = cmd.next_cmd_index;

	return push_size;
}

void BackendArm64::fc_end(int push_size, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	const Class *type = ret.type;
	if (!type)
		return;

	// return > 4b already got copied to [ret] by the function!
	if ((type != TypeVoid) and (!type->uses_return_by_memory())) {
		if (type == TypeFloat32) {
			int sreg = cmd.add_virtual_reg(Asm::RegID::S0);
			_from_register_float(sreg, ret, 0);
		//else if (type == TypeFloat64)
			//insert_cmd(Asm::InstID::MOVSD, ret, param_preg(TypeReg128, Asm::RegID::XMM0));
		} else if ((type->size == 1) or (type->size == 4)) {
			int v = cmd.add_virtual_reg(Asm::RegID::W0);
			_from_register_32(v, ret, 0);
			cmd.set_virtual_reg(v, cmd.next_cmd_index - 2, cmd.next_cmd_index - 1);
		} else {
			do_error("unhandled function value receiving... " + type->long_name());
			int v = cmd.add_virtual_reg(Asm::RegID::R0);
			insert_cmd(Asm::InstID::MOV, ret, param_vreg(TypeReg32, v));
			cmd.set_virtual_reg(v, cmd.next_cmd_index - 2, cmd.next_cmd_index - 1);
		}
	}
}

static bool reachable_arm(int64 a, void *b) {
	return (abs((int_p)a - (int_p)b) < 0x10000000);
}

void BackendArm64::add_function_call(Function *f, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	serializer->call_used = true;
	int push_size = fc_begin(params, ret, f->is_static());

	if ((f->owner() == module->tree) and !f->is_extern()) {
		insert_cmd(Asm::InstID::BL, param_label(TypePointer, f->_label));
	} else {
		if (f->address == 0)
			module->do_error_link("could not link function " + f->long_name());
		if (reachable_arm(f->address, this->module->opcode)) {
			insert_cmd(Asm::InstID::BL, param_imm(TypePointer, f->address)); // the actual call
			// function pointer will be shifted later...
		} else {

			// TODO FIXME
			// really find a usable register...

			int v = cmd.add_virtual_reg(Asm::RegID::R4);//find_unused_reg(cmd.next_cmd_index-1, cmd.next_cmd_index-1, 4);
			_to_register_32(param_lookup(TypePointer, add_global_ref((void*)(int_p)f->address)), 0, v);
			//insert_cmd(Asm::InstID::MOV, param_vreg(TypePointer, v), param_lookup(TypePointer, add_global_ref(f->address)));
			insert_cmd(Asm::InstID::BL, param_vreg(TypePointer, v));
			cmd.set_virtual_reg(v, cmd.next_cmd_index-2, cmd.next_cmd_index-1);
		}
	}

	fc_end(push_size, params, ret);
}

void BackendArm64::add_function_intro_frame(int stack_alloc_size) {
	cmd.next_cmd_target(0);
	//cmd.add_cmd(Asm::InstID::STMDB, param_preg(TypePointer, Asm::RegID::R13), param_imm(TypeInt, 0x6ff0)); // {r4,r5,r6,r7,r8,r9,r10,r11,r13,r14}
	/*if (stack_max_size > 0) {
		cmd.next_cmd_target(1);
		cmd.add_cmd(Asm::InstID::SUB, param_preg(TypePointer, Asm::RegID::R31), param_preg(TypePointer, Asm::RegID::R31), param_imm(TypeInt, stack_max_size + 8));
	}*/
	cmd.add_cmd(Asm::InstID::SUB, param_preg(TypePointer, Asm::RegID::R31), param_preg(TypePointer, Asm::RegID::R31), param_imm(TypeInt, stack_max_size + 16));
	cmd.next_cmd_target(1);
	cmd.add_cmd(Asm::InstID::STP, param_preg(TypePointer, Asm::RegID::R29), param_preg(TypePointer, Asm::RegID::R30), param_local(TypePointer, stack_max_size));
	cmd.next_cmd_target(2);
	cmd.add_cmd(Asm::InstID::ADD, param_preg(TypePointer, Asm::RegID::R29), param_preg(TypePointer, Asm::RegID::R31), param_imm(TypeInt, stack_max_size));
}

void BackendArm64::assemble() {
	// intro + allocate stack memory

	/*foreachi(GlobalRef &g, global_refs, i) {
		g.label = list->create_label(format("_kaba_ref_%d_%d", cur_func_index, i));
		list->insert_location_label(g.label);
		list->add2(Asm::InstID::DD, Asm::param_imm((int_p)g.p, 4));
	}*/

	list->insert_location_label(cur_func->_label);

	if (!flags_has(cur_func->flags, Flags::NOFRAME))
		add_function_intro_frame(stack_max_size);

	//	do_error("new ARM assemble() not yet implemented");
	for (int i=0;i<cmd.cmd.num;i++) {
		if (cmd.cmd[i].inst == Asm::InstID::LABEL) {
			list->insert_location_label(cmd.cmd[i].p[0].p);
		} else if (cmd.cmd[i].inst == Asm::InstID::ASM) {
			add_asm_block(cmd.cmd[i].p[0].p);
		} else {
			assemble_cmd_arm(cmd.cmd[i]);
		}
	}

	//list->add2(Asm::InstID::ALIGN_OPCODE);
}

} // kaba