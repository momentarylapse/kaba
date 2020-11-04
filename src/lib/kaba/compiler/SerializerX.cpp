/*
 * SerializerX.cpp
 *
 *  Created on: Nov 4, 2020
 *      Author: michi
 */

#include "SerializerX.h"
#include "../kaba.h"
#include "../../file/msg.h"

namespace kaba {

SerializerX::SerializerX(Script *s, Asm::InstructionWithParamsList *l) : Serializer(s, l) {
	list->clear();
}

SerializerX::~SerializerX() {
	list->show();
	for (int i=0;i<cmd.num;i++)
		msg_write(format("%3d: ", i) + cmd[i].str(this));
}

void SerializerX::add_function_call(Function *f, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	call_used = true;
	int push_size = fc_begin(f, params, ret);

	if (f->address) {
		add_cmd(Asm::INST_CALL, param_imm(TypePointer, (int_p)f->address)); // the actual call
		// function pointer will be shifted later...
	} else if (f->_label >= 0) {
		add_cmd(Asm::INST_CALL, param_marker(TypePointer, f->_label));
	} else {
		do_error_link("could not link function " + f->signature());
	}

	fc_end(push_size, ret);
}

void SerializerX::add_virtual_function_call(Function *f, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
}

int SerializerX::fc_begin(Function *f, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	for (SerialNodeParam &p: params)
		add_cmd(Asm::INST_PUSH, p);
	return 0;
}

void SerializerX::fc_end(int push_size, const SerialNodeParam &ret) {
}

void SerializerX::add_pointer_call(const SerialNodeParam &pointer, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
}

void SerializerX::add_function_intro_params(Function *f) {
}

void SerializerX::add_function_intro_frame(int stack_alloc_size) {
}

void SerializerX::add_function_outro(Function *f) {
}

SerialNodeParam SerializerX::serialize_parameter(Node *link, Block *block, int index) {
	msg_write("p");
	SerialNodeParam p;
	p.kind = link->kind;
	p.type = link->type;
	p.p = 0;
	p.shift = 0;

	if (link->kind == NodeKind::MEMORY){
		p.p = link->link_no;
	}else if (link->kind == NodeKind::ADDRESS){
		p.p = link->link_no;
		//p.p = (int_p)&link->link_no;
		//p.kind = NodeKind::CONSTANT_BY_ADDRESS;
	}else if (link->kind == NodeKind::VAR_GLOBAL){
		p.p = link->link_no;
		/*p.p = (int_p)link->as_global_p();
		if (!p.p)
			script->do_error_link("variable is not linkable: " + link->as_global()->name);
		p.kind = NodeKind::MEMORY;*/
	}else if (link->kind == NodeKind::VAR_LOCAL){
		p.p = link->link_no;
		//p.p = link->as_local()->_offset;
		//p.kind = NodeKind::LOCAL_MEMORY;
	}else if (link->kind == NodeKind::LOCAL_MEMORY){
		p.p = link->link_no;
	}else if (link->kind == NodeKind::LOCAL_ADDRESS){
		SerialNodeParam param = param_local(TypePointer, link->link_no);
		return add_reference(param, link->type);
	}else if (link->kind == NodeKind::CONSTANT){
		p.p = link->link_no;
		/*p.p = (int_p)link->as_const()->address; // FIXME ....need a cleaner approach for compiling os...
		if (config.compile_os)
			p.kind = NodeKind::MEMORY;
		else
			p.kind = NodeKind::CONSTANT_BY_ADDRESS;
		if (syntax_tree->flag_function_pointer_as_code and (link->type == TypeFunctionP)) {
			auto *fp = (Function*)(int_p)link->as_const()->as_int64();
			p.kind = NodeKind::MARKER;
			p.p = fp->_label;
		}*/
	}else if ((link->kind == NodeKind::OPERATOR) or (link->kind == NodeKind::FUNCTION_CALL) or (link->kind == NodeKind::INLINE_CALL) or (link->kind == NodeKind::VIRTUAL_CALL) or (link->kind == NodeKind::POINTER_CALL) or (link->kind == NodeKind::STATEMENT)){
		p = serialize_node(link, block, index);
	}else if (link->kind == NodeKind::REFERENCE){
		auto param = serialize_parameter(link->params[0].get(), block, index);
		//printf("%d  -  %s\n",pk,Kind2Str(pk));
		return add_reference(param, link->type);
	}else if (link->kind == NodeKind::DEREFERENCE){
		auto param = serialize_parameter(link->params[0].get(), block, index);
		/*if ((param.kind == KindVarLocal) or (param.kind == KindVarGlobal)){
			p.type = param.type->sub_type;
			if (param.kind == KindVarLocal)		p.kind = KindRefToLocal;
			if (param.kind == KindVarGlobal)	p.kind = KindRefToGlobal;
			p.p = param.p;
		}*/
		return add_dereference(param, link->type);
	}else if (link->kind == NodeKind::VAR_TEMP){
		// only used by <new> operator
		p.p = link->link_no;
	}else{
		do_error("unexpected type of parameter: " + kind2str(link->kind));
	}
	return p;
}

void SerializerX::serialize_statement(Node *com, const SerialNodeParam &ret, Block *block, int index) {
	msg_write("s");
}

void SerializerX::serialize_inline_function(Node *com, const Array<SerialNodeParam> &param, const SerialNodeParam &ret) {
	msg_write("i");
	auto index = com->as_func()->inline_no;
	switch(index){
		case InlineID::INT_TO_FLOAT:
			add_cmd(Asm::INST_CVTSI2SS, p_xmm0, param[0]);
			add_cmd(Asm::INST_MOVSS, ret, p_xmm0);
			break;
		case InlineID::FLOAT_TO_INT:{
			int veax = add_virtual_reg(Asm::REG_EAX);
			add_cmd(Asm::INST_MOVSS, p_xmm0, param[0]);
			add_cmd(Asm::INST_CVTTSS2SI, param_vreg(TypeInt, veax), p_xmm0);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt, veax));
			}break;
		case InlineID::FLOAT_TO_FLOAT64:
			add_cmd(Asm::INST_CVTSS2SD, p_xmm0, param[0]);
			add_cmd(Asm::INST_MOVSD, ret, p_xmm0);
			break;
		case InlineID::FLOAT64_TO_FLOAT:
			add_cmd(Asm::INST_CVTSD2SS, p_xmm0, param[0]);
			add_cmd(Asm::INST_MOVSS, ret, p_xmm0);
			break;
		case InlineID::INT_TO_CHAR:{
			int veax = add_virtual_reg(Asm::REG_EAX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, veax), param[0]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeChar, veax, Asm::REG_AL));
			}break;
		case InlineID::CHAR_TO_INT:{
			int veax = add_virtual_reg(Asm::REG_EAX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, veax), param_imm(TypeInt, 0x0));
			add_cmd(Asm::INST_MOV, param_vreg(TypeChar, veax, Asm::REG_AL), param[0]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt, veax));
			}break;
		case InlineID::POINTER_TO_BOOL:
			add_cmd(Asm::INST_CMP, param[0], param_imm(TypePointer, 0));
			add_cmd(Asm::INST_SETNZ, ret);
			break;
		case InlineID::RECT_SET:
		case InlineID::VECTOR_SET:
		case InlineID::COMPLEX_SET:
		case InlineID::COLOR_SET:
			for (int i=0; i<ret.type->size/4; i++)
				add_cmd(Asm::INST_MOV, param_shift(ret, i*4, TypeFloat32), param[i]);
			break;
		case InlineID::INT_ASSIGN:
		case InlineID::INT64_ASSIGN:
		case InlineID::FLOAT_ASSIGN:
		case InlineID::FLOAT64_ASSIGN:
		case InlineID::POINTER_ASSIGN:
			add_cmd(Asm::INST_MOV, param[0], param[1]);
			break;
		case InlineID::SHARED_POINTER_INIT:
			add_cmd(Asm::INST_MOV, param[0], param_imm(TypeInt, 0));
			break;
		case InlineID::CHAR_ASSIGN:
		case InlineID::BOOL_ASSIGN:
			add_cmd(Asm::INST_MOV, param[0], param[1]);
			break;
