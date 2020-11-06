/*
 * SerializerX.cpp
 *
 *  Created on: Nov 4, 2020
 *      Author: michi
 */

#include "SerializerX.h"
#include "../kaba.h"
#include "../../file/msg.h"


// hmmm, do we want to insert "pop local" to read function parameters?
//  but what about the return reference?
//  (we would have calling convention dependency already here)
//  -> only params... insert return ref. in backend?

namespace kaba {

SerializerX::SerializerX(Script *s, Asm::InstructionWithParamsList *l) : Serializer(s, l) {
	//list->clear();
	map_reg_root.add(0); // eax
	map_reg_root.add(1); // ecx
	map_reg_root.add(2); // edx
//	MapRegRoot.add(3); // ebx
//	MapRegRoot.add(6); // esi
//	MapRegRoot.add(7); // edi
}

SerializerX::~SerializerX() {
	/*msg_write("aa");
	list->show();
	msg_write("aa2");
	for (int i=0;i<cmd.num;i++)
		msg_write(format("%3d: ", i) + cmd[i].str(this));
	list->clear();
	msg_write("aa3");*/
}

void SerializerX::add_function_call(Function *f, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	call_used = true;
	int push_size = fc_begin(f, params, ret);

	SerialNodeParam fp = {NodeKind::FUNCTION, (int_p)f, -1, TypeFunctionP, 0};
	add_cmd(Asm::INST_CALL, ret, fp); // the actual call

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
	if (f->literal_return_type == TypeVoid)
		add_cmd(Asm::INST_RET);
}

SerialNodeParam SerializerX::serialize_parameter(Node *link, Block *block, int index) {
	SerialNodeParam p;
	p.kind = link->kind;
	p.type = link->type;
	p.p = 0;
	p.shift = 0;

	if (link->kind == NodeKind::MEMORY){
		p.p = link->link_no;
	}else if (link->kind == NodeKind::ADDRESS){
		//p.p = link->link_no;
		p.p = (int_p)&link->link_no;
		p.kind = NodeKind::CONSTANT_BY_ADDRESS;
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
	auto statement = com->as_statement();
	switch(statement->id){
		case StatementID::IF:{
			int m_after_true = list->create_label("_IF_AFTER_" + i2s(num_markers ++));
			auto cond = serialize_parameter(com->params[0].get(), block, index);
			// cmp;  jz m;  -block-  m;
			add_cmd(Asm::INST_CMP, cond, param_imm(TypeBool, 0x0));
			add_cmd(Asm::INST_JZ, param_marker32(m_after_true));
			serialize_block(com->params[1]->as_block());
			add_marker(m_after_true);
			}break;
		case StatementID::IF_ELSE:{
			int m_after_true = list->create_label("_IF_AFTER_TRUE_" + i2s(num_markers ++));
			int m_after_false = list->create_label("_IF_AFTER_FALSE_" + i2s(num_markers ++));
			auto cond = serialize_parameter(com->params[0].get(), block, index);
			// cmp;  jz m1;  -block-  jmp m2;  m1;  -block-  m2;
			add_cmd(Asm::INST_CMP, cond, param_imm(TypeBool, 0x0));
			add_cmd(Asm::INST_JZ, param_marker32(m_after_true)); // jz ...
			serialize_block(com->params[1]->as_block());
			add_cmd(Asm::INST_JMP, param_marker32(m_after_false));
			add_marker(m_after_true);
			serialize_block(com->params[2]->as_block());
			add_marker(m_after_false);
			}break;
		case StatementID::WHILE:{
			int marker_before_while = list->create_label("_WHILE_BEFORE_" + i2s(num_markers ++));
			int marker_after_while = list->create_label("_WHILE_AFTER_" + i2s(num_markers ++));
			add_marker(marker_before_while);
			auto cond = serialize_parameter(com->params[0].get(), block, index);
			// m1;  cmp;  jz m2;  -block-             jmp m1;  m2;     (while)
			// m1;  cmp;  jz m2;  -block-  m3;  i++;  jmp m1;  m2;     (for)
			add_cmd(Asm::INST_CMP, cond, param_imm(TypeBool, 0x0));
			add_cmd(Asm::INST_JZ, param_marker32(marker_after_while));

			// body of loop
			LoopData l = {marker_before_while, marker_after_while, block->level, index};
			loop.add(l);
			serialize_block(com->params[1]->as_block());
			loop.pop();

			add_cmd(Asm::INST_JMP, param_marker32(marker_before_while));
			add_marker(marker_after_while);
			}break;
		case StatementID::FOR_DIGEST:{
			int marker_before_for = list->create_label("_FOR_BEFORE_" + i2s(num_markers ++));
			int marker_after_for = list->create_label("_FOR_AFTER_" + i2s(num_markers ++));
			int marker_continue = list->create_label("_FOR_CONTINUE_" + i2s(num_markers ++));
			serialize_node(com->params[0].get(), block, index); // i=0
			add_marker(marker_before_for);
			auto cond = serialize_parameter(com->params[1].get(), block, index);
			// m1;  cmp;  jz m2;  -block-             jmp m1;  m2;     (while)
			// m1;  cmp;  jz m2;  -block-  m3;  i++;  jmp m1;  m2;     (for)
			add_cmd(Asm::INST_CMP, cond, param_imm(TypeBool, 0x0));
			add_cmd(Asm::INST_JZ, param_marker32(marker_after_for));

			// body of loop
			LoopData l = {marker_continue, marker_after_for, block->level, index};
			loop.add(l);
			serialize_block(com->params[2]->as_block());
			loop.pop();

			// "i++"
			add_marker(marker_continue);
			serialize_node(com->params[3].get(), block, index);

			add_cmd(Asm::INST_JMP, param_marker32(marker_before_for));
			add_marker(marker_after_for);
			}break;
		case StatementID::BREAK:
			add_cmd(Asm::INST_JMP, param_marker32(loop.back().marker_break));
			break;
		case StatementID::CONTINUE:
			add_cmd(Asm::INST_JMP, param_marker32(loop.back().marker_continue));
			break;
		case StatementID::RETURN:
			if (com->params.num > 0) {
				auto p = serialize_parameter(com->params[0].get(), block, index);
				insert_destructors_block(block, true);
				add_cmd(Asm::INST_RET, p);
			} else {
				insert_destructors_block(block, true);
				add_cmd(Asm::INST_RET);
			}

			break;
		case StatementID::NEW:{
			// malloc()
			auto f = syntax_tree->required_func_global("@malloc");
			add_function_call(f, {param_imm(TypeInt, ret.type->param[0]->size)}, ret);

			// __init__()
			auto sub = com->params[0]->shallow_copy();
			Node *c_ret = new Node(NodeKind::VAR_TEMP, ret.p, ret.type);
			sub->set_instance(c_ret);
			serialize_node(sub.get(), block, index);
			//delete sub;
			break;}
		case StatementID::DELETE:{
			// __delete__()
			auto operand = serialize_parameter(com->params[0].get(), block, index);
			add_cmd_destructor(operand, false);

			// free()
			auto f = syntax_tree->required_func_global("@free");
			add_function_call(f, {operand}, p_none);
			break;}
		/*case StatementID::RAISE:
			AddFunctionCall();
			break;*/
		case StatementID::TRY:{
			int marker_finish = list->create_label("_TRY_AFTER_" + i2s(num_markers ++));

			// try
			serialize_block(com->params[0]->as_block());
			add_cmd(Asm::INST_JMP, param_marker32(marker_finish));

			// except
			for (int i=2; i<com->params.num; i+=2) {
				serialize_block(com->params[i]->as_block());
				if (i < com->params.num-1)
					add_cmd(Asm::INST_JMP, param_marker32(marker_finish));
			}

			add_marker(marker_finish);
			}break;
		case StatementID::ASM:
			//AddAsmBlock(list, script);
			add_cmd(INST_ASM);
			break;
		case StatementID::PASS:
			break;
		default:
			do_error("statement unimplemented: " + com->as_statement()->name);
	}
}

void SerializerX::serialize_inline_function(Node *com, const Array<SerialNodeParam> &param, const SerialNodeParam &ret) {
	auto index = com->as_func()->inline_no;
	switch(index){
/*		case InlineID::INT_TO_FLOAT:
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
			break;*/
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
/*		case InlineID::CHUNK_EQUAL:{
			int val = add_virtual_reg(Asm::REG_AL);
			add_cmd(Asm::INST_CMP, param_shift(param[0], 0, TypeInt), param_shift(param[1], 0, TypeInt));
			add_cmd(Asm::INST_SETZ, ret);
			for (int i=1; i<com->params[0]->type->size/4; i++) {
				add_cmd(Asm::INST_CMP, param_shift(param[0], i*4, TypeInt), param_shift(param[1], i*4, TypeInt));
				add_cmd(Asm::INST_SETZ, param_vreg(TypeBool, val));
				add_cmd(Asm::INST_AND, param_vreg(TypeBool, val));
			}
			}break;*/
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
			auto t = add_temp(TypeInt64, false);
			add_cmd(Asm::INST_MOVSX, t, param[1]);
			add_cmd(Asm::INST_ADD, ret, param[0], t);
			}break;
		case InlineID::INT_SUBTRACT:
		case InlineID::INT64_SUBTRACT:
			add_cmd(Asm::INST_SUB, ret, param[0], param[1]);
			break;
		case InlineID::INT_MULTIPLY:
		case InlineID::INT64_MULTIPLY:
			add_cmd(Asm::INST_IMUL, ret, param[0], param[1]);
			break;
		case InlineID::INT_DIVIDE:
		case InlineID::INT64_DIVIDE:
			add_cmd(Asm::INST_IDIV, ret, param[0], param[1]);
			break;
/*		case InlineID::INT_MODULO:{
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
			}break;*/
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
		case InlineID::INT_SHIFT_RIGHT:
		case InlineID::INT64_SHIFT_RIGHT:
			add_cmd(Asm::INST_SHR, ret, param[0], param[1]);
			break;
		case InlineID::INT_SHIFT_LEFT:
		case InlineID::INT64_SHIFT_LEFT:
			add_cmd(Asm::INST_SHL, ret, param[0], param[1]);
			break;
		case InlineID::INT_NEGATE:
		case InlineID::INT64_NEGATE:
			add_cmd(Asm::INST_SUB, ret, param_imm(TypeInt, 0x0), param[0]);
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
/*		case InlineID::INT64_TO_INT:{
			int vrax = add_virtual_reg(Asm::REG_RAX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrax), param[0]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt, vrax, Asm::REG_EAX));
			}break;*/
		case InlineID::INT_TO_INT64:
			add_cmd(Asm::INST_MOVSX, ret, param[0]);
			break;
// float
		case InlineID::FLOAT_ADD_ASSIGN:
		case InlineID::FLOAT64_ADD_ASSIGN:
			add_cmd(Asm::INST_FADD, param[0], param[1]);
			break;
		case InlineID::FLOAT_SUBTRACT_ASSIGN:
		case InlineID::FLOAT64_SUBTRACT_ASSIGN:
			add_cmd(Asm::INST_FSUB, param[0], param[1]);
			break;
		case InlineID::FLOAT_MULTIPLY_ASSIGN:
		case InlineID::FLOAT64_MULTIPLY_ASSIGN:
			add_cmd(Asm::INST_FMUL, param[0], param[1]);
			break;
		case InlineID::FLOAT_DIVIDE_ASSIGN:
		case InlineID::FLOAT64_DIVIDE_ASSIGN:
			add_cmd(Asm::INST_FDIV, param[0], param[1]);
			break;
		case InlineID::FLOAT_ADD:
		case InlineID::FLOAT64_ADD:
			add_cmd(Asm::INST_FADD, ret, param[0], param[1]);
			break;
		case InlineID::FLOAT_SUBTARCT:
		case InlineID::FLOAT64_SUBTRACT:
			add_cmd(Asm::INST_FSUB, ret, param[0], param[1]);
			break;
		case InlineID::FLOAT_MULTIPLY:
		case InlineID::FLOAT64_MULTIPLY:
			add_cmd(Asm::INST_FMUL, ret, param[0], param[1]);
			break;
		case InlineID::FLOAT_DIVIDE:
		case InlineID::FLOAT64_DIVIDE:
			add_cmd(Asm::INST_FDIV, ret, param[0], param[1]);
			break;
/*		case InlineID::FLOAT_EQUAL:
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
			break;*/

		case InlineID::FLOAT_NEGATE:
			add_cmd(Asm::INST_XOR, ret, param[0], param_imm(TypeInt, 0x80000000));
			break;
// complex
		case InlineID::COMPLEX_ADD_ASSIGN:
			add_cmd(Asm::INST_FADD, param_shift(param[0], 0, TypeFloat32), param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_FADD, param_shift(param[0], 4, TypeFloat32), param_shift(param[1], 4, TypeFloat32));
			break;
		case InlineID::COMPLEX_SUBTARCT_ASSIGN:
			add_cmd(Asm::INST_FSUB, param_shift(param[0], 0, TypeFloat32), param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_FSUB, param_shift(param[0], 4, TypeFloat32), param_shift(param[1], 4, TypeFloat32));
			break;
		case InlineID::COMPLEX_ADD:
			add_cmd(Asm::INST_FADD, param_shift(ret, 0, TypeFloat32), param_shift(param[0], 0, TypeFloat32), param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_FADD, param_shift(ret, 4, TypeFloat32), param_shift(param[0], 4, TypeFloat32), param_shift(param[1], 4, TypeFloat32));
			break;
		case InlineID::COMPLEX_SUBTRACT:
			add_cmd(Asm::INST_FSUB, param_shift(ret, 0, TypeFloat32), param_shift(param[0], 0, TypeFloat32), param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_FSUB, param_shift(ret, 4, TypeFloat32), param_shift(param[0], 4, TypeFloat32), param_shift(param[1], 4, TypeFloat32));
			break;
/*		case InlineID::COMPLEX_MULTIPLY:
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
			break;*/
// vector
		case InlineID::VECTOR_ADD_ASSIGN:
			for (int i=0;i<3;i++)
				add_cmd(Asm::INST_FADD, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32));
			break;
		case InlineID::VECTOR_MULTIPLY_ASSIGN:
			for (int i=0;i<3;i++)
				add_cmd(Asm::INST_FMUL, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32));
			break;
		case InlineID::VECTOR_DIVIDE_ASSIGN:
			for (int i=0;i<3;i++)
				add_cmd(Asm::INST_FDIV, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32));
			break;
		case InlineID::VECTOR_SUBTARCT_ASSIGN:
			for (int i=0;i<3;i++)
				add_cmd(Asm::INST_FSUB, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32));
			break;
		case InlineID::VECTOR_ADD:
			for (int i=0;i<3;i++)
				add_cmd(Asm::INST_FADD, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32), param_shift(param[1], i * 4, TypeFloat32));
			break;
		case InlineID::VECTOR_SUBTRACT:
			for (int i=0;i<3;i++)
				add_cmd(Asm::INST_FSUB, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32), param_shift(param[1], i * 4, TypeFloat32));
			break;
		case InlineID::VECTOR_MULTIPLY_VF:
			for (int i=0;i<3;i++)
				add_cmd(Asm::INST_FMUL, param_shift(ret, i * 4, TypeFloat32), param_shift(param[1], i * 4, TypeFloat32), param[1]);
			break;
		case InlineID::VECTOR_MULTIPLY_FV:
			for (int i=0;i<3;i++)
				add_cmd(Asm::INST_FMUL, param_shift(ret, i * 4, TypeFloat32), param[0], param_shift(param[1], i * 4, TypeFloat32));
			break;
		case InlineID::VECTOR_MULTIPLY_VV:{
			add_cmd(Asm::INST_FMUL, ret, param_shift(param[0], 0 * 4, TypeFloat32), param_shift(param[1], 0 * 4, TypeFloat32));
			auto t = add_temp(TypeFloat32, false);
			for (int i=1;i<3;i++) {
				add_cmd(Asm::INST_FMUL, t, param_shift(param[0], i * 4, TypeFloat32), param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::INST_FADD, ret, t);
			}
			}break;
		case InlineID::VECTOR_DIVIDE_VF:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_FDIV, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32), param[1]);
			}
			break;
		case InlineID::VECTOR_NEGATE:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_XOR, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32), param_imm(TypeInt, 0x80000000));
			}
			break;
		default:
			do_error("inline function unimplemented: " + com->as_func()->signature(TypeVoid));
	}
}

void SerializerX::do_mapping() {
}

}
