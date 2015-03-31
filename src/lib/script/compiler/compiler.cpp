#include "../script.h"
#include "../../file/file.h"
#include "../../base/set.h"

#ifdef OS_LINUX
	#include <sys/mman.h>
	#if (!defined(__x86_64__)) && (!defined(__amd64__))
		#define MAP_32BIT		0
	#endif
#endif
#ifdef OS_WINDOWS
	#include "windows.h"
#endif
#include <errno.h>

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
	if (d > 0){
		if (d > 120)
			list->add2(Asm::inst_add, Asm::param_reg(Asm::REG_ESP), Asm::param_imm(d, 4));
		else
			list->add2(Asm::inst_add, Asm::param_reg(Asm::REG_ESP), Asm::param_imm(d, 1));
	}else if (d < 0){
		if (d < -120)
			list->add2(Asm::inst_sub, Asm::param_reg(Asm::REG_ESP), Asm::param_imm(-d, 4));
		else
			list->add2(Asm::inst_sub, Asm::param_reg(Asm::REG_ESP), Asm::param_imm(-d, 1));
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
			s = c.value.num + config.super_array_size;
		}
		MemorySize += mem_align(s, 4);
	}
	if (MemorySize > 0){
#ifdef OS_WINDOWS
		Memory = (char*)VirtualAlloc(NULL, MemorySize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#else
		//Memory = (char*)mmap(0, MemorySize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS /*| MAP_EXECUTABLE*/ | MAP_32BIT, -1, 0);
		Memory = (char*)mmap(0, mem_align(MemorySize, 4096), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS /*| MAP_EXECUTABLE*/ | MAP_32BIT, -1, 0);
		if (Memory == (char*)-1)
			Memory = new char[MemorySize];
			//DoErrorInternal(format("can not allocate memory, (%d) ", errno) + strerror(errno));
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
				Stack = new char[config.stack_size];
				break;
			}
	}
}

