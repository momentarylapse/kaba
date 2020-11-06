/*
 * BackendAmd64.cpp
 *
 *  Created on: Nov 4, 2020
 *      Author: michi
 */

#include "BackendAmd64.h"
#include "serializer.h"
#include "../../file/msg.h"

namespace kaba {

const SerialNodeParam BackendAmd64::p_none = {NodeKind::NONE, -1, 0, nullptr, 0};



static int get_reg(int root, int size) {
#if 1
	if ((size != 1) and (size != 4) and (size != 8)) {
		msg_write(msg_get_trace());
		throw Asm::Exception("get_reg: bad reg size: " + i2s(size), "...", 0, 0);
	}
#endif
	return Asm::RegResize[root][size];
}

BackendAmd64::BackendAmd64(Serializer *s) : cmd(s->cmd) {
	serializer = s;
	script = s->script;
	list = s->list;

	p_eax = param_preg(TypeReg32, Asm::REG_EAX);
	p_eax_int = param_preg(TypeInt, Asm::REG_EAX);
	p_rax = param_preg(TypeReg64, Asm::REG_RAX);

	p_deref_eax = param_deref_preg(TypePointer, Asm::REG_EAX);

	p_ax = param_preg(TypeReg16, Asm::REG_AX);
	p_al = param_preg(TypeReg8, Asm::REG_AL);
	p_al_bool = param_preg(TypeBool, Asm::REG_AL);
	p_al_char = param_preg(TypeChar, Asm::REG_AL);
	p_st0 = param_preg(TypeFloat32, Asm::REG_ST0);
	p_st1 = param_preg(TypeFloat32, Asm::REG_ST1);
	p_xmm0 = param_preg(TypeReg128, Asm::REG_XMM0);
	p_xmm1 = param_preg(TypeReg128, Asm::REG_XMM1);
}

BackendAmd64::~BackendAmd64() {
}

void BackendAmd64::correct() {
	next_cmd_target(0);
	add_function_intro_params(serializer->cur_func);


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
		} else if (c.inst == Asm::INST_MOVSX) {
			msg_write("movsx");
			auto p1 = c.p[0];
			auto p2 = c.p[1];
			int reg = serializer->find_unused_reg(i, i, p2.type->size);
			msg_write(reg);
			msg_write(serializer->virtual_reg[reg].reg);
			serializer->remove_cmd(i);
			next_cmd_target(i);
			insert_cmd(Asm::INST_MOV, param_vreg(p1.type, reg), p2);
			int preg_x = serializer->reg_resize(serializer->virtual_reg[reg].reg, p1.type->size);
			insert_cmd(Asm::INST_MOV, p1, param_vreg(p1.type, reg, preg_x));
			i ++;

		} else if ((c.inst == Asm::INST_IMUL) or (c.inst == Asm::INST_IDIV) or (c.inst == Asm::INST_ADD) or (c.inst == Asm::INST_SUB)) {
			auto inst = c.inst;
			auto r = c.p[0];
			auto p1 = c.p[1];
			auto p2 = c.p[2];
			auto type = p1.type;
			int reg = serializer->find_unused_reg(i, i, type->size);

			auto t = param_vreg(type, reg);
			serializer->remove_cmd(i);
			next_cmd_target(i);
			insert_cmd(Asm::INST_MOV, t, p1);
			insert_cmd(inst, t, p2);
			insert_cmd(Asm::INST_MOV, r, t);
			set_virtual_reg(reg, i, i + 2);

			i += 2;
		} else if (c.inst == Asm::INST_PUSH) {
			func_params.add(c.p[0]);
			serializer->remove_cmd(i);
			i --;
		} else if (c.inst == Asm::INST_CALL) {
			//func_params.add(c.p[0]);
			auto *f = ((Function*)c.p[1].p);
			auto ret = c.p[0];
			serializer->remove_cmd(i);
			next_cmd_target(i);
			add_function_call(f, func_params, ret);
			func_params.clear();
			i = serializer->next_cmd_index - 1;
		} else if (c.inst == Asm::INST_RET) {
			auto p = c.p[0];
			serializer->remove_cmd(i);
			next_cmd_target(i);

			insert_cmd(Asm::INST_LEAVE);
			if (serializer->cur_func->effective_return_type->uses_return_by_memory())
				insert_cmd(Asm::INST_RET, param_imm(TypeReg16, 4));
			else
				insert_cmd(Asm::INST_RET);
			i ++;
		}
	}
	serializer->cmd_list_out("x:b", "post paramtrafo");
}


