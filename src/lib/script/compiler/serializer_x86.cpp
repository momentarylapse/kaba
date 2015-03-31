#include "../script.h"
#include "serializer_x86.h"
#include "../../file/file.h"



namespace Script{


extern int GlobalWaitingMode;
extern float GlobalTimeToWait;


int SerializerX86::fc_begin()
{
	Type *type = CompilerFunctionReturn.type;

	// return data too big... push address
	SerialCommandParam ret_ref;
	if (type->UsesReturnByMemory()){
		//add_temp(type, ret_temp);
		ret_ref = AddReference(/*ret_temp*/ CompilerFunctionReturn, TypePointer);
		//add_ref();
		//add_cmd(Asm::inst_lea, KindRegister, (char*)RegEaxCompilerFunctionReturn.kind, CompilerFunctionReturn.param);
	}

	// grow stack (down) for local variables of the calling function
//	add_cmd(- cur_func->_VarSize - LocalOffset - 8);
	long push_size = 0;

	// push parameters onto stack
	for (int p=CompilerFunctionParam.num-1;p>=0;p--){
		if (CompilerFunctionParam[p].type){
			int s = mem_align(CompilerFunctionParam[p].type->size, 4);
			for (int j=0;j<s/4;j++)
				add_cmd(Asm::inst_push, param_shift(CompilerFunctionParam[p], s - 4 - j * 4, TypeInt));
			push_size += s;
		}
	}

	if (config.abi == ABI_WINDOWS_32){
		// more than 4 byte have to be returned -> give return address as very last parameter!
		if (type->UsesReturnByMemory())
			add_cmd(Asm::inst_push, ret_ref); // nachtraegliche eSP-Korrektur macht die Funktion
	}

	// _cdecl: push class instance as first parameter
	if (CompilerFunctionInstance.type){
		add_cmd(Asm::inst_push, CompilerFunctionInstance);
		push_size += config.pointer_size;
	}
	
	if (config.abi == ABI_GNU_32){
		// more than 4 byte have to be returned -> give return address as very first parameter!
		if (type->UsesReturnByMemory())
			add_cmd(Asm::inst_push, ret_ref); // nachtraegliche eSP-Korrektur macht die Funktion
	}
	return push_size;
}

void SerializerX86::fc_end(int push_size)
{
	Type *type = CompilerFunctionReturn.type;

	if (push_size > 127)
		add_cmd(Asm::inst_add, param_reg(TypePointer, Asm::REG_ESP), param_const(TypeInt, push_size));
	else if (push_size > 0)
		add_cmd(Asm::inst_add, param_reg(TypePointer, Asm::REG_ESP), param_const(TypeChar, push_size));

	// return > 4b already got copied to [ret] by the function!
	if ((type != TypeVoid) && (!type->UsesReturnByMemory())){
		if (type == TypeFloat32)
			add_cmd(Asm::inst_fstp, CompilerFunctionReturn);
		else if (type->size == 1){
			add_cmd(Asm::inst_mov, CompilerFunctionReturn, p_al);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
		}else{
			add_cmd(Asm::inst_mov, CompilerFunctionReturn, p_eax);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
		}
	}
}

void SerializerX86::add_function_call(Script *script, int func_no)
{
	msg_db_f("AddFunctionCallX86", 4);

	int push_size = fc_begin();

	if ((script == this->script) and (!script->syntax->Functions[func_no]->is_extern)){
		add_cmd(Asm::inst_call, param_marker(list->get_label("_kaba_func_" + i2s(func_no))));
	}else{
		void *func = (void*)script->func[func_no];
		if (!func)
			DoErrorLink("could not link function " + script->syntax->Functions[func_no]->name);
		add_cmd(Asm::inst_call, param_const(TypePointer, (long)func)); // the actual call
		// function pointer will be shifted later...
	}

	fc_end(push_size);
}

void SerializerX86::add_virtual_function_call(int virtual_index)
{
	msg_db_f("AddFunctionCallX86", 4);

	int push_size = fc_begin();

	add_cmd(Asm::inst_mov, p_eax, CompilerFunctionInstance);
	add_cmd(Asm::inst_mov, p_eax, p_deref_eax);
	add_cmd(Asm::inst_add, p_eax, param_const(TypeInt, 4 * virtual_index));
	add_cmd(Asm::inst_mov, param_reg(TypePointer, Asm::REG_EDX), p_deref_eax);
	add_cmd(Asm::inst_call, param_reg(TypePointer, Asm::REG_EDX)); // the actual call

	fc_end(push_size);
}

// create data for a (function) parameter
//   and compile its command if the parameter is executable itself
SerialCommandParam SerializerX86::SerializeParameter(Command *link, int level, int index)
{
	msg_db_f("SerializeParameter", 4);
	SerialCommandParam p;
	p.kind = link->kind;
	p.type = link->type;
	p.p = 0;
	p.shift = 0;
	//Type *rt=link->;
	if (link->kind == KindVarFunction){
		p.p = (long)link->script->func[link->link_no];
		p.kind = KindVarGlobal;
		if (!p.p){
			if (link->script == script){
				p.p = link->link_no + 0xefef0000;
				script->function_vars_to_link.add(link->link_no);
			}else
				DoErrorLink("could not link function as variable: " + link->script->syntax->Functions[link->link_no]->name);
			//p.kind = Asm::PKLabel;
			//p.p = (char*)(long)list->add_label("_kaba_func_" + link->script->syntax->Functions[link->link_no]->name, false);
		}
	}else if (link->kind == KindMemory){
		p.p = link->link_no;
		p.kind = KindVarGlobal;
	}else if (link->kind == KindAddress){
		p.p = (long)&link->link_no;
		p.kind = KindRefToConst;
	}else if (link->kind == KindVarGlobal){
		p.p = (long)link->script->g_var[link->link_no];
		if (!p.p)
			script->DoErrorLink("variable is not linkable: " + link->script->syntax->RootOfAllEvil.var[link->link_no].name);
	}else if (link->kind == KindVarLocal){
		p.p = cur_func->var[link->link_no]._offset;
	}else if (link->kind == KindLocalMemory){
		p.p = link->link_no;
		p.kind = KindVarLocal;
	}else if (link->kind == KindLocalAddress){
		SerialCommandParam param = param_local(TypePointer, link->link_no);
		return AddReference(param, link->type);
	}else if (link->kind == KindConstant){
		if ((config.use_const_as_global_var) || (syntax_tree->FlagCompileOS))
			p.kind = KindVarGlobal;
		else
			p.kind = KindRefToConst;
		p.p = (long)link->script->cnst[link->link_no];
	}else if ((link->kind==KindOperator) || (link->kind==KindFunction) || (link->kind==KindVirtualFunction) || (link->kind==KindCompilerFunction) || (link->kind==KindArrayBuilder)){
		p = SerializeCommand(link, level, index);
	}else if (link->kind == KindReference){
		SerialCommandParam param = SerializeParameter(link->param[0], level, index);
		//printf("%d  -  %s\n",pk,Kind2Str(pk));
		return AddReference(param, link->type);
	}else if (link->kind == KindDereference){
		SerialCommandParam param = SerializeParameter(link->param[0], level, index);
		/*if ((param.kind == KindVarLocal) || (param.kind == KindVarGlobal)){
			p.type = param.type->sub_type;
			if (param.kind == KindVarLocal)		p.kind = KindRefToLocal;
			if (param.kind == KindVarGlobal)	p.kind = KindRefToGlobal;
			p.p = param.p;
		}*/
		return AddDereference(param);
	}else if (link->kind == KindVarTemp){
		// only used by <new> operator
		p.p = link->link_no;
	}else{
		DoError("unexpected type of parameter: " + Kind2Str(link->kind));
	}
	return p;
}


void SerializerX86::SerializeOperator(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret)
{
	msg_db_f("SerializeOperator", 4);
	switch(com->link_no){
		case OperatorIntAssign:
		case OperatorInt64Assign:
		case OperatorFloatAssign:
		case OperatorFloat64Assign:
		case OperatorPointerAssign:
			add_cmd(Asm::inst_mov, param[0], param[1]);
			break;
		case OperatorCharAssign:
		case OperatorBoolAssign:
			add_cmd(Asm::inst_mov, param[0], param[1]);
			break;
		case OperatorClassAssign:
			for (int i=0;i<signed(com->param[0]->type->size)/4;i++)
				add_cmd(Asm::inst_mov, param_shift(param[0], i * 4, TypeInt), param_shift(param[1], i * 4, TypeInt));
			for (int i=4*signed(com->param[0]->type->size/4);i<signed(com->param[0]->type->size);i++)
				add_cmd(Asm::inst_mov, param_shift(param[0], i, TypeChar), param_shift(param[1], i, TypeChar));
			break;
// int
		case OperatorIntAddS:
		case OperatorInt64AddS:
			add_cmd(Asm::inst_add, param[0], param[1]);
			break;
		case OperatorIntSubtractS:
		case OperatorInt64SubtractS:
			add_cmd(Asm::inst_sub, param[0], param[1]);
			break;
		case OperatorIntMultiplyS:
		case OperatorInt64MultiplyS:
			add_cmd(Asm::inst_imul, param[0], param[1]);
			break;
		case OperatorIntDivideS:
			add_cmd(Asm::inst_mov, p_eax_int, param[0]);
			add_cmd(Asm::inst_mov, param_reg(TypeInt, Asm::REG_EDX), p_eax_int);
			add_cmd(Asm::inst_sar, param_reg(TypeInt, Asm::REG_EDX), param_const(TypeChar, 0x1f));
			add_cmd(Asm::inst_idiv, p_eax_int, param[1]);
			add_cmd(Asm::inst_mov, param[0], p_eax_int);
			add_reg_channel(Asm::REG_EAX, cmd.num - 5, cmd.num - 1);
			add_reg_channel(Asm::REG_EDX, cmd.num - 2, cmd.num - 2);
			break;
		case OperatorInt64DivideS:
			add_cmd(Asm::inst_mov, p_rax, param[0]);
			add_cmd(Asm::inst_mov, param_reg(TypeInt64, Asm::REG_RDX), p_rax);
			add_cmd(Asm::inst_sar, param_reg(TypeInt64, Asm::REG_RDX), param_const(TypeChar, 0x1f));
			add_cmd(Asm::inst_idiv, p_rax, param[1]);
			add_cmd(Asm::inst_mov, param[0], p_rax);
			add_reg_channel(Asm::REG_RAX, cmd.num - 5, cmd.num - 1);
			add_reg_channel(Asm::REG_RDX, cmd.num - 2, cmd.num - 2);
			break;
		case OperatorIntAdd:
		case OperatorInt64Add:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_add, ret, param[1]);
			break;
		case OperatorIntSubtract:
		case OperatorInt64Subtract:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_sub, ret, param[1]);
			break;
		case OperatorIntMultiply:
			add_cmd(Asm::inst_mov, p_eax_int, param[0]);
			add_cmd(Asm::inst_imul, p_eax_int, param[1]);
			add_cmd(Asm::inst_mov, ret, p_eax_int);
			add_reg_channel(Asm::REG_EAX, cmd.num - 3, cmd.num - 1);
			break;
		case OperatorInt64Multiply:
			add_cmd(Asm::inst_mov, p_rax, param[0]);
			add_cmd(Asm::inst_imul, p_rax, param[1]);
			add_cmd(Asm::inst_mov, ret, p_rax);
			add_reg_channel(Asm::REG_RAX, cmd.num - 3, cmd.num - 1);
			break;
		case OperatorIntDivide:
			add_cmd(Asm::inst_mov, p_eax_int, param[0]);
			add_cmd(Asm::inst_mov, param_reg(TypeInt, Asm::REG_EDX), p_eax_int);
			add_cmd(Asm::inst_sar, param_reg(TypeInt, Asm::REG_EDX), param_const(TypeChar, 0x1f));
			add_cmd(Asm::inst_idiv, p_eax_int, param[1]);
			add_cmd(Asm::inst_mov, ret, p_eax_int);
			add_reg_channel(Asm::REG_EAX, cmd.num - 5, cmd.num - 1);
			add_reg_channel(Asm::REG_EDX, cmd.num - 2, cmd.num - 2);
			break;
		case OperatorInt64Divide:
			add_cmd(Asm::inst_mov, p_rax, param[0]);
			add_cmd(Asm::inst_mov, param_reg(TypeInt64, Asm::REG_RDX), p_rax);
			add_cmd(Asm::inst_sar, param_reg(TypeInt64, Asm::REG_RDX), param_const(TypeChar, 0x1f));
			add_cmd(Asm::inst_idiv, p_rax, param[1]);
			add_cmd(Asm::inst_mov, ret, p_rax);
			add_reg_channel(Asm::REG_RAX, cmd.num - 5, cmd.num - 1);
			add_reg_channel(Asm::REG_RDX, cmd.num - 2, cmd.num - 2);
			break;
		case OperatorIntModulo:
			add_cmd(Asm::inst_mov, p_eax_int, param[0]);
			add_cmd(Asm::inst_mov, param_reg(TypeInt, Asm::REG_EDX), p_eax_int);
			add_cmd(Asm::inst_sar, param_reg(TypeInt, Asm::REG_EDX), param_const(TypeChar, 0x1f));
			add_cmd(Asm::inst_idiv, p_eax_int, param[1]);
			add_cmd(Asm::inst_mov, ret, param_reg(TypeInt, Asm::REG_EDX));
			add_reg_channel(Asm::REG_EAX, cmd.num - 5, cmd.num - 2);
			add_reg_channel(Asm::REG_EDX, cmd.num - 2, cmd.num - 1);
			break;
		case OperatorInt64Modulo:
			add_cmd(Asm::inst_mov, p_rax, param[0]);
			add_cmd(Asm::inst_mov, param_reg(TypeInt64, Asm::REG_RDX), p_rax);
			add_cmd(Asm::inst_sar, param_reg(TypeInt64, Asm::REG_RDX), param_const(TypeChar, 0x1f));
			add_cmd(Asm::inst_idiv, p_rax, param[1]);
			add_cmd(Asm::inst_mov, ret, param_reg(TypeInt64, Asm::REG_RDX));
			add_reg_channel(Asm::REG_RAX, cmd.num - 5, cmd.num - 2);
			add_reg_channel(Asm::REG_RDX, cmd.num - 2, cmd.num - 1);
			break;
		case OperatorIntEqual:
		case OperatorIntNotEqual:
		case OperatorIntGreater:
		case OperatorIntGreaterEqual:
		case OperatorIntSmaller:
		case OperatorIntSmallerEqual:
		case OperatorInt64Equal:
		case OperatorInt64NotEqual:
		case OperatorInt64Greater:
		case OperatorInt64GreaterEqual:
		case OperatorInt64Smaller:
		case OperatorInt64SmallerEqual:
		case OperatorPointerEqual:
		case OperatorPointerNotEqual:
			add_cmd(Asm::inst_cmp, param[0], param[1]);
			if (com->link_no==OperatorIntEqual)			add_cmd(Asm::inst_setz, ret);
			if (com->link_no==OperatorIntNotEqual)		add_cmd(Asm::inst_setnz, ret);
			if (com->link_no==OperatorIntGreater)		add_cmd(Asm::inst_setnle, ret);
			if (com->link_no==OperatorIntGreaterEqual)	add_cmd(Asm::inst_setnl, ret);
			if (com->link_no==OperatorIntSmaller)		add_cmd(Asm::inst_setl, ret);
			if (com->link_no==OperatorIntSmallerEqual)	add_cmd(Asm::inst_setle, ret);
			if (com->link_no==OperatorInt64Equal)		add_cmd(Asm::inst_setz, ret);
			if (com->link_no==OperatorInt64NotEqual)	add_cmd(Asm::inst_setnz, ret);
			if (com->link_no==OperatorInt64Greater)		add_cmd(Asm::inst_setnle, ret);
			if (com->link_no==OperatorInt64GreaterEqual)add_cmd(Asm::inst_setnl, ret);
			if (com->link_no==OperatorInt64Smaller)		add_cmd(Asm::inst_setl, ret);
			if (com->link_no==OperatorInt64SmallerEqual)add_cmd(Asm::inst_setle, ret);
			if (com->link_no==OperatorPointerEqual)		add_cmd(Asm::inst_setz, ret);
			if (com->link_no==OperatorPointerNotEqual)	add_cmd(Asm::inst_setnz, ret);
			break;
		case OperatorIntBitAnd:
		case OperatorInt64BitAnd:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_and, ret, param[1]);
			break;
		case OperatorIntBitOr:
		case OperatorInt64BitOr:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_or, ret, param[1]);
			break;
		case OperatorIntShiftRight:
			add_cmd(Asm::inst_mov, param_reg(TypeInt, Asm::REG_ECX), param[1]);
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_shr, ret, param_reg(TypeChar, Asm::REG_CL));
			add_reg_channel(Asm::REG_ECX, cmd.num - 3, cmd.num - 1);
			break;
		case OperatorInt64ShiftRight:
			add_cmd(Asm::inst_mov, param_reg(TypeInt64, Asm::REG_RCX), param[1]);
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_shr, ret, param_reg(TypeChar, Asm::REG_CL));
			add_reg_channel(Asm::REG_RCX, cmd.num - 3, cmd.num - 1);
			break;
		case OperatorIntShiftLeft:
			add_cmd(Asm::inst_mov, param_reg(TypeInt, Asm::REG_ECX), param[1]);
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_shl, ret, param_reg(TypeChar, Asm::REG_CL));
			add_reg_channel(Asm::REG_ECX, cmd.num - 3, cmd.num - 1);
			break;
		case OperatorInt64ShiftLeft:
			add_cmd(Asm::inst_mov, param_reg(TypeInt64, Asm::REG_RCX), param[1]);
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_shl, ret, param_reg(TypeChar, Asm::REG_CL));
			add_reg_channel(Asm::REG_RCX, cmd.num - 3, cmd.num - 1);
			break;
		case OperatorIntNegate:
			add_cmd(Asm::inst_mov, ret, param_const(TypeInt, 0x0));
			add_cmd(Asm::inst_sub, ret, param[0]);
			break;
		case OperatorInt64Negate:
			add_cmd(Asm::inst_mov, ret, param_const(TypeInt64, 0x0));
			add_cmd(Asm::inst_sub, ret, param[0]);
			break;
		case OperatorIntIncrease:
			add_cmd(Asm::inst_add, param[0], param_const(TypeInt, 0x1));
			break;
		case OperatorInt64Increase:
			add_cmd(Asm::inst_add, param[0], param_const(TypeInt64, 0x1));
			break;
		case OperatorIntDecrease:
			add_cmd(Asm::inst_sub, param[0], param_const(TypeInt, 0x1));
			break;
		case OperatorInt64Decrease:
			add_cmd(Asm::inst_sub, param[0], param_const(TypeInt64, 0x1));
			break;
