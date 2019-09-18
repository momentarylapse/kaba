#include "../kaba.h"
#include "serializer_x86.h"

#include "../../file/file.h"



namespace Kaba{

void AddAsmBlock(Asm::InstructionWithParamsList *list, Script *s);



int SerializerX86::fc_begin(const SerialNodeParam &instance, const Array<SerialNodeParam> &params, const SerialNodeParam &ret)
{
	const Class *type = ret.get_type_save();

	// return data too big... push address
	SerialNodeParam ret_ref;
	if (type->uses_return_by_memory()){
		//add_temp(type, ret_temp);
		ret_ref = add_reference(/*ret_temp*/ ret);
		//add_ref();
		//add_cmd(Asm::inst_lea, KindRegister, (char*)RegEaxCompilerFunctionReturn.kind, CompilerFunctionReturn.param);
	}

	// grow stack (down) for local variables of the calling function
//	add_cmd(- cur_func->_VarSize - LocalOffset - 8);
	int64 push_size = 0;

	// push parameters onto stack
	for (int p=params.num-1;p>=0;p--){
		if (params[p].type){
			int s = mem_align(params[p].type->size, 4);
			for (int j=0;j<s/4;j++)
				add_cmd(Asm::INST_PUSH, param_shift(params[p], s - 4 - j * 4, TypeInt));
			push_size += s;
		}
	}

	if (config.abi == Abi::WINDOWS_32){
		// more than 4 byte have to be returned -> give return address as very last parameter!
		if (type->uses_return_by_memory())
			add_cmd(Asm::INST_PUSH, ret_ref); // nachtraegliche eSP-Korrektur macht die Funktion
	}

	// _cdecl: push class instance as first parameter
	if (instance.type){
		add_cmd(Asm::INST_PUSH, instance);
		push_size += config.pointer_size;
	}
	
	if (config.abi == Abi::GNU_32){
		// more than 4 byte have to be returned -> give return address as very first parameter!
		if (type->uses_return_by_memory())
			add_cmd(Asm::INST_PUSH, ret_ref); // nachtraegliche eSP-Korrektur macht die Funktion
	}
	return push_size;
}

void SerializerX86::fc_end(int push_size, const SerialNodeParam &ret)
{
	const Class *type = ret.get_type_save();

	if (push_size > 127)
		add_cmd(Asm::INST_ADD, param_preg(TypePointer, Asm::REG_ESP), param_imm(TypeInt, push_size));
	else if (push_size > 0)
		add_cmd(Asm::INST_ADD, param_preg(TypePointer, Asm::REG_ESP), param_imm(TypeChar, push_size));

	// return > 4b already got copied to [ret] by the function!
	if ((type != TypeVoid) and (!type->uses_return_by_memory())){
		if (type == TypeFloat32)
			if (config.compile_os)
				add_cmd(Asm::INST_MOVSS, ret, p_xmm0);
			else
				add_cmd(Asm::INST_FSTP, ret);
		else if (type->size == 1){
			int v = add_virtual_reg(Asm::REG_AL);
			add_cmd(Asm::INST_MOV, ret, param_vreg(type, v));
			set_virtual_reg(v, cmd.num - 2, cmd.num - 1);
		}else{
			int v = add_virtual_reg(Asm::REG_EAX);
			add_cmd(Asm::INST_MOV, ret, param_vreg(type, v));
			set_virtual_reg(v, cmd.num - 2, cmd.num - 1);
		}
	}
}

void SerializerX86::add_function_call(Function *f, const SerialNodeParam &instance, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	call_used = true;
	int push_size = fc_begin(instance, params, ret);

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

void SerializerX86::add_virtual_function_call(int virtual_index, const SerialNodeParam &instance, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) {
	call_used = true;
	int push_size = fc_begin(instance, params, ret);

	add_cmd(Asm::INST_MOV, p_eax, instance);
	add_cmd(Asm::INST_MOV, p_eax, p_deref_eax);
	add_cmd(Asm::INST_ADD, p_eax, param_imm(TypeInt, 4 * virtual_index));
	add_cmd(Asm::INST_MOV, param_preg(TypePointer, Asm::REG_EDX), p_deref_eax);
	add_cmd(Asm::INST_CALL, param_preg(TypePointer, Asm::REG_EDX)); // the actual call

	fc_end(push_size, ret);
}

void SerializerX86::add_pointer_call(const SerialNodeParam &pointer, const Array<SerialNodeParam> &param, const SerialNodeParam &ret) {
	call_used = true;
	do_error("pointer call");
}

// create data for a (function) parameter
//   and compile its command if the parameter is executable itself
SerialNodeParam SerializerX86::serialize_parameter(Node *link, Block *block, int index)
{
	SerialNodeParam p;
	p.kind = link->kind;
	p.type = link->type;
	p.p = 0;
	p.shift = 0;

	if (link->kind == NodeKind::MEMORY){
		p.p = link->link_no;
	}else if (link->kind == NodeKind::ADDRESS){
		p.p = (int_p)&link->link_no;
		p.kind = NodeKind::CONSTANT_BY_ADDRESS;
	}else if (link->kind == NodeKind::VAR_GLOBAL){
		p.p = (int_p)link->as_global_p();
		if (!p.p)
			script->do_error_link("variable is not linkable: " + link->as_global()->name);
		p.kind = NodeKind::MEMORY;
	}else if (link->kind == NodeKind::VAR_LOCAL){
		p.p = link->as_local()->_offset;
		p.kind = NodeKind::LOCAL_MEMORY;
	}else if (link->kind == NodeKind::LOCAL_MEMORY){
		p.p = link->link_no;
	}else if (link->kind == NodeKind::LOCAL_ADDRESS){
		SerialNodeParam param = param_local(TypePointer, link->link_no);
		return add_reference(param, link->type);
	}else if (link->kind == NodeKind::CONSTANT){
		p.p = (int_p)link->as_const_p();
		if (config.compile_os)
			p.kind = NodeKind::MEMORY;
		else
			p.kind = NodeKind::CONSTANT_BY_ADDRESS;
	}else if ((link->kind == NodeKind::OPERATOR) or (link->kind == NodeKind::FUNCTION_CALL) or (link->kind == NodeKind::INLINE_CALL) or (link->kind == NodeKind::VIRTUAL_CALL) or (link->kind == NodeKind::STATEMENT)){
		p = serialize_node(link, block, index);
	}else if (link->kind == NodeKind::REFERENCE){
		SerialNodeParam param = serialize_parameter(link->params[0], block, index);
		//printf("%d  -  %s\n",pk,Kind2Str(pk));
		return add_reference(param, link->type);
	}else if (link->kind == NodeKind::DEREFERENCE){
		SerialNodeParam param = serialize_parameter(link->params[0], block, index);
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

void SerializerX86::serialize_statement(Node *com, const SerialNodeParam &ret, Block *block, int index)
{
	auto statement = com->as_statement();
	switch(statement->id){
		case StatementID::IF:{
			int m_after_true = list->create_label("_IF_AFTER_" + i2s(num_markers ++));
			auto cond = serialize_parameter(com->params[0], block, index);
			// cmp;  jz m;  -block-  m;
			add_cmd(Asm::INST_CMP, cond, param_imm(TypeBool, 0x0));
			add_cmd(Asm::INST_JZ, param_marker32(m_after_true));
			serialize_block(com->params[1]->as_block());
			add_marker(m_after_true);
			}break;
		case StatementID::IF_ELSE:{
			int m_after_true = list->create_label("_IF_AFTER_TRUE_" + i2s(num_markers ++));
			int m_after_false = list->create_label("_IF_AFTER_FALSE_" + i2s(num_markers ++));
			auto cond = serialize_parameter(com->params[0], block, index);
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
			auto cond = serialize_parameter(com->params[0], block, index);
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
			serialize_node(com->params[0], block, index); // i=0
			add_marker(marker_before_for);
			auto cond = serialize_parameter(com->params[1], block, index);
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
			serialize_node(com->params[3], block, index);

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
			if (com->params.num > 0){
				auto operand = serialize_parameter(com->params[0], block, index);
					
				if (cur_func->return_type->_amd64_allow_pass_in_xmm) {
					insert_destructors_block(block, true);
					// if ((config.instruction_set == Asm::INSTRUCTION_SET_AMD64) or (config.compile_os)) ???
					//		add_cmd(Asm::INST_FLD, t); 
					if (cur_func->return_type == TypeFloat32){
						add_cmd(Asm::INST_MOVSS, p_xmm0, operand);
					}else if (cur_func->return_type == TypeFloat64){
						add_cmd(Asm::INST_MOVSD, p_xmm0, operand);
					}else if (cur_func->return_type->size == 8){ // float[2]
						add_cmd(Asm::INST_MOVLPS, p_xmm0, operand);
					}else if (cur_func->return_type->size == 12){ // float[3]
						add_cmd(Asm::INST_MOVLPS, p_xmm0, param_shift(operand, 0, TypeReg64));
						add_cmd(Asm::INST_MOVSS, p_xmm1, param_shift(operand, 8, TypeFloat32));
					}else if (cur_func->return_type->size == 16){ // float[4]
						add_cmd(Asm::INST_MOVLPS, p_xmm0, param_shift(operand, 0, TypeReg64));
						add_cmd(Asm::INST_MOVLPS, p_xmm1, param_shift(operand, 8, TypeReg64));
					} else {
						do_error("...ret xmm " + cur_func->return_type->long_name());
					}
					add_function_outro(cur_func);
				} else if (cur_func->return_type->uses_return_by_memory()){ // we already got a return address in [ebp+0x08] (> 4 byte)
					insert_destructors_block(block, true);
					// internally handled...
#if 0
					int s = mem_align(cur_func->return_type->size);

					// slow
					/*SerialCommandParam p, p_deref;
					p.kind = KindVarLocal;
					p.type = TypeReg32;
					p.p = (char*) 0x8;
					p.shift = 0;
					for (int j=0;j<s/4;j++){
						AddDereference(p, p_deref);
						add_cmd(Asm::inst_mov, p_deref, param_shift(param[0], j * 4, TypeInt));
						add_cmd(Asm::inst_add, p, param_const(TypeInt, (void*)0x4));
					}*/

					// test
					SerialNodeParam p_edx = param_reg(TypeReg32, Asm::REG_EDX), p_deref_edx;
					SerialNodeParam p_ret_addr;
					p_ret_addr.kind = NodeKind::VAR_LOCAL;
					p_ret_addr.type = TypeReg32;
					p_ret_addr.p = (char*)0x8;
					p_ret_addr.shift = 0;
					int c_0 = cmd.num;
					add_cmd(Asm::INST_MOV, p_edx, p_ret_addr);
					add_dereference(p_edx, p_deref_edx, TypeReg32);
					for (int j=0;j<s/4;j++)
						add_cmd(Asm::INST_MOV, param_shift(p_deref_edx, j * 4, TypeInt), param_shift(params[0], j * 4, TypeInt));
					add_reg_channel(Asm::REG_EDX, c_0, cmd.num - 1);
#endif

					add_function_outro(cur_func);
				}else{ // store return directly in eax / fpu stack (4 byte)
					SerialNodeParam t = add_temp(cur_func->return_type);
					add_cmd(Asm::INST_MOV, t, operand); //?????
					insert_destructors_block(block, true);
					
					if (cur_func->return_type->size == 1){
						int v = add_virtual_reg(Asm::REG_AL);
						add_cmd(Asm::INST_MOV, param_vreg(cur_func->return_type, v), t);
					}else if (cur_func->return_type->size == 8){
						int v = add_virtual_reg(Asm::REG_RAX);
						add_cmd(Asm::INST_MOV, param_vreg(cur_func->return_type, v), t);
					}else{
						int v = add_virtual_reg(Asm::REG_EAX);
						add_cmd(Asm::INST_MOV, param_vreg(cur_func->return_type, v), t);
					}
					add_function_outro(cur_func);
				}
			}else{
				insert_destructors_block(block, true);
				add_function_outro(cur_func);
			}
			break;
		case StatementID::NEW:{
			// malloc()
			Array<Node*> links = syntax_tree->get_existence("@malloc", nullptr, syntax_tree->base_class, false);
			if (links.num == 0)
				do_error("@malloc not found????");
			add_function_call(links[0]->as_func(), p_none, {param_imm(TypeInt, ret.type->parent->size)}, ret);
			clear_nodes(links);

			// __init__()
			if (com->params.num > 0) {
				Node *sub = com->params[0];
				Node *c_ret = new Node(NodeKind::VAR_TEMP, ret.p, ret.type);
				sub->instance = c_ret;
				serialize_node(sub, block, index);
				//delete sub;
			} else {
				add_cmd_constructor(ret, NodeKind::NONE);
			}
			break;}
		case StatementID::DELETE:{
			// __delete__()
			auto operand = serialize_parameter(com->params[0], block, index);
			add_cmd_destructor(operand, false);

			// free()
			Array<Node*> links = syntax_tree->get_existence("@free", nullptr, syntax_tree->base_class, false);
			if (links.num == 0)
				do_error("@free not found????");
			add_function_call(links[0]->as_func(), p_none, {operand}, p_none);
			clear_nodes(links);
			break;}
		/*case StatementID::RAISE:
			AddFunctionCall();
			break;*/
		case StatementID::TRY:{
			int marker_finish = list->create_label("_TRY_AFTER_" + i2s(num_markers ++));
			serialize_block(com->params[0]->as_block());
			add_cmd(Asm::INST_JMP, param_marker32(marker_finish));
			serialize_block(com->params[2]->as_block());
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

void SerializerX86::serialize_inline_function(Node *com, const Array<SerialNodeParam> &param, const SerialNodeParam &ret)
{
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
			add_cmd(Asm::INST_MOV, param_shift(ret, 12, TypeFloat32), param[3]);
			/* fall through */
		case InlineID::VECTOR_SET:
			add_cmd(Asm::INST_MOV, param_shift(ret, 8, TypeFloat32), param[2]);
		case InlineID::COMPLEX_SET:
			add_cmd(Asm::INST_MOV, param_shift(ret, 4, TypeFloat32), param[1]);
			add_cmd(Asm::INST_MOV, param_shift(ret, 0, TypeFloat32), param[0]);
			break;
		case InlineID::COLOR_SET:
			add_cmd(Asm::INST_MOV, param_shift(ret, 12, TypeFloat32), param[0]);
			add_cmd(Asm::INST_MOV, param_shift(ret, 0, TypeFloat32), param[1]);
			add_cmd(Asm::INST_MOV, param_shift(ret, 4, TypeFloat32), param[2]);
			add_cmd(Asm::INST_MOV, param_shift(ret, 8, TypeFloat32), param[3]);
			break;
		case InlineID::INT_ASSIGN:
		case InlineID::INT64_ASSIGN:
		case InlineID::FLOAT_ASSIGN:
		case InlineID::FLOAT64_ASSIGN:
		case InlineID::POINTER_ASSIGN:
			add_cmd(Asm::INST_MOV, param[0], param[1]);
			break;
		case InlineID::CHAR_ASSIGN:
		case InlineID::BOOL_ASSIGN:
			add_cmd(Asm::INST_MOV, param[0], param[1]);
			break;
		case InlineID::CHUNK_ASSIGN:
			for (int i=0; i<com->params[0]->type->size/4; i++)
				add_cmd(Asm::INST_MOV, param_shift(param[0], i * 4, TypeInt), param_shift(param[1], i * 4, TypeInt));
			for (int i=4*(com->params[0]->type->size/4); i<com->params[0]->type->size; i++)
				add_cmd(Asm::INST_MOV, param_shift(param[0], i, TypeChar), param_shift(param[1], i, TypeChar));
			break;
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
		case InlineID::INT_DIVIDE_ASSIGN:{
			int veax = add_virtual_reg(Asm::REG_EAX);
			int vedx = add_virtual_reg(Asm::REG_EDX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, veax), param[0]);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, vedx), param_vreg(TypeInt, veax));
			add_cmd(Asm::INST_SAR, param_vreg(TypeInt, vedx), param_imm(TypeChar, 0x1f));
			add_cmd(Asm::INST_IDIV, param_vreg(TypeInt, veax), param[1]);
			add_cmd(Asm::INST_MOV, param[0], param_vreg(TypeInt, veax));
			}break;
		case InlineID::INT64_DIVIDE_ASSIGN:{
			int vrax = add_virtual_reg(Asm::REG_RAX);
			int vrdx = add_virtual_reg(Asm::REG_RDX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrax), param[0]);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrdx), param_vreg(TypeInt64, vrax));
			add_cmd(Asm::INST_SAR, param_vreg(TypeInt64, vrdx), param_imm(TypeChar, 0x1f));
			add_cmd(Asm::INST_IDIV, param_vreg(TypeInt64, vrax), param[1]);
			add_cmd(Asm::INST_MOV, param[0], param_vreg(TypeInt64, vrax));
			}break;
		case InlineID::INT_ADD:
		case InlineID::INT64_ADD:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_ADD, ret, param[1]);
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
		case InlineID::INT_MULTIPLY:{
			int veax = add_virtual_reg(Asm::REG_EAX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, veax), param[0]);
			add_cmd(Asm::INST_IMUL, param_vreg(TypeInt, veax), param[1]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt, veax));
			}break;
		case InlineID::INT64_MULTIPLY:{
			int vrax = add_virtual_reg(Asm::REG_RAX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrax), param[0]);
			add_cmd(Asm::INST_IMUL, param_vreg(TypeInt64, vrax), param[1]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt64, vrax));
			}break;
		case InlineID::INT_DIVIDE:{
			int veax = add_virtual_reg(Asm::REG_EAX);
			int vedx = add_virtual_reg(Asm::REG_EDX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, veax), param[0]);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt, vedx), param_vreg(TypeInt, veax));
			add_cmd(Asm::INST_SAR, param_vreg(TypeInt, vedx), param_imm(TypeChar, 0x1f));
			add_cmd(Asm::INST_IDIV, param_vreg(TypeInt, veax), param[1]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt, veax));
			}break;
		case InlineID::INT64_DIVIDE:{
			int vrax = add_virtual_reg(Asm::REG_RAX);
			int vrdx = add_virtual_reg(Asm::REG_RDX);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrax), param[0]);
			add_cmd(Asm::INST_MOV, param_vreg(TypeInt64, vrdx), param_vreg(TypeInt64, vrax));
			add_cmd(Asm::INST_SAR, param_vreg(TypeInt64, vrdx), param_imm(TypeChar, 0x1f));
			add_cmd(Asm::INST_IDIV, param_vreg(TypeInt64, vrax), param[1]);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeInt64, vrax));
			}break;
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
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_AND, ret, param[1]);
			break;
		case InlineID::INT_OR:
		case InlineID::INT64_OR:
			add_cmd(Asm::INST_MOV, ret, param[0]);
			add_cmd(Asm::INST_OR, ret, param[1]);
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
		case InlineID::COMPLEX_EQUAL:{
			int val = add_virtual_reg(Asm::REG_AL);
			add_cmd(Asm::INST_CMP, param_shift(param[0], 0, TypeFloat32), param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::INST_SETZ, ret);
			add_cmd(Asm::INST_CMP, param_shift(param[0], 4, TypeFloat32), param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::INST_SETZ, param_vreg(TypeBool, val));
			add_cmd(Asm::INST_AND, param_vreg(TypeBool, val));
			}break;
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
			add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], 0 * 4, TypeFloat32));
			add_cmd(Asm::INST_MULSS, p_xmm0, param_shift(param[1], 0 * 4, TypeFloat32));
			for (int i=1;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm1, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_MULSS, p_xmm1, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::INST_ADDSS, p_xmm0, p_xmm1);
			}
			add_cmd(Asm::INST_MOVSS, ret, p_xmm0);
			break;
		case InlineID::VECTOR_DIVIDE_VF:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOVSS, p_xmm0, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_DIVSS, p_xmm0, param[1]);
				add_cmd(Asm::INST_MOVSS, param_shift(ret, i * 4, TypeFloat32), p_xmm0);
			}
			break;
		case InlineID::VECTOR_NEGATE:
			for (int i=0;i<3;i++){
				add_cmd(Asm::INST_MOV, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::INST_XOR, param_shift(ret, i * 4, TypeFloat32), param_imm(TypeInt, 0x80000000));
			}
			break;
		default:
			do_error("inline function unimplemented: #" + i2s((int)index));
	}
}