void BackendAmd64::fc_end(int push_size, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	const Class *type = ret.get_type_save();

	// return > 4b already got copied to [ret] by the function!
	if ((type != TypeVoid) and (!type->uses_return_by_memory())) {
		if (type->_amd64_allow_pass_in_xmm()) {
			if (type == TypeFloat32) {
				insert_cmd(Asm::INST_MOVSS, ret, p_xmm0);
			} else if (type == TypeFloat64) {
				insert_cmd(Asm::INST_MOVSD, ret, p_xmm0);
			} else if (type->size == 8) { // float[2]
				insert_cmd(Asm::INST_MOVLPS, ret, p_xmm0);
			} else if (type->size == 12) { // float[3]
				insert_cmd(Asm::INST_MOVLPS, param_shift(ret, 0, TypeReg64), p_xmm0);
				insert_cmd(Asm::INST_MOVSS, param_shift(ret, 8, TypeFloat32), p_xmm1);
			} else if (type->size == 16) { // float[4]
				// hmm, weird
				insert_cmd(Asm::INST_MOVLPS, param_shift(ret, 0, TypeReg64), p_xmm0);
				//add_cmd(Asm::INST_MOVHPS, param_shift(ret, 8, TypeReg64), p_xmm0);
				insert_cmd(Asm::INST_MOVLPS, param_shift(ret, 8, TypeReg64), p_xmm1);
				//add_cmd(Asm::INST_MOVUPS, ret, p_xmm0);
			} else {
				serializer->do_error("xmm return ..." + type->long_name());
			}
		} else if (type->size == 1) {
			int r = add_virtual_reg(Asm::REG_AL);
			insert_cmd(Asm::INST_MOV, ret, param_vreg(type, r));
			set_virtual_reg(r, serializer->next_cmd_index - 2, serializer->next_cmd_index - 1);
		} else if (type->size == 4) {
			int r = add_virtual_reg(Asm::REG_EAX);
			insert_cmd(Asm::INST_MOV, ret, param_vreg(type, r));
			set_virtual_reg(r, serializer->next_cmd_index - 2, serializer->next_cmd_index - 1);
		} else {
			int r = add_virtual_reg(Asm::REG_RAX);
			insert_cmd(Asm::INST_MOV, ret, param_vreg(type, r));
			set_virtual_reg(r, serializer->next_cmd_index - 2, serializer->next_cmd_index - 1);
		}
	}
}

static bool dist_fits_32bit(void *a, void *b) {
	int_p d = (int_p)a - (int_p)b;
	if (d < 0)
		d = -d;
	return (d < 0x70000000);
}

void BackendAmd64::add_function_call(Function *f, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	serializer->call_used = true;
	int push_size = fc_begin(f, params, ret);

	if (f->address) {
		if (dist_fits_32bit(f->address, script->opcode)) {
			// 32bit call distance
			insert_cmd(Asm::INST_CALL, param_imm(TypeReg32, (int_p)f->address)); // the actual call
			// function pointer will be shifted later...(asm translates to RIP-relative)
		} else {
			// 64bit call distance
			insert_cmd(Asm::INST_MOV, p_rax, param_imm(TypeReg64, (int_p)f->address));
			insert_cmd(Asm::INST_CALL, p_rax);
		}
	} else if (f->_label >= 0) {
		if (f->owner() == script->syntax) {
			// 32bit call distance
			insert_cmd(Asm::INST_CALL, param_marker(TypeInt, f->_label));
		} else {
			// 64bit call distance
			insert_cmd(Asm::INST_MOV, p_rax, param_marker(TypePointer, f->_label));
			insert_cmd(Asm::INST_CALL, p_rax);
		}
	} else {
		serializer->do_error_link("could not link function " + f->signature());
	}

	fc_end(push_size, params, ret);
}