// float
		case OperatorFloatAddS:
		case OperatorFloatSubtractS:
		case OperatorFloatMultiplyS:
		case OperatorFloatDivideS:
		case OperatorFloat64AddS:
		case OperatorFloat64SubtractS:
		case OperatorFloat64MultiplyS:
		case OperatorFloat64DivideS:
			add_cmd(Asm::inst_fld, param[0]);
			if (com->link_no==OperatorFloatAddS)			add_cmd(Asm::inst_fadd, param[1]);
			if (com->link_no==OperatorFloatSubtractS)	add_cmd(Asm::inst_fsub, param[1]);
			if (com->link_no==OperatorFloatMultiplyS)	add_cmd(Asm::inst_fmul, param[1]);
			if (com->link_no==OperatorFloatDivideS)		add_cmd(Asm::inst_fdiv, param[1]);
			if (com->link_no==OperatorFloat64AddS)			add_cmd(Asm::inst_fadd, param[1]);
			if (com->link_no==OperatorFloat64SubtractS)	add_cmd(Asm::inst_fsub, param[1]);
			if (com->link_no==OperatorFloat64MultiplyS)	add_cmd(Asm::inst_fmul, param[1]);
			if (com->link_no==OperatorFloat64DivideS)		add_cmd(Asm::inst_fdiv, param[1]);
			add_cmd(Asm::inst_fstp, param[0]);
			break;
		case OperatorFloatAdd:
		case OperatorFloatSubtract:
		case OperatorFloatMultiply:
		case OperatorFloatDivide:
		case OperatorFloat64Add:
		case OperatorFloat64Subtract:
		case OperatorFloat64Multiply:
		case OperatorFloat64Divide:
			add_cmd(Asm::inst_fld, param[0]);
			if (com->link_no==OperatorFloatAdd)			add_cmd(Asm::inst_fadd, param[1]);
			if (com->link_no==OperatorFloatSubtract)		add_cmd(Asm::inst_fsub, param[1]);
			if (com->link_no==OperatorFloatMultiply)		add_cmd(Asm::inst_fmul, param[1]);
			if (com->link_no==OperatorFloatDivide)		add_cmd(Asm::inst_fdiv, param[1]);
			if (com->link_no==OperatorFloat64Add)			add_cmd(Asm::inst_fadd, param[1]);
			if (com->link_no==OperatorFloat64Subtract)		add_cmd(Asm::inst_fsub, param[1]);
			if (com->link_no==OperatorFloat64Multiply)		add_cmd(Asm::inst_fmul, param[1]);
			if (com->link_no==OperatorFloat64Divide)		add_cmd(Asm::inst_fdiv, param[1]);
			add_cmd(Asm::inst_fstp, ret);
			break;
		case OperatorFloatMultiplyFI:
		case OperatorFloat64MultiplyFI:
			add_cmd(Asm::inst_fild, param[1]);
			add_cmd(Asm::inst_fmul, param[0]);
			add_cmd(Asm::inst_fstp, ret);
			break;
		case OperatorFloatMultiplyIF:
		case OperatorFloat64MultiplyIF:
			add_cmd(Asm::inst_fild, param[0]);
			add_cmd(Asm::inst_fmul, param[1]);
			add_cmd(Asm::inst_fstp, ret);
			break;
		case OperatorFloatEqual:
		case OperatorFloat64Equal:
			add_cmd(Asm::inst_fld, param[0]);
			add_cmd(Asm::inst_fld, param[1]);
			add_cmd(Asm::inst_fucompp, p_st0, p_st1);
			add_cmd(Asm::inst_fnstsw, p_ax);
			add_cmd(Asm::inst_and, p_ah, param_const(TypeChar, 0x45));
			add_cmd(Asm::inst_cmp, p_ah, param_const(TypeChar, 0x40));
			add_cmd(Asm::inst_setz, ret);
			add_reg_channel(Asm::REG_EAX, cmd.num - 4, cmd.num - 2);
			break;
		case OperatorFloatNotEqual:
		case OperatorFloat64NotEqual:
			add_cmd(Asm::inst_fld, param[0]);
			add_cmd(Asm::inst_fld, param[1]);
			add_cmd(Asm::inst_fucompp, p_st0, p_st1);
			add_cmd(Asm::inst_fnstsw, p_ax);
			add_cmd(Asm::inst_and, p_ah, param_const(TypeChar, 0x45));
			add_cmd(Asm::inst_cmp, p_ah, param_const(TypeChar, 0x40));
			add_cmd(Asm::inst_setnz, ret);
			add_reg_channel(Asm::REG_EAX, cmd.num - 4, cmd.num - 2);
			break;
		case OperatorFloatGreater:
		case OperatorFloat64Greater:
			add_cmd(Asm::inst_fld, param[1]);
			add_cmd(Asm::inst_fld, param[0]);
			add_cmd(Asm::inst_fucompp, p_st0, p_st1);
			add_cmd(Asm::inst_fnstsw, p_ax);
			add_cmd(Asm::inst_test, p_ah, param_const(TypeChar, 0x45));
			add_cmd(Asm::inst_setz, ret);
			add_reg_channel(Asm::REG_EAX, cmd.num - 3, cmd.num - 2);
			break;
		case OperatorFloatGreaterEqual:
		case OperatorFloat64GreaterEqual:
			add_cmd(Asm::inst_fld, param[1]);
			add_cmd(Asm::inst_fld, param[0]);
			add_cmd(Asm::inst_fucompp, p_st0, p_st1);
			add_cmd(Asm::inst_fnstsw, p_ax);
			add_cmd(Asm::inst_test, p_ah, param_const(TypeChar, 0x05));
			add_cmd(Asm::inst_setz, ret);
			add_reg_channel(Asm::REG_EAX, cmd.num - 3, cmd.num - 2);
			break;
		case OperatorFloatSmaller:
		case OperatorFloat64Smaller:
			add_cmd(Asm::inst_fld, param[0]);
			add_cmd(Asm::inst_fld, param[1]);
			add_cmd(Asm::inst_fucompp, p_st0, p_st1);
			add_cmd(Asm::inst_fnstsw, p_ax);
			add_cmd(Asm::inst_test, p_ah, param_const(TypeChar, 0x45));
			add_cmd(Asm::inst_setz, ret);
			add_reg_channel(Asm::REG_EAX, cmd.num - 3, cmd.num - 2);
			break;
		case OperatorFloatSmallerEqual:
		case OperatorFloat64SmallerEqual:
			add_cmd(Asm::inst_fld, param[0]);
			add_cmd(Asm::inst_fld, param[1]);
			add_cmd(Asm::inst_fucompp, p_st0, p_st1);
			add_cmd(Asm::inst_fnstsw, p_ax);
			add_cmd(Asm::inst_test, p_ah, param_const(TypeChar, 0x05));
			add_cmd(Asm::inst_setz, ret);
			add_reg_channel(Asm::REG_EAX, cmd.num - 3, cmd.num - 2);
			break;
		case OperatorFloatNegate:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_xor, ret, param_const(TypeInt, 0x80000000));
			break;
