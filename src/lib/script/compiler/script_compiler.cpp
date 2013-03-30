#include "../script.h"
//#include "dasm.h"
#include "../../file/file.h"

#ifdef OS_LINUX
	#include <sys/mman.h>
#endif
#ifdef OS_WINDOWS
	#include "windows.h"
#endif

namespace Script{


#define so		script_db_out
#define right	script_db_right
#define left	script_db_left


void script_db_out(const string &str); // -> script.cpp
void script_db_out(int i);
void script_db_right();
void script_db_left();


int LocalOffset,LocalOffsetMax;

/*int get_func_temp_size(Function *f)
{
}*/

inline int add_temp_var(int size)
{
	LocalOffset += size;
	if (LocalOffset > LocalOffsetMax)
		LocalOffsetMax = LocalOffset;
	return LocalOffset;
}


int TaskReturnOffset;

enum{
	inNop,
	inPushEbp,
	inMovEbpEsp,
	inMovEspM,
	inMovEdxpi8Eax,
	inLeave,
	inRet,
	inRet4,
	inMovEaxM,
	inMovMEax,
	inMovEdxM,
	inMovMEdx,
	inMovAlM8,
	inMovAhM8,
	inMovBlM8,
	inMovBhM8,
	inMovClM8,
	inMovM8Al,
	inMovMEbp,
	inMovMEsp,
	inLeaEaxM,
	inLeaEdxM,
	inPushM,
	inPushEax,
	inPushEdx,
	inPopEax,
	inPopEsp,
	inAndEaxM,
	inOrEaxM,
	inXorEaxM,
	inAddEaxM,
	inAddEdxM,
	inAddMEax,
	inSubEaxM,
	inSubMEax,
	inMulEaxM,
	inDivEaxM,
	inDivEaxEbx,
	inCmpEaxM,
	inCmpAlM8,
	inCmpM80,
	inSetzAl,
	inSetnzAl,
	inSetnleAl,
	inSetnlAl,
	inSetlAl,
	inSetleAl,
	inAndAlM8,
	inOrAlM8,
	inXorAlM8,
	inAddAlM8,
	inAddM8Al,
	inSubAlM8,
	inSubM8Al,
	inCallRel32,
	inJmpEax,
	inJmpC32,
	inJzC8,
	inJzC32,
	inLoadfM,
	inSavefM,
	inLoadfiM,
	inAddfM,
	inSubfM,
	inMulfM,
	inDivfM,
	inShrEaxCl,
	inShlEaxCl,
	NumAsmInstructions
};


#define CallRel32OCSize			5
#define AfterWaitOCSize			10



inline void OCAddChar(char *oc,int &ocs,int c)
{	oc[ocs]=(char)c;	ocs++;	}

inline void OCAddWord(char *oc,int &ocs,int i)
{	*(short*)&oc[ocs]=i;	ocs+=2;	}

inline void OCAddInt(char *oc,int &ocs,int i)
{	*(int*)&oc[ocs]=i;	ocs+=4;	}

int OCOParam;

// offset: used to shift addresses   (i.e. mov iteratively to a big local variable)
void OCAddInstruction(char *oc, int &ocs, int inst, int kind, void *param = NULL, int offset = 0)
{
	int code = 0;
	int pk[2] = {Asm::PKNone, Asm::PKNone};
	void *p[2] = {NULL, NULL};
	int m = -1;
	int size = 32;
	//msg_write(offset);

// corrections
	// lea
	if ((inst == inLeaEaxM) && (kind == KindRefToConst)){
		OCAddInstruction(oc, ocs, inst, KindVarGlobal, param, offset);
		return;
	}


	switch(inst){
		case inNop:			code = Asm::inst_nop;	break;
		case inPushEbp:		code = Asm::inst_push;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEbp;	break;
		case inMovEbpEsp:	code = Asm::inst_mov;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEbp;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegEsp;	break;
		case inMovEspM:		code = Asm::inst_mov;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEsp;	m = 1;	break;
		case inMovEdxpi8Eax:code = Asm::inst_mov;	pk[0] = Asm::PKEdxRel;	p[0] = param;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegEax;	break;
		case inLeave:		code = Asm::inst_leave;	break;
		case inRet:			code = Asm::inst_ret;	break;
		case inRet4:		code = Asm::inst_ret;	pk[0] = Asm::PKConstant16;	p[0] = (void*)4;	break;
		case inMovEaxM:		code = Asm::inst_mov;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inMovMEax:		code = Asm::inst_mov;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegEax;	m = 0;	break;
		case inMovEdxM:		code = Asm::inst_mov;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEdx;	m = 1;	break;
		case inMovMEdx:		code = Asm::inst_mov;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegEdx;	m = 0;	break;
		case inMovAlM8:		code = Asm::inst_mov_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	m = 1;	size = 8;	break;
		case inMovAhM8:		code = Asm::inst_mov_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAh;	m = 1;	size = 8;	break;
		case inMovBlM8:		code = Asm::inst_mov_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegBl;	m = 1;	size = 8;	break;
		case inMovBhM8:		code = Asm::inst_mov_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegBh;	m = 1;	size = 8;	break;
		case inMovClM8:		code = Asm::inst_mov_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegCl;	m = 1;	size = 8;	break;
		case inMovM8Al:		code = Asm::inst_mov_b;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegAl;	m = 0;	size = 8;	break;
		case inMovMEbp:		code = Asm::inst_mov;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegEbp;	m = 0;	break;
		case inMovMEsp:		code = Asm::inst_mov;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegEsp;	m = 0;	break;
		case inLeaEaxM:		code = Asm::inst_lea;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inLeaEdxM:		code = Asm::inst_lea;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEdx;	m = 1;	break;
		case inPushM:		code = Asm::inst_push;	m = 0;	break;
		case inPushEax:		code = Asm::inst_push;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	break;
		case inPushEdx:		code = Asm::inst_push;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEdx;	break;
		case inPopEax:		code = Asm::inst_pop;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	break;
		case inPopEsp:		code = Asm::inst_pop;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEsp;	break;
		case inAndEaxM:		code = Asm::inst_and;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inOrEaxM:		code = Asm::inst_or;		pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inXorEaxM:		code = Asm::inst_xor;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inAddEaxM:		code = Asm::inst_add;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inAddEdxM:		code = Asm::inst_add;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEdx;	m = 1;	break;
		case inAddMEax:		code = Asm::inst_add;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegEax;	m = 0;	break;
		case inSubEaxM:		code = Asm::inst_sub;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inSubMEax:		code = Asm::inst_sub;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegEax;	m = 0;	break;
		case inMulEaxM:		code = Asm::inst_imul;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inDivEaxM:		code = Asm::inst_div;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inDivEaxEbx:	code = Asm::inst_div;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegEbx;	break;
		case inCmpEaxM:		code = Asm::inst_cmp;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	m = 1;	break;
		case inCmpAlM8:		code = Asm::inst_cmp_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	m = 1;	size = 8;	break;
		case inCmpM80:		code = Asm::inst_cmp_b;	pk[1] = Asm::PKConstant8;	p[1] = NULL;	m = 0;	size = 8;	break;
		case inSetzAl:		code = Asm::inst_setz_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	break;
		case inSetnzAl:		code = Asm::inst_setnz_b;pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	break;
		case inSetnleAl:	code = Asm::inst_setnle_b;pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	break;
		case inSetnlAl:		code = Asm::inst_setnl_b;pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	break;
		case inSetlAl:		code = Asm::inst_setl_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	break;
		case inSetleAl:		code = Asm::inst_setle_b;pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	break;
		case inAndAlM8:		code = Asm::inst_and_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	m = 1;	size = 8;	break;
		case inOrAlM8:		code = Asm::inst_or_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	m = 1;	size = 8;	break;
		case inXorAlM8:		code = Asm::inst_xor_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	m = 1;	size = 8;	break;
		case inAddAlM8:		code = Asm::inst_add_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	m = 1;	size = 8;	break;
		case inAddM8Al:		code = Asm::inst_add_b;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegAl;	m = 0;	size = 8;	break;
		case inSubAlM8:		code = Asm::inst_sub_b;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegAl;	m = 1;	size = 8;	break;
		case inSubM8Al:		code = Asm::inst_sub_b;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegAl;	m = 0;	size = 8;	break;
		case inCallRel32:	code = Asm::inst_call;	m = 0;	break;
		case inJmpEax:		code = Asm::inst_jmp;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	break;
		case inJmpC32:		code = Asm::inst_jmp;	m = 0;	break;
		case inJzC8:		code = Asm::inst_jz_b;	m = 0;	size = 8;	break;
		case inJzC32:		code = Asm::inst_jz;		m = 0;	size = 8;	break;
		case inLoadfM:		code = Asm::inst_fld;	m = 0;	break;
		case inSavefM:		code = Asm::inst_fstp;	m = 0;	break;
		case inLoadfiM:		code = Asm::inst_fild;	m = 0;	break;
		case inAddfM:		code = Asm::inst_fadd;	m = 0;	break;
		case inSubfM:		code = Asm::inst_fsub;	m = 0;	break;
		case inMulfM:		code = Asm::inst_fmul;	m = 0;	break;
		case inDivfM:		code = Asm::inst_fdiv;	m = 0;	break;
		case inShrEaxCl:	code = Asm::inst_shr;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegCl;	break;
		case inShlEaxCl:	code = Asm::inst_shl;	pk[0] = Asm::PKRegister;	p[0] = (void*)Asm::RegEax;	pk[1] = Asm::PKRegister;	p[1] = (void*)Asm::RegCl;	break;
		default:
			msg_todo(format("unhandled instruction: %d", inst));
			cur_script->DoErrorInternal("asm error");
			return;
	}


	// const as global var?
	if (kind == KindRefToConst){
		if (!Asm::ImmediateAllowed(code)){
			//msg_write("evil");
			kind = KindVarGlobal;
		}

		if (inst == inCmpM80){
			kind = KindVarGlobal;
		}
	}


	// parameters
	if ((m >= 0) && (kind >= 0)){

		if (kind == KindVarLocal){
			pk[m] = Asm::PKLocal;
			p[m] = (void*)((long)param + offset);
		}else if (kind == KindVarGlobal){
			pk[m] = Asm::PKDerefConstant;
			p[m] = (void*)((long)param + offset);
		}else if (kind == KindConstant){
			pk[m] = (size == 8) ? Asm::PKConstant8 : Asm::PKConstant32;
			p[m] = param;
		}else if (kind == KindRefToConst){
			kind = KindConstant;
			pk[m] = (size == 8) ? Asm::PKConstant8 : Asm::PKConstant32;
			p[m] = *(void**)((long)param + offset);
		}else if ((kind == KindRefToLocal) || (kind == KindRefToGlobal)){
			if (kind == KindRefToLocal)
				OCAddInstruction(oc, ocs, inMovEdxM, KindVarLocal, param);
			else if (kind == KindRefToGlobal)
				OCAddInstruction(oc, ocs, inMovEdxM, KindVarGlobal, param);
			if (offset != 0)
				OCAddInstruction(oc, ocs, inAddEdxM, KindConstant, (char*)(long)offset);
			pk[m] = Asm::PKDerefRegister;
			p[m] = (void*)Asm::RegEdx;
		}else{
			msg_error("kind unhandled");
			msg_write(kind);
		}
	}

	// compile
	if (!Asm::AddInstruction(oc, ocs, code, pk[0], p[0], pk[1], p[1]))
		cur_script->DoErrorInternal("asm error");

	OCOParam = Asm::OCParam;
}

/*enum{
	PKInvalid,
	PKNone,
	PKRegister,			// eAX
	PKDerefRegister,	// [eAX]
	PKLocal,			// [ebp + 0x0000]
	PKStackRel,			// [esp + 0x0000]
	PKConstant32,		// 0x00000000
	PKConstant16,		// 0x0000
	PKConstant8,		// 0x00
	PKConstantDouble,   // 0x00:0x0000   ...
	PKDerefConstant		// [0x0000]
};*/
//bool AsmAddInstruction(char *oc, int &ocs, int inst, int param1_type, void *param1, int param2_type, void *param2, int offset = 0, int insert_at = -1);

void OCAddEspAdd(char *oc,int &ocs,int d)
{
	if (d>0){
		if (d>120)
			Asm::AddInstruction(oc, ocs, Asm::inst_add, Asm::PKRegister, (void*)Asm::RegEsp, Asm::PKConstant32, (void*)(long)d);
		else
			Asm::AddInstruction(oc, ocs, Asm::inst_add_b, Asm::PKRegister, (void*)Asm::RegEsp, Asm::PKConstant8, (void*)(long)d);
	}else if (d<0){
		if (d<-120)
			Asm::AddInstruction(oc, ocs, Asm::inst_sub, Asm::PKRegister, (void*)Asm::RegEsp, Asm::PKConstant32, (void*)(long)(-d));
		else
			Asm::AddInstruction(oc, ocs, Asm::inst_sub_b, Asm::PKRegister, (void*)Asm::RegEsp, Asm::PKConstant8, (void*)(long)(-d));
	}
}

void init_all_global_objects(PreScript *ps, Function *f, Array<char*> &g_var)
{
	foreachi(LocalVariable &v, f->var, i){
		foreach(ClassFunction &f, v.type->function){
			typedef void init_func(void *);
			if (f.name == "__init__"){ // TODO test signature "void __init__()"
				//msg_write("global init: " + v.type->name);
				init_func *ff = NULL;
				if (f.kind == KindCompilerFunction)
					ff = (init_func*)PreCommands[f.nr].func;
				else if (f.kind == KindFunction)
					ff = (init_func*)ps->script->func[f.nr];
				if (ff)
					ff(g_var[i]);
			}
		}
	}
}

void Script::AllocateMemory()
{
	// get memory size needed
	MemorySize = 0;
	for (int i=0;i<pre_script->RootOfAllEvil.var.num;i++)
		MemorySize += mem_align(pre_script->RootOfAllEvil.var[i].type->size);
	foreachi(Constant &c, pre_script->Constants, i){
		int s = c.type->size;
		if (c.type == TypeString){
			// const string -> variable length   (+ super array frame)
			s = strlen(c.data) + 1 + 16;
		}
		MemorySize += mem_align(s);
	}
	if (MemorySize > 0)
		Memory = new char[MemorySize];
}

void Script::AllocateStack()
{
	// use your own stack if needed
	//   wait() used -> needs to switch stacks ("tasks")
	Stack = NULL;
	foreach(Command *cmd, pre_script->Commands){
		if (cmd->kind == KindCompilerFunction)
			if ((cmd->link_nr == CommandWait) || (cmd->link_nr == CommandWaitRT) || (cmd->link_nr == CommandWaitOneFrame)){
				Stack = new char[StackSize];
				break;
			}
	}
}

void Script::AllocateOpcode()
{
	// allocate some memory for the opcode......    has to be executable!!!   (important on amd64)
#ifdef OS_WINDOWS
	Opcode=(char*)VirtualAlloc(NULL,SCRIPT_MAX_OPCODE,MEM_COMMIT | MEM_RESERVE,PAGE_EXECUTE_READWRITE);
	ThreadOpcode=(char*)VirtualAlloc(NULL,SCRIPT_MAX_THREAD_OPCODE,MEM_COMMIT | MEM_RESERVE,PAGE_EXECUTE_READWRITE);
#else
	Opcode = (char*)mmap(0, SCRIPT_MAX_OPCODE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS | MAP_EXECUTABLE, 0, 0);
	ThreadOpcode = (char*)mmap(0, SCRIPT_MAX_THREAD_OPCODE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS | MAP_EXECUTABLE, 0, 0);
#endif
	if (((long)Opcode==-1)||((long)ThreadOpcode==-1))
		_do_error_int_("CScript:  could not allocate executable memory", 2,);
	OpcodeSize=0;
	ThreadOpcodeSize=0;
}

void Script::MapConstantsToMemory()
{
	// constants -> Memory
	so("Konstanten");
	cnst.resize(pre_script->Constants.num);
	foreachi(Constant &c, pre_script->Constants, i){
		cnst[i] = &Memory[MemorySize];
		int s = c.type->size;
		if (c.type == TypeString){
			// const string -> variable length
			s = strlen(pre_script->Constants[i].data) + 1;

			*(void**)&Memory[MemorySize] = &Memory[MemorySize + 16]; // .data
			*(int*)&Memory[MemorySize + 4] = s - 1; // .num
			*(int*)&Memory[MemorySize + 8] = 0; // .reserved
			*(int*)&Memory[MemorySize + 12] = 1; // .item_size
			MemorySize += 16;
		}
		memcpy(&Memory[MemorySize], (void*)c.data, s);
		MemorySize += mem_align(s);
	}
}

void Script::MapGlobalVariablesToMemory()
{
	// global variables -> into Memory
	so("glob.Var.");
	g_var.resize(pre_script->RootOfAllEvil.var.num);
	for (int i=0;i<pre_script->RootOfAllEvil.var.num;i++){
		if (pre_script->FlagOverwriteVariablesOffset)
			g_var[i] = (char*)(long)(MemorySize + pre_script->VariablesOffset);
		else
			g_var[i] = &Memory[MemorySize];
		so(format("%d: %s", MemorySize, pre_script->RootOfAllEvil.var[i].name.c_str()));
		MemorySize += mem_align(pre_script->RootOfAllEvil.var[i].type->size);
	}
	memset(Memory, 0, MemorySize); // reset all global variables to 0
}

static int OCORA;
void Script::CompileOsEntryPoint()
{
	int nf=-1;
	foreachi(Function *ff, pre_script->Functions, index)
		if (ff->name == "main")
			nf = index;
	// call
	if (nf>=0)
		OCAddInstruction(Opcode,OpcodeSize,inCallRel32,KindConstant,NULL);
	TaskReturnOffset=OpcodeSize;
	OCORA=OCOParam;

	// put strings into Opcode!
	foreachi(Constant &c, pre_script->Constants, i){
		if ((pre_script->FlagCompileOS) || (c.type == TypeString)){
			int offset = 0;
			if (pre_script->AsmMetaInfo)
				offset = pre_script->AsmMetaInfo->CodeOrigin;
			cnst[i] = (char*)(long)(OpcodeSize + offset);
			int s = c.type->size;
			if (c.type == TypeString)
				s = strlen(c.data) + 1;
			memcpy(&Opcode[OpcodeSize], (void*)c.data, s);
			OpcodeSize += s;
		}
	}
}

void Script::LinkOsEntryPoint()
{
	int nf=-1;
	foreachi(Function *ff, pre_script->Functions, index)
		if (ff->name == "main")
			nf = index;
	if (nf>=0){
		int lll=((long)func[nf]-(long)&Opcode[TaskReturnOffset]);
		if (pre_script->FlagCompileInitialRealMode)
			lll+=5;
		else
			lll+=3;
		//printf("insert   %d  an %d\n", lll, OCORA);
		//msg_write(lll);
		//msg_write(d2h(&lll,4,false));
		*(int*)&Opcode[OCORA]=lll;
	}
}

void Script::CompileTaskEntryPoint()
{
	// "stack" usage for waiting:
	//  -4 - ebp (before execution)
	//  -8 - ebp (script continue)
	// -12 - esp (script continue)
	// -16 - eip (script continue)
	// -20 - script stack...

	first_execution=(t_func*)&ThreadOpcode[ThreadOpcodeSize];
	// intro
	OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPushEbp,-1); // within the actual program
	OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEbpEsp,-1);
	if (Stack){
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEspM,KindConstant,(char*)&Stack[StackSize]); // zum Anfang des Script-Stacks
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPushEbp,-1); // adress of the old stack
		OCAddEspAdd(ThreadOpcode,ThreadOpcodeSize,-12); // space for wait() task data
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEbpEsp,-1);
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEaxM,KindConstant,(char*)WaitingModeNone); // "reset"
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovMEax,KindVarGlobal,(char*)&WaitingMode);
	}
	// call
	int nf = -1;
	foreachi(Function *ff, pre_script->Functions, index){
		if (ff->name == "main")
			if (ff->num_params == 0)
				nf = index;
	}
	if (nf >= 0){
		// call main() ...correct adress will be put here later!
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inCallRel32,KindConstant,NULL);
		*(int*)&ThreadOpcode[OCOParam]=((long)func[nf]-(long)&ThreadOpcode[ThreadOpcodeSize]);
	}
	// outro
	if (Stack){
		OCAddEspAdd(ThreadOpcode,ThreadOpcodeSize,12); // make space for wait() task data
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPopEsp,-1);
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEbpEsp,-1);
	}
	OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inLeave,-1);
	OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inRet,-1);

