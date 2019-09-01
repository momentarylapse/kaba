#include "../kaba.h"
#include "serializer_arm.h"

#include "../../file/file.h"


namespace Asm{
	extern int ARM_DATA_INSTRUCTIONS[16]; // -> asm.cpp
};

namespace Kaba{

int func_index(Function *f);


int SerializerARM::fc_begin(const SerialNodeParam &instance, const Array<SerialNodeParam> &_params, const SerialNodeParam &ret)
{
	const Class *type = ret.type;
	if (!type)
		type = TypeVoid;

	// grow stack (down) for local variables of the calling function
//	add_cmd(- cur_func->_VarSize - LocalOffset - 8);
	int64 push_size = 0;

	Array<SerialNodeParam> params = _params;

	// instance as first parameter
	if (instance.type)
		params.insert(instance, 0);

	// return as _very_ first parameter
	if (type->uses_return_by_memory()){
		SerialNodeParam ret_ref = AddReference(ret);
		params.insert(ret_ref, 0);
	}

	// map params...
	Array<SerialNodeParam> reg_param;
	Array<SerialNodeParam> stack_param;
	Array<SerialNodeParam> xmm_param;
	for (SerialNodeParam &p: params){
		if ((p.type == TypeInt) or (p.type == TypeInt64) or (p.type == TypeChar) or (p.type == TypeBool) or p.type->is_pointer()){
			if (reg_param.num < 4){
				reg_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else if ((p.type == TypeFloat32) or (p.type == TypeFloat64)){
			if (xmm_param.num < 8){
				xmm_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else
			do_error("parameter type currently not supported: " + p.type->name);
	}

	// push parameters onto stack
/*	push_size = 4 * stack_param.num;
	if (push_size > 127)
		add_cmd(Asm::inst_add, param_reg(TypePointer, Asm::REG_RSP), param_const(TypeInt, (void*)push_size));
	else if (push_size > 0)
		add_cmd(Asm::inst_add, param_reg(TypePointer, Asm::REG_RSP), param_const(TypeChar, (void*)push_size));
	foreachb(SerialCommandParam &p, stack_param)
		add_cmd(Asm::inst_push, p);
	max_push_size = max(max_push_size, push_size);

	// xmm0-7
	foreachib(SerialCommandParam &p, xmm_param, i){
		int reg = Asm::REG_XMM0 + i;
		if (p.type == TypeFloat64)
			add_cmd(Asm::inst_movsd, param_reg(TypeReg128, reg), p);
		else
			add_cmd(Asm::inst_movss, param_reg(TypeReg128, reg), p);
	}*/

	// r0, r1, r2, r3
	foreachib(SerialNodeParam &p, reg_param, i){
		int v = add_virtual_reg(Asm::REG_R0 + i);
		add_cmd(Asm::INST_MOV, param_vreg(p.type, v), p);
		set_virtual_reg(v, cmd.num - 1, -100); // -> call
	}

	// extend reg channels to call
	for (VirtualRegister &r: virtual_reg)
		if (r.last == -100)
			r.last = cmd.num;

	return push_size;
}

void SerializerARM::fc_end(int push_size, const SerialNodeParam &ret)
{
	const Class *type = ret.type;
	if (!type)
		return;

	// return > 4b already got copied to [ret] by the function!
	if ((type != TypeVoid) and (!type->uses_return_by_memory())){
		if (type == TypeFloat32)
			add_cmd(Asm::INST_MOVSS, ret, param_preg(TypeReg128, Asm::REG_XMM0));
		else if (type == TypeFloat64)
			add_cmd(Asm::INST_MOVSD, ret, param_preg(TypeReg128, Asm::REG_XMM0));
		else if ((type->size == 1) or (type->size == 4)){
			int v = add_virtual_reg(Asm::REG_R0);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeReg32, v));
			set_virtual_reg(v, cmd.num - 2, cmd.num - 1);
		}else{
			int v = add_virtual_reg(Asm::REG_R0);
			add_cmd(Asm::INST_MOV, ret, param_vreg(TypeReg32, v));
			set_virtual_reg(v, cmd.num - 2, cmd.num - 1);
		}
	}
}

void SerializerARM::add_function_call(Function *f, const SerialNodeParam &instance, const Array<SerialNodeParam> &params, const SerialNodeParam &ret)
{
	int push_size = fc_begin(instance, params, ret);

	if ((f->owner() == syntax_tree) and (!f->is_extern)){
		add_cmd(Asm::INST_CALL, param_marker(TypePointer, f->_label));
	}else{
		if (!f->address)
			do_error_link("could not link function " + f->long_name());
		if (abs((int_p)f->address - (int_p)this->script->opcode) < 30000000){
			add_cmd(Asm::INST_CALL, param_imm(TypePointer, (int_p)f->address)); // the actual call
			// function pointer will be shifted later...
		}else{

			// TODO FIXME
			// really find a usable register...

			int v = add_virtual_reg(Asm::REG_R4);//find_unused_reg(cmd.num-1, cmd.num-1, 4);
			add_cmd(Asm::INST_MOV, param_vreg(TypePointer, v), param_lookup(TypePointer, add_global_ref(f->address)));
			add_cmd(Asm::INST_CALL, param_vreg(TypePointer, v));
			set_virtual_reg(v, cmd.num-2, cmd.num-1);
		}
	}

	fc_end(push_size, ret);
}

void SerializerARM::add_virtual_function_call(int virtual_index, const SerialNodeParam &instance, const Array<SerialNodeParam> &param, const SerialNodeParam &ret)
{
	do_error("virtual call");
}


void SerializerARM::add_pointer_call(const SerialNodeParam &pointer, const Array<SerialNodeParam> &param, const SerialNodeParam &ret)
{
	do_error("pointer call");
}

void SerializerARM::SerializeStatement(Node *com, const Array<SerialNodeParam> &param, const SerialNodeParam &ret, Block *block, int index)
{
#if 0
	switch(com->link_no){
		case STATEMENT_IF:{
			// cmp;  jz m;  -block-  m;
			add_cmd(Asm::INST_CMP, param[0], param_imm(TypeBool, 0x0));
			int m_after_true = add_marker_after_command(block->level, index + 1);
			add_cmd(Asm::ARM_COND_EQUAL, Asm::INST_B, param_marker(m_after_true), p_none, p_none);
			}break;
		case STATEMENT_IF_ELSE:{
			// cmp;  jz m1;  -block-  jmp m2;  m1;  -block-  m2;
			add_cmd(Asm::INST_CMP, param[0], param_imm(TypeBool, 0x0));
			int m_after_true = add_marker_after_command(block->level, index + 1);
			int m_after_false = add_marker_after_command(block->level, index + 2);
			add_cmd(Asm::ARM_COND_EQUAL, Asm::INST_B, param_marker(m_after_true), p_none, p_none); // jz ...
			add_jump_after_command(block->level, index + 1, m_after_false); // insert before <m_after_true> is inserted!
			}break;
		case STATEMENT_WHILE:
		case STATEMENT_FOR:{
			// m1;  cmp;  jz m2;  -block-             jmp m1;  m2;     (while)
			// m1;  cmp;  jz m2;  -block-  m3;  i++;  jmp m1;  m2;     (for)
			add_cmd(Asm::INST_CMP, param[0], param_imm(TypeBool, 0x0));
			int marker_after_while = add_marker_after_command(block->level, index + 1);
			add_cmd(Asm::ARM_COND_EQUAL, Asm::INST_B, param_marker(marker_after_while), p_none, p_none);
			add_jump_after_command(block->level, index + 1, marker_before_params); // insert before <marker_after_while> is inserted!

			int marker_continue = marker_before_params;
			if (com->link_no == STATEMENT_FOR){
				// NextCommand is a block!
				if (next_node->kind != NodeKind::BLOCK)
					do_error("command block in \"for\" loop missing");
				marker_continue = add_marker_after_command(block->level + 1, next_node->as_block()->nodes.num - 2);
			}
			LoopData l = {marker_continue, marker_after_while, block->level, index};
			loop.add(l);
			}break;
		case STATEMENT_BREAK:
			add_cmd(Asm::INST_B, param_marker(loop.back().marker_break));
			break;
		case STATEMENT_CONTINUE:
			add_cmd(Asm::INST_B, param_marker(loop.back().marker_continue));
			break;
		case STATEMENT_RETURN:
			if (com->params.num > 0){
				if (cur_func->return_type->uses_return_by_memory()){ // we already got a return address in [ebp+0x08] (> 4 byte)
					FillInDestructorsBlock(block, true);
					// internally handled...

					AddFunctionOutro(cur_func);
				}else{ // store return directly in eax / fpu stack (4 byte)
					SerialNodeParam t = add_temp(cur_func->return_type);
					FillInDestructorsBlock(block, true);
					if ((cur_func->return_type == TypeInt) or (cur_func->return_type->size == 1)){
						int v = add_virtual_reg(Asm::REG_R0);
						add_cmd(Asm::INST_MOV, param_vreg(cur_func->return_type, v), param[0]);
						set_virtual_reg(v, cmd.num-1, cmd.num);
					}else{
						do_error("return != int");
					}
					AddFunctionOutro(cur_func);
				}
			}else{
				FillInDestructorsBlock(block, true);
				AddFunctionOutro(cur_func);
			}
			break;
		case STATEMENT_NEW:{
			Array<Node*> links = syntax_tree->GetExistence("@malloc", nullptr);
			if (links.num == 0)
				do_error("@malloc not found????");
			AddFunctionCall(links[0]->script, links[0]->link_no, p_none, param_imm(TypeInt, ret.type->parent->size), ret);
			if (com->params.num > 0){
				// copy + edit command
				Node sub = *com->params[0];
				Node c_ret(NodeKind::VAR_TEMP, ret.p, script, ret.type);
				sub.instance = &c_ret;
				serialize_node(&sub, block, index);
			}else
				add_cmd_constructor(ret, -1);
			clear_nodes(links);
			break;}
		case STATEMENT_DELETE:{
			add_cmd_destructor(param[0], false);
			Array<Node*> links = syntax_tree->GetExistence("@free", nullptr);
			if (links.num == 0)
				do_error("@free not found????");
			AddFunctionCall(links[0]->script, links[0]->link_no, p_none, param[0], p_none);
			clear_nodes(links);
			break;}
		case STATEMENT_ASM:
			add_cmd(INST_ASM);
			break;
		default:
			do_error("statement unimplemented: " + Statements[com->link_no].name);
	}
#endif
	do_error("statement ksdjhksdjfhjksdhf");
}

void SerializerARM::SerializeInlineFunction(Node *com, const Array<SerialNodeParam> &param, const SerialNodeParam &ret)
{
	auto index = com->as_func()->inline_no;
	switch(index){
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
			for (int i=0; i<(com->params[0]->type->size/4); i++)
				add_cmd(Asm::INST_MOV, param_shift(param[0], i * 4, TypeInt), param_shift(param[1], i * 4, TypeInt));
			for (int i=4*(com->params[0]->type->size/4); i<com->params[0]->type->size;i++)
				add_cmd(Asm::INST_MOV, param_shift(param[0], i, TypeChar), param_shift(param[1], i, TypeChar));
			break;
// int
		case InlineID::INT_ADD_ASSIGN:
			add_cmd(Asm::INST_ADD, param[0], param[0], param[1]);
			break;
		case InlineID::INT_SUBTRACT_ASSIGN:
			add_cmd(Asm::INST_SUB, param[0], param[0], param[1]);
			break;
		case InlineID::INT_MULTIPLY_ASSIGN:
			add_cmd(Asm::INST_IMUL, param[0], param[0], param[1]);
			break;
		case InlineID::INT_ADD:
			add_cmd(Asm::INST_ADD, ret, param[0], param[1]);
			break;
		case InlineID::INT_SUBTRACT:
			add_cmd(Asm::INST_SUB, ret, param[0], param[1]);
			break;
		case InlineID::INT_MULTIPLY:
			add_cmd(Asm::INST_MUL, ret, param[0], param[1]);
			break;
		case InlineID::INT_EQUAL:
		case InlineID::POINTER_EQUAL:
			add_cmd(Asm::INST_CMP, param[0], param[1]);
			add_cmd(Asm::ARM_COND_EQUAL,     Asm::INST_MOV, ret, param_imm(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_NOT_EQUAL, Asm::INST_MOV, ret, param_imm(TypeBool, 0), p_none);
			break;
		case InlineID::INT_NOT_EQUAL:
		case InlineID::POINTER_NOT_EQUAL:
			add_cmd(Asm::INST_CMP, param[0], param[1]);
			add_cmd(Asm::ARM_COND_NOT_EQUAL, Asm::INST_MOV, ret, param_imm(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_EQUAL,     Asm::INST_MOV, ret, param_imm(TypeBool, 0), p_none);
			break;
		case InlineID::INT_GREATER:
			add_cmd(Asm::INST_CMP, param[0], param[1]);
			add_cmd(Asm::ARM_COND_GREATER_THAN, Asm::INST_MOV, ret, param_imm(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_LESS_EQUAL,   Asm::INST_MOV, ret, param_imm(TypeBool, 0), p_none);
			break;
		case InlineID::INT_GREATER_EQUAL:
			add_cmd(Asm::INST_CMP, param[0], param[1]);
			add_cmd(Asm::ARM_COND_GREATER_EQUAL, Asm::INST_MOV, ret, param_imm(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_LESS_THAN,     Asm::INST_MOV, ret, param_imm(TypeBool, 0), p_none);
			break;
		case InlineID::INT_SMALLER:
			add_cmd(Asm::INST_CMP, param[0], param[1]);
			add_cmd(Asm::ARM_COND_LESS_THAN,     Asm::INST_MOV, ret, param_imm(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_GREATER_EQUAL, Asm::INST_MOV, ret, param_imm(TypeBool, 0), p_none);
			break;
		case InlineID::INT_SMALLER_EQUAL:
			add_cmd(Asm::INST_CMP, param[0], param[1]);
			add_cmd(Asm::ARM_COND_LESS_EQUAL,   Asm::INST_MOV, ret, param_imm(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_GREATER_THAN, Asm::INST_MOV, ret, param_imm(TypeBool, 0), p_none);
			break;
		case InlineID::INT_AND:
			add_cmd(Asm::INST_AND, ret, param[0], param[1]);
			break;
		case InlineID::INT_OR:
			add_cmd(Asm::INST_OR, ret, param[0], param[1]);
			break;
		case InlineID::INT_NEGATE:
			add_cmd(Asm::INST_MOV, ret, param_imm(TypeInt, 0x0));
			add_cmd(Asm::INST_SUB, ret, ret, param[0]);
			break;
		case InlineID::INT_INCREASE:
			add_cmd(Asm::INST_ADD, param[0], param[0], param_imm(TypeInt, 0x1));
			break;
		case InlineID::INT_DECREASE:
			add_cmd(Asm::INST_SUB, param[0], param[0], param_imm(TypeInt, 0x1));
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
			else if ((index ==InlineID::CHAR_NOT_EQUAL) or (index == InlineID::BOOL_NOT_EQUAL))
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
			add_cmd(Asm::INST_AND, ret, param[0], param[1]);
			break;
		case InlineID::BOOL_OR:
			add_cmd(Asm::INST_OR, ret, param[0], param[1]);
			break;
		case InlineID::CHAR_ADD_ASSIGN:
			add_cmd(Asm::INST_ADD, param[0], param[0], param[1]);
			break;
		case InlineID::CHAR_SUBTRACT_ASSIGN:
			add_cmd(Asm::INST_SUB, param[0], param[0], param[1]);
			break;
		case InlineID::CHAR_ADD:
			add_cmd(Asm::INST_ADD, ret, param[0], param[1]);
			break;
		case InlineID::CHAR_SUBTRACT:
			add_cmd(Asm::INST_SUB, ret, param[0], param[1]);
			break;
		case InlineID::CHAR_AND:
			add_cmd(Asm::INST_AND, ret, param[0], param[1]);
			break;
		case InlineID::CHAR_OR:
			add_cmd(Asm::INST_OR, ret, param[0], param[1]);
			break;
		case InlineID::BOOL_NEGATE:
			add_cmd(Asm::INST_XOR, ret, param[0], param_imm(TypeBool, 0x1));
			break;
		case InlineID::CHAR_NEGATE:
			add_cmd(Asm::INST_MOV, ret, param_imm(TypeChar, 0x0));
			add_cmd(Asm::INST_SUB, ret, ret, param[0]);
			break;
		default:
			do_error("unimplemented inline function: #" + i2s((int)index));
	}
}

bool const_is_arm_representable(int value)
{
	for (int ex=0; ex<=30; ex+=2){
		unsigned int mask = (0xffffff00 >> ex) | (0xffffff00 << (32-ex));
		if ((value & mask) == 0)
			return true;
	}
	return false;
}

int func_index(Function *f) {
	f->owner()->do_error("ARM func_index...");
	return -1;
}

// create data for a (function) parameter
//   and compile its command if the parameter is executable itself
SerialNodeParam SerializerARM::SerializeParameter(Node *link, Block *block, int index)
{
	SerialNodeParam p;
	p.kind = link->kind;
	p.type = link->type;
	p.p = 0;
	p.shift = 0;

	if (link->kind == NodeKind::FUNCTION_POINTER){
		p.p = (int_p)link->as_func_p();
		p.kind = NodeKind::MEMORY;
		if (!p.p){
			if (link->as_func()->owner() == syntax_tree){
				int index = func_index(link->as_func());
				p.p = index + 0xefef0000;
				script->function_vars_to_link.add(index);
			}else
				do_error_link("could not link function as variable: " + link->as_func()->long_name());
			//p.kind = Asm::PKLabel;
			//p.p = (char*)(long)list->add_label("_kaba_func_" + link->script->syntax->Functions[link->link_no]->name, false);
		}
		return param_deref_lookup(p.type, add_global_ref((void*)p.p));
	}else if (link->kind == NodeKind::MEMORY){
		return param_deref_lookup(p.type, add_global_ref((void*)p.p));
	}else if (link->kind == NodeKind::ADDRESS){
		return param_lookup(p.type, add_global_ref((void*)link->link_no));
	}else if (link->kind == NodeKind::VAR_GLOBAL){
		if (!link->as_global_p())
			script->do_error_link("variable is not linkable: " + link->as_global()->name);
		return param_deref_lookup(p.type, add_global_ref(link->as_global_p()));
	}else if (link->kind == NodeKind::VAR_LOCAL){
		p.p = link->as_local()->_offset;
		p.kind = NodeKind::LOCAL_MEMORY;
	}else if (link->kind == NodeKind::LOCAL_MEMORY){
		p.p = link->link_no;
	}else if (link->kind == NodeKind::LOCAL_ADDRESS){
		SerialNodeParam param = param_local(TypePointer, link->link_no);
		return AddReference(param, link->type);
	}else if (link->kind == NodeKind::CONSTANT){
		void *pp = link->as_const_p();
		int c = *(int*)pp;
		if (const_is_arm_representable(c)){
			p.p = c;
			p.kind = NodeKind::IMMEDIATE;
		}else{
			return param_lookup(p.type, add_global_ref(*(int**)pp));
		}
	}else if ((link->kind==NodeKind::OPERATOR) or (link->kind==NodeKind::FUNCTION_CALL) or (link->kind==NodeKind::VIRTUAL_CALL) or (link->kind==NodeKind::INLINE_CALL) or (link->kind == NodeKind::STATEMENT) or (link->kind==NodeKind::ARRAY_BUILDER)){
		return serialize_node(link, block, index);
	}else if (link->kind == NodeKind::REFERENCE){
		SerialNodeParam param = SerializeParameter(link->params[0], block, index);
		//printf("%d  -  %s\n",pk,Kind2Str(pk));
		return AddReference(param, link->type);
	}else if (link->kind == NodeKind::DEREFERENCE){
		SerialNodeParam param = SerializeParameter(link->params[0], block, index);
		/*if ((param.kind == KindVarLocal) or (param.kind == KindVarGlobal)){
			p.type = param.type->sub_type;
			if (param.kind == KindVarLocal)		p.kind = KindRefToLocal;
			if (param.kind == KindVarGlobal)	p.kind = KindRefToGlobal;
			p.p = param.p;
		}*/
		return AddDereference(param);
	}else if (link->kind == NodeKind::VAR_TEMP){
		// only used by <new> operator
		p.p = link->link_no;
	}else{
		do_error("unexpected type of parameter: " + kind2str(link->kind));
	}
	return p;
}


void SerializerARM::ProcessReferences()
{
	for (int i=0;i<cmd.num;i++)
		if (cmd[i].inst == Asm::INST_LEA){
			if (cmd[i].p[1].kind == NodeKind::LOCAL_ADDRESS){
				do_error("var local/local mem");
				SerialNodeParam p0 = cmd[i].p[0];
				SerialNodeParam p1 = cmd[i].p[1];
				int r = find_unused_reg(i, i, 4);
				remove_cmd(i);
				next_cmd_target(i);
				add_cmd(Asm::INST_ADD, param_vreg(TypePointer, r), param_preg(TypePointer, Asm::REG_R13), param_imm(TypeInt, p1.p));
				next_cmd_target(i + 1);
				add_cmd(Asm::INST_MOV, p0, param_vreg(TypePointer, r));
				set_virtual_reg(r, i, i+1);
			}else{
				do_error("reference in ARM: " + cmd[i].p[1].str(this));
			}
		}
}

void SerializerARM::ProcessDereferences()
{
	for (int i=0;i<cmd.num;i++)
		for (int j=0;j<SERIAL_NODE_NUM_PARAMS;j++)
			if ((cmd[i].p[j].kind == NodeKind::DEREF_LOCAL_MEMORY) or (cmd[i].p[j].kind == NodeKind::DEREF_VAR_TEMP)){
				SerialNodeParam p = cmd[i].p[j];
				SerialNodeParam rp = cmd[i].p[j];
				if (cmd[i].p[j].kind == NodeKind::DEREF_LOCAL_MEMORY)
					rp.kind = NodeKind::LOCAL_MEMORY;
				else
					rp.kind = NodeKind::VAR_TEMP;
				rp.type = p.type->get_pointer();
				int r = find_unused_reg(i, i, 4);
				next_cmd_target(i);
				add_cmd(Asm::INST_MOV, param_vreg(TypePointer, r), rp);
				set_cmd_param(cmd[i+1], j, param_deref_vreg(p.type, r));
				set_virtual_reg(r, i, i+1);
			}
}


inline bool _____arm_param_combi_allowed(int inst, SerialNodeParam &p1, SerialNodeParam &p2, SerialNodeParam &p3)
{
//	if (inst >= Asm::inst_marker)
//		return true;
	if (inst == Asm::INST_MOV)
		return (p1.kind == NodeKind::REGISTER) and (p2.kind == NodeKind::REGISTER);
	if (inst == Asm::INST_ADD)
		return (p1.kind == NodeKind::REGISTER) and (p2.kind == NodeKind::REGISTER) and (p3.kind == NodeKind::REGISTER);
	return true;
}

void SerializerARM::transfer_by_reg_in(SerialNode &c, int &i, int pno)
{
	SerialNodeParam p = c.p[pno];
	int r = find_unused_reg(i, i, /*p.type->size*/ 4);
	SerialNodeParam pr = param_vreg(p.type, r);
	set_virtual_reg(r, i, cmd.num);
	set_cmd_param(cmd[i], pno, pr);
	next_cmd_target(i);
	add_cmd(c.cond, Asm::INST_MOV, pr, p, p_none);
	i ++;
}

void SerializerARM::transfer_by_reg_out(SerialNode &c, int &i, int pno)
{
	SerialNodeParam p = c.p[pno];
	int r = find_unused_reg(i, i, 4);//p.type->size);
	SerialNodeParam pr = param_vreg(p.type, r);
	set_virtual_reg(r, i, cmd.num);
	set_cmd_param(c, pno, pr);
	next_cmd_target(i+1);
	add_cmd(c.cond, Asm::INST_MOV, p, pr, p_none);
}

void SerializerARM::gr_transfer_by_reg_in(SerialNode &c, int &i, int pno)
{
	SerialNodeParam p = c.p[pno];
	if (config.verbose)
		msg_write("in " + c.str(this));
	if (p.kind == NodeKind::DEREF_GLOBAL_LOOKUP){
		// cmd ..., [global ref]

		// mov r2, [ref]
		// ldr r1, [r2]
		// cmd ..., r1


		int r2 = find_unused_reg(i, i, 4);
		next_cmd_target(i);
		add_cmd(c.cond, Asm::INST_MOV, param_vreg(TypePointer, r2), param_deref_marker(TypePointer, global_refs[p.p].label), p_none);

		int r1 = find_unused_reg(i+1, i+1, 4/*p.type->size*/, virtual_reg[r2].reg_root);
		next_cmd_target(i+1);
		add_cmd(c.cond, Asm::INST_LDR, param_vreg(p.type, r1), param_deref_vreg(TypePointer, r2), p_none);

		set_cmd_param(cmd[i+2], pno, param_vreg(p.type, r1));

		set_virtual_reg(r2, i, i + 1);
		set_virtual_reg(r1, i + 1, i + 2);

		i += 2;
	}else{
		// cmd ..., global

		// mov r1, [ref]
		// cmd ..., r1


		int r1 = find_unused_reg(i, i, 4);
		next_cmd_target(i);
		add_cmd(c.cond, Asm::INST_MOV, param_vreg(TypePointer, r1), param_deref_marker(TypePointer, global_refs[p.p].label), p_none);
		set_virtual_reg(r1, i, i + 1);

		set_cmd_param(cmd[i+1], pno, param_vreg(p.type, r1));

		i += 1;
	}
}

void SerializerARM::gr_transfer_by_reg_out(SerialNode &c, int &i, int pno)
{
	SerialNodeParam p = c.p[pno];
	if (config.verbose)
		msg_write("out " + c.str(this));
	if (p.kind == NodeKind::DEREF_GLOBAL_LOOKUP){
		// cmd [global ref], ...

		// cmd r1, ...
		// mov r2, [ref]
		// str r1, [r2]


		int r2 = find_unused_reg(i, i, 4);
		next_cmd_target(i+1);
		add_cmd(c.cond, Asm::INST_MOV, param_vreg(TypePointer, r2), param_deref_marker(TypePointer, global_refs[p.p].label), p_none);

		int r1 = find_unused_reg(i, i+1, 4 /*p.type->size*/, virtual_reg[r2].reg_root);
		next_cmd_target(i+2);
		add_cmd(c.cond, Asm::INST_STR, param_vreg(p.type, r1), param_deref_vreg(TypePointer, r2), p_none);

		set_cmd_param(cmd[i], pno, param_vreg(p.type, r1));

		set_virtual_reg(r1, i, i + 2);
		set_virtual_reg(r2, i+1, i+2);
	}else{
		// cmd global, ...

		// cmd r1, ...
		// str r1, [ref]


		int r1 = find_unused_reg(i, i, 4);
		next_cmd_target(i+2);
		add_cmd(c.cond, Asm::INST_STR, param_vreg(TypePointer, r1), param_deref_marker(TypePointer, global_refs[p.p].label), p_none);
		set_virtual_reg(r1, i, i+1);

		set_cmd_param(cmd[i], pno, param_vreg(p.type, r1));
	}
}

inline bool is_data_op3(int inst)
{
	if (inst == Asm::INST_MOV)
		return false;
	if (inst == Asm::INST_MUL)
		return true;
	for (int i=0; i<16; i++)
		if (inst == Asm::ARM_DATA_INSTRUCTIONS[i])
			return true;
	return false;
}

inline bool is_data_op2(int inst)
{
	if (inst == Asm::INST_MOV)
		return true;
	if (inst == Asm::INST_CMP)
		return true;
	if (inst == Asm::INST_CMN)
		return true;
	if (inst == Asm::INST_TST)
		return true;
	if (inst == Asm::INST_TEQ)
		return true;
	return false;
}

inline bool is_global_lookup(SerialNodeParam &p)
{
	return (p.kind == NodeKind::DEREF_GLOBAL_LOOKUP) or (p.kind == NodeKind::GLOBAL_LOOKUP);
}

// create global lookup accesses
void SerializerARM::ConvertGlobalLookups()
{
	for (int i=cmd.num-1;i>=0;i--){
		if (cmd[i].inst == Asm::INST_MOV){
			if (is_global_lookup(cmd[i].p[1]))
				gr_transfer_by_reg_in(cmd[i], i, 1);
			if (is_global_lookup(cmd[i].p[0]))
				gr_transfer_by_reg_out(cmd[i], i, 0);
		}else if (is_data_op2(cmd[i].inst)){
			if (is_global_lookup(cmd[i].p[0]))
				gr_transfer_by_reg_in(cmd[i], i, 0);
			if (is_global_lookup(cmd[i].p[1]))
				gr_transfer_by_reg_in(cmd[i], i, 1);
		}else if (is_data_op3(cmd[i].inst)){
			if (is_global_lookup(cmd[i].p[1]))
				gr_transfer_by_reg_in(cmd[i], i, 1);
			if (is_global_lookup(cmd[i].p[2]))
				gr_transfer_by_reg_in(cmd[i], i, 2);
			if (is_global_lookup(cmd[i].p[0]))
				gr_transfer_by_reg_out(cmd[i], i, 0);
		}
	}
	ScanTempVarUsage();
}

// create local variable accesses
void SerializerARM::CorrectUnallowedParamCombis()
{
	for (int i=cmd.num-1;i>=0;i--){
		if (cmd[i].inst == Asm::INST_MOV){
			if ((cmd[i].p[0].kind != NodeKind::REGISTER) and (cmd[i].p[1].kind != NodeKind::REGISTER))
				transfer_by_reg_in(cmd[i], i, 1);
		}else if (is_data_op2(cmd[i].inst)){
			if (cmd[i].p[1].kind != NodeKind::REGISTER)
				transfer_by_reg_in(cmd[i], i, 1);
			if (cmd[i].p[0].kind != NodeKind::REGISTER)
				transfer_by_reg_in(cmd[i], i, 0);
		}else if (is_data_op3(cmd[i].inst)){
			if (cmd[i].p[1].kind != NodeKind::REGISTER)
				transfer_by_reg_in(cmd[i], i, 1);
			if (cmd[i].p[2].kind != NodeKind::REGISTER)
				transfer_by_reg_in(cmd[i], i, 2);
			if (cmd[i].p[0].kind != NodeKind::REGISTER)
				transfer_by_reg_out(cmd[i], i, 0);
		}
	}
	ScanTempVarUsage();
}

void SerializerARM::AddFunctionIntro(Function *f)
{
	// return, instance, params
	Array<Variable*> param;
	if (f->return_type->uses_return_by_memory()){
		for (Variable *v: f->var)
			if (v->name == IDENTIFIER_RETURN_VAR){
				param.add(v);
				break;
			}
	}
	if (!f->is_static){
		for (Variable *v: f->var)
			if (v->name == IDENTIFIER_SELF){
				param.add(v);
				break;
			}
	}
	for (int i=0;i<f->num_params;i++)
		param.add(f->var[i]);

	// map params...
	Array<Variable*> reg_param;
	Array<Variable*> stack_param;
	Array<Variable*> xmm_param;
	for (Variable *p: param){
		if ((p->type == TypeInt) or (p->type == TypeChar) or (p->type == TypeBool) or p->type->is_pointer()){
			if (reg_param.num < 4){
				reg_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else if (p->type == TypeFloat32){
			do_error("arm float param");
			if (xmm_param.num < 8){
				xmm_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else
			do_error("parameter type currently not supported: " + p->type->name);
	}

	// xmm0-7
	/*foreachib(Variable &p, xmm_param, i){
		int reg = Asm::REG_XMM0 + i;
		add_cmd(Asm::inst_movss, param_local(p->type, p->_offset), param_reg(p->type, reg));
	}*/

	// rdi, rsi,rdx, rcx, r8, r9
	int param_regs[4] = {Asm::REG_R0, Asm::REG_R1, Asm::REG_R2, Asm::REG_R3};
	foreachib(Variable *p, reg_param, i){
		int reg = add_virtual_reg(param_regs[i]);
		add_cmd(Asm::INST_MOV, param_local(p->type, p->_offset), param_vreg(p->type, reg));
		set_virtual_reg(reg, cmd.num - 1, cmd.num - 1);
	}

	// get parameters from stack
	foreachb(Variable *p, stack_param){
		do_error("func with stack...");
		/*int s = 8;
		add_cmd(Asm::inst_push, p);
		push_size += s;*/
	}
}

void SerializerARM::AddFunctionOutro(Function *f)
{
	// will be translated into a "real" outro later...
	add_cmd(Asm::INST_RET);
}


void SerializerARM::DoMapping()
{
	if (config.verbose and config.allow_output(cur_func, "map:a"))
		cmd_list_out("aaa");

	MapReferencedTempVarsToStack();

	if (config.verbose and config.allow_output(cur_func, "map:a"))
		cmd_list_out("post ref map");

	ProcessDereferences();
	ProcessReferences();

	// --- remove unnecessary temp vars

	TryMapTempVarsRegisters();

	MapRemainingTempVarsToStack();

	//ResolveDerefTempAndLocal();

	if (config.verbose and config.allow_output(cur_func, "map:b"))
		cmd_list_out("pre global");

	ConvertGlobalLookups();

	if (config.verbose and config.allow_output(cur_func, "map:c"))
		cmd_list_out("post global");

	CorrectUnallowedParamCombis();

	if (config.verbose and config.allow_output(cur_func, "map:d"))
		cmd_list_out("post unallowed");

	for (int i=0; i<cmd.num; i++)
		ConvertMemMovsToLdrStr(cmd[i]);

	ConvertGlobalRefs();

	if (config.verbose and config.allow_output(cur_func, "map:e"))
		cmd_list_out("end");
}

void SerializerARM::ConvertGlobalRefs()
{
	for (int i=0; i<cmd.num; i++){
		if ((cmd[i].inst == Asm::INST_LDR) and (cmd[i].p[0].kind == NodeKind::REGISTER) and (cmd[i].p[1].kind == NodeKind::DEREF_MARKER)){
			bool found = false;
			int64 data;
			for (GlobalRef &r: global_refs){
				if (r.label == cmd[i].p[1].p){
					data = (int_p)r.p;
					found = true;
					break;
				}
			}
			if (!found)
				continue;
			cmd[i].inst = Asm::INST_MOV;
			set_cmd_param(cmd[i], 1, param_imm(TypeInt, data & 0x000000ff));
			next_cmd_target(i + 1);
			add_cmd(Asm::INST_ADD, cmd[i].p[0], cmd[i].p[0], param_imm(TypeInt, data & 0x0000ff00));
			next_cmd_target(i + 2);
			add_cmd(Asm::INST_ADD, cmd[i].p[0], cmd[i].p[0], param_imm(TypeInt, data & 0x00ff0000));
			next_cmd_target(i + 3);
			add_cmd(Asm::INST_ADD, cmd[i].p[0], cmd[i].p[0], param_imm(TypeInt, data & 0xff000000));
			i += 3;
		}
	}
}

void SerializerARM::ConvertMemMovsToLdrStr(SerialNode &c)
{
	if (c.inst == Asm::INST_MOV){
		if ((c.p[0].kind == NodeKind::LOCAL_MEMORY) or (c.p[0].kind == NodeKind::DEREF_REGISTER)){
			if (c.p[0].type->size == 1)
				c.inst = Asm::INST_STRB;
			else
				c.inst = Asm::INST_STR;
			SerialNodeParam p = c.p[0];
			set_cmd_param(c, 0, c.p[1]);
			set_cmd_param(c, 1, p);
		}else if ((c.p[1].kind == NodeKind::LOCAL_MEMORY) or (c.p[1].kind == NodeKind::DEREF_REGISTER)){
			if (c.p[1].type->size == 1)
				c.inst = Asm::INST_LDRB;
			else
				c.inst = Asm::INST_LDR;
		}else if (c.p[1].kind == NodeKind::DEREF_MARKER){
			c.inst = Asm::INST_LDR;
		}
	}
}

void SerializerARM::CorrectReturn()
{
	for (int i=0;i<cmd.num;i++)
		if (cmd[i].inst == Asm::INST_RET){
			remove_cmd(i);
			if (stack_max_size > 0){
				next_cmd_target(i ++);
				add_cmd(Asm::INST_ADD, param_preg(TypePointer, Asm::REG_R13), param_preg(TypePointer, Asm::REG_R13), param_imm(TypeInt, stack_max_size));
			}
			next_cmd_target(i);
			add_cmd(Asm::INST_LDMIA, param_preg(TypePointer, Asm::REG_R13), param_imm(TypeInt, 0x8ff0));
		}
}

};
