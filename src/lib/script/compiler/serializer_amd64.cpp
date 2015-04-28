#include "../script.h"
#include "serializer_amd64.h"
#include "../../file/file.h"



namespace Script{


int SerializerAMD64::fc_begin()
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

		
	// instance as first parameter
	if (CompilerFunctionInstance.type)
		CompilerFunctionParam.insert(CompilerFunctionInstance, 0);

	// return as _very_ first parameter
	if (type->UsesReturnByMemory()){
		//add_temp(type, ret_temp);
		ret_ref = AddReference(/*ret_temp*/ CompilerFunctionReturn, TypePointer);
		CompilerFunctionParam.insert(ret_ref, 0);
	}

	// map params...
	Array<SerialCommandParam> reg_param;
	Array<SerialCommandParam> stack_param;
	Array<SerialCommandParam> xmm_param;
	foreach(SerialCommandParam &p, CompilerFunctionParam){
		if ((p.type == TypeInt) || (p.type == TypeInt64) || (p.type == TypeChar) || (p.type == TypeBool) || (p.type->is_pointer)){
			if (reg_param.num < 6){
				reg_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else if ((p.type == TypeFloat32) || (p.type == TypeFloat64)){
			if (xmm_param.num < 8){
				xmm_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else
			DoError("parameter type currently not supported: " + p.type->name);
	}
	
	// push parameters onto stack
	push_size = 8 * stack_param.num;
	if (push_size > 127)
		add_cmd(Asm::INST_ADD, param_reg(TypePointer, Asm::REG_RSP), param_const(TypeInt, push_size));
	else if (push_size > 0)
		add_cmd(Asm::INST_ADD, param_reg(TypePointer, Asm::REG_RSP), param_const(TypeChar, push_size));
	foreachb(SerialCommandParam &p, stack_param)
		add_cmd(Asm::INST_PUSH, p);
	max_push_size = max(max_push_size, push_size);

	// xmm0-7
	foreachib(SerialCommandParam &p, xmm_param, i){
		int reg = Asm::REG_XMM0 + i;
		if (p.type == TypeFloat64)
			add_cmd(Asm::INST_MOVSD, param_reg(TypeReg128, reg), p);
		else
			add_cmd(Asm::INST_MOVSS, param_reg(TypeReg128, reg), p);
	}
	
	// rdi, rsi,rdx, rcx, r8, r9 
	int param_regs_root[6] = {7, 6, 2, 1, 8, 9};
	foreachib(SerialCommandParam &p, reg_param, i){
		int root = param_regs_root[i];
		int reg = get_reg(root, p.type->size);
		if (reg >= 0){
			add_cmd(Asm::INST_MOV, param_reg(p.type, reg), p);
			add_reg_channel(reg, cmd.num - 1, -100); // -> call
		}else{
			// some registers are not 8bit'able
			add_cmd(Asm::INST_MOV, p_al, p);
			reg = get_reg(root, 4);
			add_cmd(Asm::INST_MOV, param_reg(TypeReg32, reg), p_eax);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
			add_reg_channel(reg, cmd.num - 1, -100); // -> call
		}
	}

	// extend reg channels to call
	foreach(RegChannel &r, reg_channel)
		if (r.last == -100)
			r.last = cmd.num;

	return push_size;
}

void SerializerAMD64::fc_end(int push_size)
{
	Type *type = CompilerFunctionReturn.type;

	// return > 4b already got copied to [ret] by the function!
	if ((type != TypeVoid) && (!type->UsesReturnByMemory())){
		if (type == TypeFloat32)
			add_cmd(Asm::INST_MOVSS, CompilerFunctionReturn, param_reg(TypeReg128, Asm::REG_XMM0));
		else if (type == TypeFloat64)
			add_cmd(Asm::INST_MOVSD, CompilerFunctionReturn, param_reg(TypeReg128, Asm::REG_XMM0));
		else if (type->size == 1){
			add_cmd(Asm::INST_MOV, CompilerFunctionReturn, p_al);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
		}else if (type->size == 4){
			add_cmd(Asm::INST_MOV, CompilerFunctionReturn, p_eax);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
		}else{
			add_cmd(Asm::INST_MOV, CompilerFunctionReturn, p_rax);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
		}
	}
}

void SerializerAMD64::add_function_call(Script *script, int func_no)
{
	msg_db_f("AddFunctionCallAMD64", 4);

	int push_size = fc_begin();

	if ((script == this->script) and (!script->syntax->functions[func_no]->is_extern)){
		add_cmd(Asm::INST_CALL, param_marker(list->get_label("_kaba_func_" + i2s(func_no))));
	}else{
		void *func = (void*)script->func[func_no];
		if (!func)
			DoErrorLink("could not link function " + script->syntax->functions[func_no]->name);
		add_cmd(Asm::INST_CALL, param_const(TypeReg32, (long)func)); // the actual call
		// function pointer will be shifted later...
	}

	fc_end(push_size);
}

void SerializerAMD64::add_virtual_function_call(int virtual_index)
{
	//DoError("virtual function call on amd64 not yet implemented!");
	msg_db_f("AddFunctionCallAmd64", 4);

	int push_size = fc_begin();

	add_cmd(Asm::INST_MOV, p_rax, CompilerFunctionInstance);
	add_cmd(Asm::INST_MOV, p_eax, p_deref_eax);
	add_cmd(Asm::INST_ADD, p_eax, param_const(TypeInt, 8 * virtual_index));
	add_cmd(Asm::INST_MOV, p_eax, p_deref_eax);
	add_cmd(Asm::INST_CALL, p_eax); // the actual call

	fc_end(push_size);
}


void SerializerAMD64::AddFunctionIntro(Function *f)
{
	// return, instance, params
	Array<Variable> param;
	if (f->return_type->UsesReturnByMemory()){
		foreach(Variable &v, f->var)
			if (v.name == "-return-"){
				param.add(v);
				break;
			}
	}
	if (f->_class){
		foreach(Variable &v, f->var)
			if (v.name == "self"){
				param.add(v);
				break;
			}
	}
	for (int i=0;i<f->num_params;i++)
		param.add(f->var[i]);

	// map params...
	Array<Variable> reg_param;
	Array<Variable> stack_param;
	Array<Variable> xmm_param;
	foreach(Variable &p, param){
		if ((p.type == TypeInt) || (p.type == TypeChar) || (p.type == TypeBool) || (p.type->is_pointer)){
			if (reg_param.num < 6){
				reg_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else if (p.type == TypeFloat32){
			if (xmm_param.num < 8){
				xmm_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else
			DoError("parameter type currently not supported: " + p.type->name);
	}

	// xmm0-7
	foreachib(Variable &p, xmm_param, i){
		int reg = Asm::REG_XMM0 + i;
		add_cmd(Asm::INST_MOVSS, param_local(p.type, p._offset), param_reg(p.type, reg));
	}

	// rdi, rsi,rdx, rcx, r8, r9
	int param_regs_root[6] = {7, 6, 2, 1, 8, 9};
	foreachib(Variable &p, reg_param, i){
		int root = param_regs_root[i];
		int reg = get_reg(root, p.type->size);
		if (reg >= 0){
			add_cmd(Asm::INST_MOV, param_local(p.type, p._offset), param_reg(p.type, reg));
			add_reg_channel(reg, cmd.num - 1, cmd.num - 1);
		}else{
			// some registers are not 8bit'able
			add_cmd(Asm::INST_MOV, p_eax, param_reg(TypeReg32, get_reg(root, 4)));
			add_cmd(Asm::INST_MOV, param_local(p.type, p._offset), param_reg(p.type, get_reg(0, p.type->size)));
			add_reg_channel(reg, cmd.num - 2, cmd.num - 2);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
		}
	}
		
	// get parameters from stack
	foreachb(Variable &p, stack_param){
		DoError("func with stack...");
		/*int s = 8;
		add_cmd(Asm::inst_push, p);
		push_size += s;*/
	}
}

void SerializerAMD64::AddFunctionOutro(Function *f)
{
	add_cmd(Asm::INST_LEAVE);
	add_cmd(Asm::INST_RET);
}

void SerializerAMD64::CorrectUnallowedParamCombis2(SerialCommand &c)
{
	// push 8 bit -> push 32 bit
	if (c.inst == Asm::INST_PUSH)
		if (c.p[0].kind == KIND_REGISTER)
			c.p[0].p = reg_resize(c.p[0].p, config.pointer_size);


	// FIXME
	// evil hack to allow inconsistent param types (in address shifts)
	if (config.instruction_set == Asm::INSTRUCTION_SET_AMD64){
		if ((c.inst == Asm::INST_ADD) || (c.inst == Asm::INST_MOV)){
			if ((c.p[0].kind == KIND_REGISTER) && (c.p[1].kind == KIND_REF_TO_CONST)){
				if (c.p[0].type->is_pointer){
#ifdef debug_evil_corrections
					msg_write("----evil resize a");
					msg_write(cmd2str(c));
#endif
					c.p[0].type = TypeReg32;
					c.p[0].p = reg_resize(c.p[0].p, 4);
#ifdef debug_evil_corrections
					msg_write(cmd2str(c));
#endif
				}
			}
			if ((c.p[0].type->size == 8) && (c.p[1].type->size == 4)){
				/*if ((c.p[0].kind == KindRegister) && ((c.p[1].kind == KindRegister) || (c.p[1].kind == KindConstant) || (c.p[1].kind == KindRefToConst))){
#ifdef debug_evil_corrections
					msg_write("----evil resize b");
					msg_write(cmd2str(c));
#endif
					c.p[0].type = c.p[1].type;
					c.p[0].p = (char*)(long)reg_resize((long)c.p[0].p, c.p[1].type->size);
#ifdef debug_evil_corrections
					msg_write(cmd2str(c));
#endif
				}else*/ if (c.p[1].kind == KIND_REGISTER){
#ifdef debug_evil_corrections
					msg_write("----evil resize c");
					msg_write(cmd2str(c));
#endif
					c.p[1].type = c.p[0].type;
					c.p[1].p = reg_resize(c.p[1].p, c.p[0].type->size);
#ifdef debug_evil_corrections
					msg_write(cmd2str(c));
#endif
				}
			}
			if ((c.p[0].type->size < 8) && (c.p[1].type->size == 8)){
				if ((c.p[0].kind == KIND_REGISTER) && ((c.p[1].kind == KIND_REGISTER) || (c.p[1].kind == KIND_DEREF_REGISTER))){
#ifdef debug_evil_corrections
					msg_write("----evil resize d");
					msg_write(cmd2str(c));
#endif
					c.p[0].type = c.p[1].type;
					c.p[0].p = reg_resize(c.p[0].p, c.p[1].type->size);
#ifdef debug_evil_corrections
					msg_write(cmd2str(c));
#endif
				}
			}
			/*if (c.p[0].type->size > c.p[1].type->size){
				msg_write("size ok");
				if ((c.p[0].kind == KindRegister) && ((c.p[1].kind == KindRegister) || (c.p[1].kind == KindConstant) || (c.p[1].kind == KindRefToConst))){
					msg_error("----evil resize");
					c.p[0].type = c.p[1].type;
					c.p[0].p = (char*)(long)Asm::RegResize[Asm::RegRoot[(long)c.p[0].p]][c.p[1].type->size];
				}
			}*/
		}
	}
}

};