// chunk...
		case InlineID::CHUNK_ASSIGN:
			for (int i=0; i<com->params[0]->type->size/4; i++)
				add_cmd(Asm::INST_MOV, param_shift(param[0], i * 4, TypeInt), param_shift(param[1], i * 4, TypeInt));
			for (int i=4*(com->params[0]->type->size/4); i<com->params[0]->type->size; i++)
				add_cmd(Asm::INST_MOV, param_shift(param[0], i, TypeChar), param_shift(param[1], i, TypeChar));
			break;
		case InlineID::CHUNK_EQUAL:{
			int val = add_virtual_reg(Asm::REG_AL);
			add_cmd(Asm::INST_CMP, param_shift(param[0], 0, TypeInt), param_shift(param[1], 0, TypeInt));
			add_cmd(Asm::INST_SETZ, ret);
			for (int i=1; i<com->params[0]->type->size/4; i++) {
				add_cmd(Asm::INST_CMP, param_shift(param[0], i*4, TypeInt), param_shift(param[1], i*4, TypeInt));
				add_cmd(Asm::INST_SETZ, param_vreg(TypeBool, val));
				add_cmd(Asm::INST_AND, param_vreg(TypeBool, val));
			}
			}break;
// int
		case InlineID::INT_ADD_ASSIGN:
		case InlineID::INT64_ADD_ASSIGN:
			add_cmd(Asm::INST_ADD, param[0], param[1]);
			break;
		case InlineID::INT_SUBTRACT_ASSIGN:
		case InlineID::INT64_SUBTRACT_ASSIGN:
			add_cmd(Asm::INST_SUB, param[0], param[1]);
			break;
		case InlineID::INT_MULTIPLY_ASSIGN:
		case InlineID::INT64_MULTIPLY_ASSIGN:
			add_cmd(Asm::INST_IMUL, param[0], param[1]);
			break;
		case InlineID::INT_DIVIDE_ASSIGN:
		case InlineID::INT64_DIVIDE_ASSIGN:
			add_cmd(Asm::INST_IDIV, param[0], param[1]);
			break;
		case InlineID::INT_ADD:
		case InlineID::INT64_ADD:
			add_cmd(Asm::INST_ADD, ret, param[0], param[1]);
			break;
		case InlineID::INT64_ADD_INT:{
			int veax = add_virtual_reg(Asm::REG_EAX);
			int vrax = add_virtual_reg(Asm::REG_RAX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, veax), param[1]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt64, vrax));
			add_cmd(Asm::INST_ADD, ret, param[0]);
			}break;
		case InlineID::INT_SUBTRACT:
		case InlineID::INT64_SUBTRACT:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_SUB, ret, param[1]);
			break;
		case InlineID::INT_MULTIPLY:
		case InlineID::INT64_MULTIPLY:
			add_cmd(Asm::INST_MUL, ret, param[0], param[1]);
			break;
		case InlineID::INT_DIVIDE:
		case InlineID::INT64_DIVIDE:
			add_cmd(Asm::INST_IDIV, ret, param[0], param[1]);
			break;
		case InlineID::INT_MODULO:{
			int veax = add_virtual_reg(Asm::REG_EAX);
			int vedx = add_virtual_reg(Asm::REG_EDX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, veax), param[0]);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, vedx), param_vreg(TypeInt, veax));
			add_cmd(Asm::INST_SAR, param_vreg(TypeInt, vedx), param_imm(TypeChar, 0x1f));
			add_cmd(Asm::INST_IDIV, param_vreg(TypeInt, veax), param[1]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt, vedx));
			}break;
		case InlineID::INT64_MODULO:{
			int vrax = add_virtual_reg(Asm::REG_RAX);
			int vrdx = add_virtual_reg(Asm::REG_RDX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrax), param[0]);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrdx), param_vreg(TypeInt64, vrax));
			add_cmd(Asm::INST_SAR, param_vreg(TypeInt64, vrdx), param_imm(TypeChar, 0x1f));
			add_cmd(Asm::INST_IDIV, param_vreg(TypeInt64, vrax), param[1]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt64, vrdx));
			}break;
		case InlineID::INT_EQUAL:
		case InlineID::INT_NOT_EQUAL:
		case InlineID::INT_GREATER:
		case InlineID::INT_GREATER_EQUAL:
		case InlineID::INT_SMALLER:
		case InlineID::INT_SMALLER_EQUAL:
		case InlineID::INT64_EQUAL:
		case InlineID::INT64_NOT_EQUAL:
		case InlineID::INT64_GREATER:
		case InlineID::INT64_GREATER_EQUAL:
		case InlineID::INT64_SMALLER:
		case InlineID::INT64_SMALLER_EQUAL:
		case InlineID::POINTER_EQUAL:
		case InlineID::POINTER_NOT_EQUAL:
			add_cmd(Asm::INST_CMP, param[0], param[1]);
			if (index == InlineID::INT_EQUAL)
				add_cmd(Asm::INST_SETZ, ret);
			else if (index == InlineID::INT_NOT_EQUAL)
				add_cmd(Asm::INST_SETNZ, ret);
			else if (index == InlineID::INT_GREATER)
				add_cmd(Asm::INST_SETNLE, ret);
			else if (index == InlineID::INT_GREATER_EQUAL)
				add_cmd(Asm::INST_SETNL, ret);
			else if (index == InlineID::INT_SMALLER)
				add_cmd(Asm::INST_SETL, ret);
			else if (index == InlineID::INT_SMALLER_EQUAL)
				add_cmd(Asm::INST_SETLE, ret);
			else if (index == InlineID::INT64_EQUAL)
				add_cmd(Asm::INST_SETZ, ret);
			else if (index == InlineID::INT64_NOT_EQUAL)
				add_cmd(Asm::INST_SETNZ, ret);
			else if (index == InlineID::INT64_GREATER)
				add_cmd(Asm::INST_SETNLE, ret);
			else if (index == InlineID::INT64_GREATER_EQUAL)
				add_cmd(Asm::INST_SETNL, ret);
			else if (index == InlineID::INT64_SMALLER)
				add_cmd(Asm::INST_SETL, ret);
			else if (index == InlineID::INT64_SMALLER_EQUAL)
				add_cmd(Asm::INST_SETLE, ret);
			else if (index == InlineID::POINTER_EQUAL)
				add_cmd(Asm::INST_SETZ, ret);
			else if (index == InlineID::POINTER_NOT_EQUAL)
				add_cmd(Asm::INST_SETNZ, ret);
			break;
		case InlineID::INT_AND:
		case InlineID::INT64_AND:
			add_cmd(Asm::INST_AND, ret, param[0], param[1]);
			break;
		case InlineID::INT_OR:
		case InlineID::INT64_OR:
			add_cmd(Asm::INST_OR, ret, param[0], param[1]);
			break;
		case InlineID::INT_SHIFT_RIGHT:{
			int vecx = add_virtual_reg(Asm::REG_ECX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, vecx), param[1]);
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_SHR, ret, param_vreg(TypeChar, vecx, Asm::REG_CL));
			}break;
		case InlineID::INT64_SHIFT_RIGHT:{
			int vrcx = add_virtual_reg(Asm::REG_RCX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrcx), param[1]);
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_SHR, ret, param_vreg(TypeChar, vrcx, Asm::REG_CL));
			}break;
		case InlineID::INT_SHIFT_LEFT:{
			int vecx = add_virtual_reg(Asm::REG_ECX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, vecx), param[1]);
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_SHL, ret, param_vreg(TypeChar, vecx, Asm::REG_CL));
			}break;
		case InlineID::INT64_SHIFT_LEFT:{
			int vrcx = add_virtual_reg(Asm::REG_RCX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrcx), param[1]);
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_SHL, ret, param_vreg(TypeChar, vrcx, Asm::REG_CL));
			}break;
		case InlineID::INT_NEGATE:
			add_cmd(Asm::INST_MOV, ret, param_imm(TypeInt, 0x0));
			add_cmd(Asm::INST_SUB, ret, param[0]);
			break;
		case InlineID::INT64_NEGATE:
			add_cmd(Asm::INST_MOV, ret, param_imm(TypeInt64, 0x0));
			add_cmd(Asm::INST_SUB, ret, param[0]);
			break;
		case InlineID::INT_INCREASE:
			add_cmd(Asm::INST_ADD, param[0], param_imm(TypeInt, 0x1));
			break;
		case InlineID::INT64_INCREASE:
			add_cmd(Asm::INST_ADD, param[0], param_imm(TypeInt64, 0x1));
			break;
		case InlineID::INT_DECREASE:
			add_cmd(Asm::INST_SUB, param[0], param_imm(TypeInt, 0x1));
			break;
		case InlineID::INT64_DECREASE:
			add_cmd(Asm::INST_SUB, param[0], param_imm(TypeInt64, 0x1));
			break;
		case InlineID::INT64_TO_INT:{
			int vrax = add_virtual_reg(Asm::REG_RAX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrax), param[0]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt, vrax, Asm::REG_EAX));
			}break;
		case InlineID::INT_TO_INT64:{
			int vrax = add_virtual_reg(Asm::REG_RAX);
			add_cmd(Asm::INST_XOR, param_vreg(TypeInt64, vrax), param_vreg(TypeInt64, vrax));
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, vrax, Asm::REG_EAX), param[0]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt64, vrax));
			}break;