inline bool param_is_simple(SerialNodeParam &p)
{
	return ((p.kind == NodeKind::REGISTER) or (p.kind == NodeKind::VAR_TEMP) or (p.kind == NodeKind::NONE));
}

inline bool param_combi_allowed(int inst, SerialNodeParam &p1, SerialNodeParam &p2)
{
//	if (inst >= Asm::inst_marker)
//		return true;
	if ((!param_is_simple(p1)) and (!param_is_simple(p2)))
		return false;
	bool r1, w1, r2, w2;
	Asm::get_instruction_param_flags(inst, r1, w1, r2, w2);
	if (w1 and (p1.kind == NodeKind::IMMEDIATE))
		return false;
	if (w2 and (p2.kind == NodeKind::IMMEDIATE))
		return false;
	if ((p1.kind == NodeKind::IMMEDIATE) or (p2.kind == NodeKind::IMMEDIATE))
		if (!Asm::get_instruction_allow_const(inst))
			return false;
	return true;
}

// mov [0x..] [0x...]  ->  mov temp, [0x..]   mov [0x..] temp
/*void CorrectUnallowedParamCombis()
{
	for (int i=cmd.num-1;i>=0;i--)
		if (!param_combi_allowed(cmd[i].inst, cmd[i].p[0], cmd[i].p[1])){
			msg_write(string2("correcting param combi  cmd=%d", i));
			bool mov_first_param = (cmd[i].p[1].kind < 0) or (cmd[i].p[0].kind == KindRefToConst) or (cmd[i].p[0].kind == KindConstant);
			SerialCommandParam *pp = mov_first_param ? &cmd[i].p[0] : &cmd[i].p[1];
			SerialCommandParam temp, p = *pp;
			add_temp(p.type, temp);

			*pp = temp;
			if (p.type->Size == 1)
				add_cmd(Asm::inst_mov, temp, p);
			else
				add_cmd(Asm::inst_mov, temp, p);
			move_last_cmd(i);
		}
	ScanTempVarUsage();
}*/

