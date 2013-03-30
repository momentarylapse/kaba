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


#define CallRel32OCSize			5
#define AfterWaitOCSize			10


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
		Asm::AddInstruction(Opcode, OpcodeSize, Asm::inst_call, Asm::PKConstant32, NULL);
	TaskReturnOffset=OpcodeSize;
	OCORA = Asm::OCParam;

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

	first_execution = (t_func*)&ThreadOpcode[ThreadOpcodeSize];
	// intro
	Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_push, Asm::PKRegister, (void*)Asm::RegEbp); // within the actual program
	Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_mov, Asm::PKRegister, (void*)Asm::RegEbp, Asm::PKRegister, (void*)Asm::RegEsp);
	if (Stack){
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_mov, Asm::PKRegister, (void*)Asm::RegEsp, Asm::PKDerefConstant, (void*)&Stack[StackSize]); // start of the script stack
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_push, Asm::PKRegister, (void*)Asm::RegEbp); // address of the old stack
		OCAddEspAdd(ThreadOpcode, ThreadOpcodeSize, -12); // space for wait() task data
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_mov, Asm::PKRegister, (void*)Asm::RegEbp, Asm::PKRegister, (void*)Asm::RegEsp);
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_mov, Asm::PKRegister, (void*)Asm::RegEax, Asm::PKConstant32, (void*)WaitingModeNone); // "reset"
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_mov, Asm::PKDerefConstant, (void*)&WaitingMode, Asm::PKRegister, (void*)Asm::RegEax);
	}
	// call
	int nf = -1;
	foreachi(Function *ff, pre_script->Functions, index){
		if (ff->name == "main")
			if (ff->num_params == 0)
				nf = index;
	}
	if (nf >= 0){
		// call main() ...correct address will be put here later!
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_call, Asm::PKConstant32, NULL);
		*(int*)&ThreadOpcode[Asm::OCParam] = ((long)func[nf] - (long)&ThreadOpcode[ThreadOpcodeSize]);
	}
	// outro
	if (Stack){
		OCAddEspAdd(ThreadOpcode, ThreadOpcodeSize, 12); // make space for wait() task data
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_pop, Asm::PKRegister, (void*)Asm::RegEsp);
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_mov, Asm::PKRegister, (void*)Asm::RegEbp, Asm::PKRegister, (void*)Asm::RegEsp);
	}
	Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_leave);
	Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_ret);

// "task" for execution after some wait()
	continue_execution = (t_func*)&ThreadOpcode[ThreadOpcodeSize];
	// Intro
	if (Stack){
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_push, Asm::PKRegister, (void*)Asm::RegEbp); // within the external program
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_mov, Asm::PKRegister, (void*)Asm::RegEbp, Asm::PKRegister, (void*)Asm::RegEsp);
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_mov, Asm::PKDerefConstant, &Stack[StackSize - 4], Asm::PKRegister, (void*)Asm::RegEbp); // save the external ebp
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_mov, Asm::PKRegister, (void*)Asm::RegEsp, Asm::PKDerefConstant, &Stack[StackSize - 16]); // to the eIP of the script
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_pop, Asm::PKRegister, (void*)Asm::RegEax);
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_add, Asm::PKRegister, (void*)Asm::RegEax, Asm::PKConstant32, (void*)AfterWaitOCSize);
		Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_jmp, Asm::PKRegister, (void*)Asm::RegEax);
		//Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_leave);
		//Asm::AddInstruction(ThreadOpcode, ThreadOpcodeSize, Asm::inst_ret);
		/*OCAddChar(0x90);
		OCAddChar(0x90);
		OCAddChar(0x90);*/
	}
}

// generate opcode
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