// float
		case InlineID::FLOAT_ADD_ASSIGN:
		case InlineID::FLOAT_SUBTRACT_ASSIGN:
		case InlineID::FLOAT_MULTIPLY_ASSIGN:
		case InlineID::FLOAT_DIVIDE_ASSIGN:
			add_cmd(Asm::INST_MOVSS, p_xmm0, param[0]);
			if (index == InlineID::FLOAT_ADD_ASSIGN)
				add_cmd(Asm::INST_ADDSS, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT_SUBTRACT_ASSIGN)
				add_cmd(Asm::INST_SUBSS, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT_MULTIPLY_ASSIGN)
				add_cmd(Asm::INST_MULSS, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT_DIVIDE_ASSIGN)
				add_cmd(Asm::INST_DIVSS, p_xmm0, param[1]);
			add_cmd(Asm::INST_MOVSS, param[0], p_xmm0);
			break;
		case InlineID::FLOAT64_ADD_ASSIGN:
		case InlineID::FLOAT64_SUBTRACT_ASSIGN:
		case InlineID::FLOAT64_MULTIPLY_ASSIGN:
		case InlineID::FLOAT64_DIVIDE_ASSIGN:
			add_cmd(Asm::INST_MOVSD, p_xmm0, param[0]);
			if (index == InlineID::FLOAT64_ADD_ASSIGN)
				add_cmd(Asm::INST_ADDSD, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT64_SUBTRACT_ASSIGN)
				add_cmd(Asm::INST_SUBSD, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT64_MULTIPLY_ASSIGN)
				add_cmd(Asm::INST_MULSD, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT64_DIVIDE_ASSIGN)
				add_cmd(Asm::INST_DIVSD, p_xmm0, param[1]);
			add_cmd(Asm::INST_MOVSD, param[0], p_xmm0);
			break;
		case InlineID::FLOAT_ADD:
		case InlineID::FLOAT_SUBTARCT:
		case InlineID::FLOAT_MULTIPLY:
		case InlineID::FLOAT_DIVIDE:
			add_cmd(Asm::INST_MOVSS, p_xmm0, param[0]);
			if (index == InlineID::FLOAT_ADD)
				add_cmd(Asm::INST_ADDSS, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT_SUBTARCT)
				add_cmd(Asm::INST_SUBSS, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT_MULTIPLY)
				add_cmd(Asm::INST_MULSS, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT_DIVIDE)
				add_cmd(Asm::INST_DIVSS, p_xmm0, param[1]);
			add_cmd(Asm::INST_MOVSS, ret, p_xmm0);
			break;
		case InlineID::FLOAT64_ADD:
		case InlineID::FLOAT64_SUBTRACT:
		case InlineID::FLOAT64_MULTIPLY:
		case InlineID::FLOAT64_DIVIDE:
			add_cmd(Asm::INST_MOVSD, p_xmm0, param[0]);
			if (index == InlineID::FLOAT64_ADD)
				add_cmd(Asm::INST_ADDSD, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT64_SUBTRACT)
				add_cmd(Asm::INST_SUBSD, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT64_MULTIPLY)
				add_cmd(Asm::INST_MULSD, p_xmm0, param[1]);
			else if (index == InlineID::FLOAT64_DIVIDE)
				add_cmd(Asm::INST_DIVSD, p_xmm0, param[1]);
			add_cmd(Asm::INST_MOVSD, ret, p_xmm0);
			break;
		case InlineID::FLOAT_EQUAL:
		case InlineID::FLOAT_NOT_EQUAL:
		case InlineID::FLOAT_GREATER:
		case InlineID::FLOAT_GREATER_EQUAL:
		case InlineID::FLOAT_SMALLER:
		case InlineID::FLOAT_SMALLER_EQUAL:
			add_cmd(Asm::INST_MOVSS, p_xmm0, param[0]);
			add_cmd(Asm::INST_UCOMISS, p_xmm0, param[1]);
			if (index == InlineID::FLOAT_EQUAL)
				add_cmd(Asm::INST_SETZ, ret);
			else if (index == InlineID::FLOAT_NOT_EQUAL)
				add_cmd(Asm::INST_SETNZ, ret);
			else if (index == InlineID::FLOAT_GREATER)
				add_cmd(Asm::INST_SETNBE, ret);
			else if (index == InlineID::FLOAT_GREATER_EQUAL)
				add_cmd(Asm::INST_SETNB, ret);
			else if (index == InlineID::FLOAT_SMALLER)
				add_cmd(Asm::INST_SETB, ret);
			else if (index == InlineID::FLOAT_SMALLER_EQUAL)
				add_cmd(Asm::INST_SETBE, ret);
			break;
		case InlineID::FLOAT64_EQUAL:
		case InlineID::FLOAT64_NOT_EQUAL:
		case InlineID::FLOAT64_GREATER:
		case InlineID::FLOAT64_GREATER_EQUAL:
		case InlineID::FLOAT64_SMALLER:
		case InlineID::FLOAT64_SMALLER_EQUAL:
			add_cmd(Asm::INST_MOVSD, p_xmm0, param[0]);
			add_cmd(Asm::INST_UCOMISD, p_xmm0, param[1]);
			if (index == InlineID::FLOAT64_EQUAL)
				add_cmd(Asm::INST_SETZ, ret);
			else if (index == InlineID::FLOAT64_NOT_EQUAL)
				add_cmd(Asm::INST_SETNZ, ret);
			else if (index == InlineID::FLOAT64_GREATER)
				add_cmd(Asm::INST_SETNBE, ret);
			else if (index == InlineID::FLOAT64_GREATER_EQUAL)
				add_cmd(Asm::INST_SETNB, ret);
			else if (index == InlineID::FLOAT64_SMALLER)
				add_cmd(Asm::INST_SETB, ret);
			else if (index == InlineID::FLOAT64_SMALLER_EQUAL)
				add_cmd(Asm::INST_SETBE, ret);
			break;

		case InlineID::FLOAT_NEGATE:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_XOR, ret, param_imm(TypeInt, 0x80000000));
			break;