void Script::AllocateOpcode()
{
	int max_opcode = SCRIPT_MAX_OPCODE;
	if (syntax->FlagCompileOS)
		max_opcode *= 10;
	// allocate some memory for the opcode......    has to be executable!!!   (important on amd64)
#ifdef OS_WINDOWS
	Opcode=(char*)VirtualAlloc(NULL,max_opcode,MEM_COMMIT | MEM_RESERVE,PAGE_EXECUTE_READWRITE);
	ThreadOpcode=(char*)VirtualAlloc(NULL,SCRIPT_MAX_THREAD_OPCODE,MEM_COMMIT | MEM_RESERVE,PAGE_EXECUTE_READWRITE);
#else
	Opcode = (char*)mmap(0, max_opcode, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS | MAP_EXECUTABLE | MAP_32BIT, -1, 0);
	ThreadOpcode = (char*)mmap(0, SCRIPT_MAX_THREAD_OPCODE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_ANONYMOUS | MAP_EXECUTABLE | MAP_32BIT, -1, 0);
	if (((long)Opcode==-1)||((long)ThreadOpcode==-1)){
		Opcode = new char[max_opcode];
		ThreadOpcode = new char[SCRIPT_MAX_THREAD_OPCODE];
	}
#endif
	if (((long)Opcode==-1)||((long)ThreadOpcode==-1))
		DoErrorInternal("Script:  could not allocate executable memory");
	if (syntax->AsmMetaInfo->code_origin == 0)
		syntax->AsmMetaInfo->code_origin = (long)Opcode;
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
			s = syntax->Constants[i].value.num;

			*(void**)&Memory[MemorySize] = &Memory[MemorySize + config.super_array_size]; // .data
			*(int*)&Memory[MemorySize + config.pointer_size    ] = s; // .num
			*(int*)&Memory[MemorySize + config.pointer_size + 4] = 0; // .reserved
			*(int*)&Memory[MemorySize + config.pointer_size + 8] = 1; // .item_size
			MemorySize += config.super_array_size;
		}
		memcpy(&Memory[MemorySize], (void*)c.value.data, s);
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
	int ocs_new = mem_align(OpcodeSize, config.function_align);
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
		Asm::AddInstruction(Opcode, OpcodeSize, Asm::inst_call, Asm::param_imm(0, 4));
	TaskReturnOffset=OpcodeSize;
	OCORA = Asm::OCParam;

	// put strings into Opcode!
	foreachi(Constant &c, syntax->Constants, i){
		if (syntax->FlagCompileOS){// && (c.type == TypeCString)){
			cnst[i] = (char*)(OpcodeSize + syntax->AsmMetaInfo->code_origin);
			int s = c.type->size;
			if (c.type == TypeString){
				// const string -> variable length
				s = syntax->Constants[i].value .num;

				*(void**)&Opcode[OpcodeSize] = (char*)(OpcodeSize + syntax->AsmMetaInfo->code_origin + config.super_array_size); // .data
				*(int*)&Opcode[OpcodeSize + config.pointer_size    ] = s; // .num
				*(int*)&Opcode[OpcodeSize + config.pointer_size + 4] = 0; // .reserved
				*(int*)&Opcode[OpcodeSize + config.pointer_size + 8] = 1; // .item_size
				OpcodeSize += config.super_array_size;
			}else if (c.type == TypeCString){
				s = syntax->Constants[i].value .num;
			}
			memcpy(&Opcode[OpcodeSize], (void*)c.value.data, s);
			OpcodeSize += s;

			// cstring -> 0 terminated
			if (c.type == TypeCString)
				Opcode[OpcodeSize ++] = 0;
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
		int lll = (long)func[nf] - syntax->AsmMetaInfo->code_origin - TaskReturnOffset;
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

	int label_first = list->add_label("_first_execution");

	first_execution = (t_func*)&ThreadOpcode[ThreadOpcodeSize];
	// intro
	list->add2(Asm::inst_push, Asm::param_reg(Asm::REG_EBP)); // within the actual program
	list->add2(Asm::inst_mov, Asm::param_reg(Asm::REG_EBP), Asm::param_reg(Asm::REG_ESP));
	list->add2(Asm::inst_mov, Asm::param_reg(Asm::REG_ESP), Asm::param_deref_imm((long)&Stack[config.stack_size], 4)); // start of the script stack
	list->add2(Asm::inst_push, Asm::param_reg(Asm::REG_EBP)); // address of the old stack
	AddEspAdd(list, -12); // space for wait() task data
	list->add2(Asm::inst_mov, Asm::param_reg(Asm::REG_EBP), Asm::param_reg(Asm::REG_ESP));
	list->add2(Asm::inst_mov, Asm::param_reg(Asm::REG_EAX), Asm::param_imm(WaitingModeNone, 4)); // "reset"
	list->add2(Asm::inst_mov, Asm::param_deref_imm((long)&WaitingMode, 4), Asm::param_reg(Asm::REG_EAX));

	// call main()
	list->add2(Asm::inst_call, Asm::param_imm((long)_main_, 4));

	// outro
	AddEspAdd(list, 12); // make space for wait() task data
	list->add2(Asm::inst_pop, Asm::param_reg(Asm::REG_ESP));
	list->add2(Asm::inst_mov, Asm::param_reg(Asm::REG_EBP), Asm::param_reg(Asm::REG_ESP));
	list->add2(Asm::inst_leave);
	list->add2(Asm::inst_ret);

	// "task" for execution after some wait()
	int label_cont = list->add_label("_continue_execution");

	// Intro
	list->add2(Asm::inst_push, Asm::param_reg(Asm::REG_EBP)); // within the external program
	list->add2(Asm::inst_mov, Asm::param_reg(Asm::REG_EBP), Asm::param_reg(Asm::REG_ESP));
	list->add2(Asm::inst_mov, Asm::param_deref_imm((long)&Stack[config.stack_size - 4], 4), Asm::param_reg(Asm::REG_EBP)); // save the external ebp
	list->add2(Asm::inst_mov, Asm::param_reg(Asm::REG_ESP), Asm::param_deref_imm((long)&Stack[config.stack_size - 16], 4)); // to the eIP of the script
	list->add2(Asm::inst_pop, Asm::param_reg(Asm::REG_EAX));
	list->add2(Asm::inst_add, Asm::param_reg(Asm::REG_EAX), Asm::param_imm(AfterWaitOCSize, 4));
	list->add2(Asm::inst_jmp, Asm::param_reg(Asm::REG_EAX));
	//list->add2(Asm::inst_leave);
	//list->add2(Asm::inst_ret);
	/*OCAddChar(0x90);
	OCAddChar(0x90);
	OCAddChar(0x90);*/

	list->Compile(ThreadOpcode, ThreadOpcodeSize);

	first_execution = (t_func*)(long)list->label[label_first].value;
	continue_execution = (t_func*)(long)list->label[label_cont].value;

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

void relink_calls(SyntaxTree *ps, SyntaxTree *a, SyntaxTree *b, int const_off, int var_off, int func_off)
{
	foreach(Command *c, ps->Commands){
		// keep commands... just redirect var/const/func
		//msg_write(p2s(c->script));
		if (c->script != b->script)
			continue;
		if (c->kind == KindVarGlobal){
			c->link_no += var_off;
			c->script = a->script;
		}else if (c->kind == KindConstant){
			c->link_no += const_off;
			c->script = a->script;
		}else if ((c->kind == KindFunction) || (c->kind == KindVarFunction)){
			c->link_no += func_off;
			c->script = a->script;
		}
	}
}

struct IncludeTranslationData
{
	int const_off;
	int func_off;
	int var_off;
	SyntaxTree *source;
};

IncludeTranslationData import_deep(SyntaxTree *a, SyntaxTree *b)
{
	IncludeTranslationData d;
	d.const_off = a->Constants.num;
	d.var_off = a->RootOfAllEvil.var.num;
	d.func_off = a->Functions.num;
	d.source = b;

	a->Constants.append(b->Constants);

	a->RootOfAllEvil.var.append(b->RootOfAllEvil.var);

	foreach(Function *f, b->Functions){
		Function *ff = a->AddFunction(f->name, f->return_type);
		*ff = *f;
		// keep block pointing to include file...
	}

	//int asm_off = a->AsmBlocks.num;
	foreach(AsmBlock &ab, b->AsmBlocks){
		a->AsmBlocks.add(ab);
	}

	return d;
}

void add_includes(Script *s, Set<Script*> &includes)
{
	foreach(Script *i, s->syntax->Includes){
		if (i->Filename.find(".kaba") < 0)
			continue;
		includes.add(i);
		add_includes(i, includes);
	}
}

void import_includes(Script *s)
{
	Set<Script*> includes;
	add_includes(s, includes);
	Array<IncludeTranslationData> da;
	foreach(Script *i, includes)
		da.add(import_deep(s->syntax, i->syntax));

	foreach(Script *i, includes){
		foreach(IncludeTranslationData &d, da){
			relink_calls(s->syntax, s->syntax, d.source, d.const_off, d.var_off, d.func_off);
			relink_calls(i->syntax, s->syntax, d.source, d.const_off, d.var_off, d.func_off);
		}
	}
}

// generate opcode
void Script::Compiler()
{
	msg_db_f("Compiler",2);
	Asm::CurrentMetaInfo = syntax->AsmMetaInfo;

	if (syntax->FlagCompileOS)
		import_includes(this);

	syntax->MapLocalVariablesToStack();

	syntax->BreakDownComplicatedCommands();
#ifdef ScriptDebug
	syntax->Show();
#endif

	syntax->Simplify();
	syntax->PreProcessor();

	if (config.verbose)
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



	syntax->PreProcessorAddresses();

	if (config.verbose)
		syntax->Show();


// compile functions into Opcode
	CompileFunctions(Opcode, OpcodeSize);

// link functions
	foreach(Asm::WantedLabel &l, functions_to_link){
		string name = l.name.substr(10, -1);
		bool found = false;
		foreachi(Function *f, syntax->Functions, i)
			if (f->name == name){
				*(int*)&Opcode[l.pos] = (long)func[i] - (syntax->AsmMetaInfo->code_origin + l.pos + 4);
				found = true;
				break;
			}
		if (!found)
			DoErrorLink("could not link function: " + name);
	}
	foreach(int n, function_vars_to_link){
		void *p = (void*)(long)(n + 0xefef0000);
		void *q = (void*)func[n];
		if (!find_and_replace(Opcode, OpcodeSize, (char*)&p, config.pointer_size, (char*)&q))
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