// complex
		case OperatorComplexAddS:
		case OperatorComplexSubtractS:
		//case OperatorComplexMultiplySCF:
		//case OperatorComplexDivideS:
			add_cmd(Asm::inst_fld, param_shift(param[0], 0, TypeFloat32));
			if (com->link_no == OperatorComplexAddS)			add_cmd(Asm::inst_fadd, param_shift(param[1], 0, TypeFloat32));
			if (com->link_no == OperatorComplexSubtractS)	add_cmd(Asm::inst_fsub, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::inst_fld, param_shift(param[0], 4, TypeFloat32));
			if (com->link_no == OperatorComplexAddS)			add_cmd(Asm::inst_fadd, param_shift(param[1], 4, TypeFloat32));
			if (com->link_no == OperatorComplexSubtractS)	add_cmd(Asm::inst_fsub, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(param[0], 4, TypeFloat32));
			break;
		case OperatorComplexAdd:
		case OperatorComplexSubtract:
//		case OperatorFloatMultiply:
//		case OperatorFloatDivide:
			add_cmd(Asm::inst_fld, param_shift(param[0], 0, TypeFloat32));
			if (com->link_no == OperatorComplexAdd)		add_cmd(Asm::inst_fadd, param_shift(param[1], 0, TypeFloat32));
			if (com->link_no == OperatorComplexSubtract)	add_cmd(Asm::inst_fsub, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(ret, 0, TypeFloat32));
			add_cmd(Asm::inst_fld, param_shift(param[0], 4, TypeFloat32));
			if (com->link_no == OperatorComplexAdd)		add_cmd(Asm::inst_fadd, param_shift(param[1], 4, TypeFloat32));
			if (com->link_no == OperatorComplexSubtract)	add_cmd(Asm::inst_fsub, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(ret, 4, TypeFloat32));
			break;
		case OperatorComplexMultiply:
			// r.x = a.y * b.y
			add_cmd(Asm::inst_fld, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::inst_fmul, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(ret, 0, TypeFloat32));
			// r.x = a.x * b.x - r.x
			add_cmd(Asm::inst_fld, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::inst_fmul, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::inst_fsub, param_shift(ret, 0, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(ret, 0, TypeFloat32));
			// r.y = a.y * b.x
			add_cmd(Asm::inst_fld, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::inst_fmul, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(ret, 4, TypeFloat32));
			// r.y += a.x * b.y
			add_cmd(Asm::inst_fld, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::inst_fmul, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::inst_fadd, param_shift(ret, 4, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(ret, 4, TypeFloat32));
			break;
		case OperatorComplexMultiplyFC:
			add_cmd(Asm::inst_fld, param[0]);
			add_cmd(Asm::inst_fmul, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(ret, 0, TypeFloat32));
			add_cmd(Asm::inst_fld, param[0]);
			add_cmd(Asm::inst_fmul, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::inst_fstp, param_shift(ret, 4, TypeFloat32));
			break;
		case OperatorComplexMultiplyCF:
			add_cmd(Asm::inst_fld, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::inst_fmul, param[1]);
			add_cmd(Asm::inst_fstp, param_shift(ret, 0, TypeFloat32));
			add_cmd(Asm::inst_fld, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::inst_fmul, param[1]);
			add_cmd(Asm::inst_fstp, param_shift(ret, 4, TypeFloat32));
			break;
		case OperatorComplexEqual:
			add_cmd(Asm::inst_fld, param_shift(param[0], 0, TypeFloat32));
			add_cmd(Asm::inst_fld, param_shift(param[1], 0, TypeFloat32));
			add_cmd(Asm::inst_fucompp, p_st0, p_st1);
			add_cmd(Asm::inst_fnstsw, p_ax);
			add_cmd(Asm::inst_and, p_ah, param_const(TypeChar, 0x45));
			add_cmd(Asm::inst_cmp, p_ah, param_const(TypeChar, 0x40));
			add_cmd(Asm::inst_setz, ret);
			add_reg_channel(Asm::REG_EAX, cmd.num - 4, cmd.num - 2);
			add_cmd(Asm::inst_fld, param_shift(param[0], 4, TypeFloat32));
			add_cmd(Asm::inst_fld, param_shift(param[1], 4, TypeFloat32));
			add_cmd(Asm::inst_fucompp, p_st0, p_st1);
			add_cmd(Asm::inst_fnstsw, p_ax);
			add_cmd(Asm::inst_and, p_ah, param_const(TypeChar, 0x45));
			add_cmd(Asm::inst_cmp, p_ah, param_const(TypeChar, 0x40));
			add_cmd(Asm::inst_setz, p_ah);
			add_cmd(Asm::inst_and, ret, p_ah);
			add_reg_channel(Asm::REG_EAX, cmd.num - 5, cmd.num - 1);
			break;
// bool/char
		case OperatorCharEqual:
		case OperatorCharNotEqual:
		case OperatorBoolEqual:
		case OperatorBoolNotEqual:
		case OperatorBoolGreater:
		case OperatorBoolGreaterEqual:
		case OperatorBoolSmaller:
		case OperatorBoolSmallerEqual:
			add_cmd(Asm::inst_cmp, param[0], param[1]);
			if ((com->link_no == OperatorCharEqual) || (com->link_no == OperatorBoolEqual))
				add_cmd(Asm::inst_setz, ret);
			else if ((com->link_no ==OperatorCharNotEqual) || (com->link_no == OperatorBoolNotEqual))
				add_cmd(Asm::inst_setnz, ret);
			else if (com->link_no == OperatorBoolGreater)		add_cmd(Asm::inst_setnle, ret);
			else if (com->link_no == OperatorBoolGreaterEqual)	add_cmd(Asm::inst_setnl, ret);
			else if (com->link_no == OperatorBoolSmaller)		add_cmd(Asm::inst_setl, ret);
			else if (com->link_no == OperatorBoolSmallerEqual)	add_cmd(Asm::inst_setle, ret);
			break;
		case OperatorBoolAnd:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_and, ret, param[1]);
			break;
		case OperatorBoolOr:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_or, ret, param[1]);
			break;
		case OperatorCharAddS:
			add_cmd(Asm::inst_add, param[0], param[1]);
			break;
		case OperatorCharSubtractS:
			add_cmd(Asm::inst_sub, param[0], param[1]);
			break;
		case OperatorCharAdd:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_add, ret, param[1]);
			break;
		case OperatorCharSubtract:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_sub, ret, param[1]);
			break;
		case OperatorCharBitAnd:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_and, ret, param[1]);
			break;
		case OperatorCharBitOr:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_or, ret, param[1]);
			break;
		case OperatorBoolNegate:
			add_cmd(Asm::inst_mov, ret, param[0]);
			add_cmd(Asm::inst_xor, ret, param_const(TypeBool, 0x1));
			break;
		case OperatorCharNegate:
			add_cmd(Asm::inst_mov, ret, param_const(TypeChar, 0x0));
			add_cmd(Asm::inst_sub, ret, param[0]);
			break;