// mov [0x..] [0x...]  ->  mov eax, [0x..]   mov [0x..] eax    (etc)
void SerializerX86::correct_unallowed_param_combis()
{
	for (int i=cmd.num-1;i>=0;i--){
		if (cmd[i].inst >= INST_MARKER)
			continue;

		// bad?
		if (param_combi_allowed(cmd[i].inst, cmd[i].p[0], cmd[i].p[1]))
			continue;

		// correct
//		msg_write(format("correcting param combi  cmd=%d", i));
		bool mov_first_param = (cmd[i].p[1].kind == NodeKind::NONE) or (cmd[i].p[0].kind == NodeKind::CONSTANT_BY_ADDRESS) or (cmd[i].p[0].kind == NodeKind::IMMEDIATE);
		int p_index = mov_first_param ? 0 : 1;
		SerialNodeParam p = cmd[i].p[p_index];
		SerialNodeParam p2 = p;

		//msg_error("correct");
		//msg_write(p.type->name);
		int reg = find_unused_reg(i, i, p.type->size);
		p2 = param_vreg(p.type, reg);
		next_cmd_target(i);
		add_cmd(Asm::INST_MOV, p2, p);
		set_cmd_param(cmd[i+1], p_index, p2);
		set_virtual_reg(reg, i, i + 1);
	}
	scan_temp_var_usage();
}