// "task" for execution after some wait()
	continue_execution=(t_func*)&ThreadOpcode[ThreadOpcodeSize];
	// Intro
	if (Stack){
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPushEbp,-1); // within the external program
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEbpEsp,-1);
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovMEbp,KindVarGlobal,&Stack[StackSize-4]); // save the external ebp
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inMovEspM,KindConstant,&Stack[StackSize-16]); // to the eIP of the script
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inPopEax,-1);
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inAddEaxM,KindConstant,(char*)AfterWaitOCSize);
		OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inJmpEax,-1);
		//OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inLeave,-1);
		//OCAddInstruction(ThreadOpcode,ThreadOpcodeSize,inRet,-1);
		/*OCAddChar(0x90);
		OCAddChar(0x90);
		OCAddChar(0x90);*/
	}
}

// Opcode generieren
void Script::Compiler()
{
	if (Error)	return;
	msg_db_r("Compiler",2);

	pre_script->MapLocalVariablesToStack();

	if (!Error)
		pre_script->BreakDownComplicatedCommands();
#ifdef ScriptDebug
	pre_script->Show();
#endif

	if (!Error)
		pre_script->Simplify();
	if (!Error)
		pre_script->PreProcessor(this);


	AllocateMemory();
	AllocateStack();

	MemorySize = 0;
	MapGlobalVariablesToMemory();
	MapConstantsToMemory();

	AllocateOpcode();


	if (!Error)
		pre_script->PreProcessorAddresses(this);



// compiling an operating system?
//   -> create an entry point for execution... so we can just call Opcode like a function
	if ((pre_script->FlagCompileOS)||(pre_script->FlagCompileInitialRealMode))
		CompileOsEntryPoint();


// compile functions into Opcode
	so("Funktionen");
	func.resize(pre_script->Functions.num);
	foreachi(Function *f, pre_script->Functions, i){
		right();
		func[i] = (t_func*)&Opcode[OpcodeSize];
		CompileFunction(f, Opcode, OpcodeSize);
		left();

		if (!Error)
			if (pre_script->AsmMetaInfo)
				if (pre_script->AsmMetaInfo->wanted_label.num > 0)
					_do_error_(format("unknown name in assembler code:  \"%s\"", pre_script->AsmMetaInfo->wanted_label[0].Name.c_str()), 2,);
	}


// "task" for the first execution of main() -> ThreadOpcode
	if (!pre_script->FlagCompileOS)
		CompileTaskEntryPoint();




	if (pre_script->FlagCompileOS)
		LinkOsEntryPoint();


	// initialize global super arrays and objects
	init_all_global_objects(pre_script, &pre_script->RootOfAllEvil, g_var);

	//msg_db_out(1,GetAsm(Opcode,OpcodeSize));

	//_expand(Opcode,OpcodeSize);

	WaitingMode = WaitingModeFirst;

	if (ShowCompilerStats){
		msg_write("--------------------------------");
		msg_write(format("Opcode: %db, Memory: %db",OpcodeSize,MemorySize));
	}
	msg_db_l(2);
}

};



