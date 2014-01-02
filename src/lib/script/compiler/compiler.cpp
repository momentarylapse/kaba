#include "../script.h"
#include "../../file/file.h"

#ifdef OS_LINUX
	#include <sys/mman.h>
	#if (!defined(__x86_64__)) && (!defined(__amd64__))
		#define MAP_32BIT		0
	#endif
#endif
#ifdef OS_WINDOWS
	#include "windows.h"
#endif

namespace Script{


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

void AddEspAdd(Asm::InstructionWithParamsList *list,int d)
{
	if (d>0){
		if (d>120)
			list->add_easy(Asm::inst_add, Asm::PKRegister, 4, (void*)Asm::RegEsp, Asm::PKConstant, 4, (void*)(long)d);
		else
			list->add_easy(Asm::inst_add, Asm::PKRegister, 4, (void*)Asm::RegEsp, Asm::PKConstant, 1, (void*)(long)d);
	}else if (d<0){
		if (d<-120)
			list->add_easy(Asm::inst_sub, Asm::PKRegister, 4, (void*)Asm::RegEsp, Asm::PKConstant, 4, (void*)(long)(-d));
		else
			list->add_easy(Asm::inst_sub, Asm::PKRegister, 4, (void*)Asm::RegEsp, Asm::PKConstant, 1, (void*)(long)(-d));
	}
}

void try_init_global_var(Type *type, char* g_var)
{
	if (type->is_array){
		for (int i=0;i<type->array_length;i++)
			try_init_global_var(type->parent, g_var + i * type->parent->size);
		return;
	}
	ClassFunction *cf = type->GetDefaultConstructor();
	if (!cf)
		return;
	typedef void init_func(void *);
	//msg_write("global init: " + v.type->name);
	init_func *ff = (init_func*)cf->script->func[cf->nr];
	if (ff)
		ff(g_var);
}

void init_all_global_objects(SyntaxTree *ps, Array<char*> &g_var)
{
	foreachi(Variable &v, ps->RootOfAllEvil.var, i)
		try_init_global_var(v.type, g_var[i]);
}

void Script::AllocateMemory()
{
	// get memory size needed
	MemorySize = 0;
	for (int i=0;i<syntax->RootOfAllEvil.var.num;i++)
		if (!syntax->RootOfAllEvil.var[i].is_extern)
			MemorySize += mem_align(syntax->RootOfAllEvil.var[i].type->size, 4);
	foreachi(Constant &c, syntax->Constants, i){
		int s = c.type->size;
		if (c.type == TypeString){
			// const string -> variable length   (+ super array frame)
			s = strlen(c.data) + 1 + config.SuperArraySize;
		}
		MemorySize += mem_align(s, 4);
	}
	if (MemorySize > 0){
#ifdef OS_WINDOWS
		Memory = (char*)VirtualAlloc(NULL, MemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
		Memory = (char*)mmap(0, MemorySize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_EXECUTABLE | MAP_32BIT, 0, 0);
#endif
		//Memory = new char[MemorySize];
	}
}

void Script::AllocateStack()
{
	// use your own stack if needed
	//   wait() used -> needs to switch stacks ("tasks")
	Stack = NULL;
	foreach(Command *cmd, syntax->Commands){
		if (cmd->kind == KindCompilerFunction)
			if ((cmd->link_no == CommandWait) || (cmd->link_no == CommandWaitRT) || (cmd->link_no == CommandWaitOneFrame)){
				Stack = new char[config.StackSize];
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
	Opcode = (char*)mmap(0, SCRIPT_MAX_OPCODE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS | MAP_EXECUTABLE | MAP_32BIT, 0, 0);
	ThreadOpcode = (char*)mmap(0, SCRIPT_MAX_THREAD_OPCODE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS | MAP_EXECUTABLE | MAP_32BIT, 0, 0);
#endif
	if (((long)Opcode==-1)||((long)ThreadOpcode==-1))
		DoErrorInternal("CScript:  could not allocate executable memory");
	if (syntax->AsmMetaInfo->CodeOrigin == 0)
		syntax->AsmMetaInfo->CodeOrigin = (long)Opcode;
	OpcodeSize=0;
	ThreadOpcodeSize=0;
}

void Script::MapConstantsToMemory()
{
	// constants -> Memory
	cnst.resize(syntax->Constants.num);
	foreachi(Constant &c, syntax->Constants, i){
		cnst[i] = &Memory[MemorySize];
		int s = c.type->size;
		if (c.type == TypeString){
			// const string -> variable length
			s = strlen(syntax->Constants[i].data) + 1;

			*(void**)&Memory[MemorySize] = &Memory[MemorySize + config.SuperArraySize]; // .data
			*(int*)&Memory[MemorySize + config.PointerSize    ] = s - 1; // .num
			*(int*)&Memory[MemorySize + config.PointerSize + 4] = 0; // .reserved
			*(int*)&Memory[MemorySize + config.PointerSize + 8] = 1; // .item_size
			MemorySize += config.SuperArraySize;
		}
		memcpy(&Memory[MemorySize], (void*)c.data, s);
		MemorySize += mem_align(s, 4);
	}
}

void Script::MapGlobalVariablesToMemory()
{
	// global variables -> into Memory
	g_var.resize(syntax->RootOfAllEvil.var.num);
	foreachi(Variable &v, syntax->RootOfAllEvil.var, i){
		if (v.is_extern){
			g_var[i] = (char*)GetExternalLink(v.name);
			if (!g_var[i])
				DoErrorLink("external variable " + v.name + " was not linked");
		}else{
			if (syntax->FlagOverwriteVariablesOffset)
				g_var[i] = (char*)(long)(MemorySize + syntax->VariablesOffset);
			else
				g_var[i] = &Memory[MemorySize];
			MemorySize += mem_align(v.type->size, 4);
		}
	}
	memset(Memory, 0, MemorySize); // reset all global variables to 0
}

void Script::AlignOpcode()
{
	int ocs_new = mem_align(OpcodeSize, config.FunctionAlign);
	for (int i=OpcodeSize;i<ocs_new;i++)
		Opcode[i] = 0x90;
	OpcodeSize = ocs_new;
}

static int OCORA;
void Script::CompileOsEntryPoint()
{
	int nf=-1;
	foreachi(Function *ff, syntax->Functions, index)
		if (ff->name == "main")
			nf = index;
	// call
	if (nf>=0)
		Asm::AddInstruction(Opcode, OpcodeSize, Asm::inst_call, Asm::PKConstant, 4, NULL);
	TaskReturnOffset=OpcodeSize;
	OCORA = Asm::OCParam;

	// put strings into Opcode!
	foreachi(Constant &c, syntax->Constants, i){
		if (syntax->FlagCompileOS){// && (c.type == TypeCString)){
			cnst[i] = (char*)(OpcodeSize + syntax->AsmMetaInfo->CodeOrigin);
			int s = c.type->size;
			if (c.type == TypeString){
				// const string -> variable length
				s = strlen(syntax->Constants[i].data) + 1;

				*(void**)&Opcode[OpcodeSize] = (char*)(OpcodeSize + syntax->AsmMetaInfo->CodeOrigin + config.SuperArraySize); // .data
				*(int*)&Opcode[OpcodeSize + config.PointerSize    ] = s - 1; // .num
				*(int*)&Opcode[OpcodeSize + config.PointerSize + 4] = 0; // .reserved
				*(int*)&Opcode[OpcodeSize + config.PointerSize + 8] = 1; // .item_size
				OpcodeSize += config.SuperArraySize;
			}else if (c.type == TypeCString){
				s = strlen(c.data) + 1;
			}
			memcpy(&Opcode[OpcodeSize], (void*)c.data, s);
			OpcodeSize += s;
		}
	}

	AlignOpcode();
}

void Script::LinkOsEntryPoint()
{
	int nf = -1;
	foreachi(Function *ff, syntax->Functions, index)
		if (ff->name == "main")
			nf = index;
	if (nf >= 0){
		int lll = (long)func[nf] - syntax->AsmMetaInfo->CodeOrigin - TaskReturnOffset;
		//printf("insert   %d  an %d\n", lll, OCORA);
		//msg_write(lll);
		//msg_write(d2h(&lll,4,false));
		*(int*)&Opcode[OCORA] = lll;
	}
}

void Script::CompileTaskEntryPoint()
{
	// "stack" usage for waiting:
	//  -4| -8 - ebp (before execution)
	//  -8|-16 - ebp (script continue)
	// -12|-24 - esp (script continue)
	// -16|-32 - eip (script continue)
	// -20|-40 - script stack...

	// call
	void *_main_ = MatchFunction("main", "void", 0);

	if ((!Stack) || (!_main_)){
		first_execution = (t_func*)_main_;
		continue_execution = NULL;
		return;
	}

	Asm::InstructionWithParamsList *list = new Asm::InstructionWithParamsList(0);

	int label_first = list->add_label("_first_execution", true);

	first_execution = (t_func*)&ThreadOpcode[ThreadOpcodeSize];
	// intro
	list->add_easy(Asm::inst_push, Asm::PKRegister, 4, (void*)Asm::RegEbp); // within the actual program
	list->add_easy(Asm::inst_mov, Asm::PKRegister, 4, (void*)Asm::RegEbp, Asm::PKRegister, 4, (void*)Asm::RegEsp);
	list->add_easy(Asm::inst_mov, Asm::PKRegister, 4, (void*)Asm::RegEsp, Asm::PKDerefConstant, 4, (void*)&Stack[config.StackSize]); // start of the script stack
	list->add_easy(Asm::inst_push, Asm::PKRegister, 4, (void*)Asm::RegEbp); // address of the old stack
	AddEspAdd(list, -12); // space for wait() task data
	list->add_easy(Asm::inst_mov, Asm::PKRegister, 4, (void*)Asm::RegEbp, Asm::PKRegister, 4, (void*)Asm::RegEsp);
	list->add_easy(Asm::inst_mov, Asm::PKRegister, 4, (void*)Asm::RegEax, Asm::PKConstant, 4, (void*)WaitingModeNone); // "reset"
	list->add_easy(Asm::inst_mov, Asm::PKDerefConstant, 4, (void*)&WaitingMode, Asm::PKRegister, 4, (void*)Asm::RegEax);

	// call main()
	list->add_easy(Asm::inst_call, Asm::PKConstant, 4, _main_);

	// outro
	AddEspAdd(list, 12); // make space for wait() task data
	list->add_easy(Asm::inst_pop, Asm::PKRegister, 4, (void*)Asm::RegEsp);
	list->add_easy(Asm::inst_mov, Asm::PKRegister, 4, (void*)Asm::RegEbp, Asm::PKRegister, 4, (void*)Asm::RegEsp);
	list->add_easy(Asm::inst_leave);
	list->add_easy(Asm::inst_ret);

	// "task" for execution after some wait()
	int label_cont = list->add_label("_continue_execution", true);

	// Intro
	list->add_easy(Asm::inst_push, Asm::PKRegister, 4, (void*)Asm::RegEbp); // within the external program
	list->add_easy(Asm::inst_mov, Asm::PKRegister, 4, (void*)Asm::RegEbp, Asm::PKRegister, 4, (void*)Asm::RegEsp);
	list->add_easy(Asm::inst_mov, Asm::PKDerefConstant, 4, &Stack[config.StackSize - 4], Asm::PKRegister, 4, (void*)Asm::RegEbp); // save the external ebp
	list->add_easy(Asm::inst_mov, Asm::PKRegister, 4, (void*)Asm::RegEsp, Asm::PKDerefConstant, 4, &Stack[config.StackSize - 16]); // to the eIP of the script
	list->add_easy(Asm::inst_pop, Asm::PKRegister, 4, (void*)Asm::RegEax);
	list->add_easy(Asm::inst_add, Asm::PKRegister, 4, (void*)Asm::RegEax, Asm::PKConstant, 4, (void*)AfterWaitOCSize);
	list->add_easy(Asm::inst_jmp, Asm::PKRegister, 4, (void*)Asm::RegEax);
	//list->add_easy(Asm::inst_leave);
	//list->add_easy(Asm::inst_ret);
	/*OCAddChar(0x90);
	OCAddChar(0x90);
	OCAddChar(0x90);*/

	list->Compile(ThreadOpcode, ThreadOpcodeSize);

	first_execution = (t_func*)(long)list->label[label_first].Value;
	continue_execution = (t_func*)(long)list->label[label_cont].Value;

	delete(list);
}

bool find_and_replace(char *opcode, int opcode_size, char *pattern, int size, char *insert)
{
	for (int i=0;i<opcode_size - size;i++){
		bool match = true;
		for (int j=0;j<size;j++)
			if (pattern[j] != opcode[i + j]){
				match = false;
				break;
			}
		if (match){
			for (int j=0;j<size;j++)
				opcode[i + j] = insert[j];
			return true;
		}
	}
	return false;
}

// generate opcode
void Script::Compiler()
{
	msg_db_f("Compiler",2);
	Asm::CurrentMetaInfo = syntax->AsmMetaInfo;

	syntax->MapLocalVariablesToStack();

	syntax->BreakDownComplicatedCommands();
#ifdef ScriptDebug
	syntax->Show();
#endif

	syntax->Simplify();
	syntax->PreProcessor(this);

	if (syntax->FlagShow)
		syntax->Show();

	AllocateMemory();
	AllocateStack();

	MemorySize = 0;
	MapGlobalVariablesToMemory();
	MapConstantsToMemory();

	AllocateOpcode();



// compiling an operating system?
//   -> create an entry point for execution... so we can just call Opcode like a function
	if (syntax->FlagAddEntryPoint)
		CompileOsEntryPoint();



	syntax->PreProcessorAddresses(this);


// compile functions into Opcode
	func.resize(syntax->Functions.num);
	foreachi(Function *f, syntax->Functions, i){
		if (f->is_extern){
			func[i] = (t_func*)GetExternalLink(f->name);
			if (!func[i])
				DoErrorLink("external function " + f->name + " not linkable");
			//func[i] = (t_func*)((long)func[i] + (long)Opcode - syntax->AsmMetaInfo->CodeOrigin);
		}else{
			func[i] = (t_func*)(syntax->AsmMetaInfo->CodeOrigin + OpcodeSize);
			CompileFunction(f, Opcode, OpcodeSize);
		}
	}

// link functions
	foreach(Asm::WantedLabel &l, functions_to_link){
		string name = l.Name.substr(10, -1);
		bool found = false;
		foreachi(Function *f, syntax->Functions, i)
			if (f->name == name){
				*(int*)&Opcode[l.Pos] = (char*)func[i] - (char*)&Opcode[l.Pos + 4];
				found = true;
				break;
			}
		if (!found)
			DoErrorLink("could not link function: " + name);
	}
	foreach(int n, function_vars_to_link){
		void *p = (void*)(long)(n + 0xefef0000);
		void *q = (void*)func[n];
		if (!find_and_replace(Opcode, OpcodeSize, (char*)&p, config.PointerSize, (char*)&q))
			DoErrorLink("could not link function as variable: " + syntax->Functions[n]->name);
	}

// link virtual functions into vtables
	foreach(Type *t, syntax->Types)
		t->LinkVirtualTable();


// "task" for the first execution of main() -> ThreadOpcode
	if (!syntax->FlagCompileOS)
		CompileTaskEntryPoint();




	if (syntax->FlagAddEntryPoint)
		LinkOsEntryPoint();


	// initialize global objects
	if (!syntax->FlagCompileOS)
		init_all_global_objects(syntax, g_var);

	//msg_db_out(1,GetAsm(Opcode,OpcodeSize));

	//_expand(Opcode,OpcodeSize);

	if (first_execution)
		WaitingMode = WaitingModeFirst;
	else
		WaitingMode = WaitingModeNone;

	if (ShowCompilerStats){
		msg_write("--------------------------------");
		msg_write(format("Opcode: %db, Memory: %db",OpcodeSize,MemorySize));
	}
}

};