void SerializerX86::add_function_intro_params(Function *f)
{
	/*add_cmd(Asm::inst_push, param_reg(TypeReg32, Asm::REG_EBP));
	add_cmd(Asm::inst_mov, param_reg(TypeReg32, Asm::REG_EBP), param_reg(TypeReg32, Asm::REG_ESP));
	if (stack_alloc_size > 127){
		add_easy(inst_sub, PK_REGISTER, s, (void*)reg_sp, PK_CONSTANT, SIZE_32, (void*)(long)stack_alloc_size);
	}else if (stack_alloc_size > 0){
		add_easy(inst_sub, PK_REGISTER, s, (void*)reg_sp, PK_CONSTANT, SIZE_8, (void*)(long)stack_alloc_size);
	}*/
}

void SerializerX86::add_function_outro(Function *f)
{
	add_cmd(Asm::INST_LEAVE);
	if (f->return_type->uses_return_by_memory())
		add_cmd(Asm::INST_RET, param_imm(TypeReg16, 4));
	else
		add_cmd(Asm::INST_RET);
}



void SerializerX86::process_references()
{
	for (int i=0;i<cmd.num;i++)
		if (cmd[i].inst == Asm::INST_LEA){
			if (cmd[i].p[1].kind == NodeKind::LOCAL_MEMORY){
				SerialNodeParam p0 = cmd[i].p[0];
				SerialNodeParam p1 = cmd[i].p[1];
				remove_cmd(i);

				if (config.instruction_set == Asm::InstructionSet::AMD64){
					int r = add_virtual_reg(Asm::REG_RAX);
					next_cmd_target(i);
					add_cmd(Asm::INST_LEA, param_vreg(TypeReg64, r), p1);
					next_cmd_target(i+1);
					add_cmd(Asm::INST_MOV, p0, param_vreg(TypeReg64, r));
					set_virtual_reg(r, i, i+1);
				}else{
					int r = add_virtual_reg(Asm::REG_EAX);
					next_cmd_target(i);
					add_cmd(Asm::INST_LEA, param_vreg(TypeReg32, r), p1);
					next_cmd_target(i+1);
					add_cmd(Asm::INST_MOV, p0, param_vreg(TypeReg32, r));
					set_virtual_reg(r, i, i+1);
				}
			}else{
				do_error("reference in x86: " + cmd[i].p[1].str(this));
			}
		}
}