// complex
		case InlineID::COMPLEX_ADD_ASSIGN:
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::INST_ADDSS, p_xmm0, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(param[0], 0, TypeFloat32), p_xmm0);
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::INST_ADDSS, p_xmm0, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(param[0], 4, TypeFloat32), p_xmm0);
			break;
		case InlineID::COMPLEX_SUBTARCT_ASSIGN:
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::INST_SUBSS, p_xmm0, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(param[0], 0, TypeFloat32), p_xmm0);
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::INST_SUBSS, p_xmm0, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(param[0], 4, TypeFloat32), p_xmm0);
			break;
		case InlineID::COMPLEX_ADD:
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::INST_ADDSS, p_xmm0, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 0, TypeFloat32), p_xmm0);
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::INST_ADDSS, p_xmm0, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 4, TypeFloat32), p_xmm0);
			break;
		case InlineID::COMPLEX_SUBTRACT:
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::INST_SUBSS, p_xmm0, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 0, TypeFloat32), p_xmm0);
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::INST_SUBSS, p_xmm0, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 4, TypeFloat32), p_xmm0);
			break;
		case InlineID::COMPLEX_MULTIPLY:
			// xmm1 = a.y * b.y
			add_cmd(Asm::INST_MOVSS, p_xmm1, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::INST_MULSS, p_xmm1, param_shift(param[1], 4, TypeFloat32));
			// r.x = a.x * b.x - xmm1
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::INST_MULSS, p_xmm0, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_SUBSS, p_xmm0, p_xmm1);
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 0, TypeFloat32), p_xmm0);
			// xmm1 = a.y * b.x
			add_cmd(Asm::INST_MOVSS, p_xmm1, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::INST_MULSS, p_xmm1, param_shift(param[1], 0, TypeFloat32));
			// r.y = a.x * b.y + xmm1
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::INST_MULSS, p_xmm0, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::INST_ADDSS, p_xmm0, p_xmm1);
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 4, TypeFloat32), p_xmm0);
			break;
		case InlineID::COMPLEX_MULTIPLY_FC:
			add_cmd(Asm::INST_MOVSS, p_xmm0, param[0]);
			add_cmd(Asm::INST_MULSS, p_xmm0, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 0, TypeFloat32), p_xmm0);
			add_cmd(Asm::INST_MOVSS, p_xmm0, param[0]);
			add_cmd(Asm::INST_MULSS, p_xmm0, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 4, TypeFloat32), p_xmm0);
			break;
		case InlineID::COMPLEX_MULTIPLY_CF:
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::INST_MULSS, p_xmm0, param[1]);
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 0, TypeFloat32), p_xmm0);
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::INST_MULSS, p_xmm0, param[1]);
			add_cmd(Asm::INST_MOVSS, param_shift(ret, 4, TypeFloat32), p_xmm0);
			break;