// vector
		case OperatorVectorAddS:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_fld, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fadd, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fstp, param_shift(param[0], i * 4, TypeFloat32));
			}
			break;
		case OperatorVectorMultiplyS:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_fld, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fmul, param[1]);
				add_cmd(Asm::inst_fstp, param_shift(param[0], i * 4, TypeFloat32));
			}
			break;
		case OperatorVectorDivideS:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_fld, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fdiv, param[1]);
				add_cmd(Asm::inst_fstp, param_shift(param[0], i * 4, TypeFloat32));
			}
			break;
		case OperatorVectorSubtractS:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_fld, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fsub, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fstp, param_shift(param[0], i * 4, TypeFloat32));
			}
			break;
		case OperatorVectorAdd:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_fld, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fadd, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fstp, param_shift(ret, i * 4, TypeFloat32));
			}
			break;
		case OperatorVectorSubtract:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_fld, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fsub, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fstp, param_shift(ret, i * 4, TypeFloat32));
			}
			break;
		case OperatorVectorMultiplyVF:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_fld, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fmul, param[1]);
				add_cmd(Asm::inst_fstp, param_shift(ret, i * 4, TypeFloat32));
			}
			break;
		case OperatorVectorMultiplyFV:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_fld, param[0]);
				add_cmd(Asm::inst_fmul, param_shift(param[1], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fstp, param_shift(ret, i * 4, TypeFloat32));
			}
			break;
		case OperatorVectorDivideVF:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_fld, param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::inst_fdiv, param[1]);
				add_cmd(Asm::inst_fstp, param_shift(ret, i * 4, TypeFloat32));
			}
			break;
		case OperatorVectorNegate:
			for (int i=0;i<3;i++){
				add_cmd(Asm::inst_mov, param_shift(ret, i * 4, TypeFloat32), param_shift(param[0], i * 4, TypeFloat32));
				add_cmd(Asm::inst_xor, param_shift(ret, i * 4, TypeFloat32), param_const(TypeInt, 0x80000000));
			}
			break;
		default:
			DoError("unimplemented operator: " + PreOperators[com->link_no].str());
	}
}