void SerializerX86::do_mapping()
{
	map_referenced_temp_vars_to_stack();


	process_references();

	try_map_temp_vars_to_registers();

	if (config.verbose and config.allow_output(cur_func, "map:a"))
		cmd_list_out("post temp -> reg");

	map_remaining_temp_vars_to_stack();

	if (config.verbose and config.allow_output(cur_func, "map:b"))
		cmd_list_out("post temp -> stack");

	resolve_deref_temp_and_local();

	if (config.verbose and config.allow_output(cur_func, "map:c"))
		cmd_list_out("post deref t&l");

	correct_unallowed_param_combis();

	if (config.verbose and config.allow_output(cur_func, "map:d"))
		cmd_list_out("unallowed");

	/*MapReferencedTempVars();

	//HandleDerefTemp();

	DisentangleShiftedTempVars();

	ResolveDerefTempAndLocal();

	RemoveUnusedTempVars();

	if (config.allow_simplification){
	SimplifyMovs();

	SimplifyFPUStack();
	}

	MapTempVars();

	ResolveDerefRegShift();

	//ResolveDerefLocal();

	CorrectUnallowedParamCombis();*/


	for (int i=0; i<cmd.num; i++)
		correct_unallowed_param_combis2(cmd[i]);

	if (config.verbose and config.allow_output(cur_func, "map:z"))
		cmd_list_out("end");
}