// bool/char
		case InlineID::CHAR_EQUAL:
		case InlineID::CHAR_NOT_EQUAL:
		case InlineID::BOOL_EQUAL:
		case InlineID::BOOL_NOT_EQUAL:
		case InlineID::CHAR_GREATER:
		case InlineID::CHAR_GREATER_EQUAL:
		case InlineID::CHAR_SMALLER:
		case InlineID::CHAR_SMALLER_EQUAL:
			add_cmd(Asm::INST_CMP, param[0], param[1]);
			if ((index == InlineID::CHAR_EQUAL) or (index == InlineID::BOOL_EQUAL))
				add_cmd(Asm::INST_SETZ, ret);
			else if ((index == InlineID::CHAR_NOT_EQUAL) or (index == InlineID::BOOL_NOT_EQUAL))
				add_cmd(Asm::INST_SETNZ, ret);
			else if (index == InlineID::CHAR_GREATER)
				add_cmd(Asm::INST_SETNLE, ret);
			else if (index == InlineID::CHAR_GREATER_EQUAL)
				add_cmd(Asm::INST_SETNL, ret);
			else if (index == InlineID::CHAR_SMALLER)
				add_cmd(Asm::INST_SETL, ret);
			else if (index == InlineID::CHAR_SMALLER_EQUAL)
				add_cmd(Asm::INST_SETLE, ret);
			break;
		case InlineID::BOOL_AND:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_AND, ret, param[1]);
			break;
		case InlineID::BOOL_OR:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_OR, ret, param[1]);
			break;
		case InlineID::CHAR_ADD_ASSIGN:
			add_cmd(Asm::INST_ADD, param[0], param[1]);
			break;
		case InlineID::CHAR_SUBTRACT_ASSIGN:
			add_cmd(Asm::INST_SUB, param[0], param[1]);
			break;
		case InlineID::CHAR_ADD:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_ADD, ret, param[1]);
			break;
		case InlineID::CHAR_SUBTRACT:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_SUB, ret, param[1]);
			break;
		case InlineID::CHAR_AND:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_AND, ret, param[1]);
			break;
		case InlineID::CHAR_OR:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_OR, ret, param[1]);
			break;
		case InlineID::BOOL_NEGATE:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_XOR, ret, param_imm(TypeBool, 0x1));
			break;
		case InlineID::CHAR_NEGATE:
			add_cmd(Asm::INST_MOV, ret, param_imm(TypeChar, 0x0));
			add_cmd(Asm::INST_SUB, ret, param[0]);
			break;