void SerializerX86::SerializeCompilerFunction(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret, int level, int index, int marker_before_params)
{
	switch(com->link_no){
		/*case CommandSine:
			break;*/
		case CommandIf:{
			// cmp;  jz m;  -block-  m;
			add_cmd(Asm::inst_cmp, param[0], param_const(TypeBool, 0x0));
			int m_after_true = add_marker_after_command(level, index + 1);
			add_cmd(Asm::inst_jz, param_marker(m_after_true));
			}break;
		case CommandIfElse:{
			// cmp;  jz m1;  -block-  jmp m2;  m1;  -block-  m2;
			add_cmd(Asm::inst_cmp, param[0], param_const(TypeBool, 0x0));
			int m_after_true = add_marker_after_command(level, index + 1);
			int m_after_false = add_marker_after_command(level, index + 2);
			add_cmd(Asm::inst_jz, param_marker(m_after_true)); // jz ...
			add_jump_after_command(level, index + 1, m_after_false); // insert before <m_after_true> is inserted!
			}break;
		case CommandWhile:
		case CommandFor:{
			// m1;  cmp;  jz m2;  -block-             jmp m1;  m2;     (while)
			// m1;  cmp;  jz m2;  -block-  m3;  i++;  jmp m1;  m2;     (for)
			add_cmd(Asm::inst_cmp, param[0], param_const(TypeBool, 0x0));
			int marker_after_while = add_marker_after_command(level, index + 1);
			add_cmd(Asm::inst_jz, param_marker(marker_after_while));
			add_jump_after_command(level, index + 1, marker_before_params); // insert before <marker_after_while> is inserted!

			int marker_continue = marker_before_params;
			if (com->link_no == CommandFor){
				// NextCommand is a block!
				if (next_command->kind != KindBlock)
					DoError("command block in \"for\" loop missing");
				marker_continue = add_marker_after_command(level + 1, next_command->block()->command.num - 2);
			}
			LoopData l = {marker_continue, marker_after_while, level, index};
			loop.add(l);
			}break;
		case CommandBreak:
			add_cmd(Asm::inst_jmp, param_marker(loop.back().marker_break));
			break;
		case CommandContinue:
			add_cmd(Asm::inst_jmp, param_marker(loop.back().marker_continue));
			break;
		case CommandReturn:
			if (com->num_params > 0){
				if (cur_func->return_type->UsesReturnByMemory()){ // we already got a return address in [ebp+0x08] (> 4 byte)
					FillInDestructors(false);
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
					SerialCommandParam p_edx = param_reg(TypeReg32, Asm::REG_EDX), p_deref_edx;
					SerialCommandParam p_ret_addr;
					p_ret_addr.kind = KindVarLocal;
					p_ret_addr.type = TypeReg32;
					p_ret_addr.p = (char*)0x8;
					p_ret_addr.shift = 0;
					int c_0 = cmd.num;
					add_cmd(Asm::inst_mov, p_edx, p_ret_addr);
					AddDereference(p_edx, p_deref_edx, TypeReg32);
					for (int j=0;j<s/4;j++)
						add_cmd(Asm::inst_mov, param_shift(p_deref_edx, j * 4, TypeInt), param_shift(param[0], j * 4, TypeInt));
					add_reg_channel(Asm::REG_EDX, c_0, cmd.num - 1);
#endif

					AddFunctionOutro(cur_func);
				}else{ // store return directly in eax / fpu stack (4 byte)
					SerialCommandParam t;
					add_temp(cur_func->return_type, t);
					add_cmd(Asm::inst_mov, t, param[0]);
					FillInDestructors(false);
					if (cur_func->return_type == TypeFloat32){
						if (config.instruction_set == Asm::INSTRUCTION_SET_AMD64)
							add_cmd(Asm::inst_movss, param_reg(TypeReg128, Asm::REG_XMM0), t);
						else
							add_cmd(Asm::inst_fld, t);
					}else if (cur_func->return_type->size == 1){
						add_reg_channel(Asm::REG_EAX, cmd.num, cmd.num);
						add_cmd(Asm::inst_mov, param_reg(cur_func->return_type, Asm::REG_AL), t);
					}else if (cur_func->return_type->size == 8){
						add_reg_channel(Asm::REG_EAX, cmd.num, cmd.num);
						add_cmd(Asm::inst_mov, param_reg(cur_func->return_type, Asm::REG_RAX), t);
					}else{
						add_reg_channel(Asm::REG_EAX, cmd.num, cmd.num);
						add_cmd(Asm::inst_mov, param_reg(cur_func->return_type, Asm::REG_EAX), t);
					}
					AddFunctionOutro(cur_func);
				}
			}else{
				FillInDestructors(false);
				AddFunctionOutro(cur_func);
			}
			break;
		case CommandNew:
			AddFuncParam(param_const(TypeInt, ret.type->parent->size));
			AddFuncReturn(ret);
			if (!syntax_tree->GetExistence("@malloc", cur_func))
				DoError("@malloc not found????");
			AddFunctionCall(syntax_tree->GetExistenceLink.script, syntax_tree->GetExistenceLink.link_no);
			if (com->param[0]){
				// copy + edit command
				Command sub = *com->param[0];
				Command c_ret(KindVarTemp, (long)ret.p, script, ret.type);
				sub.instance = &c_ret;
				SerializeCommand(&sub, level, index);
			}else
				add_cmd_constructor(ret, -1);
			break;
		case CommandDelete:
			add_cmd_destructor(param[0], false);
			AddFuncParam(param[0]);
			if (!syntax_tree->GetExistence("@free", cur_func))
				DoError("@free not found????");
			AddFunctionCall(syntax_tree->GetExistenceLink.script, syntax_tree->GetExistenceLink.link_no);
			break;
		case CommandWaitOneFrame:
		case CommandWait:
		case CommandWaitRT:{
			DoError("wait commands are deprecated");
				// set waiting state
					// GlobalWaitingMode = mode
					// GlobalWaitingTime = time
					SerialCommandParam p_mode = param_global(TypeInt, &GlobalWaitingMode);
					SerialCommandParam p_ttw = param_global(TypeFloat32, &GlobalTimeToWait);
					if (com->link_no == CommandWaitOneFrame){
						add_cmd(Asm::inst_mov, p_mode, param_const(TypeInt, WaitingModeRT));
						add_cmd(Asm::inst_mov, p_ttw, param_const(TypeFloat32, 0));
					}else if (com->link_no == CommandWait){
						add_cmd(Asm::inst_mov, p_mode, param_const(TypeInt, WaitingModeGT));
						add_cmd(Asm::inst_mov, p_ttw, param[0]);
					}else if (com->link_no == CommandWaitRT){
						add_cmd(Asm::inst_mov, p_mode, param_const(TypeInt, WaitingModeRT));
						add_cmd(Asm::inst_mov, p_ttw, param[0]);
					}
					if (config.instruction_set == Asm::INSTRUCTION_SET_AMD64){
						SerialCommandParam p_deref_rax;
						p_deref_rax.kind = KindDerefRegister;
						p_deref_rax.p = Asm::REG_RAX;
						p_deref_rax.type = TypePointer;
						p_deref_rax.shift = 0;
					
				// save script state
					// stack[-16] = rbp
					// stack[-24] = rsp
					// stack[-32] = rip
					add_cmd(Asm::inst_mov, p_rax, param_const(TypePointer, (long)&script->Stack[config.stack_size-16]));
					add_cmd(Asm::inst_mov, p_deref_rax, param_reg(TypeReg64, Asm::REG_RBP));
					add_cmd(Asm::inst_mov, p_rax, param_const(TypePointer, (long)&script->Stack[config.stack_size-24]));
					add_cmd(Asm::inst_mov, p_deref_rax, param_reg(TypeReg64, Asm::REG_RSP));
					add_cmd(Asm::inst_mov, param_reg(TypeReg64, Asm::REG_RSP), param_const(TypePointer, (long)&script->Stack[config.stack_size-24]));
					add_cmd(Asm::inst_call, param_const(TypePointer, 0)); // push rip
				// load return
					// mov rsp, &stack[-8]
					// pop rsp
					// mov rbp, rsp
					// leave
					// ret
					add_cmd(Asm::inst_mov, param_reg(TypeReg64, Asm::REG_RSP), param_const(TypePointer, (long)&script->Stack[config.stack_size-8])); // start of the script stack
					add_cmd(Asm::inst_pop, param_reg(TypeReg64, Asm::REG_RSP)); // old stackpointer (real program)
					add_cmd(Asm::inst_mov, param_reg(TypeReg64, Asm::REG_RBP), param_reg(TypeReg64, Asm::REG_RSP));
					add_cmd(Asm::inst_leave);
					add_cmd(Asm::inst_ret);
				// here comes the "waiting"...

				// reload script state (rip already loaded)
					// rbp = &stack[-16]
					// rsp = &stack[-24]
					// GlobalWaitingMode = WaitingModeNone
					add_cmd(Asm::inst_mov, p_rax, param_const(TypePointer, (long)&script->Stack[config.stack_size-16]));
					add_cmd(Asm::inst_mov, param_reg(TypeReg64, Asm::REG_RBP), p_deref_rax);
					add_cmd(Asm::inst_mov, p_rax, param_const(TypePointer, (long)&script->Stack[config.stack_size-24]));
					add_cmd(Asm::inst_mov, param_reg(TypeReg64, Asm::REG_RSP), p_deref_rax);
					add_cmd(Asm::inst_mov, p_mode, param_const(TypeInt, WaitingModeNone));

					}else{

						// save script state
							// stack[ -8] = ebp
							// stack[-12] = esp
							// stack[-16] = eip
							add_cmd(Asm::inst_mov, p_eax, param_const(TypePointer, (long)&script->Stack[config.stack_size-8]));
							add_cmd(Asm::inst_mov, p_deref_eax, param_reg(TypeReg32, Asm::REG_EBP));
							add_cmd(Asm::inst_mov, p_eax, param_const(TypePointer, (long)&script->Stack[config.stack_size-12]));
							add_cmd(Asm::inst_mov, p_deref_eax, param_reg(TypeReg32, Asm::REG_ESP));
							add_cmd(Asm::inst_mov, param_reg(TypeReg32, Asm::REG_ESP), param_const(TypePointer, (long)&script->Stack[config.stack_size-12]));
							add_cmd(Asm::inst_call, param_const(TypePointer, 0)); // push eip
						// load return
							// mov esp, &stack[-4]
							// pop esp
							// mov ebp, esp
							// leave
							// ret
							add_cmd(Asm::inst_mov, param_reg(TypeReg32, Asm::REG_ESP), param_const(TypePointer, (long)&script->Stack[config.stack_size-4])); // start of the script stack
							add_cmd(Asm::inst_pop, param_reg(TypeReg32, Asm::REG_ESP)); // old stackpointer (real program)
							add_cmd(Asm::inst_mov, param_reg(TypeReg32, Asm::REG_EBP), param_reg(TypeReg32, Asm::REG_ESP));
							add_cmd(Asm::inst_leave);
							add_cmd(Asm::inst_ret);
						// here comes the "waiting"...

						// reload script state (eip already loaded)
							// ebp = &stack[-8]
							// esp = &stack[-12]
							// GlobalWaitingMode = WaitingModeNone
							add_cmd(Asm::inst_mov, p_eax, param_const(TypePointer, (long)&script->Stack[config.stack_size-8]));
							add_cmd(Asm::inst_mov, param_reg(TypeReg32, Asm::REG_EBP), p_deref_eax);
							add_cmd(Asm::inst_mov, p_eax, param_const(TypePointer, (long)&script->Stack[config.stack_size-12]));
							add_cmd(Asm::inst_mov, param_reg(TypeReg32, Asm::REG_ESP), p_deref_eax);
							add_cmd(Asm::inst_mov, p_mode, param_const(TypeInt, WaitingModeNone));
					}
					}break;
		case CommandInlineIntToFloat:
			add_cmd(Asm::inst_fild, param[0]);
			add_cmd(Asm::inst_fstp, ret);
			break;
		case CommandInlineFloatToInt:
			// round to nearest...
			//add_cmd(Asm::inst_fld, param[0]);
			//add_cmd(Asm::inst_fistp, ret);

			// round to zero...
			SerialCommandParam t1, t2;
			add_temp(TypeReg16, t1);
			add_temp(TypeInt, t2);
			add_cmd(Asm::inst_fld, param[0]);
			add_cmd(Asm::inst_fnstcw, t1);
			add_cmd(Asm::inst_movzx, p_eax, t1);
			add_cmd(Asm::inst_mov, p_ah, param_const(TypeChar, 0x0c));
			add_cmd(Asm::inst_mov, t2, p_eax);
			add_reg_channel(Asm::REG_EAX, cmd.num - 3, cmd.num - 1);
			add_cmd(Asm::inst_fldcw, param_shift(t2, 0, TypeReg16));
			add_cmd(Asm::inst_fistp, ret);
			add_cmd(Asm::inst_fldcw, t1);
			break;
		case CommandInlineIntToChar:
			add_cmd(Asm::inst_mov, p_eax_int, param[0]);
			add_cmd(Asm::inst_mov, ret, p_al_char);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
			break;
		case CommandInlineCharToInt:
			add_cmd(Asm::inst_mov, p_eax_int, param_const(TypeInt, 0x0));
			add_cmd(Asm::inst_mov, p_al_char, param[0]);
			add_cmd(Asm::inst_mov, ret, p_eax);
			add_reg_channel(Asm::REG_EAX, cmd.num - 3, cmd.num - 1);
			break;
		case CommandInlinePointerToBool:
			add_cmd(Asm::inst_cmp, param[0], param_const(TypePointer, 0));
			add_cmd(Asm::inst_setnz, ret);
			break;
		case CommandAsm:
			add_cmd(inst_asm);
			break;
		case CommandInlineRectSet:
			add_cmd(Asm::inst_mov, param_shift(ret, 12, TypeFloat32), param[3]);
		case CommandInlineVectorSet:
			add_cmd(Asm::inst_mov, param_shift(ret, 8, TypeFloat32), param[2]);
		case CommandInlineComplexSet:
			add_cmd(Asm::inst_mov, param_shift(ret, 4, TypeFloat32), param[1]);
			add_cmd(Asm::inst_mov, param_shift(ret, 0, TypeFloat32), param[0]);
			break;
		case CommandInlineColorSet:
			add_cmd(Asm::inst_mov, param_shift(ret, 12, TypeFloat32), param[0]);
			add_cmd(Asm::inst_mov, param_shift(ret, 0, TypeFloat32), param[1]);
			add_cmd(Asm::inst_mov, param_shift(ret, 4, TypeFloat32), param[2]);
			add_cmd(Asm::inst_mov, param_shift(ret, 8, TypeFloat32), param[3]);
			break;
		default:
			DoError("compiler function unimplemented: " + PreCommands[com->link_no].name);
	}
}