int BackendAmd64::fc_begin(Function *__f, const Array<SerialNodeParam> &_params, const SerialNodeParam &ret) {
	const Class *type = ret.get_type_save();


	// grow stack (down) for local variables of the calling function
//	add_cmd(- cur_func->_VarSize - LocalOffset - 8);
	int64 push_size = 0;

	auto params = _params;

	// instance as first parameter
/*	if (instance.type)
		params.insert(instance, 0);*/

	// return as _very_ first parameter
	if (type->uses_return_by_memory()) {
		params.insert(insert_reference(ret), 0);
	}

	// map params...
	Array<SerialNodeParam> reg_param;
	Array<SerialNodeParam> stack_param;
	Array<SerialNodeParam> xmm_param;
	for (auto &p: params) {
		if ((p.type == TypeInt) or (p.type == TypeInt64) or (p.type == TypeChar) or (p.type == TypeBool) or p.type->is_some_pointer()) {
			if (reg_param.num < 6) {
				reg_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else if ((p.type == TypeFloat32) or (p.type == TypeFloat64)) {
			if (xmm_param.num < 8) {
				xmm_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else {
			serializer->do_error("parameter type currently not supported: " + p.type->name);
		}
	}

	// push parameters onto stack
	push_size = 8 * stack_param.num;
	if (push_size > 127)
		insert_cmd(Asm::INST_ADD, param_preg(TypePointer, Asm::REG_RSP), param_imm(TypeInt, push_size));
	else if (push_size > 0)
		insert_cmd(Asm::INST_ADD, param_preg(TypePointer, Asm::REG_RSP), param_imm(TypeChar, push_size));
	foreachb(SerialNodeParam &p, stack_param) {
		insert_cmd(Asm::INST_MOV, param_preg(p.type, get_reg(0, p.type->size)), p);
		insert_cmd(Asm::INST_PUSH, p_rax);
	}
	serializer->max_push_size = max(serializer->max_push_size, (int)push_size);

	// xmm0-7
	foreachib(auto &p, xmm_param, i) {
		int reg = Asm::REG_XMM0 + i;
		if (p.type == TypeFloat64)
			insert_cmd(Asm::INST_MOVSD, param_preg(TypeReg128, reg), p);
		else
			insert_cmd(Asm::INST_MOVSS, param_preg(TypeReg128, reg), p);
	}

	Array<int> virts;

	// rdi, rsi, rdx, rcx, r8, r9
	int param_regs_root[6] = {7, 6, 2, 1, 8, 9};
	foreachib(auto &p, reg_param, i) {
		int root = param_regs_root[i];
		int preg = get_reg(root, p.type->size);
		if (preg >= 0) {
			int v = add_virtual_reg(preg);
			virts.add(v);
			insert_cmd(Asm::INST_MOV, param_vreg(p.type, v), p);
		} else {
			// some registers are not 8bit'able
			int v = add_virtual_reg(get_reg(root, 4));
			virts.add(v);
			int va = add_virtual_reg(Asm::REG_EAX);
			insert_cmd(Asm::INST_MOV, param_vreg(p.type, va, Asm::REG_AL), p);
			insert_cmd(Asm::INST_MOV, param_vreg(TypeReg32, v), param_vreg(p.type, va));
		}
	}

	// extend reg channels to call
	for (int v: virts)
		use_virtual_reg(v, serializer->next_cmd_index-1, serializer->next_cmd_index);

	return push_size;
}

int BackendAmd64::add_virtual_reg(int preg) {
	return serializer->add_virtual_reg(preg);
}

void BackendAmd64::set_virtual_reg(int v, int first, int last) {
	serializer->set_virtual_reg(v, first, last);
}

void BackendAmd64::use_virtual_reg(int v, int first, int last) {
	serializer->use_virtual_reg(v, first, last);
}

SerialNodeParam BackendAmd64::param_vreg(const Class *type, int vreg, int preg) {
	return serializer->param_vreg(type, vreg, preg);
	/*if (preg < 0)
		preg = virtual_reg[vreg].reg;
	return {NodeKind::REGISTER, preg, vreg, type, 0};*/
}

SerialNodeParam BackendAmd64::param_deref_vreg(const Class *type, int vreg, int preg) {
	return serializer->param_deref_vreg(type, vreg, preg);
	/*if (preg < 0)
		preg = virtual_reg[vreg].reg;
	return {NodeKind::DEREF_REGISTER, preg, vreg, type, 0};*/
}


void BackendAmd64::next_cmd_target(int index) {
	serializer->next_cmd_target(index);
}

void BackendAmd64::insert_cmd(int inst, const SerialNodeParam &p1, const SerialNodeParam &p2, const SerialNodeParam &p3) {
	int i = serializer->next_cmd_index;
	serializer->add_cmd(inst, p1, p2, p3);
	serializer->next_cmd_target(i + 1);
}

void BackendAmd64::remove_cmd(int index) {
	serializer->remove_cmd(index);
}

SerialNodeParam BackendAmd64::insert_reference(const SerialNodeParam &param, const Class *type) {
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
		ret = serializer->add_temp(type);
		insert_cmd(Asm::INST_LEA, ret, param);
	}
	return ret;
}


bool BackendAmd64::is_reg_root_used_in_interval(int reg_root, int first, int last) {
	return serializer->is_reg_root_used_in_interval(reg_root, first, last);
}

int BackendAmd64::find_unused_reg(int first, int last, int size, int exclude) {
	//vr_list_out();
	for (int r: map_reg_root)
		if (r != exclude)
			if (!is_reg_root_used_in_interval(r, first, last)) {
				return add_virtual_reg(get_reg(r, size));
			}
	serializer->cmd_list_out("fur", "find unused reg");
	serializer->do_error(format("no free register of size %d   in %d:%d", size, first, last));
	return -1;
}

void BackendAmd64::add_function_outro(Function *f) {
	insert_cmd(Asm::INST_LEAVE, p_none);
	if (f->effective_return_type->uses_return_by_memory())
		insert_cmd(Asm::INST_RET, param_imm(TypeReg16, 4));
	else
		insert_cmd(Asm::INST_RET, p_none);
}

void BackendAmd64::add_function_intro_params(Function *f) {
	// return, instance, params
	Array<Variable*> param;
	if (f->effective_return_type->uses_return_by_memory()) {
		for (Variable *v: weak(f->var))
			if (v->name == IDENTIFIER_RETURN_VAR) {
				param.add(v);
				break;
			}
	}
	if (!f->is_static()) {
		for (Variable *v: weak(f->var))
			if (v->name == IDENTIFIER_SELF) {
				param.add(v);
				break;
			}
	}
	for (int i=0;i<f->num_params;i++)
		param.add(f->var[i].get());

	// map params...
	Array<Variable*> reg_param;
	Array<Variable*> stack_param;
	Array<Variable*> xmm_param;
	for (Variable *p: param) {
		if ((p->type == TypeInt) or (p->type == TypeChar) or (p->type == TypeBool) or p->type->is_some_pointer()) {
			if (reg_param.num < 6) {
				reg_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else if ((p->type == TypeFloat32) or (p->type == TypeFloat64)) {
			if (xmm_param.num < 8) {
				xmm_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else {
			serializer->do_error("parameter type currently not supported: " + p->type->name);
		}
	}

	// xmm0-7
	foreachib(Variable *p, xmm_param, i){
		int reg = Asm::REG_XMM0 + i;
		if (p->type == TypeFloat64)
			insert_cmd(Asm::INST_MOVSD, param_local(p->type, p->_offset), param_preg(TypeReg128, reg));
		else
			insert_cmd(Asm::INST_MOVSS, param_local(p->type, p->_offset), param_preg(TypeReg128, reg));


	}

	// rdi, rsi,rdx, rcx, r8, r9
	int param_regs_root[6] = {7, 6, 2, 1, 8, 9};
	foreachib(Variable *p, reg_param, i) {
		int root = param_regs_root[i];
		int preg = get_reg(root, p->type->size);
		if (preg >= 0) {
			int v = add_virtual_reg(preg);
			insert_cmd(Asm::INST_MOV, param_local(p->type, p->_offset), param_vreg(p->type, v));
			set_virtual_reg(v, 0, serializer->next_cmd_index-1);
		} else {
			// some registers are not 8bit'able
			int v = add_virtual_reg(get_reg(root, 4));
			int va = add_virtual_reg(Asm::REG_EAX);
			insert_cmd(Asm::INST_MOV, param_vreg(TypeReg32, va), param_vreg(TypeReg32, v));
			insert_cmd(Asm::INST_MOV, param_local(p->type, p->_offset), param_vreg(p->type, va, get_reg(0, p->type->size)));
			set_virtual_reg(v, 0, serializer->next_cmd_index-2);
			set_virtual_reg(va, serializer->next_cmd_index-2, serializer->next_cmd_index-1);
		}
	}

	// get parameters from stack
	foreachb(Variable *p, stack_param) {
		serializer->do_error("func with stack...");
		/*int s = 8;
		add_cmd(Asm::inst_push, p);
		push_size += s;*/
	}
}

}