// vector
		case InlineID::VECTOR_ADD_ASSIGN:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_ADDSS, p_xmm0, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::INST_MOVSS, param_shift(param[0], i * 4, TypeFloat32), p_xmm0);
			}
			break;
		case InlineID::VECTOR_MULTIPLY_ASSIGN:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_MULSS, p_xmm0, param[1]);
				add_cmd(Asm::INST_MOVSS, param_shift(param[0], i * 4, TypeFloat32), p_xmm0);
			}
			break;
		case InlineID::VECTOR_DIVIDE_ASSIGN:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_DIVSS, p_xmm0, param[1]);
				add_cmd(Asm::INST_MOVSS, param_shift(param[0], i * 4, TypeFloat32), p_xmm0);
			}
			break;
		case InlineID::VECTOR_SUBTARCT_ASSIGN:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_SUBSS, p_xmm0, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::INST_MOVSS, param_shift(param[0], i * 4, TypeFloat32), p_xmm0);
			}
			break;
		case InlineID::VECTOR_ADD:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_ADDSS, p_xmm0, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::INST_MOVSS, param_shift(ret, i * 4, TypeFloat32), p_xmm0);
			}
			break;
		case InlineID::VECTOR_SUBTRACT:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_SUBSS, p_xmm0, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::INST_MOVSS, param_shift(ret, i * 4, TypeFloat32), p_xmm0);
			}
			break;
		case InlineID::VECTOR_MULTIPLY_VF:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_MULSS, p_xmm0, param[1]);
				add_cmd(Asm::INST_MOVSS, param_shift(ret, i * 4, TypeFloat32), p_xmm0);
			}
			break;
		case InlineID::VECTOR_MULTIPLY_FV:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm0, param[0]);
				add_cmd(Asm::INST_MULSS, p_xmm0, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::INST_MOVSS, param_shift(ret, i * 4, TypeFloat32), p_xmm0);
			}
			break;
		case InlineID::VECTOR_MULTIPLY_VV:
			add_cmd(Asm::INST_MULSS, ret, param_shift(param[0], 0 * 4, TypeFloat32), param_shift(param[1], 0 * 4, TypeFloat32));
			for (int i=1;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm1, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_MULSS, p_xmm1, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::INST_ADDSS, p_xmm0, p_xmm1);
			}
			add_cmd(Asm::INST_MOVSS, ret, p_xmm0);
			break;
		case InlineID::VECTOR_DIVIDE_VF:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_DIVSS, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32), param[1]);
			}
			break;
		case InlineID::VECTOR_NEGATE:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_XOR, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32), param_imm(TypeInt, 0x80000000));
			}
			break;
		default:
			do_error("inline function unimplemented: #" + i2s((int)index));
	}
}

void SerializerX::do_mapping() {
}

}