inline bool param_is_simple(SerialCommandParam &p)
{
	return ((p.kind == KindRegister) || (p.kind == KindVarTemp) || (p.kind < 0));
}

inline bool param_combi_allowed(int inst, SerialCommandParam &p1, SerialCommandParam &p2)
{
//	if (inst >= Asm::inst_marker)
//		return true;
	if ((!param_is_simple(p1)) && (!param_is_simple(p2)))
		return false;
	bool r1, w1, r2, w2;
	Asm::GetInstructionParamFlags(inst, r1, w1, r2, w2);
	if ((w1) && (p1.kind == KindConstant))
		return false;
	if ((w2) && (p2.kind == KindConstant))
		return false;
	if ((p1.kind == KindConstant) || (p2.kind == KindConstant))
		if (!Asm::GetInstructionAllowConst(inst))
			return false;
	return true;
}

// mov [0x..] [0x...]  ->  mov temp, [0x..]   mov [0x..] temp
/*void CorrectUnallowedParamCombis()
{
	msg_db_f("CorrectCombis", 3);
	for (int i=cmd.num-1;i>=0;i--)
		if (!param_combi_allowed(cmd[i].inst, cmd[i].p[0], cmd[i].p[1])){
			msg_write(string2("correcting param combi  cmd=%d", i));
			bool mov_first_param = (cmd[i].p[1].kind < 0) || (cmd[i].p[0].kind == KindRefToConst) || (cmd[i].p[0].kind == KindConstant);
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
void SerializerX86::CorrectUnallowedParamCombis()
{
	msg_db_f("CorrectCombis", 3);
	for (int i=cmd.num-1;i>=0;i--){
		if (cmd[i].inst >= inst_marker)
			continue;

		// bad?
		if (param_combi_allowed(cmd[i].inst, cmd[i].p[0], cmd[i].p[1]))
			continue;

		// correct
//		msg_write(format("correcting param combi  cmd=%d", i));
		bool mov_first_param = (cmd[i].p[1].kind < 0) || (cmd[i].p[0].kind == KindRefToConst) || (cmd[i].p[0].kind == KindConstant);
		SerialCommandParam *pp = mov_first_param ? &cmd[i].p[0] : &cmd[i].p[1];
		SerialCommandParam p = *pp;

		//msg_error("correct");
		//msg_write(p.type->name);
		*pp = param_reg(p.type, get_reg(0, p.type->size));
		add_cmd(Asm::inst_mov, *pp, p);
		move_last_cmd(i);
	}
	ScanTempVarUsage();
}

void SerializerX86::AddFunctionIntro(Function *f)
{
	/*add_cmd(Asm::inst_push, param_reg(TypeReg32, Asm::REG_EBP));
	add_cmd(Asm::inst_mov, param_reg(TypeReg32, Asm::REG_EBP), param_reg(TypeReg32, Asm::REG_ESP));
	if (stack_alloc_size > 127){
		add_easy(inst_sub, PK_REGISTER, s, (void*)reg_sp, PK_CONSTANT, SIZE_32, (void*)(long)stack_alloc_size);
	}else if (stack_alloc_size > 0){
		add_easy(inst_sub, PK_REGISTER, s, (void*)reg_sp, PK_CONSTANT, SIZE_8, (void*)(long)stack_alloc_size);
	}*/
}

void SerializerX86::AddFunctionOutro(Function *f)
{
	add_cmd(Asm::inst_leave);
	if (f->return_type->UsesReturnByMemory())
		add_cmd(Asm::inst_ret, param_const(TypeReg16, 4));
	else
		add_cmd(Asm::inst_ret);
}

void SerializerX86::DoMapping()
{
	FindReferencedTempVars();

	TryMapTempVarsRegisters();

	MapRemainingTempVarsToStack();

	ResolveDerefTempAndLocal();

	CorrectUnallowedParamCombis();

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
		CorrectUnallowedParamCombis2(cmd[i]);

	if (config.verbose)
		cmd_list_out();
}

void SerializerX86::CorrectUnallowedParamCombis2(SerialCommand &c)
{
	// push 8 bit -> push 32 bit
	if (c.inst == Asm::inst_push)
		if (c.p[0].kind == KindRegister)
			c.p[0].p = reg_resize(c.p[0].p, config.pointer_size);
}

};