void SerializerX86::correct_unallowed_param_combis2(SerialNode &c)
{
	// push 8 bit -> push 32 bit
	if (c.inst == Asm::INST_PUSH)
		if (c.p[0].kind == NodeKind::REGISTER)
			c.p[0].p = reg_resize(c.p[0].p, config.pointer_size);
}

void SerializerX86::add_function_intro_frame(int stack_alloc_size) {
	int_p reg_bp = (config.instruction_set == Asm::InstructionSet::AMD64) ? Asm::REG_RBP : Asm::REG_EBP;
	int_p reg_sp = (config.instruction_set == Asm::InstructionSet::AMD64) ? Asm::REG_RSP : Asm::REG_ESP;
	//int s = config.pointer_size;
	list->add2(Asm::INST_PUSH, Asm::param_reg(reg_bp));
	list->add2(Asm::INST_MOV, Asm::param_reg(reg_bp), Asm::param_reg(reg_sp));
	if (stack_alloc_size > 127){
		list->add2(Asm::INST_SUB, Asm::param_reg(reg_sp), Asm::param_imm(stack_alloc_size, Asm::SIZE_32));
	}else if (stack_alloc_size > 0){
		list->add2(Asm::INST_SUB, Asm::param_reg(reg_sp), Asm::param_imm(stack_alloc_size, Asm::SIZE_8));
	}
}



};
