#include "../script.h"
#include "serializer.h"
#include "../../file/file.h"


namespace Asm{
	extern int ARMDataInstructions[16]; // -> asm.cpp
};

namespace Script{


//#define debug_evil_corrections	1

//#ifdef ScriptDebug


extern int GlobalWaitingMode;
extern float GlobalTimeToWait;


static SerialCommandParam p_eax, p_eax_int, p_deref_eax;
static SerialCommandParam p_rax;
static SerialCommandParam p_ax, p_al, p_ah, p_al_bool, p_al_char;
static SerialCommandParam p_st0, p_st1;
static const SerialCommandParam p_none = {-1, 0, NULL, 0};


void Serializer::add_reg_channel(int reg, int first, int last)
{
	RegChannel c = {Asm::RegRoot[reg], first, last};
	reg_channel.add(c);
}

void Serializer::add_temp(Type *t, SerialCommandParam &param, bool add_constructor)
{
	if (t != TypeVoid){
		TempVar v;
		v.first = -1;
		v.type = t;
		v.force_stack = (t->size > config.pointer_size) || (t->is_super_array) || (t->is_array) || (t->element.num > 0);
		v.entangled = 0;
		temp_var.add(v);
		param.kind = KindVarTemp;
		param.p = temp_var.num - 1;
		param.type = t;
		param.shift = 0;

		if (add_constructor)
			add_cmd_constructor(param, KindVarTemp);
		else
			inserted_constructor_temp.add(param);
	}else{
		param = p_none;
	}
}

inline Type *get_subtype(Type *t)
{
	if (t->parent)
		return t->parent;
	msg_error("subtype wanted of... " + t->name);
	//msg_write(cur_func->Name);
	return TypeUnknown;
}

inline void deref_temp(SerialCommandParam &param, SerialCommandParam &deref)
{
	//deref = param;
	deref.kind = KindDerefVarTemp;
	deref.p = param.p;
	deref.type = get_subtype(param.type);
	deref.shift = 0;
}

inline SerialCommandParam param_shift(const SerialCommandParam &param, int shift, Type *t)
{
	SerialCommandParam p = param;
	p.shift += shift;
	p.type = t;
	return p;
}

inline SerialCommandParam param_global(Type *type, void *v)
{
	SerialCommandParam p;
	p.type = type;
	p.kind = KindVarGlobal;
	p.p = (long)v;
	p.shift = 0;
	return p;
}

inline SerialCommandParam param_local(Type *type, int offset)
{
	SerialCommandParam p;
	p.type = type;
	p.kind = KindVarLocal;
	p.p = offset;
	p.shift = 0;
	return p;
}

inline SerialCommandParam param_const(Type *type, long c)
{
	SerialCommandParam p;
	p.type = type;
	p.kind = KindConstant;
	p.p = c;
	p.shift = 0;
	return p;
}

inline SerialCommandParam param_marker(int m)
{
	SerialCommandParam p;
	p.type = TypeInt;
	p.kind = KindMarker;
	p.p = m;
	p.shift = 0;
	return p;
}

inline SerialCommandParam param_reg(Type *type, int reg)
{
	SerialCommandParam p;
	p.kind = KindRegister;
	p.p = reg;
	p.type = type;
	p.shift = 0;
	return p;
}

inline SerialCommandParam param_deref_reg(Type *type, int reg)
{
	SerialCommandParam p;
	p.kind = KindDerefRegister;
	p.p = reg;
	p.type = type;
	p.shift = 0;
	return p;
}

string SerialCommandParam::str() const
{
	string str;
	if (kind >= 0){
		string n = p2s((void*)p);
		if ((kind == KindRegister) || (kind == KindDerefRegister))
			n = Asm::GetRegName(p);
		else if ((kind == KindVarLocal) or (kind == KindVarGlobal) or (kind == KindVarTemp) or (kind == KindDerefVarTemp) or (kind == KindMarker))
			n = i2s(p);
		str = "  (" + type->name + ") " + Kind2Str(kind) + " " + n;
		if (shift > 0)
			str += format(" + shift %d", shift);
	}
	return str;
}

string SerialCommand::str() const
{
	//msg_db_f("cmd_out", 4);
	if (inst == inst_marker)
		return format("-- Label %d --", p[0].p);
	if (inst == inst_asm)
		return format("-- Asm %d --", p[0].p);
	string t;
	if (cond != Asm::ARM_COND_ALWAYS)
		t += "[cond]";
	t += Asm::GetInstructionName(inst);
	t += p[0].str();
	if (p[1].kind >= 0)
		t += "," + p[1].str();
	if (p[2].kind >= 0)
		t += "," + p[2].str();
	return t;
}

void Serializer::cmd_list_out()
{
	msg_db_f("cmd_list_out", 4);
	msg_write("--------------------------------");
	for (int i=0;i<cmd.num;i++)
		msg_write(format("%3d: ", i) + cmd[i].str());
	if (false){
		msg_write("-----------");
		for (int i=0;i<reg_channel.num;i++)
			msg_write(format("  %d   %d -> %d", reg_channel[i].reg_root, reg_channel[i].first, reg_channel[i].last));
		msg_write("-----------");
		if (temp_var_ranges_defined)
			for (int i=0;i<temp_var.num;i++)
				msg_write(format("  %d   %d -> %d", i, temp_var[i].first, temp_var[i].last));
		msg_write("--------------------------------");
	}
}

inline int get_reg(int root, int size)
{
#if 1
	if ((size != 1) && (size != 4) && (size != 8)){
		msg_write(msg_get_trace());
		throw Asm::Exception("get_reg: bad reg size: " + i2s(size), "...", 0, 0);
	}
#endif
	return Asm::RegResize[root][size];
}

void Serializer::add_cmd(int cond, int inst, const SerialCommandParam &p1, const SerialCommandParam &p2, const SerialCommandParam &p3)
{
	SerialCommand c;
	c.inst = inst;
	c.cond = cond;
	c.p[0] = p1;
	c.p[1] = p2;
	c.p[2] = p3;
	cmd.add(c);

	// call violates all used registers...
	if (inst == Asm::inst_call)
		for (int i=0;i<map_reg_root.num;i++){
			add_reg_channel(get_reg(i, 4), cmd.num - 1, cmd.num - 1);
		}
}

void Serializer::add_cmd(int inst, const SerialCommandParam &p1, const SerialCommandParam &p2, const SerialCommandParam &p3)
{
	add_cmd(Asm::ARM_COND_ALWAYS, inst, p1, p2, p3);
}

void Serializer::add_cmd(int inst, const SerialCommandParam &p1, const SerialCommandParam &p2)
{
	add_cmd(Asm::ARM_COND_ALWAYS, inst, p1, p2, p_none);
}

void Serializer::add_cmd(int inst, const SerialCommandParam &p)
{
	add_cmd(Asm::ARM_COND_ALWAYS, inst, p, p_none, p_none);
}

void Serializer::add_cmd(int inst)
{
	add_cmd(Asm::ARM_COND_ALWAYS, inst, p_none, p_none, p_none);
}

void Serializer::move_last_cmd(int index)
{
	SerialCommand c = cmd.back();
	for (int i=cmd.num-1;i>index;i--)
		cmd[i] = cmd[i - 1];
	cmd[index] = c;

	// adjust temp vars
	if (temp_var_ranges_defined){
		foreach(TempVar &v, temp_var){
			if (v.first >= index)
				v.first ++;
			if (v.last >= index)
				v.last ++;
		}
	}

	// adjust reg channels
	foreach(RegChannel &r, reg_channel){
		if (r.first >= index)
			r.first ++;
		if (r.last >= index)
			r.last ++;
	}
}

void Serializer::remove_cmd(int index)
{
	cmd.erase(index);

	// adjust temp vars
	foreach(TempVar &v, temp_var){
		if (v.first >= index)
			v.first --;
		if (v.last >= index)
			v.last --;
	}

	// adjust reg channels
	foreach(RegChannel &r, reg_channel){
		if (r.first >= index)
			r.first --;
		if (r.last >= index)
			r.last --;
	}
}

void Serializer::remove_temp_var(int v)
{
	foreach(SerialCommand &c, cmd){
		for (int i=0; i<SERIAL_COMMAND_NUM_PARAMS; i++)
			if ((c.p[i].kind == KindVarTemp) or (c.p[i].kind == KindDerefVarTemp))
				if (c.p[i].p > v)
					c.p[i].p --;
	}
	temp_var.erase(v);
}

void Serializer::move_param(SerialCommandParam &p, int from, int to)
{
	if ((p.kind == KindVarTemp) || (p.kind == KindDerefVarTemp)){
		// move_param temp
		long v = (long)p.p;
		if (temp_var[v].last < max(from, to))
			temp_var[v].last = max(from, to);
		if (temp_var[v].first > min(from, to))
			temp_var[v].first = min(from, to);
	}else if ((p.kind == KindRegister) || (p.kind == KindDerefRegister)){
		// move_param reg
		long r = Asm::RegRoot[(long)p.p];
		bool found = false;
		foreach(RegChannel &rc, reg_channel)
			if ((r == rc.reg_root) && (from >= rc.first) && (from >= rc.first)){
				if (rc.last < max(from, to))
					rc.last = max(from, to);
				if (rc.first > min(from, to))
					rc.first = min(from, to);
				found = true;
			}
		if (!found){
			msg_error(format("move_param: no RegChannel...  reg_root=%d  from=%d", r, from));
			msg_write(script->Filename + " : " + cur_func->name);
		}
	}
}

// l is an asm label index
int Serializer::add_marker(int l)
{
	SerialCommandParam p = p_none;
	if (l < 0)
		l = list->get_label("_kaba_" + i2s(cur_func_index) + "_" + i2s(num_markers ++));
	p.kind = KindMarker;
	p.p = l;
	add_cmd(inst_marker, p);
	return l;
}

int Serializer::add_marker_after_command(int level, int index)
{
	int n = list->get_label("_kaba_" + i2s(cur_func_index) + "_" + i2s(num_markers ++));
	AddLaterData m = {STUFF_KIND_MARKER, n, level, index};
	add_later.add(m);
	return n;
}

void Serializer::add_jump_after_command(int level, int index, int marker)
{
	AddLaterData j = {STUFF_KIND_JUMP, marker, level, index};
	add_later.add(j);
}

inline int reg_resize(int reg, int size)
{
	if (size == 2){
		msg_error("size = 2");
		msg_write(msg_get_trace());
		throw Asm::Exception("size=2", "kjlkjl", 0, 0);
		//Asm::DoError("size=2");
	}
	return get_reg(Asm::RegRoot[reg], size);
}


static Array<SerialCommandParam> CompilerFunctionParam;
static SerialCommandParam CompilerFunctionReturn = {-1, 0, NULL};
static SerialCommandParam CompilerFunctionInstance = {-1, 0, NULL};

void AddFuncParam(const SerialCommandParam &p)
{
	CompilerFunctionParam.add(p);
}

void AddFuncReturn(const SerialCommandParam &r)
{
	CompilerFunctionReturn = r;
}

void AddFuncInstance(const SerialCommandParam &inst)
{
	CompilerFunctionInstance = inst;
}

int SerializerX86::fc_begin()
{
	Type *type = CompilerFunctionReturn.type;

	// return data too big... push address
	SerialCommandParam ret_temp, ret_ref;
	if (type->UsesReturnByMemory()){
		//add_temp(type, ret_temp);
		AddReference(/*ret_temp*/ CompilerFunctionReturn, TypePointer, ret_ref);
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

	if (script == this->script){
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

int SerializerAMD64::fc_begin()
{
	Type *type = CompilerFunctionReturn.type;

	// return data too big... push address
	SerialCommandParam ret_temp, ret_ref;
	if (type->UsesReturnByMemory()){
		//add_temp(type, ret_temp);
		AddReference(/*ret_temp*/ CompilerFunctionReturn, TypePointer, ret_ref);
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
		AddReference(/*ret_temp*/ CompilerFunctionReturn, TypePointer, ret_ref);
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
		add_cmd(Asm::inst_add, param_reg(TypePointer, Asm::REG_RSP), param_const(TypeInt, push_size));
	else if (push_size > 0)
		add_cmd(Asm::inst_add, param_reg(TypePointer, Asm::REG_RSP), param_const(TypeChar, push_size));
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
	}
	
	// rdi, rsi,rdx, rcx, r8, r9 
	int param_regs_root[6] = {7, 6, 2, 1, 8, 9};
	foreachib(SerialCommandParam &p, reg_param, i){
		int root = param_regs_root[i];
		int reg = get_reg(root, p.type->size);
		if (reg >= 0){
			add_cmd(Asm::inst_mov, param_reg(p.type, reg), p);
			add_reg_channel(reg, cmd.num - 1, -100); // -> call
		}else{
			// some registers are not 8bit'able
			add_cmd(Asm::inst_mov, p_al, p);
			reg = get_reg(root, 4);
			add_cmd(Asm::inst_mov, param_reg(TypeReg32, reg), p_eax);
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
			add_cmd(Asm::inst_movss, CompilerFunctionReturn, param_reg(TypeReg128, Asm::REG_XMM0));
		else if (type == TypeFloat64)
			add_cmd(Asm::inst_movsd, CompilerFunctionReturn, param_reg(TypeReg128, Asm::REG_XMM0));
		else if (type->size == 1){
			add_cmd(Asm::inst_mov, CompilerFunctionReturn, p_al);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
		}else if (type->size == 4){
			add_cmd(Asm::inst_mov, CompilerFunctionReturn, p_eax);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
		}else{
			add_cmd(Asm::inst_mov, CompilerFunctionReturn, p_rax);
			add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
		}
	}
}

void SerializerAMD64::add_function_call(Script *script, int func_no)
{
	msg_db_f("AddFunctionCallAMD64", 4);

	int push_size = fc_begin();

	if (script == this->script){
		add_cmd(Asm::inst_call, param_marker(list->get_label("_kaba_func_" + i2s(func_no))));
	}else{
		void *func = (void*)script->func[func_no];
		if (!func)
			DoErrorLink("could not link function " + script->syntax->Functions[func_no]->name);
		add_cmd(Asm::inst_call, param_const(TypeReg32, (long)func)); // the actual call
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

void SerializerAMD64::add_virtual_function_call(int virtual_index)
{
	//DoError("virtual function call on amd64 not yet implemented!");
	msg_db_f("AddFunctionCallAmd64", 4);

	int push_size = fc_begin();

	add_cmd(Asm::inst_mov, p_rax, CompilerFunctionInstance);
	add_cmd(Asm::inst_mov, p_eax, p_deref_eax);
	add_cmd(Asm::inst_add, p_eax, param_const(TypeInt, 8 * virtual_index));
	add_cmd(Asm::inst_mov, p_eax, p_deref_eax);
	add_cmd(Asm::inst_call, p_eax); // the actual call

	fc_end(push_size);
}

int SerializerARM::fc_begin()
{
	Type *type = CompilerFunctionReturn.type;

	// return data too big... push address
	SerialCommandParam ret_temp, ret_ref;
	if (type->UsesReturnByMemory()){
		//add_temp(type, ret_temp);
		AddReference(/*ret_temp*/ CompilerFunctionReturn, TypePointer, ret_ref);
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
		AddReference(/*ret_temp*/ CompilerFunctionReturn, TypePointer, ret_ref);
		CompilerFunctionParam.insert(ret_ref, 0);
	}

	// map params...
	Array<SerialCommandParam> reg_param;
	Array<SerialCommandParam> stack_param;
	Array<SerialCommandParam> xmm_param;
	foreach(SerialCommandParam &p, CompilerFunctionParam){
		if ((p.type == TypeInt) || (p.type == TypeInt64) || (p.type == TypeChar) || (p.type == TypeBool) || (p.type->is_pointer)){
			if (reg_param.num < 4){
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
	foreachib(SerialCommandParam &p, reg_param, i){
		int reg = Asm::REG_R0 + i;
		add_cmd(Asm::inst_mov, param_reg(p.type, reg), p);
		add_reg_channel(reg, cmd.num - 1, -100); // -> call
	}

	// extend reg channels to call
	foreach(RegChannel &r, reg_channel)
		if (r.last == -100)
			r.last = cmd.num;

	return push_size;
}

void SerializerARM::fc_end(int push_size)
{
	Type *type = CompilerFunctionReturn.type;

	// return > 4b already got copied to [ret] by the function!
	if ((type != TypeVoid) && (!type->UsesReturnByMemory())){
		if (type == TypeFloat32)
			add_cmd(Asm::inst_movss, CompilerFunctionReturn, param_reg(TypeReg128, Asm::REG_XMM0));
		else if (type == TypeFloat64)
			add_cmd(Asm::inst_movsd, CompilerFunctionReturn, param_reg(TypeReg128, Asm::REG_XMM0));
		else if ((type->size == 1) or (type->size == 4)){
			add_cmd(Asm::inst_mov, CompilerFunctionReturn, param_reg(TypeReg32, Asm::REG_R0));
			add_reg_channel(Asm::REG_R0, cmd.num - 2, cmd.num - 1);
		}else{
			add_cmd(Asm::inst_mov, CompilerFunctionReturn, param_reg(TypeReg32, Asm::REG_R0));
			add_reg_channel(Asm::REG_R0, cmd.num - 2, cmd.num - 1);
		}
	}
}

void SerializerARM::add_function_call(Script *script, int func_no)
{
	int push_size = fc_begin();

	if (script == this->script){
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

void SerializerARM::add_virtual_function_call(int virtual_index){}

void SerializerARM::SerializeCompilerFunction(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret, int level, int index, int marker_before_params)
{

	switch(com->link_no){
		case CommandIf:{
			// cmp;  jz m;  -block-  m;
			add_cmd(Asm::inst_cmp, param[0], param_const(TypeBool, 0x0));
			int m_after_true = add_marker_after_command(level, index + 1);
			add_cmd(Asm::ARM_COND_EQUAL, Asm::inst_b, param_marker(m_after_true), p_none, p_none);
			}break;
		case CommandIfElse:{
			// cmp;  jz m1;  -block-  jmp m2;  m1;  -block-  m2;
			add_cmd(Asm::inst_cmp, param[0], param_const(TypeBool, 0x0));
			int m_after_true = add_marker_after_command(level, index + 1);
			int m_after_false = add_marker_after_command(level, index + 2);
			add_cmd(Asm::ARM_COND_EQUAL, Asm::inst_b, param_marker(m_after_true), p_none, p_none); // jz ...
			add_jump_after_command(level, index + 1, m_after_false); // insert before <m_after_true> is inserted!
			}break;
		case CommandWhile:
		case CommandFor:{
			// m1;  cmp;  jz m2;  -block-             jmp m1;  m2;     (while)
			// m1;  cmp;  jz m2;  -block-  m3;  i++;  jmp m1;  m2;     (for)
			add_cmd(Asm::inst_cmp, param[0], param_const(TypeBool, 0x0));
			int marker_after_while = add_marker_after_command(level, index + 1);
			add_cmd(Asm::ARM_COND_EQUAL, Asm::inst_b, param_marker(marker_after_while), p_none, p_none);
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
			add_cmd(Asm::inst_b, param_marker(loop.back().marker_break));
			break;
		case CommandContinue:
			add_cmd(Asm::inst_b, param_marker(loop.back().marker_continue));
			break;
		case CommandReturn:
			if (com->num_params > 0){
				if (cur_func->return_type->UsesReturnByMemory()){ // we already got a return address in [ebp+0x08] (> 4 byte)
					FillInDestructors(false);
					// internally handled...

					AddFunctionOutro(cur_func);
				}else{ // store return directly in eax / fpu stack (4 byte)
					SerialCommandParam t;
					add_temp(cur_func->return_type, t);
					FillInDestructors(false);
					if ((cur_func->return_type == TypeInt) or (cur_func->return_type->size == 1)){
						add_reg_channel(Asm::REG_R0, cmd.num, cmd.num);
						add_cmd(Asm::inst_mov, param_reg(cur_func->return_type, Asm::REG_R0), param[0]);
					}else{
						DoError("return != int");
					}
					AddFunctionOutro(cur_func);
				}
			}else{
				FillInDestructors(false);
				AddFunctionOutro(cur_func);
			}
			break;
		case CommandAsm:
			add_cmd(inst_asm);
			break;
		default:
			DoError("compiler function unimplemented: " + PreCommands[com->link_no].name);
	}
}

void SerializerARM::SerializeOperator(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret)
{
	msg_db_f("SerializeOperatorARM", 4);
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
			add_cmd(Asm::inst_add, param[0], param[0], param[1]);
			break;
		case OperatorIntSubtractS:
			add_cmd(Asm::inst_sub, param[0], param[0], param[1]);
			break;
		case OperatorIntMultiplyS:
			add_cmd(Asm::inst_imul, param[0], param[0], param[1]);
			break;
		case OperatorIntAdd:
			add_cmd(Asm::inst_add, ret, param[0], param[1]);
			break;
		case OperatorIntSubtract:
			add_cmd(Asm::inst_sub, ret, param[0], param[1]);
			break;
		case OperatorIntMultiply:
			add_cmd(Asm::inst_mul, ret, param[0], param[1]);
			break;
		case OperatorIntEqual:
		case OperatorPointerEqual:
			add_cmd(Asm::inst_cmp, param[0], param[1]);
			add_cmd(Asm::ARM_COND_EQUAL,     Asm::inst_mov, ret, param_const(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_NOT_EQUAL, Asm::inst_mov, ret, param_const(TypeBool, 0), p_none);
			break;
		case OperatorIntNotEqual:
		case OperatorPointerNotEqual:
			add_cmd(Asm::inst_cmp, param[0], param[1]);
			add_cmd(Asm::ARM_COND_NOT_EQUAL,     Asm::inst_mov, ret, param_const(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_EQUAL, Asm::inst_mov, ret, param_const(TypeBool, 0), p_none);
			break;
		case OperatorIntGreater:
			add_cmd(Asm::inst_cmp, param[0], param[1]);
			add_cmd(Asm::ARM_COND_GREATER_THAN,     Asm::inst_mov, ret, param_const(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_LESS_EQUAL, Asm::inst_mov, ret, param_const(TypeBool, 0), p_none);
			break;
		case OperatorIntGreaterEqual:
			add_cmd(Asm::inst_cmp, param[0], param[1]);
			add_cmd(Asm::ARM_COND_GREATER_EQUAL,     Asm::inst_mov, ret, param_const(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_LESS_THAN, Asm::inst_mov, ret, param_const(TypeBool, 0), p_none);
			break;
		case OperatorIntSmaller:
			add_cmd(Asm::inst_cmp, param[0], param[1]);
			add_cmd(Asm::ARM_COND_LESS_THAN,     Asm::inst_mov, ret, param_const(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_GREATER_EQUAL, Asm::inst_mov, ret, param_const(TypeBool, 0), p_none);
			break;
		case OperatorIntSmallerEqual:
			add_cmd(Asm::inst_cmp, param[0], param[1]);
			add_cmd(Asm::ARM_COND_LESS_EQUAL,     Asm::inst_mov, ret, param_const(TypeBool, 1), p_none);
			add_cmd(Asm::ARM_COND_GREATER_THAN, Asm::inst_mov, ret, param_const(TypeBool, 0), p_none);
			break;
		default:
			DoError("unimplemented operator: " + PreOperators[com->link_no].str());
	}
}


void Serializer::AddFunctionCall(Script *script, int func_no)
{
	call_used = true;
	if (!CompilerFunctionReturn.type)
		CompilerFunctionReturn.type = TypeVoid;

	add_function_call(script, func_no);

	// clean up for next call
	CompilerFunctionParam.clear();
	CompilerFunctionReturn.type = TypeVoid;
	CompilerFunctionInstance.type = NULL;
}

void Serializer::AddClassFunctionCall(ClassFunction *cf)
{
	if (cf->virtual_index < 0){
		AddFunctionCall(cf->script, cf->nr);
		return;
	}
	call_used = true;
	if (!CompilerFunctionReturn.type)
		CompilerFunctionReturn.type = TypeVoid;

	add_virtual_function_call(cf->virtual_index);

	// clean up for next call
	CompilerFunctionParam.clear();
	CompilerFunctionReturn.type = TypeVoid;
	CompilerFunctionInstance.type = NULL;
}


// creates res...
void Serializer::AddReference(SerialCommandParam &param, Type *type, SerialCommandParam &ret)
{
	msg_db_f("AddReference", 3);
	ret.type = type;
	ret.shift = 0;
	if (param.kind == KindRefToConst){
		ret.kind = KindConstant;
		ret.p = param.p;
	}else if ((param.kind == KindConstant) || (param.kind == KindVarGlobal)){
		ret.kind = KindConstant;
		ret.p = param.p;
	}else if (param.kind == KindDerefVarTemp){
		ret = param;
		param.kind = KindVarTemp;
	}else{
		add_temp(type, ret);
		if (config.instruction_set == Asm::INSTRUCTION_SET_AMD64){
			add_cmd(Asm::inst_lea, p_rax, param);
			add_cmd(Asm::inst_mov, ret, p_rax);
		}else{
			add_cmd(Asm::inst_lea, p_eax, param);
			add_cmd(Asm::inst_mov, ret, p_eax);
		}
		add_reg_channel(Asm::REG_EAX, cmd.num - 2, cmd.num - 1);
	}
}

void Serializer::AddDereference(SerialCommandParam &param, SerialCommandParam &ret, Type *force_type)
{
	msg_db_f("AddDereference", 4);
	/*add_temp(TypePointer, ret);
	SerialCommandParam temp;
	add_temp(TypePointer, temp);
	add_cmd(Asm::inst_mov, temp, param);
	temp.kind = KindDerefVarTemp;
	add_cmd(Asm::inst_mov, ret, temp);*/
	if (param.kind == KindVarTemp){
		deref_temp(param, ret);
	}else if (param.kind == KindRegister){
		ret = param;
		ret.kind = KindDerefRegister;
		ret.type = force_type ? force_type : get_subtype(param.type);
		ret.shift = 0;
	}else{
		//msg_error(string("unhandled deref ", Kind2Str(param.kind)));
		SerialCommandParam temp;
		add_temp(param.type, temp);
		add_cmd(Asm::inst_mov, temp, param);
		deref_temp(temp, ret);
	}
}


int Serializer::add_global_ref(void *p)
{
	foreachi(GlobalRef &g, global_refs, i)
		if (g.p == p)
			return i;
	GlobalRef g;
	g.p = p;
	g.label = -1;
	global_refs.add(g);
	return global_refs.num - 1;
}

// create data for a (function) parameter
//   and compile its command if the parameter is executable itself
void Serializer::SerializeParameter(Command *link, int level, int index, SerialCommandParam &p)
{
	msg_db_f("SerializeParameter", 4);
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
		if (config.instruction_set == Asm::INSTRUCTION_SET_ARM){
			p.p = add_global_ref((void*)p.p);
			p.kind = KindGlobalLookup;
		}
	}else if (link->kind == KindMemory){
		p.p = link->link_no;
		p.kind = KindVarGlobal;
		if (config.instruction_set == Asm::INSTRUCTION_SET_ARM){
			p.p = add_global_ref((void*)p.p);
			p.kind = KindGlobalLookup;
		}
	}else if (link->kind == KindAddress){
		p.p = (long)&link->link_no;
		p.kind = KindRefToConst;
	}else if (link->kind == KindVarGlobal){
		p.p = (long)link->script->g_var[link->link_no];
		if (!p.p)
			script->DoErrorLink("variable is not linkable: " + link->script->syntax->RootOfAllEvil.var[link->link_no].name);
		if (config.instruction_set == Asm::INSTRUCTION_SET_ARM){
			p.p = add_global_ref((void*)p.p);
			p.kind = KindGlobalLookup;
		}
	}else if (link->kind == KindVarLocal){
		p.p = cur_func->var[link->link_no]._offset;
	}else if (link->kind == KindLocalMemory){
		p.p = link->link_no;
		p.kind = KindVarLocal;
	}else if (link->kind == KindLocalAddress){
		SerialCommandParam param;
		param.p = link->link_no;
		param.kind = KindVarLocal;
		param.type = TypePointer;
		param.shift = 0;

		AddReference(param, link->type, p);
	}else if (link->kind == KindConstant){
		if ((config.use_const_as_global_var) || (syntax_tree->FlagCompileOS))
			p.kind = KindVarGlobal;
		else
			p.kind = KindRefToConst;
		p.p = (long)link->script->cnst[link->link_no];
		if (config.instruction_set == Asm::INSTRUCTION_SET_ARM){
			p.p = add_global_ref((void*)p.p);
			p.kind = KindGlobalLookup;
		}
	}else if ((link->kind==KindOperator) || (link->kind==KindFunction) || (link->kind==KindVirtualFunction) || (link->kind==KindCompilerFunction) || (link->kind==KindArrayBuilder)){
		p = SerializeCommand(link, level, index);
	}else if (link->kind == KindReference){
		SerialCommandParam param;
		SerializeParameter(link->param[0], level, index, param);
		//printf("%d  -  %s\n",pk,Kind2Str(pk));
		AddReference(param, link->type, p);
	}else if (link->kind == KindDereference){
		SerialCommandParam param;
		SerializeParameter(link->param[0], level, index, param);
		/*if ((param.kind == KindVarLocal) || (param.kind == KindVarGlobal)){
			p.type = param.type->sub_type;
			if (param.kind == KindVarLocal)		p.kind = KindRefToLocal;
			if (param.kind == KindVarGlobal)	p.kind = KindRefToGlobal;
			p.p = param.p;
		}*/
		AddDereference(param, p);
	}else if (link->kind == KindVarTemp){
		// only used by <new> operator
		p.p = link->link_no;
	}else{
		DoError("unexpected type of parameter: " + Kind2Str(link->kind));
	}
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


SerialCommandParam Serializer::SerializeCommand(Command *com, int level, int index)
{
	msg_db_f("SerializeCommand", 4);

	// for/while need a marker to this point
	int marker_before_params = -1;
	if ((com->kind == KindCompilerFunction) && ((com->link_no == CommandWhile) || (com->link_no == CommandFor)))
		marker_before_params = add_marker();

	// return value
	SerialCommandParam ret;
	bool create_constructor_for_return = ((com->kind != KindCompilerFunction) && (com->kind != KindFunction) && (com->kind != KindVirtualFunction));
	add_temp(com->type, ret, create_constructor_for_return);


	// special new-operator work-around
	if ((com->kind == KindCompilerFunction) && (com->link_no == CommandNew)){
		if (com->num_params == 0)
			com->param[0] = NULL;
		com->num_params = 0;
	}

	// compile parameters
	Array<SerialCommandParam> param;
	param.resize(com->num_params);
	for (int p=0;p<com->num_params;p++)
		SerializeParameter(com->param[p], level, index, param[p]);

	// class function -> compile instance
	bool is_class_function = false;
	if (com->kind == KindFunction){
		if (com->script->syntax->Functions[com->link_no]->_class)
			is_class_function = true;
	}else if (com->kind == KindVirtualFunction){
		is_class_function = true;
	}
	SerialCommandParam instance = {-1, 0, NULL};
	if (is_class_function){
		SerializeParameter(com->instance, level, index, instance);
		// super_array automatically referenced...
	}


	if (com->kind == KindOperator){
		SerializeOperator(com, param, ret);

	}else if (com->kind == KindFunction){
		// inline function?
		if (com->script->syntax->Functions[com->link_no]->inline_no >= 0){
			Command c = *com;
			c.kind = KindCompilerFunction;
			c.link_no = com->script->syntax->Functions[com->link_no]->inline_no;

			SerializeCompilerFunction(&c, param, ret, level, index, marker_before_params);
			return ret;
		}

		for (int p=0;p<com->num_params;p++)
			AddFuncParam(param[p]);

		AddFuncReturn(ret);

		if (is_class_function)
			AddFuncInstance(instance);

		AddFunctionCall(com->script, com->link_no);

	}else if (com->kind == KindVirtualFunction){

		for (int p=0;p<com->num_params;p++)
			AddFuncParam(param[p]);

		AddFuncReturn(ret);
		AddFuncInstance(instance);

		AddClassFunctionCall(instance.type->parent->GetVirtualFunction(com->link_no));
	}else if (com->kind == KindCompilerFunction){
		SerializeCompilerFunction(com, param, ret, level, index, marker_before_params);
	}else if (com->kind == KindArrayBuilder){
		ClassFunction *cf = com->type->GetFunc("add", TypeVoid, 1);
		if (!cf)
			DoError(format("[..]: can not find %s.add() function???", com->type->name.c_str()));
		AddReference(ret, com->type->GetPointer(), instance);
		for (int i=0; i<com->num_params; i++){
			AddFuncInstance(instance);
			AddFuncParam(param[i]);
			AddFunctionCall(cf->script, cf->nr);
		}
	}else if (com->kind == KindBlock){
		SerializeBlock(com->block(), level + 1);
	}else{
		//DoError(string("type of command is unimplemented (call Michi!): ",Kind2Str(com->Kind)));
	}
	return ret;
}

void Serializer::SerializeBlock(Block *block, int level)
{
	msg_db_f("SerializeBlock", 4);
	for (int i=0;i<block->command.num;i++){
		stack_offset = cur_func->_var_size;
		next_command = NULL;
		if (block->command.num > i + 1)
			next_command = block->command[i + 1];

		// serialize
		SerializeCommand(block->command[i], level, i);
		
		// destruct new temp vars
		FillInDestructors(true);

		// any markers / jumps to add?
		for (int j=add_later.num-1;j>=0;j--)
			if ((level == add_later[j].level) && (i == add_later[j].index)){
				if (add_later[j].kind == STUFF_KIND_MARKER)
					add_marker(add_later[j].label);
				else if (add_later[j].kind == STUFF_KIND_JUMP)
					add_cmd(Asm::inst_jmp, param_marker(add_later[j].label));
				add_later.erase(j);
			}

		// end of loop?
		if (loop.num > 0)
			if ((loop.back().level == level) && (loop.back().index == i - 1))
				loop.pop();
	}
}

// modus: KindVarLocal/KindVarTemp
//    -1: -return-/new   -> don't destruct
void Serializer::add_cmd_constructor(SerialCommandParam &param, int modus)
{
	Type *class_type = param.type;
	if (modus == -1)
		class_type = class_type->parent;
	ClassFunction *f = class_type->GetDefaultConstructor();
	if (!f)
		return;
	if (modus == -1){
		AddFuncInstance(param);
	}else{
		SerialCommandParam inst;
		AddReference(param, TypePointer, inst);
		AddFuncInstance(inst);
	}

	AddClassFunctionCall(f);
	if (modus == KindVarTemp)
		inserted_constructor_temp.add(param);
	else if (modus == KindVarLocal)
		inserted_constructor_func.add(param);
}

void Serializer::add_cmd_destructor(SerialCommandParam &param, bool ref)
{
	if (ref){
		ClassFunction *f = param.type->GetDestructor();
		if (!f)
			return;
		SerialCommandParam inst;
		AddReference(param, TypePointer, inst);
		AddFuncInstance(inst);
		AddClassFunctionCall(f);
	}else{
		ClassFunction *f = param.type->parent->GetDestructor();
		if (!f)
			return;
		AddFuncInstance(param);
		AddClassFunctionCall(f);
	}
}

void Serializer::FillInConstructorsFunc()
{
	msg_db_f("FillInConstructorsFunc", 4);
	foreach(Variable &v, cur_func->var){
		SerialCommandParam param = param_local(v.type, v._offset);
		add_cmd_constructor(param, (v.name == "-return-") ? -1 : KindVarLocal);
	}
}

void Serializer::FillInDestructors(bool from_temp)
{
	msg_db_f("FillInDestructors", 4);
	if (from_temp){
		for (int i=0;i<inserted_constructor_temp.num;i++)
			add_cmd_destructor(inserted_constructor_temp[i]);
		inserted_constructor_temp.clear();
	}else{
		for (int i=0;i<inserted_constructor_func.num;i++)
			add_cmd_destructor(inserted_constructor_func[i]);
	}
}

int Serializer::temp_in_cmd(int c, int v)
{
	if (cmd[c].inst >= inst_marker)
		return 0;
	int r = 0;
	for (int i=0; i<SERIAL_COMMAND_NUM_PARAMS; i++)
		if ((cmd[c].p[i].kind == KindVarTemp) or (cmd[c].p[i].kind == KindDerefVarTemp))
			if ((long)cmd[c].p[i].p == v)
				r += (1<<i) + ((cmd[c].p[i].kind == KindDerefVarTemp) ? (8<<i) : 0);
	return r;
}

void Serializer::ScanTempVarUsage()
{
	msg_db_f("ScanTempVarUsage", 4);
	foreachi(TempVar &v, temp_var, i){
		v.first = -1;
		v.last = -1;
		v.count = 0;
		for (int c=0;c<cmd.num;c++){
			if (temp_in_cmd(c, i) > 0){
				v.count ++;
				if (v.first < 0)
					v.first = c;
				v.last = c;
			}
		}
	}
	temp_var_ranges_defined = true;
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

inline bool _____arm_param_combi_allowed(int inst, SerialCommandParam &p1, SerialCommandParam &p2, SerialCommandParam &p3)
{
//	if (inst >= Asm::inst_marker)
//		return true;
	if (inst == Asm::inst_mov)
		return (p1.kind == KindRegister) and (p2.kind == KindRegister);
	if (inst == Asm::inst_add)
		return (p1.kind == KindRegister) and (p2.kind == KindRegister) and (p3.kind == KindRegister);
	return true;
}

void inline arm_transfer_by_reg_in(Serializer *s, SerialCommand &c, int &i, int pno)
{
	SerialCommandParam p = c.p[pno];
	int r = s->find_unused_reg(i, i, /*p.type->size*/ 4, false);
	SerialCommandParam pr = param_reg(p.type, r);
	s->add_reg_channel(r, i, s->cmd.num);
	s->cmd[i].p[pno]  = pr;
	s->add_cmd(c.cond, Asm::inst_mov, pr, p, p_none);
	s->move_last_cmd(i);
	i ++;
}

void inline arm_transfer_by_reg_out(Serializer *s, SerialCommand &c, int &i, int pno)
{
	SerialCommandParam p = c.p[pno];
	int r = s->find_unused_reg(i, i, p.type->size, false);
	SerialCommandParam pr = param_reg(p.type, r);
	s->add_reg_channel(r, i, s->cmd.num);
	c.p[pno]  = pr;
	s->add_cmd(c.cond, Asm::inst_mov, p, pr, p_none);
	s->move_last_cmd(i+1);
}

void inline arm_gr_transfer_by_reg_in(Serializer *s, SerialCommand &c, int &i, int pno)
{
	SerialCommandParam p = c.p[pno];
	msg_write("in " + Kind2Str(p.kind));
	// cmd ..., global

	// mov r2, [ref]
	// ldr r1, [r2]
	// cmd ..., r1


	int r2 = s->find_unused_reg(i, i, 4, false);
	s->add_cmd(c.cond, Asm::inst_mov, param_reg(TypePointer, r2), param_marker(s->global_refs[p.p].label), p_none);
	s->move_last_cmd(i);

	int r1 = s->find_unused_reg(i+1, i+1, p.type->size, false);
	s->add_cmd(c.cond, Asm::inst_ldr, param_reg(p.type, r1), param_deref_reg(TypePointer, r2), p_none);
	s->move_last_cmd(i+1);

	s->cmd[i+2].p[pno] = param_reg(p.type, r1);

	s->add_reg_channel(r1, i + 1, i + 2);
	s->add_reg_channel(r2, i, i + 1);
	i += 2;
}

void inline arm_gr_transfer_by_reg_out(Serializer *s, SerialCommand &c, int &i, int pno)
{
	SerialCommandParam p = c.p[pno];
	msg_write("out " + c.str());
	// cmd global, ...

	// cmd r1, ...
	// mov r2, [ref]
	// str r1, [r2]


	int r2 = s->find_unused_reg(i, i, 4, false);
	s->add_cmd(c.cond, Asm::inst_mov, param_reg(TypePointer, r2), param_marker(s->global_refs[p.p].label), p_none);
	s->move_last_cmd(i+1);
	s->add_reg_channel(r2, i+1, i+1); // TODO... not exactly what we want...

	int r1 = s->find_unused_reg(i, i+1, p.type->size, false);
	s->add_cmd(c.cond, Asm::inst_str, param_reg(p.type, r1), param_deref_reg(TypePointer, r2), p_none);
	s->move_last_cmd(i+2);

	s->cmd[i].p[pno] = param_reg(p.type, r1);

	s->add_reg_channel(r1, i, i + 2);
}

inline bool is_data_op3(int inst)
{
	if (inst == Asm::inst_mov)
		return false;
	if (inst == Asm::inst_mul)
		return true;
	for (int i=0; i<16; i++)
		if (inst == Asm::ARMDataInstructions[i])
			return true;
	return false;
}

inline bool is_data_op2(int inst)
{
	if (inst == Asm::inst_mov)
		return true;
	if (inst == Asm::inst_cmp)
		return true;
	if (inst == Asm::inst_cmn)
		return true;
	if (inst == Asm::inst_tst)
		return true;
	if (inst == Asm::inst_teq)
		return true;
	return false;
}

void SerializerARM::CorrectUnallowedParamCombisGlobal()
{
	msg_db_f("CorrectCombis", 3);
	for (int i=cmd.num-1;i>=0;i--){
		if (cmd[i].inst >= inst_marker)
			continue;

		if (cmd[i].inst == Asm::inst_mov){
			if (cmd[i].p[1].kind == KindGlobalLookup){
				arm_gr_transfer_by_reg_in(this, cmd[i], i, 1);
			}
			if (cmd[i].p[0].kind == KindGlobalLookup){
				arm_gr_transfer_by_reg_out(this, cmd[i], i, 0);
			}
		}/*else if (is_data_op2(cmd[i].inst)){
			if (cmd[i].p[1].kind != KindRegister){
				arm_transfer_by_reg_in(this, cmd[i], i, 1);
			}
			if (cmd[i].p[0].kind != KindRegister){
				arm_transfer_by_reg_in(this, cmd[i], i, 0);
			}
		}else if (is_data_op3(cmd[i].inst)){
			if (cmd[i].p[1].kind != KindRegister){
				arm_transfer_by_reg_in(this, cmd[i], i, 1);
			}
			if (cmd[i].p[2].kind != KindRegister){
				arm_transfer_by_reg_in(this, cmd[i], i, 2);
			}
			if (cmd[i].p[0].kind != KindRegister){
				arm_transfer_by_reg_out(this, cmd[i], i, 0);
			}
		}*/
	}
	ScanTempVarUsage();
}

void SerializerARM::CorrectUnallowedParamCombis()
{
	msg_db_f("CorrectCombis", 3);
	for (int i=cmd.num-1;i>=0;i--){
		if (cmd[i].inst >= inst_marker)
			continue;

		if (cmd[i].inst == Asm::inst_mov){
			if ((cmd[i].p[0].kind != KindRegister) and (cmd[i].p[1].kind != KindRegister)){
				arm_transfer_by_reg_in(this, cmd[i], i, 1);
			}
		}else if (is_data_op2(cmd[i].inst)){
			if (cmd[i].p[1].kind != KindRegister){
				arm_transfer_by_reg_in(this, cmd[i], i, 1);
			}
			if (cmd[i].p[0].kind != KindRegister){
				arm_transfer_by_reg_in(this, cmd[i], i, 0);
			}
		}else if (is_data_op3(cmd[i].inst)){
			if (cmd[i].p[1].kind != KindRegister){
				arm_transfer_by_reg_in(this, cmd[i], i, 1);
			}
			if (cmd[i].p[2].kind != KindRegister){
				arm_transfer_by_reg_in(this, cmd[i], i, 2);
			}
			if (cmd[i].p[0].kind != KindRegister){
				arm_transfer_by_reg_out(this, cmd[i], i, 0);
			}
		}
	}
	ScanTempVarUsage();
}

int Serializer::find_unused_reg(int first, int last, int size, bool allow_eax)
{
	for (int r=0;r<map_reg_root.num;r++)
		if (!is_reg_root_used_in_interval(map_reg_root[r], first, last))
			return get_reg(map_reg_root[r], size);
	if (allow_eax)
		if (!is_reg_root_used_in_interval(0, first, last))
			return get_reg(0, size);
	return -1;
}

// inst ... [local] ...
// ->      mov reg, local     inst ...[reg]...
void Serializer::solve_deref_temp_local(int c, int np, bool is_local)
{
	SerialCommandParam *pp = (np == 0) ? &cmd[c].p[0] : &cmd[c].p[1];
	SerialCommandParam p = *pp;
	int shift = p.shift;

	Type *type_pointer = is_local ? TypePointer : temp_var[(long)p.p].type;
	Type *type_data = p.type;
	
	p.kind = is_local ? KindVarLocal : KindVarTemp;
	p.shift = 0;
	p.type = type_pointer;

	int reg = find_unused_reg(c, c, config.pointer_size, true);
	if (reg < 0)
		script->DoErrorInternal("solve_deref_temp_local... no registers available");
	SerialCommandParam p_reg = param_reg(type_pointer, reg);
	SerialCommandParam p_deref_reg;
	p_deref_reg.kind = KindDerefRegister;
	p_deref_reg.p = reg;
	p_deref_reg.type = type_data;
	p_deref_reg.shift = 0;
	
	*pp = p_deref_reg;
		
	add_cmd(Asm::inst_mov, p_reg, p);
	move_last_cmd(c);
	if (shift > 0){
		// solve_deref_temp_local
		add_cmd(Asm::inst_add, p_reg, param_const(TypeInt, shift));
		move_last_cmd(c + 1);
		add_reg_channel(reg, c, c + 2);
	}else
		add_reg_channel(reg, c, c + 1);
}

#if 0
void ResolveDerefLocal()
{
	msg_db_f("ResolveDerefLocal", 3);
	for (int i=cmd.num-1;i>=0;i--){
		if (cmd[i].inst >= Asm::inst_marker)
			continue;
		bool dl1 = (cmd[i].p[0].kind == KindDerefVarLocal);
		bool dl2 = (cmd[i].p[1].kind == KindDerefVarLocal);
		if (!(dl1 || dl2))
			continue;
		
		so(string2("deref local... cmd=%d", i));
		if (!dl2){
			solve_deref_temp(i, 0);
			i ++;
		}else if (!dl1){
			solve_deref_temp(i, 1);
			i ++;
		}else{
			// hopefully... p2 is read-only

			int reg = find_unused_reg(i, i, true);
			if (reg < 0)
				script->DoErrorInternal("deref local... both sides... .no registers available");
			SerialCommandParam p_reg = param_reg(TypeReg32, reg);
			add_reg_channel(reg, i, i); // temp
			
			int reg2 = find_unused_reg(i, i, true);
			if (reg2 < 0)
				script->DoErrorInternal("deref local... both sides... .no registers available");
			SerialCommandParam p_reg2 = param_reg(TypeReg32, reg2);
			SerialCommandParam p_deref_reg2;
			p_deref_reg2.kind = KindDerefRegister;
			p_deref_reg2.p = (char*)reg2;
			p_deref_reg2.type = TypePointer;
			p_deref_reg2.shift = 0;
			reg_channel.resize(reg_channel.num - 1); // remove temp reg channel...

			// inst [l1] [l2]
			// ->
			// mov reg2, l1
			// mov reg, [reg2]
			// mov reg2, l2
			// inst [reg2], reg
			SerialCommandParam p1 = cmd[i].p[0];
			SerialCommandParam p2 = cmd[i].p[1];
			if (p1.shift + p2.shift > 0)
				script->DoErrorInternal("deref local... both sides... shift");
			p1.kind = p2.kind = KindVarLocal;
			cmd[i].p[0] = p_deref_reg2;
			cmd[i].p[1] = p_reg;
	
			add_cmd(Asm::inst_mov, p_reg2, p2);
			move_last_cmd(i);
	
			add_cmd(Asm::inst_mov, p_reg, p_deref_reg2);
			move_last_cmd(i + 1);
	
			add_cmd(Asm::inst_mov, p_reg2, p1);
			move_last_cmd(i + 2);

			add_reg_channel(reg, i + 1, i + 3);
			add_reg_channel(reg2, i, i + 3);
				
			i += 3;
		}
	}
}
#endif


void Serializer::ResolveDerefTempAndLocal()
{
	msg_db_f("ResolveDerefTempAndLocal", 3);
	for (int i=cmd.num-1;i>=0;i--){
		if (cmd[i].inst >= inst_marker)
			continue;
		bool dl1 = ((cmd[i].p[0].kind == KindDerefVarLocal) || (cmd[i].p[0].kind == KindDerefVarTemp));
		bool dl2 = ((cmd[i].p[1].kind == KindDerefVarLocal) || (cmd[i].p[1].kind == KindDerefVarTemp));
		if (!(dl1 || dl2))
			continue;

		bool is_local1 = (cmd[i].p[0].kind == KindDerefVarLocal);
		bool is_local2 = (cmd[i].p[1].kind == KindDerefVarLocal);
		
		//msg_write(format("deref temp/local... cmd=%d", i));
		if (!dl2){
			solve_deref_temp_local(i, 0, is_local1);
			i ++;
		}else if (!dl1){
			solve_deref_temp_local(i, 1, is_local2);
			i ++;
		}else{
			// hopefully... p2 is read-only

			Type *type_pointer = TypePointer;
			Type *type_data = cmd[i].p[0].type;

			int reg = find_unused_reg(i, i, type_data->size, true);
			if (reg < 0)
				DoError("deref local... both sides... .no registers available");
			
			SerialCommandParam p_reg = param_reg(type_data, reg);
			add_reg_channel(reg, i, i); // temp
			
			int reg2 = find_unused_reg(i, i, config.pointer_size, true);
			if (reg2 < 0)
				DoError("deref temp/local... both sides... .no registers available");
			SerialCommandParam p_reg2 = param_reg(type_pointer, reg2);
			SerialCommandParam p_deref_reg2;
			p_deref_reg2.kind = KindDerefRegister;
			p_deref_reg2.p = reg2;
			p_deref_reg2.type = type_data;
			p_deref_reg2.shift = 0;
			reg_channel.pop(); // remove temp reg channel...

			// inst [l1] [l2]
			// ->
			// mov reg2, l2
			//   (add reg2, shift2)
			// mov reg, [reg2]
			// mov reg2, l1
			//   (add reg2, shift1)
			// inst [reg2], reg
			SerialCommandParam p1 = cmd[i].p[0];
			SerialCommandParam p2 = cmd[i].p[1];
			long shift1 = p1.shift;
			long shift2 = p2.shift;
			p1.shift = p2.shift = 0;
			
			p1.kind = is_local1 ? KindVarLocal : KindVarTemp;
			p2.kind = is_local2 ? KindVarLocal : KindVarTemp;
			p1.type = type_pointer;
			p2.type = type_pointer;
			cmd[i].p[0] = p_deref_reg2;
			cmd[i].p[1] = p_reg;
			int cmd_pos = i;

			int r2_first = cmd_pos;
			add_cmd(Asm::inst_mov, p_reg2, p2);
			move_last_cmd(cmd_pos ++);

			if (shift2 > 0){
				// resolve deref temp&loc 2
				add_cmd(Asm::inst_add, p_reg2, param_const(TypeInt, shift2));
				move_last_cmd(cmd_pos ++);
			}

			int r1_first = cmd_pos;
			add_cmd(Asm::inst_mov, p_reg, p_deref_reg2);
			move_last_cmd(cmd_pos ++);
	
			add_cmd(Asm::inst_mov, p_reg2, p1);
			move_last_cmd(cmd_pos ++);

			if (shift1 > 0){
				// resolve deref temp&loc 1
				add_cmd(Asm::inst_add, p_reg2, param_const(TypeInt, shift1));
				move_last_cmd(cmd_pos ++);
			}

			add_reg_channel(reg, r1_first, cmd_pos);
			add_reg_channel(reg2, r2_first, cmd_pos);
				
			i = cmd_pos;
		}
	}
}

bool Serializer::ParamUntouchedInInterval(SerialCommandParam &p, int first, int last)
{
	// direct usage?
	for (int i=first;i<=last;i++)
		if ((cmd[i].p[0] == p) || (cmd[i].p[1] == p))
			return false;
	
	// registers may be more subtle..
	if ((p.kind == KindRegister) || (p.kind == KindDerefRegister)){
		for (int i=first;i<=last;i++){
			
			// call violates all!
			if (cmd[i].inst == Asm::inst_call)
				return false;

			// div violates eax and edx
			if (cmd[i].inst == Asm::inst_div)
				if (((long)p.p == Asm::REG_EDX) || ((long)p.p == Asm::REG_EAX))
					return false;

			// registers used? (may be part of the same meta-register)
			if ((cmd[i].p[0].kind == KindRegister) || (cmd[i].p[0].kind == KindDerefRegister))
				if (Asm::RegRoot[(long)cmd[i].p[0].p] == Asm::RegRoot[(long)p.p])
					return false;
			if ((cmd[i].p[1].kind == KindRegister) || (cmd[i].p[1].kind == KindDerefRegister))
				if (Asm::RegRoot[(long)cmd[i].p[1].p] == Asm::RegRoot[(long)p.p])
					return false;
		}
	}
	return true;
}

void Serializer::SimplifyFPUStack()
{
	msg_db_f("SimplifyFPUStack", 3);

// fstp temp
// fld temp
	for (int vi=temp_var.num-1;vi>=0;vi--){
		TempVar &v = temp_var[vi];
		if (v.first < 0)
			continue;

		// may only appear two times
		if (v.count > 2)
			continue;

		// stored then loaded...?
		if ((cmd[v.first].inst != Asm::inst_fstp) || (cmd[v.last].inst != Asm::inst_fld))
			continue;

		// value still on the stack?
		int d_stack = 0, min_d_stack = 0, max_d_stack = 0;
		for (int i=v.first + 1;i<v.last;i++){
			if (cmd[i].inst == Asm::inst_fld)
				d_stack ++;
			else if (cmd[i].inst == Asm::inst_fstp)
				d_stack --;
			min_d_stack = min(min_d_stack, d_stack);
			max_d_stack = max(max_d_stack, d_stack);
		}
		if ((d_stack != 0) || (min_d_stack < 0) || (max_d_stack > 5))
			continue;

		// reuse value on the stack
//		msg_write(format("fpu (a)  var=%d first=%d last=%d stack: d=%d min=%d max=%d", vi, v.first, v.last, d_stack, min_d_stack, max_d_stack));
		remove_cmd(v.last);
		remove_cmd(v.first);
		remove_temp_var(vi);
	}

// fstp temp
// mov xxx, temp
	for (int vi=temp_var.num-1;vi>=0;vi--){
		TempVar &v = temp_var[vi];
		if (v.first < 0)
			continue;

		// may only appear two times
		if (v.count > 2)
			continue;

		// stored then moved...?
		if ((cmd[v.first].inst != Asm::inst_fstp) || (cmd[v.last].inst != Asm::inst_mov))
			continue;
		if (temp_in_cmd(v.last, vi) != 2)
			continue;
		// moved into fstore'able?
		int kind = cmd[v.last].p[0].kind;
		if ((kind != KindVarLocal) && (kind != KindVarGlobal) && (kind != KindVarTemp) && (kind != KindDerefVarTemp) && (kind != KindDerefRegister))
		    continue;

		// check, if mov target is used in between
		SerialCommandParam target = cmd[v.last].p[0];
		if (!ParamUntouchedInInterval(target, v.first + 1 ,v.last - 1))
			continue;
		// ...we are lazy...
		//if (v.last - v.first != 1)
		//	continue;

		// store directly into target
//		msg_write(format("fpu (b)  var=%d first=%d last=%d", v, v.first, v.last));
		cmd[v.first].p[0] = target;
		move_param(target, v.last, v.first);
		remove_cmd(v.last);
		remove_temp_var(vi);
	}
}

// remove  mov x,...    mov ...,x   (and similar)
//    (does not affect reg channels)
void Serializer::SimplifyMovs()
{
	// TODO: count > 2 .... first == input && all_other == output?  (only if first == mov (!=eax?)... else count == 2)
	// should take care of fpu simplification (b)...

	msg_db_f("SimplifyMovs", 3);
	for (int vi=temp_var.num-1;vi>=0;vi--){
		TempVar &v = temp_var[vi];
		if (v.first < 0)
			continue;
		
		// may only appear two times
		if (v.count > 2)
			continue;

		// both times in a mov command (or fld as second)
		if (cmd[v.first].inst != Asm::inst_mov)
			continue;
		int n = cmd[v.last].inst;
		bool fld = (n == Asm::inst_fld) || (n == Asm::inst_fadd) || (n == Asm::inst_fadd) || (n == Asm::inst_fsub) || (n == Asm::inst_fmul) || (n == Asm::inst_fdiv);
		if ((cmd[v.last].inst != Asm::inst_mov) && (!fld))
			continue;
		
		// used as source/target?   no deref?
		if ((temp_in_cmd(v.first, vi) != 1) || (temp_in_cmd(v.last, vi) != (fld ? 1 : 2)))
			continue;

		// new construction allowed?
		SerialCommandParam target = cmd[v.last].p[0];
		SerialCommandParam source = cmd[v.first].p[1];
		if (fld){
			if (!param_combi_allowed(cmd[v.last].inst, source, cmd[v.last].p[1]))
				continue;
		}else{
			if (!param_combi_allowed(cmd[v.last].inst, cmd[v.last].p[0], source))
				continue;
		}

		// check, if mov source or target are used in between
		if (!ParamUntouchedInInterval(target, v.first + 1 ,v.last - 1))
			continue;
		if (!ParamUntouchedInInterval(source, v.first + 1 ,v.last - 1))
			continue;
		
//		msg_write(format("mov simplification allowed  v=%d first=%d last=%d", vi, v.first, v.last));
		if (fld)
			cmd[v.last].p[0] = source;
		else
			cmd[v.last].p[1] = source;
		move_param(source, v.first, v.last);
		remove_cmd(v.first);
		remove_temp_var(vi);
	}

	// TODO: should happen automatically...
	//ScanTempVarUsage();
	//cmd_list_out();
}

void Serializer::RemoveUnusedTempVars()
{
	// unused temp vars...
	for (int v=temp_var.num-1;v>=0;v--)
		if (temp_var[v].first < 0){
			remove_temp_var(v);
		}
}

/*inline void test_reg_usage(int c)
{
	// call -> violates all...
	if (cmd[c].inst == Asm::inst_call){
		for (int i=0;i<max_reg;i++)
			RegUsed[i] = true;
		return;
	}
	if ((cmd[c].p[0].kind == KindRegister) || (cmd[c].p[0].kind == KindDerefRegister))
		set_reg_used((long)cmd[c].p[0].p);
	if ((cmd[c].p[1].kind == KindRegister) || (cmd[c].p[1].kind == KindDerefRegister))
		set_reg_used((long)cmd[c].p[1].p);
}*/

void Serializer::MapTempVarToReg(int vi, int reg)
{
	msg_db_f("reg", 4);
	TempVar &v = temp_var[vi];
//	msg_write(format("temp=reg:  %d - %d:   tv %d := reg %d", v.first, v.last, vi, reg));
	
	SerialCommandParam p = param_reg(v.type, reg);
	
	// map register
	for (int i=v.first;i<=v.last;i++){
		int r = temp_in_cmd(i, vi);
		if (r & 1){
			p.shift = cmd[i].p[0].shift;
			cmd[i].p[0] = p;
			if (r & 4)
				cmd[i].p[0].kind = KindDerefRegister;
		}
		if (r & 2){
			p.shift = cmd[i].p[1].shift;
			cmd[i].p[1] = p;
			if (r & 8)
				cmd[i].p[1].kind = KindDerefRegister;
		}
	}
	add_reg_channel(reg, v.first, v.last);
}

void Serializer::add_stack_var(Type *type, int first, int last, SerialCommandParam &p)
{
	// find free stack space for the life span of the variable....
	// don't mind... use old algorithm: ALWAYS grow stack... remove ALL on each command in a block

	int s = mem_align(type->size, 4);
	stack_offset += s;
	int offset = - stack_offset;
	if (config.instruction_set == Asm::INSTRUCTION_SET_ARM)
		offset = stack_offset;
	if (stack_offset > stack_max_size)
		stack_max_size = stack_offset;

	p.kind = KindVarLocal;
	p.p = offset;
	p.type = type;
	p.shift = 0;
}

void Serializer::MapTempVarToStack(int vi)
{
	msg_db_f("stack", 4);
	TempVar &v = temp_var[vi];
//	msg_write(format("temp=stack: %d   (%d - %d)", vi, v.first, v.last));

	SerialCommandParam p;
	add_stack_var(v.type, v.first, v.last, p);
	
	// map
	for (int i=v.first;i<=v.last;i++){
		int r = temp_in_cmd(i, vi);
		if (r == 0)
			continue;

		if ((r & 3) == 3)
			script->DoErrorInternal("asm error: (MapTempVar) temp var on both sides of command");
		
		SerialCommandParam *p_own;
		if ((r & 1) > 0){
			p_own = &cmd[i].p[0];
		}else{
			p_own = &cmd[i].p[1];
		}
		bool deref = (r > 3);

		p_own->kind = deref ? KindDerefVarLocal : KindVarLocal;
		p_own->p = p.p;
		
#if 0
		SerialCommandParam *p_own, *p_other;
		if ((r & 1) > 0){
			p_own = &cmd[i].p[0];
			p_other = &cmd[i].p[1];
		}else{
			p_own = &cmd[i].p[1];
			p_other = &cmd[i].p[0];
		}
		bool deref = (r > 3);

		// allowed directly?
		if (!deref){
			if (param_is_simple(*p_other)){
				*p_own = p;
				continue;
			}
		}

		// TODO: map literally... solve unallowed combis later...?

		// is our variable used for writing... or reading?
		bool var_read = false;
		bool var_write = false;
		int c = cmd[i].inst;
		bool dummy1, dummy2;
		if ((r & 1) > 0)
			GetInstructionParamFlags(cmd[i].inst, var_read, var_write, dummy1, dummy2);
		else if ((r & 2) > 0)
			GetInstructionParamFlags(cmd[i].inst, dummy1, dummy2, var_read, var_write);

		if ((var_write) && (var_read)){ // rw
			if (deref)
				script->DoErrorInternal("map_stack_var (read, write, deref)");
			*p_own = p_eax;
			add_cmd(Asm::inst_mov, p_eax, p);
			move_last_cmd(i);
			add_cmd(Asm::inst_mov, p, p_eax);
			move_last_cmd(i+2);
			add_reg_channel(REG_EAX, i, i + 2);
			i += 2;
			
		}else if (var_write){ // write only
			if (deref){
				//script->DoErrorInternal("map_stack_var (write, deref)");
				int shift = p_own->shift;
				*p_own = p_deref_eax;
				add_cmd(Asm::inst_mov, p_eax, p);
				move_last_cmd(i);
				if (shift > 0){
					add_cmd(Asm::inst_add, p_eax, param_const(TypeInt, (void*)shift));
					move_last_cmd(i + 1);
					add_reg_channel(REG_EAX, i, i + 2);
				}else
					add_reg_channel(REG_EAX, i, i + 1);
			}else{
				*p_own = p_eax;
				add_cmd(Asm::inst_mov, p, p_eax);
				move_last_cmd(i+1);
				add_reg_channel(REG_EAX, i, i + 1);
			}
			i ++;
		}else{ // read only
			int shift = p_own->shift;
			*p_own = deref ? p_deref_eax : p_eax;
			add_cmd(Asm::inst_mov, p_eax, p);
			move_last_cmd(i);
			if ((deref) && (shift > 0)){
				add_cmd(Asm::inst_add, p_eax, param_const(TypeInt, (void*)shift));
				move_last_cmd(i + 1);
				add_reg_channel(REG_EAX, i, i + 2);
			}else
				add_reg_channel(REG_EAX, i, i + 1);
			i ++;
		}
#endif
	}
}

bool Serializer::is_reg_root_used_in_interval(int reg_root, int first, int last)
{
	for (int i=0;i<reg_channel.num;i++)
		if (reg_channel[i].reg_root == reg_root){
			if ((reg_channel[i].first <= last) && (reg_channel[i].last >= first)){
				return true;
			}
		}
	return false;
}

void Serializer::MapTempVar(int vi)
{
	msg_db_f("MapTempVar", 4);
	TempVar &v = temp_var[vi];
	int first = v.first;
	int last = v.last;
	if (first < 0)
		return;

	bool reg_allowed = true;
	for (int i=first;i<=last;i++)
		if (temp_in_cmd(i, vi))
			if (!Asm::GetInstructionAllowGenReg(cmd[i].inst)){
				reg_allowed = false;
				break;
			}

	int reg = -1;
	if (reg_allowed){

		// any register not used in this interval?
		for (int i=0;i<max_reg;i++)
			reg_root_used[i] = false;
		for (int i=0;i<reg_channel.num;i++)
			if ((reg_channel[i].first <= last) && (reg_channel[i].last >= first))
				reg_root_used[reg_channel[i].reg_root] = true;
		for (int i=0;i<map_reg_root.num;i++)
			if (!reg_root_used[map_reg_root[i]]){
				reg = get_reg(map_reg_root[i], v.type->size);
				break;
			}
	}

	if (reg >= 0)
		MapTempVarToReg(vi, reg);
	else
		MapTempVarToStack(vi);
}

void Serializer::MapTempVars()
{
	msg_db_f("MapTempVars", 3);

	for (int i=0;i<temp_var.num;i++)
		MapTempVar(i);
	
	//cmd_list_out();
}

inline void try_map_param_to_stack(SerialCommandParam &p, int v, SerialCommandParam &stackvar)
{
	if ((p.kind == KindVarTemp) && ((long)p.p == v)){
		p.kind = KindVarLocal;//stackvar.kind;
		p.p = stackvar.p;
	}else if ((p.kind == KindDerefVarTemp) && ((long)p.p == v)){
		p.kind = KindDerefVarLocal;
		p.p = stackvar.p;
	}
}

void Serializer::MapReferencedTempVars()
{
	msg_db_f("MapReferencedTempVars", 3);
	for (int i=0;i<cmd.num;i++)
		if (cmd[i].inst == Asm::inst_lea)
			if (cmd[i].p[1].kind == KindVarTemp){
				temp_var[(long)cmd[i].p[1].p].force_stack = true;
			}

	for (int i=temp_var.num-1;i>=0;i--)
		if (temp_var[i].force_stack){
			SerialCommandParam stackvar;
			add_stack_var(temp_var[i].type, temp_var[i].first, temp_var[i].last, stackvar);
			for (int j=0;j<cmd.num;j++){
				try_map_param_to_stack(cmd[j].p[0], i, stackvar);
				try_map_param_to_stack(cmd[j].p[1], i, stackvar);
			}
			remove_temp_var(i);
		}
}

void Serializer::DisentangleShiftedTempVars()
{
	msg_db_f("DisentangleShiftedTempVars", 3);
	for (int i=0;i<cmd.num;i++){
		if ((cmd[i].p[0].kind == KindVarTemp) && (cmd[i].p[0].shift > 0)){
			temp_var[(long)cmd[i].p[0].p].entangled = max(temp_var[(long)cmd[i].p[0].p].entangled, cmd[i].p[0].shift);
		}
		if ((cmd[i].p[1].kind == KindVarTemp) && (cmd[i].p[1].shift > 0)){
			temp_var[(long)cmd[i].p[1].p].entangled = max(temp_var[(long)cmd[i].p[1].p].entangled, cmd[i].p[1].shift);
		}
	}
	for (int i=temp_var.num-1;i>=0;i--)
		if (temp_var[i].entangled > 0){
			int n = temp_var[i].entangled / 4 + 1;
			Type *t = temp_var[i].type;
			// entangled
			SerialCommandParam *p = new SerialCommandParam[n];

			// create small temp vars
			for (int j=0;j<n;j++){
				Type *tt = TypeReg32;
				// corresponding to element in a class?
				for (int k=0;k<t->element.num;k++)
					if (t->element[k].offset == j * 4)
						if (t->element[k].type->size == 4)
							tt = t->element[k].type;
				add_temp(tt, p[j]);
			}
			
			for (int j=0;j<cmd.num;j++){
				if ((cmd[j].p[0].kind == KindVarTemp) && ((long)cmd[j].p[0].p == i))
					cmd[j].p[0] = p[cmd[j].p[0].shift / 4];
				if ((cmd[j].p[1].kind == KindVarTemp) && ((long)cmd[j].p[1].p == i))
					cmd[j].p[1] = p[cmd[j].p[1].shift / 4];
			}
			delete[]p;
			remove_temp_var(i);
		}

	ScanTempVarUsage();
}

inline void _resolve_deref_reg_shift_(Serializer *_s, SerialCommandParam &p, int i)
{
	long s = p.shift;
	p.shift = 0;
	msg_write("_resolve_deref_reg_shift_");
	int reg = reg_resize((long)p.p, 4);
	_s->add_cmd(Asm::inst_add, param_reg(TypeReg32, reg), param_const(TypeInt, s));
	_s->move_last_cmd(i);
	_s->add_cmd(Asm::inst_sub, param_reg(TypeReg32, reg), param_const(TypeInt, s));
	_s->move_last_cmd(i + 2);
}

// TODO....
void Serializer::ResolveDerefRegShift()
{
	msg_db_f("ResolveDerefRegShift", 3);
	for (int i=cmd.num-1;i>=0;i--){
		if ((cmd[i].p[0].kind == KindDerefRegister) && (cmd[i].p[0].shift > 0)){
			_resolve_deref_reg_shift_(this, cmd[i].p[0], i);
			continue;
		}
		if ((cmd[i].p[1].kind == KindDerefRegister) && (cmd[i].p[1].shift > 0)){
			_resolve_deref_reg_shift_(this, cmd[i].p[1], i);
			continue;
		}
	}
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
		add_cmd(Asm::inst_movss, param_local(p.type, p._offset), param_reg(p.type, reg));
	}

	// rdi, rsi,rdx, rcx, r8, r9
	int param_regs_root[6] = {7, 6, 2, 1, 8, 9};
	foreachib(Variable &p, reg_param, i){
		int root = param_regs_root[i];
		int reg = get_reg(root, p.type->size);
		if (reg >= 0){
			add_cmd(Asm::inst_mov, param_local(p.type, p._offset), param_reg(p.type, reg));
			add_reg_channel(reg, cmd.num - 1, cmd.num - 1);
		}else{
			// some registers are not 8bit'able
			add_cmd(Asm::inst_mov, p_eax, param_reg(TypeReg32, get_reg(root, 4)));
			add_cmd(Asm::inst_mov, param_local(p.type, p._offset), param_reg(p.type, get_reg(0, p.type->size)));
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
	add_cmd(Asm::inst_leave);
	add_cmd(Asm::inst_ret);
}

void SerializerARM::AddFunctionIntro(Function *f)
{

	// return, instance, params
	Array<Variable> param;
	if (f->return_type->UsesReturnByMemory()){
		DoError("arm return by mem");
		foreach(Variable &v, f->var)
			if (v.name == "-return-"){
				param.add(v);
				break;
			}
	}
	if (f->_class){
		DoError("arm self");
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
			if (reg_param.num < 4){
				reg_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else if (p.type == TypeFloat32){
			DoError("arm float param");
			if (xmm_param.num < 8){
				xmm_param.add(p);
			}else{
				stack_param.add(p);
			}
		}else
			DoError("parameter type currently not supported: " + p.type->name);
	}

	// xmm0-7
	/*foreachib(Variable &p, xmm_param, i){
		int reg = Asm::REG_XMM0 + i;
		add_cmd(Asm::inst_movss, param_local(p.type, p._offset), param_reg(p.type, reg));
	}*/

	// rdi, rsi,rdx, rcx, r8, r9
	int param_regs[4] = {Asm::REG_R0, Asm::REG_R1, Asm::REG_R2, Asm::REG_R3};
	foreachib(Variable &p, reg_param, i){
		int reg = param_regs[i];
		add_cmd(Asm::inst_mov, param_local(p.type, p._offset), param_reg(p.type, reg));
		add_reg_channel(reg, cmd.num - 1, cmd.num - 1);
	}

	// get parameters from stack
	foreachb(Variable &p, stack_param){
		DoError("func with stack...");
		/*int s = 8;
		add_cmd(Asm::inst_push, p);
		push_size += s;*/
	}
}

void SerializerARM::AddFunctionOutro(Function *f)
{
	// will be translated into a "real" outro later...
	add_cmd(Asm::inst_ret);
}

void init_serializing()
{
	p_eax = param_reg(TypeReg32, Asm::REG_EAX);
	p_eax_int = param_reg(TypeInt, Asm::REG_EAX);
	p_rax = param_reg(TypeReg64, Asm::REG_RAX);

	p_deref_eax = param_deref_reg(TypePointer, Asm::REG_EAX);
	
	p_ax = param_reg(TypeReg16, Asm::REG_AX);
	p_al = param_reg(TypeReg8, Asm::REG_AL);
	p_al_bool = param_reg(TypeBool, Asm::REG_AL);
	p_al_char = param_reg(TypeChar, Asm::REG_AL);
	p_ah = param_reg(TypeReg8, Asm::REG_AH);
	p_st0 = param_reg(TypeFloat32, Asm::REG_ST0);
	p_st1 = param_reg(TypeFloat32, Asm::REG_ST1);
}

void Serializer::SerializeFunction(Function *f)
{
	msg_db_f("SerializeFunction", 2);

	init_serializing();

	syntax_tree->CreateAsmMetaInfo();
	syntax_tree->AsmMetaInfo->line_offset = 0;
	Asm::CurrentMetaInfo = syntax_tree->AsmMetaInfo;

	cur_func = f;
	num_markers = 0;
	call_used = false;
	stack_offset = f->_var_size;
	stack_max_size = f->_var_size;
	temp_var_ranges_defined = false;
	

	if (config.instruction_set == Asm::INSTRUCTION_SET_ARM){
		for (int i=0; i<12; i++)
			map_reg_root.add(i);//Asm::REG_R0+i);

	}else{
	if (config.allow_registers){
	//	MapRegRoot.add(0); // eax
		map_reg_root.add(1); // ecx
		map_reg_root.add(2); // edx
	//	MapRegRoot.add(3); // ebx
	//	MapRegRoot.add(6); // esi
	//	MapRegRoot.add(7); // edi
	}
	}

// serialize

	AddFunctionIntro(f);

	FillInConstructorsFunc();

	// function
	SerializeBlock(f->block, 0);
	ScanTempVarUsage();

	SimplifyIfStatements();
	TryMergeTempVars();
	SimplifyFloatStore();

	if (script->syntax->FlagShow)
		cmd_list_out();
	


	// outro (if last command != return)
	bool need_outro = true;
	if (f->block->command.num > 0)
		if ((f->block->command.back()->kind == KindCompilerFunction) && (f->block->command.back()->link_no == CommandReturn))
			need_outro = false;
	if (need_outro){
		FillInDestructors(false);
		AddFunctionOutro(f);
	}


	if (add_later.num > 0){
		msg_write(f->name);
		msg_write(add_later.num);
		for (int i=0;i<add_later.num;i++){
			msg_write(add_later[i].kind);
			msg_write(add_later[i].label);
			msg_write(add_later[i].index);
			msg_write(add_later[i].level);
		}
		DoError("StuffToAdd");
	}

	//cmd_list_out();
}


void Serializer::SimplifyIfStatements()
{
	for (int i=0;i<cmd.num - 4;i++){
		if ((cmd[i].inst == Asm::inst_cmp) && (cmd[i+2].inst == Asm::inst_cmp) && (cmd[i+3].inst == Asm::inst_jz)){
			if (cmd[i+1].inst == Asm::inst_setl)
				cmd[i+3].inst = Asm::inst_jnl;
			else if (cmd[i+1].inst == Asm::inst_setle)
				cmd[i+3].inst = Asm::inst_jnle;
			else if (cmd[i+1].inst == Asm::inst_setnl)
				cmd[i+3].inst = Asm::inst_jl;
			else if (cmd[i+1].inst == Asm::inst_setnle)
				cmd[i+3].inst = Asm::inst_jle;
			else if (cmd[i+1].inst == Asm::inst_setz)
				cmd[i+3].inst = Asm::inst_jnz;
			else if (cmd[i+1].inst == Asm::inst_setnz)
				cmd[i+3].inst = Asm::inst_jz;
			else
				continue;

			remove_cmd(i + 2);
			remove_cmd(i + 1);
		}
	}
}

void Serializer::TryMergeTempVars()
{
	return;
	for (int i=0;i<cmd.num;i++)
		if (cmd[i].inst == Asm::inst_mov)
			if ((cmd[i].p[0].kind == KindVarTemp) && (cmd[i].p[1].kind == KindVarTemp)){
				int v1 = (long)cmd[i].p[0].p;
				int v2 = (long)cmd[i].p[1].p;
				if ((temp_var[v1].first == i) && (temp_var[v2].last == i)){
					// swap v1 -> v2
					for (int j=i+1;j<=temp_var[v1].last;j++){
						if (((cmd[j].p[0].kind == KindVarTemp) || (cmd[j].p[0].kind == KindDerefVarTemp)) && ((long)cmd[j].p[0].p == v1))
							cmd[j].p[0].p = v2;
						if (((cmd[j].p[1].kind == KindVarTemp) || (cmd[j].p[1].kind == KindDerefVarTemp)) && ((long)cmd[j].p[1].p == v1))
							cmd[j].p[1].p = v2;
					}
					temp_var[v2].last = temp_var[v1].last;
				}
				remove_cmd(i);
				remove_temp_var(v1);
			}
}

void Serializer::SimplifyFloatStore()
{
	for (int i=0;i<cmd.num - 1;i++){
		if ((cmd[i].inst == Asm::inst_fstp) && (cmd[i+1].inst == Asm::inst_mov)){
			if (cmd[i].p[0].kind == KindVarTemp){
				int v = (long)cmd[i].p[0].p;
				if ((temp_var[v].first == i) && (temp_var[v].last == i+1)){
					cmd[i].p[0] = cmd[i+1].p[0];
					remove_cmd(i + 1);
					remove_temp_var(v);
				}
			}
		}
	}
}


void Serializer::FindReferencedTempVars()
{
	msg_db_f("MapRemainingTempVarsToStack", 3);
	for (int i=0;i<cmd.num;i++)
		if (cmd[i].inst == Asm::inst_lea)
			if (cmd[i].p[1].kind == KindVarTemp){
				temp_var[(long)cmd[i].p[1].p].force_stack = true;
			}
}
void Serializer::TryMapTempVarsRegisters()
{
	msg_db_f("TryMapTempVarsRegisters", 3);
	for (int i=temp_var.num-1;i>=0;i--){
		if (temp_var[i].force_stack)
			continue;
	}
}
void Serializer::MapRemainingTempVarsToStack()
{
	msg_db_f("MapRemainingTempVarsToStack", 3);
	for (int i=temp_var.num-1;i>=0;i--){
		SerialCommandParam stackvar;
		add_stack_var(temp_var[i].type, temp_var[i].first, temp_var[i].last, stackvar);
		for (int j=0;j<cmd.num;j++){
			for (int k=0; k<SERIAL_COMMAND_NUM_PARAMS; k++)
				try_map_param_to_stack(cmd[j].p[k], i, stackvar);
		}
		remove_temp_var(i);
	}
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

	if (script->syntax->FlagShow)
		cmd_list_out();
}

void SerializerARM::ConvertGlobalLookups()
{
	for (int i=0; i<cmd.num; i++)
		for (int k=0; k<3; k++)
			if (cmd[i].p[k].kind == KindGlobalLookup){

			}
}


void SerializerARM::DoMapping()
{
	FindReferencedTempVars();

	// --- remove unnecessary temp vars

	TryMapTempVarsRegisters();

	MapRemainingTempVarsToStack();

	//ResolveDerefTempAndLocal();

	CorrectUnallowedParamCombisGlobal();

	if (script->syntax->FlagShow){
		msg_write("post global:");
		cmd_list_out();
	}

	CorrectUnallowedParamCombis();

	if (script->syntax->FlagShow){
		msg_write("post local:");
		cmd_list_out();
	}

	for (int i=0; i<cmd.num; i++)
		CorrectUnallowedParamCombis2(cmd[i]);

	if (script->syntax->FlagShow)
		cmd_list_out();
}


Asm::InstructionParam Serializer::get_param(int inst, SerialCommandParam &p)
{
	if (p.kind < 0){
		return Asm::param_none;
	}else if (p.kind == KindMarker){
		return Asm::param_label(p.p, 4);
	}else if (p.kind == KindRegister){
		if (p.shift > 0)
			script->DoErrorInternal("get_param: reg + shift");
		return Asm::param_reg(p.p);
		//param_size = p.type->size;
	}else if (p.kind == KindDerefRegister){
		if (p.shift != 0)
			return Asm::param_deref_reg_shift(p.p, p.shift, p.type->size);
		else
			return Asm::param_deref_reg(p.p, p.type->size);
	}else if (p.kind == KindVarGlobal){
		int size = p.type->size;
		if ((size != 1) && (size != 2) && (size != 4) && (size != 8))
			script->DoErrorInternal("get_param: evil global of type " + p.type->name);
		return Asm::param_deref_imm(p.p + p.shift, size);
	}else if (p.kind == KindVarLocal){
		if (config.instruction_set == Asm::INSTRUCTION_SET_ARM){
			return Asm::param_deref_reg_shift(Asm::REG_R13, p.p + p.shift, p.type->size);
		}else{
			return Asm::param_deref_reg_shift(Asm::REG_EBP, p.p + p.shift, p.type->size);
		}
		//if ((param_size != 1) && (param_size != 2) && (param_size != 4) && (param_size != 8))
		//	param_size = -1; // lea doesn't need size...
			//s->DoErrorInternal("get_param: evil local of type " + p.type->name);
	}else if (p.kind == KindRefToConst){
		bool imm_allowed = Asm::GetInstructionAllowConst(inst);
		if ((imm_allowed) && (p.type->is_pointer)){
			return Asm::param_imm(*(int*)(p.p + p.shift), 4);
		}else if ((p.type->size <= 4) && (imm_allowed)){
			return Asm::param_imm(*(int*)(p.p + p.shift), p.type->size);
		}else{
			return Asm::param_deref_imm(p.p + p.shift, p.type->size);
		}
	}else if (p.kind == KindConstant){
		if (p.shift > 0)
			script->DoErrorInternal("get_param: const + shift");
		return Asm::param_imm(p.p, p.type->size);
	}else
		script->DoErrorInternal("get_param: unexpected param..." + Kind2Str(p.kind));
	return Asm::param_none;
}


void Serializer::assemble_cmd(SerialCommand &c)
{
	// translate parameters
	Asm::InstructionParam p1 = get_param(c.inst, c.p[0]);
	Asm::InstructionParam p2 = get_param(c.inst, c.p[1]);

	// assemble instruction
	//list->current_line = c.
	list->add2(c.inst, p1, p2);
}

void Serializer::assemble_cmd_arm(SerialCommand &c)
{
	// translate parameters
	Asm::InstructionParam p1 = get_param(c.inst, c.p[0]);
	Asm::InstructionParam p2 = get_param(c.inst, c.p[1]);
	Asm::InstructionParam p3 = get_param(c.inst, c.p[2]);

	// assemble instruction
	//list->current_line = c.
	list->add_arm(c.cond, c.inst, p1, p2, p3);
}

void AddAsmBlock(Asm::InstructionWithParamsList *list, Script *s)
{
	msg_db_f("AddAsmBlock", 4);
	//msg_write(".------------------------------- asm");
	SyntaxTree *ps = s->syntax;
	if (ps->AsmBlocks.num == 0)
		s->DoError("asm block mismatch");
	ps->AsmMetaInfo->line_offset = ps->AsmBlocks[0].line;
	list->AppendFromSource(ps->AsmBlocks[0].block);
	ps->AsmBlocks.erase(0);
}

void SerializerX86::CorrectUnallowedParamCombis2(SerialCommand &c)
{
	// push 8 bit -> push 32 bit
	if (c.inst == Asm::inst_push)
		if (c.p[0].kind == KindRegister)
			c.p[0].p = reg_resize(c.p[0].p, config.pointer_size);
}

void SerializerAMD64::CorrectUnallowedParamCombis2(SerialCommand &c)
{
	// push 8 bit -> push 32 bit
	if (c.inst == Asm::inst_push)
		if (c.p[0].kind == KindRegister)
			c.p[0].p = reg_resize(c.p[0].p, config.pointer_size);


	// FIXME
	// evil hack to allow inconsistent param types (in address shifts)
	if (config.instruction_set == Asm::INSTRUCTION_SET_AMD64){
		if ((c.inst == Asm::inst_add) || (c.inst == Asm::inst_mov)){
			if ((c.p[0].kind == KindRegister) && (c.p[1].kind == KindRefToConst)){
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
				}else*/ if (c.p[1].kind == KindRegister){
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
				if ((c.p[0].kind == KindRegister) && ((c.p[1].kind == KindRegister) || (c.p[1].kind == KindDerefRegister))){
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

void SerializerARM::CorrectUnallowedParamCombis2(SerialCommand &c)
{
	if (c.inst == Asm::inst_mov){
		if (c.p[0].kind == KindVarLocal){
			if (c.p[0].type->size == 1)
				c.inst = Asm::inst_strb;
			else
				c.inst = Asm::inst_str;
			SerialCommandParam p = c.p[0];
			c.p[0] = c.p[1];
			c.p[1] = p;
		}else if (c.p[1].kind == KindVarLocal){
			if (c.p[1].type->size == 1)
				c.inst = Asm::inst_ldrb;
			else
				c.inst = Asm::inst_ldr;
		}
	}
}

void SerializerARM::CorrectReturn()
{
	for (int i=0;i<cmd.num;i++)
		if (cmd[i].inst == Asm::inst_ret){
			remove_cmd(i);
			if (stack_max_size > 0){
				add_cmd(Asm::inst_add, param_reg(TypePointer, Asm::REG_R13), param_reg(TypePointer, Asm::REG_R13), param_const(TypeInt, stack_max_size));
				move_last_cmd(i ++);
			}
			add_cmd(Asm::inst_ldmia, param_reg(TypePointer, Asm::REG_R13), param_const(TypeInt, 0x8ff0));
			move_last_cmd(i);
		}
}

void Serializer::Assemble()
{
	msg_db_f("Serializer.Assemble", 2);

	// intro + allocate stack memory
	if (config.instruction_set != Asm::INSTRUCTION_SET_ARM)
		stack_max_size += max_push_size;
	stack_max_size = mem_align(stack_max_size, config.stack_frame_align);

	if (config.instruction_set == Asm::INSTRUCTION_SET_ARM){
		foreachi(GlobalRef &g, global_refs, i){
			g.label = add_marker();
			list->add2(Asm::inst_dd, Asm::param_imm((long)g.p, 4));
		}
	}

	list->add_label("_kaba_func_" + i2s(cur_func_index));

	if (!syntax_tree->FlagNoFunctionFrame){
		if (config.instruction_set == Asm::INSTRUCTION_SET_ARM){
			add_cmd(Asm::inst_stmdb, param_reg(TypePointer, Asm::REG_R13), param_const(TypeInt, 0x4ff0));
			move_last_cmd(0);
			if (stack_max_size > 0){
				add_cmd(Asm::inst_sub, param_reg(TypePointer, Asm::REG_R13), param_reg(TypePointer, Asm::REG_R13), param_const(TypeInt, stack_max_size));
				move_last_cmd(1);
			}
		}else{
			list->add_func_intro(stack_max_size);
		}
	}
	CorrectReturn();

	for (int i=0;i<cmd.num;i++){

		if (cmd[i].inst == inst_marker){
			list->add_label(list->label[cmd[i].p[0].p].name);
		}else if (cmd[i].inst == inst_asm){
			AddAsmBlock(list, script);
		}else{

			if (config.instruction_set == Asm::INSTRUCTION_SET_ARM)
				assemble_cmd_arm(cmd[i]);
			else
				assemble_cmd(cmd[i]);
		}
	}
}

void Serializer::DoError(const string &msg)
{
	script->DoErrorInternal(msg);
}

void Serializer::DoErrorLink(const string &msg)
{
	script->DoErrorLink(msg);
}

Serializer::Serializer(Script *s, Asm::InstructionWithParamsList *_list)
{
	script = s;
	syntax_tree = s->syntax;
	list = _list;
	max_push_size = 0;
}

Serializer::~Serializer()
{
}

Serializer *CreateSerializer(Script *s, Asm::InstructionWithParamsList *list)
{
	if (config.instruction_set == Asm::INSTRUCTION_SET_AMD64)
		return new SerializerAMD64(s, list);
	if (config.instruction_set == Asm::INSTRUCTION_SET_X86)
		return new SerializerX86(s, list);
	if (config.instruction_set == Asm::INSTRUCTION_SET_ARM)
		return new SerializerARM(s, list);
	return NULL;
}

void Script::AssembleFunction(int index, Function *f, Asm::InstructionWithParamsList *list)
{
	msg_db_f("Compile Function", 2);

	if (syntax->FlagShow)
		msg_write("serializing " + f->name + " -------------------");

	cur_func = f;
	Serializer *d = CreateSerializer(this, list);

	try{
		d->cur_func_index = index;
		d->SerializeFunction(f);
		d->DoMapping();
		d->Assemble();
	}catch(Exception &e){
		throw e;
	}catch(Asm::Exception &e){
		throw Exception(e, this);
	}
	functions_to_link.append(d->list->wanted_label);
	AlignOpcode();
	delete(d);
}

void Script::CompileFunctions(char *oc, int &ocs)
{
	Asm::InstructionWithParamsList *list = new Asm::InstructionWithParamsList(0);

	// create assembler
	func.resize(syntax->Functions.num);
	foreachi(Function *f, syntax->Functions, i){
		if (f->is_extern){
			func[i] = (t_func*)GetExternalLink(f->name);
			if (!func[i])
				DoErrorLink("external function " + f->name + " not linkable");
		}else{
			AssembleFunction(i, f, list);
		}
	}


	list->show();

	// assemble into opcode
	try{
		list->Optimize(oc, ocs);
		list->Compile(oc, ocs);
	}catch(Asm::Exception &e){
		throw Exception(e, this);
	}


	// get function addresses
	foreachi(Function *f, syntax->Functions, i){
		if (!f->is_extern){
			func[i] = (t_func*)list->get_label_value("_kaba_func_" + i2s(i));
		}
	}

	delete(list);
}

};
