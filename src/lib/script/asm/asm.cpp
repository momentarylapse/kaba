#include "../../base/base.h"
#include "../../file/file.h"
#include "asm.h"
#include <stdio.h>

namespace Asm
{


int OCParam;



InstructionSetData InstructionSet;

struct ParserState
{
	bool EndOfLine;
	bool EndOfCode;
	int LineNo;
	int ColumnNo;
	int DefaultSize;
	int ParamSize, AddrSize;
	bool ExtendModRMBase;
	bool ExtendModRMReg;
	bool ExtendModRMIndex;
	int FullRegisterSize;
	void init()
	{
		DefaultSize = SIZE_32;
		FullRegisterSize = InstructionSet.pointer_size;

		if (CurrentMetaInfo)
			if (CurrentMetaInfo->mode16)
				DefaultSize = SIZE_16;
	}
	void reset()
	{
		ParamSize = DefaultSize;
		AddrSize = DefaultSize;
		ExtendModRMBase = false;
		ExtendModRMReg = false;
		ExtendModRMIndex = false;
	}
};
static ParserState state;

const char *code_buffer;
MetaInfo *CurrentMetaInfo = NULL;
MetaInfo DummyMetaInfo;

Exception::Exception(const string &_message, const string &_expression, int _line, int _column)
{
	if (_expression.num > 0)
		message += "\"" + _expression + "\": ";
	message += _message;
	line = _line;
	column = _column;
	if (line >= 0)
		message += "\nline " + i2s(line);
}

Exception::~Exception(){}

void Exception::print() const
{
	msg_error(message);
}

void SetError(const string &str)
{
	//msg_error(str + format("\nline %d", LineNo + 1));
	throw Exception(str, "", state.LineNo, state.ColumnNo);
}


bool DebugAsm = false;

static void so(const char *str)
{
	if (DebugAsm)
		printf("%s\n",str);
}

static void so(const string &str)
{
	if (DebugAsm)
		printf("%s\n",str.c_str());
}

static void so(int i)
{
	if (DebugAsm)
		printf("%d\n",i);
}

// penalty:  0 -> max output
#define ASM_DB_LEVEL	10



MetaInfo::MetaInfo()
{
	mode16 = false;
	code_origin = 0;
	line_offset = 0;
}



// groups of registers
enum
{
	RegGroupNone,
	RegGroupGeneral,
	RegGroupGeneral2,
	RegGroupSegment,
	RegGroupFlags,
	RegGroupControl,
	RegGroupX87,
	RegGroupXmm,
};


struct Register
{
	string name;
	int id, group, size;
	bool extend_mod_rm;
};
Array<Register> Registers;
Array<Register*> RegisterByID;
int RegRoot[NUM_REGISTERS];
int RegResize[NUM_REG_ROOTS][MAX_REG_SIZE + 1];

void add_reg(const string &name, int id, int group, int size, int root = -1)
{
	Register r;
	r.extend_mod_rm = false;
	r.name = name;
	r.id = id;
	r.group = group;
	if (group == RegGroupGeneral2){
		r.group = RegGroupGeneral;
		r.extend_mod_rm = true;
	}
	r.size = size;
	Registers.add(r);
	if (root < 0)
		root = NUM_REG_ROOTS - 1;
	RegRoot[id] = root;
	RegResize[root][size] = id;
}

string GetRegName(int reg)
{
	if ((reg < 0) || (reg >= NUM_REGISTERS))
		return "INVALID REG: " + i2s(reg);
	return RegisterByID[reg]->name;
}

struct InstructionName
{
	int inst;
	string name;
	int rw1, rw2; // parameter is read(1), modified(2) or both (3)
};

// rw1/2: 
InstructionName InstructionNames[NUM_INSTRUCTION_NAMES + 1] = {
	{inst_db,		"db"},
	{inst_dw,		"dw"},
	{inst_dd,		"dd"},
	{inst_ds,		"ds"},
	{inst_dz,		"dz"},

	{inst_add,		"add",		3, 1},
	{inst_adc,		"adc",		3, 1},
	{inst_sub,		"sub",		3, 1},
	{inst_sbb,		"sbb",		3, 1},
	{inst_inc,		"inc",		3},
	{inst_dec,		"dec",		3},
	{inst_mul,		"mul",		3, 1},
	{inst_imul,		"imul",		3, 1},
	{inst_div,		"div",		3, 1},
	{inst_idiv,		"idiv",		3, 1},
	{inst_mov,		"mov",		2, 1},
	{inst_movzx,	"movzx",	2, 1},
	{inst_movsx,	"movsx",	2, 1},
	{inst_and,		"and",		3, 1},
	{inst_or,		"or",		3, 1},
	{inst_xor,		"xor",		3, 1},
	{inst_not,		"not",		3},
	{inst_neg,		"neg",		3},
	{inst_pop,		"pop",		2},
	{inst_popa,		"popa",		2},
	{inst_push,		"push",		1},
	{inst_pusha,	"pusha",	1},
	
	{inst_jo,		"jo",		1},
	{inst_jno,		"jno",		1},
	{inst_jb,		"jb",		1},
	{inst_jnb,		"jnb",		1},
	{inst_jz,		"jz",		1},
	{inst_jnz,		"jnz",		1},
	{inst_jbe,		"jbe",		1},
	{inst_jnbe,		"jnbe",		1},
	{inst_js,		"js",		1},
	{inst_jns,		"jns",		1},
	{inst_jp,		"jp",		1},
	{inst_jnp,		"jnp",		1},
	{inst_jl,		"jl",		1},
	{inst_jnl,		"jnl",		1},
	{inst_jle,		"jle",		1},
	{inst_jnle,		"jnle",		1},
	
	{inst_cmp,		"cmp",		1, 1},
	
	{inst_seto,		"seto",		2},
	{inst_setno,	"setno",	2},
	{inst_setb,		"setb",		2},
	{inst_setnb,	"setnb",	2},
	{inst_setz,		"setz",		2},
	{inst_setnz,	"setnz",	2},
	{inst_setbe,	"setbe",	2},
	{inst_setnbe,	"setnbe",	2},
	{inst_sets,		"sets",		2},
	{inst_setns,	"setns",	2},
	{inst_setp,		"setp",		2},
	{inst_setnp,	"setnp",	2},
	{inst_setl,		"setl",		2},
	{inst_setnl,	"setnl",	2},
	{inst_setle,	"setle",	2},
	{inst_setnle,	"setnle",	2},
	
	{inst_sldt,		"sldt"},
	{inst_str,		"str"},
	{inst_lldt,		"lldt"},
	{inst_ltr,		"ltr"},
	{inst_verr,		"verr"},
	{inst_verw,		"verw"},
	{inst_sgdt,		"sgdt"},
	{inst_sidt,		"sidt"},
	{inst_lgdt,		"lgdt"},
	{inst_lidt,		"lidt"},
	{inst_smsw,		"smsw"},
	{inst_lmsw,		"lmsw"},
	
	{inst_test,		"test",		1, 1},
	{inst_xchg,		"xchg",		3, 3},
	{inst_lea,		"lea", 		2, 1},
	{inst_nop,		"nop"},
	{inst_cbw_cwde,	"cbw/cwde"},
	{inst_cgq_cwd,	"cgq/cwd"},
	{inst_movs_ds_esi_es_edi,	"movs_ds:esi,es:edi"},
	{inst_movs_b_ds_esi_es_edi,	"movs.b_ds:esi,es:edi"},
	{inst_cmps_ds_esi_es_edi,	"cmps_ds:esi,es:edi"},
	{inst_cmps_b_ds_esi_es_edi,	"cmps.b_ds:esi,es:edi"},
	{inst_rol,		"rol",		3, 1},
	{inst_ror,		"ror",		3, 1},
	{inst_rcl,		"rcl",		3, 1},
	{inst_rcr,		"rcr",		3, 1},
	{inst_shl,		"shl",		3, 1},
	{inst_shr,		"shr",		3, 1},
	{inst_sar,		"sar",		3, 1},
	{inst_ret,		"ret",		1},
	{inst_leave,	"leave",	1},
	{inst_ret_far,	"ret_far",	1},
	{inst_int,		"int",		1},
	{inst_iret,		"iret",		1},
	
	{inst_fadd,		"fadd",		1},
	{inst_fmul,		"fmul",		1},
	{inst_fsub,		"fsub",		1},
	{inst_fdiv,		"fdiv",		1},
	{inst_fld,		"fld",		1},
	{inst_fld1,		"fld1",		0},
	{inst_fldz,		"fldz",		0},
	{inst_fldpi,	"fldpi",	0},
	{inst_fst,		"fst",		2},
	{inst_fstp,		"fstp",		2},
	{inst_fild,		"fild",		1},
	{inst_faddp,	"faddp",	1},
	{inst_fmulp,	"fmulp",	1},
	{inst_fsubp,	"fsubp",	1},
	{inst_fdivp,	"fdivp",	1},
	{inst_fldcw,	"fldcw",	1},
	{inst_fnstcw,	"fnstcw",	2},
	{inst_fnstsw,	"fnstsw",	2},
	{inst_fistp,	"fistp",	2},
	{inst_fxch,		"fxch",		3, 3},
	{inst_fsqrt,	"fsqrt",	3},
	{inst_fsin,		"fsin",		3},
	{inst_fcos,		"fcos",		3},
	{inst_fptan,	"fptan",	3},
	{inst_fpatan,	"fpatan",	3},
	{inst_fyl2x,	"fyl2x",	3},
	{inst_fchs,		"fchs",		3},
	{inst_fabs,		"fabs",		3},
	{inst_fucompp,	"fucompp",	1, 1},
	
	{inst_loop,		"loop"},
	{inst_loope,	"loope"},
	{inst_loopne,	"loopne"},
	{inst_in,		"in",		2, 1},
	{inst_out,		"out",		1, 1},
	
	{inst_call,		"call",		1},
	{inst_call_far,	"call_far", 1},
	{inst_jmp,		"jmp",		1},
	{inst_jmp_far,	"jmp_far",		1},
	{inst_lock,		"lock"},
	{inst_rep,		"rep"},
	{inst_repne,	"repne"},
	{inst_hlt,		"hlt"},
	{inst_cmc,		"cmc"},
	{inst_clc,		"clc"},
	{inst_stc,		"stc"},
	{inst_cli,		"cli"},
	{inst_sti,		"sti"},
	{inst_cld,		"cld"},
	{inst_std,		"std"},

	{inst_movss,		"movss"},
	{inst_movsd,		"movsd"},

	{inst_b,		"b"},
	{inst_bl,		"bl"},

	{inst_ldr,		"ldr"},
	{inst_ldrb,		"ldrb"},
//	{inst_str,		"str"},
	{inst_strb,		"strb"},

	{inst_ldmia,		"ldmia"},
	{inst_ldmib,		"ldmib"},
	{inst_ldmda,		"ldmda"},
	{inst_ldmdb,		"ldmdb"},
	{inst_stmia,		"stmia"},
	{inst_stmib,		"stmib"},
	{inst_stmda,		"stmda"},
	{inst_stmdb,		"stmdb"},

	{inst_eor,	"eor"},
	{inst_rsb,	"rsb"},
	{inst_sbc,	"sbc"},
	{inst_rsc,	"rsc"},
	{inst_tst,	"tst"},
	{inst_teq,	"teq"},
	{inst_cmn,	"cmn"},
	{inst_orr,	"orr"},
	{inst_bic,	"bic"},
	{inst_mvn,	"mvn"},
	
	{-1,			"???"}
};



// parameter types
enum
{
	PARAMT_IMMEDIATE,
	PARAMT_REGISTER,
	PARAMT_REGISTER_OR_MEM, // ...
	PARAMT_MEMORY,
	PARAMT_REGISTER_SET,
	//PARAMT_SIB,
	PARAMT_NONE,
	PARAMT_INVALID
};


InstructionParam param_none;

// short parameter type
enum
{
	__Dummy__ = 10000,
	Eb,Ew,Ed,Eq,E48,
	Gb,Gw,Gd,Gq,
	Ib,Iw,Id,Iq,I48,
	Ob,Ow,Od,Oq,
	Rb,Rw,Rd,Rq,
	Cb,Cw,Cd,Cq,
	Mb,Mw,Md,Mq,
	Jb,Jw,Jd,Jq,
	Sw,Xx,
};

// displacement for registers
enum
{
	DISP_MODE_NONE,     // reg
	DISP_MODE_8,        // reg + 8bit
	DISP_MODE_16,       // reg + 16bit
	DISP_MODE_32,       // reg + 32bit
	DISP_MODE_SIB,      // SIB-byte
	DISP_MODE_8_SIB,    // SIB-byte + 8bit
	DISP_MODE_REG2,     // reg + reg2
	DISP_MODE_8_REG2,   // reg + reg2 + 8bit
	DISP_MODE_16_REG2   // reg + reg2 + 16bit
};


InstructionWithParamsList::InstructionWithParamsList(int line_no)
{
	current_inst = 0;
	current_line = line_no;
	current_col = 0;
}

InstructionWithParamsList::~InstructionWithParamsList()
{}

InstructionParam param_reg(int reg)
{
	InstructionParam p;
	p.type = PARAMT_REGISTER;
	p.reg = RegisterByID[reg];
	p.size = p.reg->size;
	return p;
}

InstructionParam param_deref_reg(int reg, int size)
{
	InstructionParam p;
	p.type = PARAMT_REGISTER;
	p.reg = RegisterByID[reg];
	p.size = size;
	p.deref = true;
	return p;
}

InstructionParam param_reg_set(int set)
{
	InstructionParam p;
	p.type = PARAMT_REGISTER_SET;
	p.size = SIZE_32;
	p.value = set;
	return p;
}

InstructionParam param_deref_reg_shift(int reg, int shift, int size)
{
	InstructionParam p;
	p.type = PARAMT_REGISTER;
	p.reg = RegisterByID[reg];
	p.size = size;
	p.deref = true;
	p.value = shift;
	p.disp = ((shift < 120) && (shift > -120)) ? DISP_MODE_8 : DISP_MODE_32;
	return p;
}

InstructionParam param_deref_reg_shift_reg(int reg, int reg2, int size)
{
	InstructionParam p;
	p.type = PARAMT_REGISTER;
	p.reg = RegisterByID[reg];
	p.size = size;
	p.reg2 = RegisterByID[reg2];
	p.deref = true;
	p.value = 1;
	p.disp = DISP_MODE_REG2;
	return p;
}

InstructionParam param_imm(long long value, int size)
{
	InstructionParam p;
	p.type = PARAMT_IMMEDIATE;
	p.size = size;
	p.value = value;
	return p;
}

InstructionParam param_deref_imm(long long value, int size)
{
	InstructionParam p;
	p.type = PARAMT_IMMEDIATE;
	p.size = size;
	p.value = value;
	p.deref = true;
	return p;
}

InstructionParam param_label(long long value, int size)
{
	InstructionParam p;
	p.type = PARAMT_IMMEDIATE;
	p.size = size;
	p.value = value;
	p.is_label = true;
	return p;
}

void InstructionWithParamsList::add_arm(int cond, int inst, const InstructionParam &p1 = param_none, const InstructionParam &p2, const InstructionParam &p3)
{
	InstructionWithParams i;
	i.inst = inst;
	i.condition = cond;
	i.p[0] = p1;
	i.p[1] = p2;
	i.p[2] = p3;
	i.line = current_line;
	i.col = current_col;
	add(i);
}

void InstructionWithParamsList::add2(int inst, const InstructionParam &p1, const InstructionParam &p2)
{
	InstructionWithParams i;
	i.inst = inst;
	i.condition = ARM_COND_ALWAYS;
	i.p[0] = p1;
	i.p[1] = p2;
	i.p[2] = param_none;
	i.line = current_line;
	i.col = current_col;
	add(i);
}

void InstructionWithParamsList::show()
{
	msg_write("--------------");
	foreach(Asm::InstructionWithParams &i, *this)
		msg_write(i.str());
}

int InstructionWithParamsList::add_label(const string &name, bool declaring)
{
	so("add_label: " + name);
	// label already in use? (used before declared)
	if (declaring){
		foreachi(Label &l, label, i)
			if (l.inst_no < 0)
				if (l.name == name){
					l.inst_no = num;
					so("----redecl");
					return i;
				}
	}else{
		foreachi(Label &l, label, i)
			if (l.name == name){
				so("----reuse");
				return i;
			}
	}
	Label l;
	l.name = name;
	l.inst_no = declaring ? num : -1;
	l.value = -1;
	label.add(l);
	return label.num - 1;
}

void InstructionWithParamsList::add_func_intro(int stack_alloc_size)
{
	if (InstructionSet.set == INSTRUCTION_SET_ARM)
		return;
	long reg_bp = (InstructionSet.set == INSTRUCTION_SET_AMD64) ? REG_RBP : REG_EBP;
	long reg_sp = (InstructionSet.set == INSTRUCTION_SET_AMD64) ? REG_RSP : REG_ESP;
	int s = InstructionSet.pointer_size;
	add2(inst_push, param_reg(reg_bp));
	add2(inst_mov, param_reg(reg_bp), param_reg(reg_sp));
	if (stack_alloc_size > 127){
		add2(inst_sub, param_reg(reg_sp), param_imm(stack_alloc_size, SIZE_32));
	}else if (stack_alloc_size > 0){
		add2(inst_sub, param_reg(reg_sp), param_imm(stack_alloc_size, SIZE_8));
	}
}

void InstructionWithParamsList::add_func_return(int return_size)
{
	add2(inst_leave);
	if (return_size > 4)
		add2(inst_ret, param_imm(4, SIZE_16));
	else
		add2(inst_ret);
}

// which part of the modr/m byte is used
enum
{
	MRM_NONE,
	MRM_REG,
	MRM_MOD_RM
};

string SizeOut(int size)
{
	if (size == SIZE_8)		return "8";
	if (size == SIZE_16)		return "16";
	if (size == SIZE_32)		return "32";
	if (size == SIZE_48)		return "48";
	if (size == SIZE_64)		return "64";
	if (size == SIZE_128)		return "128";
	return "???";
}


string get_size_name(int size)
{
	if (size == SIZE_8)
		return "byte";
	if (size == SIZE_16)
		return "word";
	if (size == SIZE_32)
		return "dword";
	if (size == SIZE_48)
		return "s48";
	if (size == SIZE_64)
		return "qword";
	if (size == SIZE_128)
		return "dqword";
	return "";
}

// parameter definition (filter for real parameters)
struct InstructionParamFuzzy
{
	bool used;
	bool allow_memory_address;	// [0x12.34...]
	bool allow_memory_indirect;	// [eax]    [eax + ...]
	bool allow_immediate;		// 0x12.34...
	bool allow_register;		// eax
	int _type_;					// approximate type.... (UnFuzzy without mod/rm)
	Register *reg;				// if != NULL  -> force a single register
	int reg_group;
	int mrm_mode;				// which part of the modr/m byte is used?
	int size;
	bool immediate_is_relative;	// for jump


	bool match(InstructionParam &p);
	void print() const;
};

void InstructionParamFuzzy::print() const
{
	string t;
	if (used){
		if (allow_register)
			t += "	Reg";
		if (allow_immediate)
			t += "	Im";
		if (allow_memory_address)
			t += "	[Mem]";
		if (allow_memory_indirect)
			t += "	[Mem + ind]";
		if (reg)
			t += "  " + reg->name;
		if (size != SIZE_UNKNOWN)
			t += "  " + SizeOut(size);
		if (mrm_mode == MRM_REG)
			t += "   /r";
		else if (mrm_mode == MRM_MOD_RM)
			t += "   /m";
	}else{
		t += "	None";
	}
	printf("%s\n", t.c_str());
}

// an instruction/opcode the cpu offers
struct CPUInstruction
{
	int inst;
	int code, code_size, cap;
	bool has_modrm, has_small_param, has_small_addr, has_big_param, has_big_addr, has_fixed_param;
	bool ignore;
	InstructionParamFuzzy param1, param2;
	string name;

	bool match(InstructionWithParams &iwp);
	void print() const
	{
		printf("inst: %s   %.4x (%d) %d  %s\n", name.c_str(), code, code_size, cap, has_modrm ? "modr/m" : "");
		param1.print();
		param2.print();
	}
};

Array<CPUInstruction> CPUInstructions;

bool CPUInstruction::match(InstructionWithParams &iwp)
{
	if (inst != iwp.inst)
		return false;

	//return (param1.match(iwp.p[0])) && (param2.match(iwp.p[1]));
	bool b = (param1.match(iwp.p[0])) && (param2.match(iwp.p[1]));
	/*if (b){
		msg_write("source: " + iwp.p[0].str() + " " + iwp.p[1].str());
		print();
	}*/
	return b;
}

// expands the short instruction parameters
//   returns true if mod/rm byte needed
bool _get_inst_param_(int param, InstructionParamFuzzy &ip)
{
	ip.reg = NULL;
	ip.reg_group = RegGroupNone;
	ip.mrm_mode = MRM_NONE;
	ip.reg_group = -1;
	ip._type_ = PARAMT_INVALID;
	ip.allow_register = false;
	ip.allow_immediate = false;
	ip.allow_memory_address = false;
	ip.allow_memory_indirect = false;
	ip.immediate_is_relative = false;
	if (param < 0){	ip.used = false;	ip._type_ = PARAMT_NONE;	return false;	}
	ip.used = true;

	// is it a register?
	for (int i=0;i<Registers.num;i++)
		if (Registers[i].id == param){
			ip._type_ = PARAMT_REGISTER;
			ip.reg = &Registers[i];
			ip.allow_register = true;
			ip.reg_group = Registers[i].group;
			ip.size = Registers[i].size;
			return false;
		}
	// general reg / mem
	if ((param == Eb) || (param == Eq) || (param == Ew) || (param == Ed) || (param == E48)){
		ip._type_ = PARAMT_INVALID;//ParamTRegisterOrMem;
		ip.allow_register = true;
		ip.allow_memory_address = true;
		ip.allow_memory_indirect = true;
		ip.reg_group = RegGroupGeneral;
		ip.mrm_mode = MRM_MOD_RM;
		if (param == Eb)	ip.size = SIZE_8;
		if (param == Ew)	ip.size = SIZE_16;
		if (param == Ed)	ip.size = SIZE_32;
		if (param == Eq)	ip.size = SIZE_64;
		if (param == E48)	ip.size = SIZE_48;
		return true;
	}
	// general reg (reg)
	if ((param == Gb) || (param == Gq) || (param == Gw) || (param == Gd)){
		ip._type_ = PARAMT_REGISTER;
		ip.allow_register = true;
		ip.reg_group = RegGroupGeneral;
		ip.mrm_mode = MRM_REG;
		if (param == Gb)	ip.size = SIZE_8;
		if (param == Gw)	ip.size = SIZE_16;
		if (param == Gd)	ip.size = SIZE_32;
		if (param == Gq)	ip.size = SIZE_64;
		return true;
	}
	// general reg (mod)
	if ((param == Rb) || (param == Rq) || (param == Rw) || (param == Rd)){
		ip._type_ = PARAMT_REGISTER;
		ip.allow_register = true;
		ip.reg_group = RegGroupGeneral;
		ip.mrm_mode = MRM_MOD_RM;
		if (param == Rb)	ip.size = SIZE_8;
		if (param == Rw)	ip.size = SIZE_16;
		if (param == Rd)	ip.size = SIZE_32;
		if (param == Rq)	ip.size = SIZE_64;
		return true;
	}
	// immediate
	if ((param == Ib) || (param == Iq) || (param == Iw) || (param == Id) || (param == I48)){
		ip._type_ = PARAMT_IMMEDIATE;
		ip.allow_immediate = true;
		if (param == Ib)	ip.size = SIZE_8;
		if (param == Iw)	ip.size = SIZE_16;
		if (param == Id)	ip.size = SIZE_32;
		if (param == Iq)	ip.size = SIZE_64;
		if (param == I48)	ip.size = SIZE_48;
		return false;
	}
	// immediate (relative)
	if ((param == Jb) || (param == Jq) || (param == Jw) || (param == Jd)){
		ip._type_ = PARAMT_IMMEDIATE;
		ip.allow_immediate = true;
		ip.immediate_is_relative = true;
		if (param == Jb)	ip.size = SIZE_8;
		if (param == Jw)	ip.size = SIZE_16;
		if (param == Jd)	ip.size = SIZE_32;
		if (param == Jq)	ip.size = SIZE_64;
		return false;
	}
	// mem
	if ((param == Ob) || (param == Oq) || (param == Ow) || (param == Od)){
		ip._type_ = PARAMT_MEMORY;
		ip.allow_memory_address = true;
		if (param == Ob)	ip.size = SIZE_8;
		if (param == Ow)	ip.size = SIZE_16;
		if (param == Od)	ip.size = SIZE_32;
		if (param == Oq)	ip.size = SIZE_64;
		return false;
	}
	// mem
	if ((param == Mb) || (param == Mq) || (param == Mw) || (param == Md)){
		ip._type_ = PARAMT_INVALID; // ...
		ip.allow_memory_address = true;
		ip.allow_memory_indirect = true;
		ip.reg_group = RegGroupGeneral;
		ip.mrm_mode = MRM_MOD_RM;
		if (param == Mb)	ip.size = SIZE_8;
		if (param == Mw)	ip.size = SIZE_16;
		if (param == Md)	ip.size = SIZE_32;
		if (param == Mq)	ip.size = SIZE_64;
		return true;
	}
	// control reg
	if ((param == Cb) || (param == Cd) || (param == Cw) || (param == Cd)){
		ip._type_ = PARAMT_REGISTER;
		ip.allow_register = true;
		ip.reg_group = RegGroupControl;
		ip.mrm_mode = MRM_REG;
		if (param == Cb)	ip.size = SIZE_8;
		if (param == Cw)	ip.size = SIZE_16;
		if (param == Cd)	ip.size = SIZE_32;
		if (param == Cq)	ip.size = SIZE_64;
		return true;
	}
	// segment reg
	if (param == Sw){
		ip._type_ = PARAMT_REGISTER;
		ip.allow_register = true;
		ip.reg_group = RegGroupSegment;
		ip.mrm_mode = MRM_REG;
		ip.size = SIZE_16;
		return true;
	}
	// xmm reg (reg)
	if (param == Xx){
		ip._type_ = PARAMT_REGISTER;
		ip.allow_register = true;
		ip.reg_group = RegGroupXmm;
		ip.mrm_mode = MRM_REG;
		ip.size = SIZE_128;
		return true;
	}
	msg_error("asm: unknown instparam (call Michi!)");
	msg_write(param);
	exit(0);
	return false;
}

enum
{
	OPT_SMALL_PARAM = 1,
	OPT_SMALL_ADDR,
	OPT_BIG_PARAM,
	OPT_BIG_ADDR,
	OPT_MEDIUM_PARAM,
};

void add_inst(int inst, int code, int code_size, int cap, int param1, int param2, int opt = 0, bool ignore = false)
{
	CPUInstruction i;
	memset(&i.param1, 0, sizeof(i.param1));
	memset(&i.param2, 0, sizeof(i.param2));
	i.inst = inst;
	i.code = code;
	i.code_size = code_size;
	i.cap = cap;
	i.ignore = ignore;
	bool m1 = _get_inst_param_(param1, i.param1);
	bool m2 = _get_inst_param_(param2, i.param2);
	i.has_modrm  = m1 || m2 || (cap >= 0);
	i.has_small_param = (opt == OPT_SMALL_PARAM);
	i.has_small_addr = (opt == OPT_SMALL_ADDR);
	i.has_big_param = (opt == OPT_BIG_PARAM);
	i.has_big_addr = (opt == OPT_BIG_ADDR);
	i.has_fixed_param = (opt != OPT_SMALL_PARAM) && (opt != OPT_MEDIUM_PARAM) && (opt != OPT_BIG_PARAM);
	if ((i.has_big_param) && (InstructionSet.set != INSTRUCTION_SET_AMD64))
		return;

	if (inst == inst_lea)
		i.param2.size = SIZE_UNKNOWN;
	
	i.name = InstructionNames[NUM_INSTRUCTION_NAMES].name;
	for (int j=0;j<NUM_INSTRUCTION_NAMES;j++)
		if (inst == InstructionNames[j].inst)
			i.name = InstructionNames[j].name;
	CPUInstructions.add(i);
}

enum
{
	AP_NONE,
	AP_REG_12,
	AP_REG_16,
	AP_OFFSET24_0,
	AP_IMM12_0,
	AP_SHIFTED12_0,
};


void add_inst_arm(int inst, int code, int param1, int param2 = AP_NONE, int param3 = AP_NONE)
{
	CPUInstruction i;
	memset(&i.param1, 0, sizeof(i.param1));
	memset(&i.param2, 0, sizeof(i.param2));
	i.inst = inst;
	i.code = code;
	i.code_size = 4;
	i.cap = 0;

	i.name = GetInstructionName(inst);
	CPUInstructions.add(i);
}

string GetInstructionName(int inst)
{
	for (int i=0;i<Asm::NUM_INSTRUCTION_NAMES;i++)
		if (inst == InstructionNames[i].inst)
			return Asm::InstructionNames[i].name;
	return "???";
}

void GetInstructionParamFlags(int inst, bool &p1_read, bool &p1_write, bool &p2_read, bool &p2_write)
{
	for (int i=0;i<NUM_INSTRUCTION_NAMES;i++)
		if (InstructionNames[i].inst == inst){
			p1_read = ((InstructionNames[i].rw1 & 1) > 0);
			p1_write = ((InstructionNames[i].rw1 & 2) > 0);
			p2_read = ((InstructionNames[i].rw2 & 1) > 0);
			p2_write = ((InstructionNames[i].rw2 & 2) > 0);
		}
}

bool GetInstructionAllowConst(int inst)
{
	if ((inst == inst_div) || (inst == inst_idiv) || (inst == inst_movss))
		return false;
	for (int i=0;i<NUM_INSTRUCTION_NAMES;i++)
		if (InstructionNames[i].inst == inst)
			return (InstructionNames[i].name[0] != 'f');
	return true;
}

bool GetInstructionAllowGenReg(int inst)
{
	if (inst == inst_lea)
		return false;
	for (int i=0;i<NUM_INSTRUCTION_NAMES;i++)
		if (InstructionNames[i].inst == inst)
			return (InstructionNames[i].name[0] != 'f');
	return true;
}



int QueryInstructionSet()
{
#ifdef CPU_AMD64
	return INSTRUCTION_SET_AMD64;
#endif
#ifdef CPU_X86
	return INSTRUCTION_SET_X86;
#endif
#ifdef CPU_ARM
	return INSTRUCTION_SET_ARM;
#endif
	msg_error("Asm: unknown instruction set");
	return INSTRUCTION_SET_X86;
}


void InitARM()
{
	Registers.clear();
	add_reg("r0",	REG_R0,	RegGroupGeneral,	SIZE_32,	0);
	add_reg("r1",	REG_R1,	RegGroupGeneral,	SIZE_32,	1);
	add_reg("r2",	REG_R2,	RegGroupGeneral,	SIZE_32,	2);
	add_reg("r3",	REG_R3,	RegGroupGeneral,	SIZE_32,	3);
	add_reg("r4",	REG_R4,	RegGroupGeneral,	SIZE_32,	4);
	add_reg("r5",	REG_R5,	RegGroupGeneral,	SIZE_32,	5);
	add_reg("r6",	REG_R6,	RegGroupGeneral,	SIZE_32,	6);
	add_reg("r7",	REG_R7,	RegGroupGeneral,	SIZE_32,	7);
	add_reg("r8",	REG_R8,	RegGroupGeneral,	SIZE_32,	8);
	add_reg("r9",	REG_R9,	RegGroupGeneral,	SIZE_32,	9);
	add_reg("r10",	REG_R10,	RegGroupGeneral,	SIZE_32,	10);
	add_reg("r11",	REG_R11,	RegGroupGeneral,	SIZE_32,	11);
	add_reg("r12",	REG_R12,	RegGroupGeneral,	SIZE_32,	12);
	add_reg("r13",	REG_R13,	RegGroupGeneral,	SIZE_32,	13);
	add_reg("r14",	REG_R14,	RegGroupGeneral,	SIZE_32,	14);
	add_reg("r15",	REG_R15,	RegGroupGeneral,	SIZE_32,	15);

	// create easy to access array
	RegisterByID.clear();
	for (int i=0;i<Registers.num;i++){
		if (RegisterByID.num <= Registers[i].id)
			RegisterByID.resize(Registers[i].id + 1);
		RegisterByID[Registers[i].id] = &Registers[i];
	}

	CPUInstructions.clear();
	add_inst_arm(inst_b,    0x0a000000 ,0);
	add_inst_arm(inst_bl,   0x0b000000 ,0);
}

void InitX86()
{
	int set = InstructionSet.set;

	Registers.clear();
	add_reg("rax",	REG_RAX,	RegGroupGeneral,	SIZE_64,	0);
	add_reg("eax",	REG_EAX,	RegGroupGeneral,	SIZE_32,	0);
	add_reg("ax",	REG_AX,	RegGroupGeneral,	SIZE_16,	0);
	add_reg("ah",	REG_AH,	RegGroupGeneral,	SIZE_8,	0); // RegResize[] will be overwritten by al
	add_reg("al",	REG_AL,	RegGroupGeneral,	SIZE_8,	0);
	add_reg("rcx",	REG_RCX,	RegGroupGeneral,	SIZE_64,	1);
	add_reg("ecx",	REG_ECX,	RegGroupGeneral,	SIZE_32,	1);
	add_reg("cx",	REG_CX,	RegGroupGeneral,	SIZE_16,	1);
	add_reg("ch",	REG_CH,	RegGroupGeneral,	SIZE_8,	1);
	add_reg("cl",	REG_CL,	RegGroupGeneral,	SIZE_8,	1);
	add_reg("rdx",	REG_RDX,	RegGroupGeneral,	SIZE_64,	2);
	add_reg("edx",	REG_EDX,	RegGroupGeneral,	SIZE_32,	2);
	add_reg("dx",	REG_DX,	RegGroupGeneral,	SIZE_16,	2);
	add_reg("dh",	REG_DH,	RegGroupGeneral,	SIZE_8,	2);
	add_reg("dl",	REG_DL,	RegGroupGeneral,	SIZE_8,	2);
	add_reg("rbx",	REG_RBX,	RegGroupGeneral,	SIZE_64,	3);
	add_reg("ebx",	REG_EBX,	RegGroupGeneral,	SIZE_32,	3);
	add_reg("bx",	REG_BX,	RegGroupGeneral,	SIZE_16,	3);
	add_reg("bh",	REG_BH,	RegGroupGeneral,	SIZE_8,	3);
	add_reg("bl",	REG_BL,	RegGroupGeneral,	SIZE_8,	3);

	add_reg("rsp",	REG_RSP,	RegGroupGeneral,	SIZE_64,	4);
	add_reg("esp",	REG_ESP,	RegGroupGeneral,	SIZE_32,	4);
	add_reg("sp",	REG_SP,	RegGroupGeneral,	SIZE_16,	4);
	add_reg("rbp",	REG_RBP,	RegGroupGeneral,	SIZE_64,	5);
	add_reg("ebp",	REG_EBP,	RegGroupGeneral,	SIZE_32,	5);
	add_reg("bp",	REG_BP,	RegGroupGeneral,	SIZE_16,	5);
	add_reg("rsi",	REG_RSI,	RegGroupGeneral,	SIZE_64,	6);
	add_reg("esi",	REG_ESI,	RegGroupGeneral,	SIZE_32,	6);
	add_reg("si",	REG_SI,	RegGroupGeneral,	SIZE_16,	6);
	add_reg("rdi",	REG_RDI,	RegGroupGeneral,	SIZE_64,	7);
	add_reg("edi",	REG_EDI,	RegGroupGeneral,	SIZE_32,	7);
	add_reg("di",	REG_DI,	RegGroupGeneral,	SIZE_16,	7);

	add_reg("r8",	REG_R8,	RegGroupGeneral2,	SIZE_64,	8);
	add_reg("r8d",	REG_R8D,	RegGroupGeneral2,	SIZE_32,	8);
	add_reg("r9",	REG_R9,	RegGroupGeneral2,	SIZE_64,	9);
	add_reg("r9d",	REG_R9D,	RegGroupGeneral2,	SIZE_32,	9);
	add_reg("r10",	REG_R10,	RegGroupGeneral2,	SIZE_64,	10);
	add_reg("r10d",	REG_R10D,RegGroupGeneral2,	SIZE_32,	10);
	add_reg("r11",	REG_R11,	RegGroupGeneral2,	SIZE_64,	10);
	add_reg("r11d",	REG_R11D,RegGroupGeneral2,	SIZE_32,	11);
	add_reg("r12",	REG_R12,	RegGroupGeneral2,	SIZE_64,	12);
	add_reg("r12d",	REG_R12D,RegGroupGeneral2,	SIZE_32,	12);
	add_reg("r13",	REG_R13,	RegGroupGeneral2,	SIZE_64,	13);
	add_reg("r13d",	REG_R13D,RegGroupGeneral2,	SIZE_32,	13);
	add_reg("r14",	REG_R14,	RegGroupGeneral2,	SIZE_64,	14);
	add_reg("r14d",	REG_R14D,RegGroupGeneral2,	SIZE_32,	14);
	add_reg("r15",	REG_R15,	RegGroupGeneral2,	SIZE_64,	15);
	add_reg("r15d",	REG_R15D,RegGroupGeneral2,	SIZE_32,	15);

	add_reg("cs",	REG_CS,	RegGroupSegment,	SIZE_16);
	add_reg("ss",	REG_SS,	RegGroupSegment,	SIZE_16);
	add_reg("ds",	REG_DS,	RegGroupSegment,	SIZE_16);
	add_reg("es",	REG_ES,	RegGroupSegment,	SIZE_16);
	add_reg("fs",	REG_FS,	RegGroupSegment,	SIZE_16);
	add_reg("gs",	REG_GS,	RegGroupSegment,	SIZE_16);

	add_reg("cr0",	REG_CR0,	RegGroupControl,	SIZE_32);
	add_reg("cr1",	REG_CR1,	RegGroupControl,	SIZE_32);
	add_reg("cr2",	REG_RC2,	RegGroupControl,	SIZE_32);
	add_reg("cr3",	REG_CR3,	RegGroupControl,	SIZE_32);

	add_reg("st0",	REG_ST0,	RegGroupX87,	SIZE_32,	16); // ??? 32
	add_reg("st1",	REG_ST1,	RegGroupX87,	SIZE_32,	17);
	add_reg("st2",	REG_ST2,	RegGroupX87,	SIZE_32,	18);
	add_reg("st3",	REG_ST3,	RegGroupX87,	SIZE_32,	19);
	add_reg("st4",	REG_ST4,	RegGroupX87,	SIZE_32,	20);
	add_reg("st5",	REG_ST5,	RegGroupX87,	SIZE_32,	21);
	add_reg("st6",	REG_ST6,	RegGroupX87,	SIZE_32,	22);
	add_reg("st7",	REG_ST7,	RegGroupX87,	SIZE_32,	23);

	add_reg("xmm0",	REG_XMM0,	RegGroupXmm,	SIZE_128);
	add_reg("xmm1",	REG_XMM1,	RegGroupXmm,	SIZE_128);
	add_reg("xmm2",	REG_XMM2,	RegGroupXmm,	SIZE_128);
	add_reg("xmm3",	REG_XMM3,	RegGroupXmm,	SIZE_128);
	add_reg("xmm4",	REG_XMM4,	RegGroupXmm,	SIZE_128);
	add_reg("xmm5",	REG_XMM5,	RegGroupXmm,	SIZE_128);
	add_reg("xmm6",	REG_XMM6,	RegGroupXmm,	SIZE_128);
	add_reg("xmm7",	REG_XMM7,	RegGroupXmm,	SIZE_128);

	// create easy to access array
	RegisterByID.clear();
	for (int i=0;i<Registers.num;i++){
		if (RegisterByID.num <= Registers[i].id)
			RegisterByID.resize(Registers[i].id + 1);
		RegisterByID[Registers[i].id] = &Registers[i];
	}

	CPUInstructions.clear();
	add_inst(inst_db		,0x00	,0	,-1	,Ib	,-1);
	add_inst(inst_dw		,0x00	,0	,-1	,Iw	,-1);
	add_inst(inst_dd		,0x00	,0	,-1	,Id	,-1);
	add_inst(inst_add		,0x00	,1	,-1	,Eb	,Gb);
	add_inst(inst_add		,0x01	,1	,-1	,Ew	,Gw, OPT_SMALL_PARAM);
	add_inst(inst_add		,0x01	,1	,-1	,Ed	,Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_add		,0x01	,1	,-1	,Eq	,Gq, OPT_BIG_PARAM);
	add_inst(inst_add		,0x02	,1	,-1	,Gb	,Eb);
	add_inst(inst_add		,0x03	,1	,-1	,Gw	,Eq, OPT_SMALL_PARAM);
	add_inst(inst_add		,0x03	,1	,-1	,Gd	,Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_add		,0x03	,1	,-1	,Gq	,Eq, OPT_BIG_PARAM);
	add_inst(inst_add		,0x04	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_add		,0x05	,1	,-1	,REG_AX, Iw, OPT_SMALL_PARAM);
	add_inst(inst_add		,0x05	,1	,-1	,REG_EAX, Id, OPT_MEDIUM_PARAM);
	add_inst(inst_add		,0x05	,1	,-1	,REG_RAX, Id, OPT_BIG_PARAM);
	add_inst(inst_push		,0x06	,1	,-1	,REG_ES	,-1);
	add_inst(inst_pop		,0x07	,1	,-1	,REG_ES	,-1);
	add_inst(inst_or		,0x08	,1	,-1	,Eb	,Gb);
	add_inst(inst_or		,0x09	,1	,-1	,Ew	,Gw, OPT_SMALL_PARAM);
	add_inst(inst_or		,0x09	,1	,-1	,Ed	,Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_or		,0x09	,1	,-1	,Eq	,Gq, OPT_BIG_PARAM);
	add_inst(inst_or		,0x0a	,1	,-1	,Gb	,Eb);
	add_inst(inst_or,	0x0b,	1,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_or,	0x0b,	1,	-1,	Gd,	Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_or,	0x0b,	1,	-1,	Gq,	Eq, OPT_BIG_PARAM);
	add_inst(inst_or		,0x0c	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_or		,0x0d	,1	,-1	,REG_AX,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_or		,0x0d	,1	,-1	,REG_EAX,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_or		,0x0d	,1	,-1	,REG_RAX,	Id, OPT_BIG_PARAM);
	add_inst(inst_push		,0x0e	,1	,-1	,REG_CS	,-1);
	add_inst(inst_sldt		,0x000f	,2	,0	,Ew	,-1);
	add_inst(inst_str		,0x000f	,2	,1	,Ew	,-1);
	add_inst(inst_lldt		,0x000f	,2	,2	,Ew	,-1);
	add_inst(inst_ltr		,0x000f	,2	,3	,Ew	,-1);
	add_inst(inst_verr		,0x000f	,2	,4	,Ew	,-1);
	add_inst(inst_verw		,0x000f	,2	,5	,Ew	,-1);
	add_inst(inst_sgdt,	0x010f,	2,	0,	Mw,	-1, OPT_SMALL_PARAM);
	add_inst(inst_sgdt,	0x010f,	2,	0,	Md,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_sgdt,	0x010f,	2,	0,	Mq,	-1, OPT_BIG_PARAM);
	add_inst(inst_sidt,	0x010f,	2,	1,	Mw,	-1, OPT_SMALL_PARAM);
	add_inst(inst_sidt,	0x010f,	2,	1,	Md,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_sidt,	0x010f,	2,	1,	Mq,	-1, OPT_BIG_PARAM);
	add_inst(inst_lgdt,	0x010f,	2,	2,	Mw,	-1, OPT_SMALL_PARAM);
	add_inst(inst_lgdt,	0x010f,	2,	2,	Md,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_lgdt,	0x010f,	2,	2,	Mq,	-1, OPT_BIG_PARAM);
	add_inst(inst_lidt,	0x010f,	2,	3,	Mw,	-1, OPT_SMALL_PARAM);
	add_inst(inst_lidt,	0x010f,	2,	3,	Md,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_lidt,	0x010f,	2,	3,	Mq,	-1, OPT_BIG_PARAM);
	add_inst(inst_smsw		,0x010f	,2	,4	,Ew	,-1);
	add_inst(inst_lmsw		,0x010f	,2	,6	,Ew	,-1);
	add_inst(inst_mov		,0x200f	,2	,-1	,Rd	,Cd); // Fehler im Algorhytmus!!!!  (wirklich ???) -> Fehler in Tabelle?!?
	add_inst(inst_mov		,0x220f	,2	,-1	,Cd	,Rd);
	add_inst(inst_jo		,0x800f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM); // 16/32 bit???
	add_inst(inst_jno		,0x810f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jb		,0x820f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jnb		,0x830f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jz		,0x840f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jnz		,0x850f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jbe		,0x860f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jnbe		,0x870f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_js		,0x880f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jns		,0x890f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jp		,0x8a0f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jnp		,0x8b0f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jl		,0x8c0f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jnl		,0x8d0f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jle		,0x8e0f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jnle		,0x8f0f	,2	,-1	,Id	,-1, OPT_MEDIUM_PARAM);
	add_inst(inst_seto		,0x900f	,2	,-1	,Eb	,-1);
	add_inst(inst_setno		,0x910f	,2	,-1	,Eb	,-1);
	add_inst(inst_setb		,0x920f	,2	,-1	,Eb	,-1);
	add_inst(inst_setnb		,0x930f	,2	,-1	,Eb	,-1);
	add_inst(inst_setz		,0x940f	,2	,-1	,Eb	,-1);
	add_inst(inst_setnz		,0x950f	,2	,-1	,Eb	,-1);
	add_inst(inst_setbe		,0x960f	,2	,-1	,Eb	,-1);
	add_inst(inst_setnbe	,0x970f	,2	,-1	,Eb	,-1);
	add_inst(inst_sets		,0x980f	,2	,-1	,Eb	,-1); // error in table... "Ev"
	add_inst(inst_setns		,0x990f	,2	,-1	,Eb	,-1);
	add_inst(inst_setp		,0x9a0f	,2	,-1	,Eb	,-1);
	add_inst(inst_setnp		,0x9b0f	,2	,-1	,Eb	,-1);
	add_inst(inst_setl		,0x9c0f	,2	,-1	,Eb	,-1);
	add_inst(inst_setnl		,0x9d0f	,2	,-1	,Eb	,-1);
	add_inst(inst_setle		,0x9e0f	,2	,-1	,Eb	,-1);
	add_inst(inst_setnle	,0x9f0f	,2	,-1	,Eb	,-1);
	add_inst(inst_imul,	0xaf0f,	2,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_imul,	0xaf0f,	2,	-1,	Gd,	Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_imul,	0xaf0f,	2,	-1,	Gq,	Eq, OPT_BIG_PARAM);
	add_inst(inst_movzx,	0xb60f,	2,	-1,	Gw,	Eb, OPT_SMALL_PARAM);
	add_inst(inst_movzx,	0xb60f,	2,	-1,	Gd,	Eb, OPT_MEDIUM_PARAM);
	add_inst(inst_movzx,	0xb60f,	2,	-1,	Gq,	Eb, OPT_BIG_PARAM);
	add_inst(inst_movzx,	0xb70f,	2,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_movzx,	0xb70f,	2,	-1,	Gd,	Ew, OPT_MEDIUM_PARAM);
	add_inst(inst_movzx,	0xb70f,	2,	-1,	Gq,	Ew, OPT_BIG_PARAM);
	add_inst(inst_movsx,	0xbe0f,	2,	-1,	Gw,	Eb, OPT_SMALL_PARAM);
	add_inst(inst_movsx,	0xbe0f,	2,	-1,	Gd,	Eb, OPT_MEDIUM_PARAM);
	add_inst(inst_movsx,	0xbe0f,	2,	-1,	Gq,	Eb, OPT_BIG_PARAM);
	add_inst(inst_movsx,	0xbf0f,	2,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_movsx,	0xbf0f,	2,	-1,	Gd,	Ew, OPT_MEDIUM_PARAM);
	add_inst(inst_movsx,	0xbf0f,	2,	-1,	Gq,	Ew, OPT_BIG_PARAM);
	add_inst(inst_adc,	0x10	,1	,-1	,Eb	,Gb);
	add_inst(inst_adc,	0x11,	1,	-1,	Ew,	Gw, OPT_SMALL_PARAM);
	add_inst(inst_adc,	0x11,	1,	-1,	Ed,	Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_adc,	0x11,	1,	-1,	Eq,	Gq, OPT_BIG_PARAM);
	add_inst(inst_adc,	0x12	,1	,-1	,Gb	,Eb);
	add_inst(inst_adc,	0x13,	1,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_adc,	0x13,	1,	-1,	Gd,	Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_adc,	0x13,	1,	-1,	Gq,	Eq, OPT_BIG_PARAM);
	add_inst(inst_adc,	0x14	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_adc,	0x15	,1	,-1	,REG_AX,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_adc,	0x15	,1	,-1	,REG_EAX, Id, OPT_MEDIUM_PARAM);
	add_inst(inst_adc,	0x15	,1	,-1	,REG_RAX, Id, OPT_BIG_PARAM);
	add_inst(inst_push,	0x16	,1	,-1	,REG_SS, -1);
	add_inst(inst_pop,	0x17	,1	,-1	,REG_SS, -1);
	add_inst(inst_sbb,	0x18	,1	,-1	,Eb	,Gb);
	add_inst(inst_sbb,	0x19,	1,	-1,	Ew,	Gw, OPT_SMALL_PARAM);
	add_inst(inst_sbb,	0x19,	1,	-1,	Ed,	Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_sbb,	0x19,	1,	-1,	Eq,	Gq, OPT_BIG_PARAM);
	add_inst(inst_sbb,	0x1a	,1	,-1	,Gb	,Eb);
	add_inst(inst_sbb,	0x1b,	1,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_sbb,	0x1b,	1,	-1,	Gd,	Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_sbb,	0x1b,	1,	-1,	Gq,	Eq, OPT_BIG_PARAM);
	add_inst(inst_sbb,	0x1c	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_sbb,	0x1d	,1	,-1	,REG_AX	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_sbb,	0x1d	,1	,-1	,REG_EAX	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_sbb,	0x1d	,1	,-1	,REG_RAX	,Id, OPT_BIG_PARAM);
	add_inst(inst_push,	0x1e	,1	,-1	,REG_DS	,-1);
	add_inst(inst_pop,	0x1f	,1	,-1	,REG_DS	,-1);
	add_inst(inst_and,	0x20	,1	,-1	,Eb	,Gb);
	add_inst(inst_and,	0x21,	1,	-1,	Ew,	Gw, OPT_SMALL_PARAM);
	add_inst(inst_and,	0x21,	1,	-1,	Ed,	Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_and,	0x21,	1,	-1,	Eq,	Gq, OPT_BIG_PARAM);
	add_inst(inst_and,	0x22	,1	,-1	,Gb	,Eb);
	add_inst(inst_and,	0x23,	1,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_and,	0x23,	1,	-1,	Gd,	Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_and,	0x23,	1,	-1,	Gq,	Eq, OPT_BIG_PARAM);
	add_inst(inst_and,	0x24	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_and,	0x25	,1	,-1	,REG_AX	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_and,	0x25	,1	,-1	,REG_EAX	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_and,	0x25	,1	,-1	,REG_RAX	,Id, OPT_BIG_PARAM);
	add_inst(inst_sub,	0x28	,1	,-1	,Eb	,Gb);
	add_inst(inst_sub,	0x29,	1,	-1,	Ew,	Gw, OPT_SMALL_PARAM);
	add_inst(inst_sub,	0x29,	1,	-1,	Ed,	Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_sub,	0x29,	1,	-1,	Eq,	Gq, OPT_BIG_PARAM);
	add_inst(inst_sub,	0x2a	,1	,-1	,Gb	,Eb);
	add_inst(inst_sub,	0x2b,	1,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_sub,	0x2b,	1,	-1,	Gd,	Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_sub,	0x2b,	1,	-1,	Gq,	Eq, OPT_BIG_PARAM);
	add_inst(inst_sub,	0x2c	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_sub,	0x2d	,1	,-1	,REG_AX	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_sub,	0x2d	,1	,-1	,REG_EAX	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_sub,	0x2d	,1	,-1	,REG_RAX	,Id, OPT_BIG_PARAM);
	add_inst(inst_xor,	0x30	,1	,-1	,Eb	,Gb);
	add_inst(inst_xor,	0x31,	1,	-1,	Ew,	Gw, OPT_SMALL_PARAM);
	add_inst(inst_xor,	0x31,	1,	-1,	Ed,	Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_xor,	0x31,	1,	-1,	Eq,	Gq, OPT_BIG_PARAM);
	add_inst(inst_xor,	0x32	,1	,-1	,Gb	,Eb);
	add_inst(inst_xor,	0x33,	1,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_xor,	0x33,	1,	-1,	Gd,	Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_xor,	0x33,	1,	-1,	Gq,	Eq, OPT_BIG_PARAM);
	add_inst(inst_xor,	0x34	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_xor,	0x35	,1	,-1	,REG_AX	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_xor,	0x35	,1	,-1	,REG_EAX	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_xor,	0x35	,1	,-1	,REG_RAX	,Iq, OPT_BIG_PARAM);
	add_inst(inst_cmp,	0x38	,1	,-1	,Eb	,Gb);
	add_inst(inst_cmp,	0x39,	1,	-1,	Ew,	Gw, OPT_SMALL_PARAM);
	add_inst(inst_cmp,	0x39,	1,	-1,	Ed,	Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_cmp,	0x39,	1,	-1,	Eq,	Gq, OPT_BIG_PARAM);
	add_inst(inst_cmp,	0x3a	,1	,-1	,Gb	,Eb);
	add_inst(inst_cmp,	0x3b,	1,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_cmp,	0x3b,	1,	-1,	Gd,	Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_cmp,	0x3b,	1,	-1,	Gq,	Eq, OPT_BIG_PARAM);
	add_inst(inst_cmp,	0x3c	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_cmp,	0x3d	,1	,-1	,REG_AX	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_cmp,	0x3d	,1	,-1	,REG_EAX	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_cmp,	0x3d	,1	,-1	,REG_RAX	,Id, OPT_BIG_PARAM);
	if (set == INSTRUCTION_SET_X86){
		add_inst(inst_inc		,0x40	,1	,-1	,REG_EAX	,-1);
		add_inst(inst_inc		,0x41	,1	,-1	,REG_ECX	,-1);
		add_inst(inst_inc		,0x42	,1	,-1	,REG_EDX	,-1);
		add_inst(inst_inc		,0x43	,1	,-1	,REG_EBX	,-1);
		add_inst(inst_inc		,0x44	,1	,-1	,REG_ESP	,-1);
		add_inst(inst_inc		,0x45	,1	,-1	,REG_EBP	,-1);
		add_inst(inst_inc		,0x46	,1	,-1	,REG_ESI	,-1);
		add_inst(inst_inc		,0x47	,1	,-1	,REG_EDI	,-1);
		add_inst(inst_dec		,0x48	,1	,-1	,REG_EAX	,-1);
		add_inst(inst_dec		,0x49	,1	,-1	,REG_ECX	,-1);
		add_inst(inst_dec		,0x4a	,1	,-1	,REG_EDX	,-1);
		add_inst(inst_dec		,0x4b	,1	,-1	,REG_EBX	,-1);
		add_inst(inst_dec		,0x4c	,1	,-1	,REG_ESP	,-1);
		add_inst(inst_dec		,0x4d	,1	,-1	,REG_EBP	,-1);
		add_inst(inst_dec		,0x4e	,1	,-1	,REG_ESI	,-1);
		add_inst(inst_dec		,0x4f	,1	,-1	,REG_EDI	,-1);
	}
	if (set == INSTRUCTION_SET_X86){
		add_inst(inst_push		,0x50	,1	,-1	,REG_EAX	,-1);
		add_inst(inst_push		,0x51	,1	,-1	,REG_ECX	,-1);
		add_inst(inst_push		,0x52	,1	,-1	,REG_EDX	,-1);
		add_inst(inst_push		,0x53	,1	,-1	,REG_EBX	,-1);
		add_inst(inst_push		,0x54	,1	,-1	,REG_ESP	,-1);
		add_inst(inst_push		,0x55	,1	,-1	,REG_EBP	,-1);
		add_inst(inst_push		,0x56	,1	,-1	,REG_ESI	,-1);
		add_inst(inst_push		,0x57	,1	,-1	,REG_EDI	,-1);
		add_inst(inst_pop		,0x58	,1	,-1	,REG_EAX	,-1);
		add_inst(inst_pop		,0x59	,1	,-1	,REG_ECX	,-1);
		add_inst(inst_pop		,0x5a	,1	,-1	,REG_EDX	,-1);
		add_inst(inst_pop		,0x5b	,1	,-1	,REG_EBX	,-1);
		add_inst(inst_pop		,0x5c	,1	,-1	,REG_ESP	,-1);
		add_inst(inst_pop		,0x5d	,1	,-1	,REG_EBP	,-1);
		add_inst(inst_pop		,0x5e	,1	,-1	,REG_ESI	,-1);
		add_inst(inst_pop		,0x5f	,1	,-1	,REG_EDI	,-1);
	}else if (set == INSTRUCTION_SET_AMD64){
		add_inst(inst_push		,0x50	,1	,-1	,REG_RAX	,-1);
		add_inst(inst_push		,0x51	,1	,-1	,REG_RCX	,-1);
		add_inst(inst_push		,0x52	,1	,-1	,REG_RDX	,-1);
		add_inst(inst_push		,0x53	,1	,-1	,REG_RBX	,-1);
		add_inst(inst_push		,0x54	,1	,-1	,REG_RSP	,-1);
		add_inst(inst_push		,0x55	,1	,-1	,REG_RBP	,-1);
		add_inst(inst_push		,0x56	,1	,-1	,REG_RSI	,-1);
		add_inst(inst_push		,0x57	,1	,-1	,REG_RDI	,-1);
		add_inst(inst_pop		,0x58	,1	,-1	,REG_RAX	,-1);
		add_inst(inst_pop		,0x59	,1	,-1	,REG_RCX	,-1);
		add_inst(inst_pop		,0x5a	,1	,-1	,REG_RDX	,-1);
		add_inst(inst_pop		,0x5b	,1	,-1	,REG_RBX	,-1);
		add_inst(inst_pop		,0x5c	,1	,-1	,REG_RSP	,-1);
		add_inst(inst_pop		,0x5d	,1	,-1	,REG_RBP	,-1);
		add_inst(inst_pop		,0x5e	,1	,-1	,REG_RSI	,-1);
		add_inst(inst_pop		,0x5f	,1	,-1	,REG_RDI	,-1);
	}
	add_inst(inst_pusha		,0x60	,1	,-1	,-1	,-1);
	add_inst(inst_popa		,0x61	,1	,-1	,-1	,-1);
	add_inst(inst_push,	0x68,	1,	-1,	Iw,	-1, OPT_SMALL_PARAM);
	add_inst(inst_push,	0x68,	1,	-1,	Id,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_push,	0x68,	1,	-1,	Iq,	-1, OPT_BIG_PARAM);
	add_inst(inst_imul,	0x69,	1,	-1,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_imul,	0x69,	1,	-1,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_imul,	0x69,	1,	-1,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_push		,0x6a	,1	,-1	,Ib	,-1);
	add_inst(inst_jo		,0x70	,1	,-1	,Jb	,-1);
	add_inst(inst_jno		,0x71	,1	,-1	,Jb	,-1);
	add_inst(inst_jb		,0x72	,1	,-1	,Jb	,-1);
	add_inst(inst_jnb		,0x73	,1	,-1	,Jb	,-1);
	add_inst(inst_jz		,0x74	,1	,-1	,Jb	,-1);
	add_inst(inst_jnz		,0x75	,1	,-1	,Jb	,-1);
	add_inst(inst_jbe		,0x76	,1	,-1	,Jb	,-1);
	add_inst(inst_jnbe		,0x77	,1	,-1	,Jb	,-1);
	add_inst(inst_js		,0x78	,1	,-1	,Jb	,-1);
	add_inst(inst_jns		,0x79	,1	,-1	,Jb	,-1);
	add_inst(inst_jp		,0x7a	,1	,-1	,Jb	,-1);
	add_inst(inst_jnp		,0x7b	,1	,-1	,Jb	,-1);
	add_inst(inst_jl		,0x7c	,1	,-1	,Jb	,-1);
	add_inst(inst_jnl		,0x7d	,1	,-1	,Jb	,-1);
	add_inst(inst_jle		,0x7e	,1	,-1	,Jb	,-1);
	add_inst(inst_jnle		,0x7f	,1	,-1	,Jb	,-1);
	// Immediate Group 1
	add_inst(inst_add		,0x80	,1	,0	,Eb	,Ib);
	add_inst(inst_or		,0x80	,1	,1	,Eb	,Ib);
	add_inst(inst_adc		,0x80	,1	,2	,Eb	,Ib);
	add_inst(inst_sbb		,0x80	,1	,3	,Eb	,Ib);
	add_inst(inst_and		,0x80	,1	,4	,Eb	,Ib);
	add_inst(inst_sub		,0x80	,1	,5	,Eb	,Ib);
	add_inst(inst_xor		,0x80	,1	,6	,Eb	,Ib);
	add_inst(inst_cmp		,0x80	,1	,7	,Eb	,Ib);
	add_inst(inst_add,	0x81,	1,	0,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_add,	0x81,	1,	0,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_add,	0x81,	1,	0,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_or,	0x81,	1,	1,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_or,	0x81,	1,	1,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_or,	0x81,	1,	1,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_adc,	0x81,	1,	2,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_adc,	0x81,	1,	2,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_adc,	0x81,	1,	2,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_sbb,	0x81,	1,	3,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_sbb,	0x81,	1,	3,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_sbb,	0x81,	1,	3,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_and,	0x81,	1,	4,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_and,	0x81,	1,	4,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_and,	0x81,	1,	4,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_sub,	0x81,	1,	5,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_sub,	0x81,	1,	5,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_sub,	0x81,	1,	5,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_xor,	0x81,	1,	6,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_xor,	0x81,	1,	6,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_xor,	0x81,	1,	6,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_cmp,	0x81,	1,	7,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_cmp,	0x81,	1,	7,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_cmp,	0x81,	1,	7,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_add,	0x83,	1,	0,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_add,	0x83,	1,	0,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_add,	0x83,	1,	0,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_or,	0x83,	1,	1,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_or,	0x83,	1,	1,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_or,	0x83,	1,	1,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_adc,	0x83,	1,	2,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_adc,	0x83,	1,	2,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_adc,	0x83,	1,	2,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_sbb,	0x83,	1,	3,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_sbb,	0x83,	1,	3,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_sbb,	0x83,	1,	3,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_and,	0x83,	1,	4,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_and,	0x83,	1,	4,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_and,	0x83,	1,	4,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_sub,	0x83,	1,	5,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_sub,	0x83,	1,	5,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_sub,	0x83,	1,	5,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_xor,	0x83,	1,	6,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_xor,	0x83,	1,	6,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_xor,	0x83,	1,	6,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_cmp,	0x83,	1,	7,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_cmp,	0x83,	1,	7,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_cmp,	0x83,	1,	7,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_test,	0x84	,1	,-1	,Eb	,Gb);
	add_inst(inst_test,	0x85,	1,	-1,	Ew,	Gw, OPT_SMALL_PARAM);
	add_inst(inst_test,	0x85,	1,	-1,	Ed,	Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_test,	0x85,	1,	-1,	Eq,	Gq, OPT_BIG_PARAM);
	add_inst(inst_xchg,	0x86	,1	,-1	,Eb	,Gb);
	add_inst(inst_xchg,	0x87,	1,	-1,	Ew,	Gw, OPT_SMALL_PARAM);
	add_inst(inst_xchg,	0x87,	1,	-1,	Ed,	Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_xchg,	0x87,	1,	-1,	Eq,	Gq, OPT_BIG_PARAM);
	add_inst(inst_mov,	0x88	,1	,-1	,Eb	,Gb);
	add_inst(inst_mov,	0x89,	1,	-1,	Ew,	Gw, OPT_SMALL_PARAM);
	add_inst(inst_mov,	0x89,	1,	-1,	Ed,	Gd, OPT_MEDIUM_PARAM);
	add_inst(inst_mov,	0x89,	1,	-1,	Eq,	Gq, OPT_BIG_PARAM);
	add_inst(inst_mov,	0x8a	,1	,-1	,Gb	,Eb);
	add_inst(inst_mov,	0x8b,	1,	-1,	Gw,	Ew, OPT_SMALL_PARAM);
	add_inst(inst_mov,	0x8b,	1,	-1,	Gd,	Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_mov,	0x8b,	1,	-1,	Gq,	Eq, OPT_BIG_PARAM);
	add_inst(inst_mov,	0x8c	,1	,-1	,Ew	,Sw	);
	//add_inst(inst_lea,	0x8d,	1,	-1,	Gw,	Ew, OptSmallParam);
	//add_inst(inst_lea,	0x8d,	1,	-1,	Gd,	Ed, OptMediumParam);
	//add_inst(inst_lea,	0x8d,	1,	-1,	Gq,	Eq, OptBigParam);
	add_inst(inst_lea,	0x8d,	1,	-1,	Gw,	Mw, OPT_SMALL_PARAM);
	add_inst(inst_lea,	0x8d,	1,	-1,	Gd,	Md, OPT_MEDIUM_PARAM);
	add_inst(inst_lea,	0x8d,	1,	-1,	Gq,	Mq, OPT_BIG_PARAM);
	add_inst(inst_mov,	0x8e	,1	,-1	,Sw	,Ew, OPT_MEDIUM_PARAM);
	add_inst(inst_pop,	0x8f,	1,	-1,	Ew,	-1, OPT_SMALL_PARAM);
	add_inst(inst_pop,	0x8f,	1,	-1,	Ed,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_pop,	0x8f,	1,	-1,	Eq,	-1, OPT_BIG_PARAM);
	add_inst(inst_nop		,0x90	,1	,-1	,-1	,-1);
	add_inst(inst_xchg		,0x91	,1	,-1	,REG_AX	,REG_CX, OPT_SMALL_PARAM);
	add_inst(inst_xchg		,0x92	,1	,-1	,REG_AX	,REG_DX, OPT_SMALL_PARAM);
	add_inst(inst_xchg		,0x93	,1	,-1	,REG_AX	,REG_BX, OPT_SMALL_PARAM);
	add_inst(inst_xchg		,0x94	,1	,-1	,REG_AX	,REG_SP, OPT_SMALL_PARAM);
	add_inst(inst_xchg		,0x95	,1	,-1	,REG_AX	,REG_BP, OPT_SMALL_PARAM);
	add_inst(inst_xchg		,0x96	,1	,-1	,REG_AX	,REG_SI, OPT_SMALL_PARAM);
	add_inst(inst_xchg		,0x97	,1	,-1	,REG_AX	,REG_DI, OPT_SMALL_PARAM);
	add_inst(inst_xchg		,0x91	,1	,-1	,REG_EAX	,REG_ECX, OPT_MEDIUM_PARAM);
	add_inst(inst_xchg		,0x92	,1	,-1	,REG_EAX	,REG_EDX, OPT_MEDIUM_PARAM);
	add_inst(inst_xchg		,0x93	,1	,-1	,REG_EAX	,REG_EBX, OPT_MEDIUM_PARAM);
	add_inst(inst_xchg		,0x94	,1	,-1	,REG_EAX	,REG_ESP, OPT_MEDIUM_PARAM);
	add_inst(inst_xchg		,0x95	,1	,-1	,REG_EAX	,REG_EBP, OPT_MEDIUM_PARAM);
	add_inst(inst_xchg		,0x96	,1	,-1	,REG_EAX	,REG_ESI, OPT_MEDIUM_PARAM);
	add_inst(inst_xchg		,0x97	,1	,-1	,REG_EAX	,REG_EDI, OPT_MEDIUM_PARAM);
	add_inst(inst_xchg		,0x91	,1	,-1	,REG_RAX	,REG_RCX, OPT_BIG_PARAM);
	add_inst(inst_xchg		,0x92	,1	,-1	,REG_RAX	,REG_RDX, OPT_BIG_PARAM);
	add_inst(inst_xchg		,0x93	,1	,-1	,REG_RAX	,REG_RBX, OPT_BIG_PARAM);
	add_inst(inst_xchg		,0x94	,1	,-1	,REG_RAX	,REG_RSP, OPT_BIG_PARAM);
	add_inst(inst_xchg		,0x95	,1	,-1	,REG_RAX	,REG_RBP, OPT_BIG_PARAM);
	add_inst(inst_xchg		,0x96	,1	,-1	,REG_RAX	,REG_RSI, OPT_BIG_PARAM);
	add_inst(inst_xchg		,0x97	,1	,-1	,REG_RAX	,REG_RDI, OPT_BIG_PARAM);
	add_inst(inst_cbw_cwde	,0x98	,1	,-1	,-1 ,-1);
	add_inst(inst_cgq_cwd	,0x99	,1	,-1	,-1 ,-1);
	add_inst(inst_mov		,0xa0	,1	,-1	,REG_AL	,Ob, 0, true);
	add_inst(inst_mov		,0xa1	,1	,-1	,REG_AX	,Ow, OPT_SMALL_PARAM, true);
	add_inst(inst_mov		,0xa1	,1	,-1	,REG_EAX	,Od, OPT_MEDIUM_PARAM, true);
	add_inst(inst_mov		,0xa1	,1	,-1	,REG_RAX	,Oq, OPT_BIG_PARAM, true);
	add_inst(inst_mov		,0xa2	,1	,-1	,Ob	,REG_AL, 0, true);
	add_inst(inst_mov,	0xa3,	1,	-1,	Ow,	REG_AX, OPT_SMALL_PARAM, true);
	add_inst(inst_mov,	0xa3,	1,	-1,	Od,	REG_EAX, OPT_MEDIUM_PARAM, true);
	add_inst(inst_mov,	0xa3,	1,	-1,	Oq,	REG_RAX, OPT_BIG_PARAM, true);
	add_inst(inst_movs_b_ds_esi_es_edi	,0xa4	,1	,-1	,-1,-1);
	add_inst(inst_movs_ds_esi_es_edi	,0xa5	,1	,-1	,-1,-1);
	add_inst(inst_cmps_b_ds_esi_es_edi	,0xa6	,1	,-1	,-1,-1);
	add_inst(inst_cmps_ds_esi_es_edi	,0xa7	,1	,-1	,-1,-1);
	add_inst(inst_mov		,0xb0	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_mov		,0xb1	,1	,-1	,REG_CL	,Ib);
	add_inst(inst_mov		,0xb2	,1	,-1	,REG_DL	,Ib);
	add_inst(inst_mov		,0xb3	,1	,-1	,REG_BL	,Ib);
	add_inst(inst_mov		,0xb4	,1	,-1	,REG_AH	,Ib);
	add_inst(inst_mov		,0xb5	,1	,-1	,REG_CH	,Ib);
	add_inst(inst_mov		,0xb6	,1	,-1	,REG_DH	,Ib);
	add_inst(inst_mov		,0xb7	,1	,-1	,REG_BH	,Ib);
	add_inst(inst_mov		,0xb8	,1	,-1	,REG_EAX	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_mov		,0xb9	,1	,-1	,REG_ECX	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_mov		,0xba	,1	,-1	,REG_EDX	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_mov		,0xbb	,1	,-1	,REG_EBX	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_mov		,0xbc	,1	,-1	,REG_ESP	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_mov		,0xbd	,1	,-1	,REG_EBP	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_mov		,0xbe	,1	,-1	,REG_ESI	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_mov		,0xbf	,1	,-1	,REG_EDI	,Id, OPT_MEDIUM_PARAM);
	add_inst(inst_mov		,0xb8	,1	,-1	,REG_AX	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_mov		,0xb9	,1	,-1	,REG_CX	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_mov		,0xba	,1	,-1	,REG_DX	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_mov		,0xbb	,1	,-1	,REG_BX	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_mov		,0xbc	,1	,-1	,REG_SP	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_mov		,0xbd	,1	,-1	,REG_BP	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_mov		,0xbe	,1	,-1	,REG_SI	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_mov		,0xbf	,1	,-1	,REG_DI	,Iw, OPT_SMALL_PARAM);
	add_inst(inst_mov		,0xb8	,1	,-1	,REG_RAX	,Iq, OPT_BIG_PARAM);
	add_inst(inst_mov		,0xb9	,1	,-1	,REG_RCX	,Iq, OPT_BIG_PARAM);
	add_inst(inst_mov		,0xba	,1	,-1	,REG_RDX	,Iq, OPT_BIG_PARAM);
	add_inst(inst_mov		,0xbb	,1	,-1	,REG_RBX	,Iq, OPT_BIG_PARAM);
	add_inst(inst_mov		,0xbc	,1	,-1	,REG_RSP	,Iq, OPT_BIG_PARAM);
	add_inst(inst_mov		,0xbd	,1	,-1	,REG_RBP	,Iq, OPT_BIG_PARAM);
	add_inst(inst_mov		,0xbe	,1	,-1	,REG_RSI	,Iq, OPT_BIG_PARAM);
	add_inst(inst_mov		,0xbf	,1	,-1	,REG_RDI	,Iq, OPT_BIG_PARAM);
	// Shift Group 2
	add_inst(inst_rol		,0xc0	,1	,0	,Eb	,Ib);
	add_inst(inst_ror		,0xc0	,1	,1	,Eb	,Ib);
	add_inst(inst_rcl		,0xc0	,1	,2	,Eb	,Ib);
	add_inst(inst_rcr		,0xc0	,1	,3	,Eb	,Ib);
	add_inst(inst_shl		,0xc0	,1	,4	,Eb	,Ib);
	add_inst(inst_shr		,0xc0	,1	,5	,Eb	,Ib);
	add_inst(inst_sar		,0xc0	,1	,7	,Eb	,Ib);
	add_inst(inst_rol,	0xc1,	1,	0,	Ew,	Ib, OPT_SMALL_PARAM); // even though the table says Iv
	add_inst(inst_rol,	0xc1,	1,	0,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_rol,	0xc1,	1,	0,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_ror,	0xc1,	1,	1,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_ror,	0xc1,	1,	1,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_ror,	0xc1,	1,	1,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_rcl,	0xc1,	1,	2,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_rcl,	0xc1,	1,	2,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_rcl,	0xc1,	1,	2,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_rcr,	0xc1,	1,	3,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_rcr,	0xc1,	1,	3,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_rcr,	0xc1,	1,	3,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_shl,	0xc1,	1,	4,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_shl,	0xc1,	1,	4,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_shl,	0xc1,	1,	4,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_shr,	0xc1,	1,	5,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_shr,	0xc1,	1,	5,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_shr,	0xc1,	1,	5,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_sar,	0xc1,	1,	7,	Ew,	Ib, OPT_SMALL_PARAM);
	add_inst(inst_sar,	0xc1,	1,	7,	Ed,	Ib, OPT_MEDIUM_PARAM);
	add_inst(inst_sar,	0xc1,	1,	7,	Eq,	Ib, OPT_BIG_PARAM);
	add_inst(inst_ret		,0xc2	,1	,-1	,Iw	,-1);
	add_inst(inst_ret		,0xc3	,1	,-1	,-1	,-1);
	add_inst(inst_mov		,0xc6	,1	,-1	,Eb	,Ib);
	add_inst(inst_mov,	0xc7,	1,	-1,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_mov,	0xc7,	1,	-1,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_mov,	0xc7,	1,	-1,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_leave		,0xc9	,1	,-1	,-1	,-1);
	add_inst(inst_ret_far	,0xca	,1	,-1	,Iw	,-1);
	add_inst(inst_ret_far	,0xcb	,1	,-1	,-1	,-1);
	add_inst(inst_int		,0xcd	,1	,-1	,Ib	,-1);
	add_inst(inst_iret		,0xcf	,1	,-1	,-1	,-1);
	add_inst(inst_rol,	0xd3,	1,	0,	Ew,	REG_CL, OPT_SMALL_PARAM);
	add_inst(inst_rol,	0xd3,	1,	0,	Ed,	REG_CL, OPT_MEDIUM_PARAM);
	add_inst(inst_rol,	0xd3,	1,	0,	Eq,	REG_CL, OPT_BIG_PARAM);
	add_inst(inst_ror,	0xd3,	1,	1,	Ew,	REG_CL, OPT_SMALL_PARAM);
	add_inst(inst_ror,	0xd3,	1,	1,	Ed,	REG_CL, OPT_MEDIUM_PARAM);
	add_inst(inst_ror,	0xd3,	1,	1,	Eq,	REG_CL, OPT_BIG_PARAM);
	add_inst(inst_rcl,	0xd3,	1,	2,	Ew,	REG_CL, OPT_SMALL_PARAM);
	add_inst(inst_rcl,	0xd3,	1,	2,	Ed,	REG_CL, OPT_MEDIUM_PARAM);
	add_inst(inst_rcl,	0xd3,	1,	2,	Eq,	REG_CL, OPT_BIG_PARAM);
	add_inst(inst_rcr,	0xd3,	1,	3,	Ew,	REG_CL, OPT_SMALL_PARAM);
	add_inst(inst_rcr,	0xd3,	1,	3,	Ed,	REG_CL, OPT_MEDIUM_PARAM);
	add_inst(inst_rcr,	0xd3,	1,	3,	Eq,	REG_CL, OPT_BIG_PARAM);
	add_inst(inst_shl,	0xd3,	1,	4,	Ew,	REG_CL, OPT_SMALL_PARAM);
	add_inst(inst_shl,	0xd3,	1,	4,	Ed,	REG_CL, OPT_MEDIUM_PARAM);
	add_inst(inst_shl,	0xd3,	1,	4,	Eq,	REG_CL, OPT_BIG_PARAM);
	add_inst(inst_shr,	0xd3,	1,	5,	Ew,	REG_CL, OPT_SMALL_PARAM);
	add_inst(inst_shr,	0xd3,	1,	5,	Ed,	REG_CL, OPT_MEDIUM_PARAM);
	add_inst(inst_shr,	0xd3,	1,	5,	Eq,	REG_CL, OPT_BIG_PARAM);
	add_inst(inst_sar,	0xd3,	1,	7,	Ew,	REG_CL, OPT_SMALL_PARAM);
	add_inst(inst_sar,	0xd3,	1,	7,	Ed,	REG_CL, OPT_MEDIUM_PARAM);
	add_inst(inst_sar,	0xd3,	1,	7,	Eq,	REG_CL, OPT_BIG_PARAM);
	add_inst(inst_fadd,	0xd8,	1,	0,	Ed,	-1);
	add_inst(inst_fadd,	0xdc,	1,	0,	Eq,	-1);
	add_inst(inst_fmul,	0xd8,	1,	1,	Ed,	-1);
	add_inst(inst_fmul,	0xdc,	1,	1,	Eq,	-1);
	add_inst(inst_fsub,	0xd8,	1,	4,	Ed,	-1);
	add_inst(inst_fsub,	0xdc,	1,	4,	Eq,	-1);
	add_inst(inst_fdiv,	0xd8,	1,	6,	Ed,	-1);
	add_inst(inst_fdiv,	0xdc,	1,	6,	Eq,	-1);
	add_inst(inst_fld,	0xd9,	1,	0,	Md,	-1);
	add_inst(inst_fld,	0xdd,	1,	0,	Mq,	-1);
	add_inst(inst_fld1,	0xe8d9,	2,	-1,	-1,	-1);
	add_inst(inst_fldz,	0xeed9,	2,	-1,	-1,	-1);
	add_inst(inst_fldpi,	0xebd9,	2,	-1,	-1,	-1);
	add_inst(inst_fst,	0xd9,	1,	2,	Md,	-1);
	add_inst(inst_fst,	0xdd,	1,	2,	Mq,	-1);
	add_inst(inst_fstp,	0xd9,	1,	3,	Md,	-1);
	add_inst(inst_fstp,	0xdd,	1,	3,	Mq,	-1);
	add_inst(inst_fldcw,	0xd9,	1,	5,	Mw,	-1);
	add_inst(inst_fnstcw,	0xd9,	1,	7,	Mw,	-1);
	add_inst(inst_fxch		,0xc9d9	,2	,-1	,REG_ST0	,REG_ST1);
	add_inst(inst_fucompp	,0xe9da	,2	,-1	,REG_ST0	,REG_ST1);

	add_inst(inst_fsqrt,	0xfad9,	2,	-1,	-1, -1);
	add_inst(inst_fsin,	0xfed9,	2,	-1,	-1, -1);
	add_inst(inst_fcos,	0xffd9,	2,	-1,	-1, -1);
	add_inst(inst_fptan,	0xf2d9,	2,	-1,	-1, -1);
	add_inst(inst_fpatan,	0xf3d9,	2,	-1,	-1, -1);
	add_inst(inst_fyl2x,	0xf1d9,	2,	-1,	-1, -1);
	add_inst(inst_fistp,	0xdb	,1	,3	,Md	,-1);
	add_inst(inst_fild,	0xdb,	1,	0,	Ed,	-1);
	add_inst(inst_faddp,	0xde,	1,	0,	Ed,	-1);
	add_inst(inst_fmulp,	0xde,	1,	1,	Ed,	-1);
	add_inst(inst_fsubp,	0xde,	1,	5,	Ed,	-1);
	add_inst(inst_fdivp,	0xde,	1,	7,	Ed,	-1); // de.f9 ohne Parameter...?
	add_inst(inst_fnstsw	,0xe0df	,2	,-1	,REG_AX	,-1);
	add_inst(inst_loopne	,0xe0	,1	,-1	,Jb	,-1);
	add_inst(inst_loope		,0xe1	,1	,-1	,Jb	,-1);
	add_inst(inst_loop		,0xe2	,1	,-1	,Jb	,-1);
	add_inst(inst_in		,0xe4	,1	,-1	,REG_AL	,Ib);
	add_inst(inst_in		,0xe5	,1	,-1	,REG_EAX,Ib);
	add_inst(inst_out		,0xe6	,1	,-1	,Ib	,REG_AL);
	add_inst(inst_out		,0xe7	,1	,-1	,Ib	,REG_EAX);
	add_inst(inst_call,	0xe8,	1,	-1,	Jw,	-1, OPT_SMALL_PARAM); // well... "Av" in tyble
	add_inst(inst_call,	0xe8,	1,	-1,	Jd,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_call,	0xe8,	1,	-1,	Jq,	-1, OPT_BIG_PARAM);
	add_inst(inst_jmp,	0xe9,	1,	-1,	Jw,	-1, OPT_SMALL_PARAM); // miswritten in the table
	add_inst(inst_jmp,	0xe9,	1,	-1,	Jd,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_jmp,	0xe9,	1,	-1,	Jq,	-1, OPT_BIG_PARAM);
//	add_inst(inst_jmp		,0xea	,1	,-1, Ap, -1); TODO
	add_inst(inst_jmp_far, 0xea, 1, -1, Id, -1, OPT_SMALL_PARAM);
	add_inst(inst_jmp_far, 0xea, 1, -1, I48, -1, OPT_MEDIUM_PARAM);
	add_inst(inst_jmp		,0xeb	,1	,-1, Jb, -1);
	add_inst(inst_in		,0xec	,1	,-1, REG_AL, REG_DX);
	add_inst(inst_in		,0xed	,1	,-1, REG_EAX, REG_DX);
	add_inst(inst_out		,0xee	,1	,-1, REG_DX, REG_AL);
	add_inst(inst_out		,0xef	,1	,-1, REG_DX, REG_EAX);
	add_inst(inst_lock		,0xf0	,1	,-1	,-1	,-1);
	/*add_inst(inst_repne		,0xf2	,1	,-1	,-1	,-1);
	add_inst(inst_rep		,0xf3	,1	,-1	,-1	,-1);*/
	add_inst(inst_hlt		,0xf4	,1	,-1	,-1	,-1);
	add_inst(inst_cmc		,0xf5	,1	,-1	,-1	,-1);
	// Unary Group 3
	add_inst(inst_test		,0xf6	,1	,0	,Eb	,Ib);
	add_inst(inst_not		,0xf6	,1	,2	,Eb	,-1);
	add_inst(inst_neg		,0xf6	,1	,3	,Eb	,-1);
	add_inst(inst_mul		,0xf6	,1	,4	,REG_AL	,Eb);
	add_inst(inst_imul		,0xf6	,1	,5	,REG_AL	,Eb);
	add_inst(inst_div		,0xf6	,1	,6	,REG_AL	,Eb);
	add_inst(inst_idiv		,0xf6	,1	,7	,Eb	,-1);
	add_inst(inst_test,	0xf7,	1,	0,	Ew,	Iw, OPT_SMALL_PARAM);
	add_inst(inst_test,	0xf7,	1,	0,	Ed,	Id, OPT_MEDIUM_PARAM);
	add_inst(inst_test,	0xf7,	1,	0,	Eq,	Id, OPT_BIG_PARAM);
	add_inst(inst_not,	0xf7,	1,	2,	Ew,	-1, OPT_SMALL_PARAM);
	add_inst(inst_not,	0xf7,	1,	2,	Ed,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_not,	0xf7,	1,	2,	Eq,	-1, OPT_BIG_PARAM);
	add_inst(inst_neg,	0xf7,	1,	3,	Ew,	-1, OPT_SMALL_PARAM);
	add_inst(inst_neg,	0xf7,	1,	3,	Ed,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_neg,	0xf7,	1,	3,	Eq,	-1, OPT_BIG_PARAM);
	add_inst(inst_mul		,0xf7	,1	,4	,REG_EAX	,Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_imul		,0xf7	,1	,5	,REG_EAX	,Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_div		,0xf7	,1	,6	,REG_EAX	,Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_idiv		,0xf7	,1	,7	,REG_EAX	,Ed, OPT_MEDIUM_PARAM);
	add_inst(inst_mul		,0xf7	,1	,4	,REG_AX	,Ed, OPT_SMALL_PARAM);
	add_inst(inst_imul		,0xf7	,1	,5	,REG_AX	,Ed, OPT_SMALL_PARAM);
	add_inst(inst_div		,0xf7	,1	,6	,REG_AX	,Ed, OPT_SMALL_PARAM);
	add_inst(inst_idiv		,0xf7	,1	,7	,REG_AX	,Ed, OPT_SMALL_PARAM);
	add_inst(inst_mul		,0xf7	,1	,4	,REG_RAX	,Eq, OPT_BIG_PARAM);
	add_inst(inst_imul		,0xf7	,1	,5	,REG_RAX	,Eq, OPT_BIG_PARAM);
	add_inst(inst_div		,0xf7	,1	,6	,REG_RAX	,Eq, OPT_BIG_PARAM);
	add_inst(inst_idiv		,0xf7	,1	,7	,REG_RAX	,Eq, OPT_BIG_PARAM);
	add_inst(inst_clc		,0xf8	,1	,-1	,-1	,-1);
	add_inst(inst_stc		,0xf9	,1	,-1	,-1	,-1);
	add_inst(inst_cli		,0xfa	,1	,-1	,-1	,-1);
	add_inst(inst_sti		,0xfb	,1	,-1	,-1	,-1);
	add_inst(inst_cld		,0xfc	,1	,-1	,-1	,-1);
	add_inst(inst_std		,0xfd	,1	,-1	,-1	,-1);
	add_inst(inst_inc		,0xfe	,1	,0	,Eb	,-1);
	add_inst(inst_dec		,0xfe	,1	,1	,Eb	,-1);
	add_inst(inst_inc,	0xff,	1,	0,	Ew,	-1, OPT_SMALL_PARAM);
	add_inst(inst_inc,	0xff,	1,	0,	Ed,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_inc,	0xff,	1,	0,	Eq,	-1, OPT_BIG_PARAM);
	add_inst(inst_dec,	0xff,	1,	1,	Ew,	-1, OPT_SMALL_PARAM);
	add_inst(inst_dec,	0xff,	1,	1,	Ed,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_dec,	0xff,	1,	1,	Eq,	-1, OPT_BIG_PARAM);
	add_inst(inst_call,	0xff,	1,	2,	Ew,	-1, OPT_SMALL_PARAM);
	add_inst(inst_call,	0xff,	1,	2,	Ed,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_call,	0xff,	1,	2,	Eq,	-1, OPT_BIG_PARAM);
	add_inst(inst_call_far,	0xff,	1,	3,	Ew,	-1, OPT_SMALL_PARAM); // Ep instead of Ev...
	add_inst(inst_call_far,	0xff,	1,	3,	Ed,	-1, OPT_MEDIUM_PARAM);
	add_inst(inst_call_far,	0xff,	1,	3,	Eq,	-1, OPT_BIG_PARAM);
	add_inst(inst_jmp, 0xff, 1,	4, Ew, -1, OPT_SMALL_PARAM);
	add_inst(inst_jmp, 0xff, 1,	4, Ed, -1, OPT_MEDIUM_PARAM);
	add_inst(inst_jmp, 0xff, 1,	4, Eq, -1, OPT_BIG_PARAM);
	add_inst(inst_jmp_far, 0xff, 1, 5, Ed, -1, OPT_SMALL_PARAM);
	add_inst(inst_jmp_far, 0xff, 1, 5, E48, -1, OPT_MEDIUM_PARAM);
	add_inst(inst_push, 0xff, 1, 6, Ew, -1, OPT_SMALL_PARAM);
	add_inst(inst_push, 0xff, 1, 6, Ed, -1, OPT_MEDIUM_PARAM);
	add_inst(inst_push, 0xff, 1, 6, Eq, -1, OPT_BIG_PARAM);

	// sse
	add_inst(inst_movss,	0x100ff3,	3,	-1,	Xx, Ed);
	add_inst(inst_movss,	0x110ff3,	3,	-1,	Ed, Xx);
	add_inst(inst_movsd,	0x100ff2,	3,	-1,	Xx, Eq);
	add_inst(inst_movsd,	0x110ff2,	3,	-1,	Eq, Xx);
}



void Init(int set)
{
	if (set < 0)
		set = QueryInstructionSet();

	InstructionSet.set = set;
	InstructionSet.pointer_size = 4;
	if (set == INSTRUCTION_SET_AMD64)
		InstructionSet.pointer_size = 8;

	for (int i=0;i<NUM_REG_ROOTS;i++)
		for (int j=0;j<=MAX_REG_SIZE;j++)
			RegResize[i][j] = -1;

	if (set == INSTRUCTION_SET_ARM)
		InitARM();
	else
		InitX86();
}

InstructionParam::InstructionParam()
{
	type = PARAMT_NONE;
	disp = DISP_MODE_NONE;
	reg = NULL;
	reg2 = NULL;
	deref = false;
	size = SIZE_UNKNOWN;
	value = 0;
	is_label = false;
	write_back = false;
}

// convert an asm parameter into a human readable expression
string InstructionParam::str(bool hide_size)
{
	//msg_write("----");
	//msg_write(p.type);
	if (type == PARAMT_INVALID){
		return "-\?\?\?-";
	}else if (type == PARAMT_NONE){
		return "";
	}else if (type == PARAMT_REGISTER){
		string post;
		if (write_back)
			post = "!";
			//msg_write((long)reg);
			//msg_write((long)disp);
		if (deref){
			//msg_write("deref");
			string ss;
			if (!hide_size)
				ss = get_size_name(size) + " ";
			string s = reg->name;
			if (disp == DISP_MODE_8){
				if (value > 0)
					s += format("+0x%02x", (value & 0xff));
				else
					s += format("-0x%02x", ((-value) & 0xff));
			}else if (disp == DISP_MODE_16)
				s += format("+0x%04x", (value & 0xffff));
			else if (disp == DISP_MODE_32)
				s += format("+0x%08x", value);
			else if (disp == DISP_MODE_SIB)
				return "SIB[...][...]";
			else if (disp == DISP_MODE_8_SIB)
				s += format("::SIB...+0x%02x", value);
			else if (disp == DISP_MODE_8_REG2)
				s += format("%s+0x%02x", reg2->name.c_str(), value);
			else if (disp == DISP_MODE_REG2)
				s += "+" + reg2->name;
			return ss + "[" + s + "]";
		}else
			return reg->name + post;
	}else if (type == PARAMT_REGISTER_SET){
		Array<string> s;
		for (int i=0; i<16; i++)
			if (value & (1<<i))
				s.add(RegisterByID[REG_R0 + i]->name);
		return "{" + implode(s, ",") + "}";
	}else if (type == PARAMT_IMMEDIATE){
		//msg_write("im");
		if (deref)
			return get_size_name(size) + " " + format("[%s]", d2h(&value, state.AddrSize).c_str());
		return d2h(&value, size);
	/*}else if (type == ParamTImmediateExt){
		//msg_write("im");
		return format("%s:%s", d2h(&((char*)&value)[4], 2).c_str(), d2h(&value, state.ParamSize).c_str());*/
	}
	return "\?\?\?";
}

string ARMConditions[16] = {
	"eq",
	"ne",
	"cs",
	"cc",
	"mi",
	"pl",
	"vs",
	"vc",
	"hi",
	"ls",
	"ge",
	"lt",
	"gt",
	"le",
	"al",
	"???",
};

string InstructionWithParams::str(bool hide_size)
{
	string s;
	if (condition != ARM_COND_ALWAYS)
		s += ARMConditions[condition & 0xf] + ":";
	s += GetInstructionName(inst);
	s += "  " + p[0].str(hide_size);
	if (p[1].type != PARAMT_NONE)
		s += ",  " + p[1].str(hide_size);
	if (p[2].type != PARAMT_NONE)
		s += ",  " + p[2].str(hide_size);
	return s;
}

inline void UnfuzzyParam(InstructionParam &p, InstructionParamFuzzy &pf)
{
	msg_db_f("UnfuzzyParam", 2+ASM_DB_LEVEL);
	p.type = pf._type_;
	p.reg2 = NULL;
	p.disp = DISP_MODE_NONE;
	p.reg = pf.reg;
	if ((p.reg) && (state.ExtendModRMBase)){
		if ((p.reg->id >= REG_RAX) && (p.reg->id <= REG_RBP))
			p.reg = RegisterByID[p.reg->id + REG_R8 - REG_RAX];
	}
	p.size = pf.size;
	p.deref = false; // well... FIXME
	p.value = 0;
	p.is_label = false;
	if (pf._type_ == PARAMT_MEMORY){
		p.type = PARAMT_IMMEDIATE;
		p.deref = true;
	}
}

int GetModRMRegister(int reg, int size)
{
	if (size == SIZE_8){
		if (reg == 0x00)	return REG_AL;
		if (reg == 0x01)	return REG_CL;
		if (reg == 0x02)	return REG_DL;
		if (reg == 0x03)	return REG_BL;
		if (reg == 0x04)	return REG_AH;
		if (reg == 0x05)	return REG_CH;
		if (reg == 0x06)	return REG_DH;
		if (reg == 0x07)	return REG_BH;
	}else if (size == SIZE_16){
		if (reg == 0x00)	return REG_AX;
		if (reg == 0x01)	return REG_CX;
		if (reg == 0x02)	return REG_DX;
		if (reg == 0x03)	return REG_BX;
		if (reg == 0x04)	return REG_SP;
		if (reg == 0x05)	return REG_BP;
		if (reg == 0x06)	return REG_SI;
		if (reg == 0x07)	return REG_DI;
	}else if (size == SIZE_32){
		if (reg == 0x00)	return REG_EAX;
		if (reg == 0x01)	return REG_ECX;
		if (reg == 0x02)	return REG_EDX;
		if (reg == 0x03)	return REG_EBX;
		if (reg == 0x04)	return REG_ESP;
		if (reg == 0x05)	return REG_EBP;
		if (reg == 0x06)	return REG_ESI;
		if (reg == 0x07)	return REG_EDI;
		if (reg == 0x08)	return REG_R8D;
		if (reg == 0x09)	return REG_R9D;
		if (reg == 0x0a)	return REG_R10D;
		if (reg == 0x0b)	return REG_R11D;
		if (reg == 0x0c)	return REG_R12D;
		if (reg == 0x0d)	return REG_R13D;
		if (reg == 0x0e)	return REG_R14D;
		if (reg == 0x0f)	return REG_R15D;
	}else if (size == SIZE_64){
		if (reg == 0x00)	return REG_RAX;
		if (reg == 0x01)	return REG_RCX;
		if (reg == 0x02)	return REG_RDX;
		if (reg == 0x03)	return REG_RBX;
		if (reg == 0x04)	return REG_RSP;
		if (reg == 0x05)	return REG_RBP;
		if (reg == 0x06)	return REG_RSI;
		if (reg == 0x07)	return REG_RDI;
		if (reg == 0x08)	return REG_R8;
		if (reg == 0x09)	return REG_R9;
		if (reg == 0x0a)	return REG_R10;
		if (reg == 0x0b)	return REG_R11;
		if (reg == 0x0c)	return REG_R12;
		if (reg == 0x0d)	return REG_R13;
		if (reg == 0x0e)	return REG_R14;
		if (reg == 0x0f)	return REG_R15;
	}
	msg_error("unhandled mod/rm register: " + i2s(reg) + " (size " + i2s(size) + ")");
	return 0;
}

inline void GetFromModRM(InstructionParam &p, InstructionParamFuzzy &pf, unsigned char modrm)
{
	msg_db_f("GetFromModRM", 2+ASM_DB_LEVEL);
	if (pf.mrm_mode == MRM_REG){
		unsigned char reg = modrm & 0x38; // bits 5, 4, 3
		p.type = PARAMT_REGISTER;
		p.deref = false;
		if (pf.reg_group == RegGroupSegment){
			if (reg == 0x00)	p.reg = RegisterByID[REG_ES];
			if (reg == 0x08)	p.reg = RegisterByID[REG_CS];
			if (reg == 0x10)	p.reg = RegisterByID[REG_SS];
			if (reg == 0x18)	p.reg = RegisterByID[REG_DS];
			if (reg == 0x20)	p.reg = RegisterByID[REG_FS];
			if (reg == 0x28)	p.reg = RegisterByID[REG_GS];
		}else if (pf.reg_group == RegGroupControl){
			if (reg == 0x00)	p.reg = RegisterByID[REG_CR0];
			if (reg == 0x08)	p.reg = RegisterByID[REG_CR1];
			if (reg == 0x10)	p.reg = RegisterByID[REG_RC2];
			if (reg == 0x18)	p.reg = RegisterByID[REG_CR3];
		}else if (pf.reg_group == RegGroupXmm){
			p.reg = RegisterByID[REG_XMM0 + (reg >> 3)];
		}else{
			reg = (reg >> 3) | (state.ExtendModRMReg ? 0x08 : 0x00);
			p.reg = RegisterByID[GetModRMRegister(reg, p.size)];
		}
	}else if (pf.mrm_mode == MRM_MOD_RM){
		unsigned char mod = modrm & 0xc0; // bits 7, 6
		unsigned char rm = modrm & 0x07; // bits 2, 1, 0
		if (state.ExtendModRMBase)	rm |= 0x08;
		if (mod == 0x00){
			if (state.AddrSize == SIZE_16){
				p.type = PARAMT_REGISTER;
				p.deref = true;
				if (rm == 0x00){p.reg = RegisterByID[REG_BX];	p.reg2 = RegisterByID[REG_SI];	p.disp = DISP_MODE_REG2;	}
				if (rm == 0x01){p.reg = RegisterByID[REG_BX];	p.reg2 = RegisterByID[REG_DI];	p.disp = DISP_MODE_REG2;	}
				if (rm == 0x02){p.reg = RegisterByID[REG_BP];	p.reg2 = RegisterByID[REG_SI];	p.disp = DISP_MODE_REG2;	}
				if (rm == 0x03){p.reg = RegisterByID[REG_BP];	p.reg2 = RegisterByID[REG_DI];	p.disp = DISP_MODE_REG2;	}
				if (rm == 0x04)	p.reg = RegisterByID[REG_SI];
				if (rm == 0x05)	p.reg = RegisterByID[REG_DI];
				if (rm == 0x06){p.reg = NULL;	p.type = PARAMT_IMMEDIATE;	}
				if (rm == 0x07)	p.reg = RegisterByID[REG_BX];
			}else{
				p.type = PARAMT_REGISTER;
				p.deref = true;
				//if (rm == 0x04){p.reg = NULL;	p.disp = DispModeSIB;	p.type = ParamTImmediate;}//p.type = ParamTInvalid;	Error("kein SIB byte...");}
				if (rm == 0x04){p.reg = RegisterByID[REG_EAX];	p.disp = DISP_MODE_SIB;	} // eax = provisoric
				else if (rm == 0x05){p.reg = NULL;	p.type = PARAMT_IMMEDIATE;	}
				else
					p.reg = RegisterByID[GetModRMRegister(rm, SIZE_32)];
			}
		}else if ((mod == 0x40) || (mod == 0x80)){
			if (state.AddrSize == SIZE_16){
				p.type = PARAMT_REGISTER;
				p.deref = true;
				if (rm == 0x00){p.reg = RegisterByID[REG_BX];	p.reg2 = RegisterByID[REG_SI];	p.disp = (mod == 0x40) ? DISP_MODE_8_REG2 : DISP_MODE_16_REG2;	}
				if (rm == 0x01){p.reg = RegisterByID[REG_BX];	p.reg2 = RegisterByID[REG_DI];	p.disp = (mod == 0x40) ? DISP_MODE_8_REG2 : DISP_MODE_16_REG2;	}
				if (rm == 0x02){p.reg = RegisterByID[REG_BP];	p.reg2 = RegisterByID[REG_SI];	p.disp = (mod == 0x40) ? DISP_MODE_8_REG2 : DISP_MODE_16_REG2;	}
				if (rm == 0x03){p.reg = RegisterByID[REG_BP];	p.reg2 = RegisterByID[REG_DI];	p.disp = (mod == 0x40) ? DISP_MODE_8_REG2 : DISP_MODE_16_REG2;	}
				if (rm == 0x04){p.reg = RegisterByID[REG_SI];	p.disp = (mod == 0x40) ? DISP_MODE_8 : DISP_MODE_16;	}
				if (rm == 0x05){p.reg = RegisterByID[REG_DI];	p.disp = (mod == 0x40) ? DISP_MODE_8 : DISP_MODE_16;	}
				if (rm == 0x06){p.reg = RegisterByID[REG_BP];	p.disp = (mod == 0x40) ? DISP_MODE_8 : DISP_MODE_16;	}
				if (rm == 0x07){p.reg = RegisterByID[REG_BX];	p.disp = (mod == 0x40) ? DISP_MODE_8 : DISP_MODE_16;	}
			}else{
				p.type = PARAMT_REGISTER;
				p.deref = true;
				p.disp = (mod == 0x40) ? DISP_MODE_8 : DISP_MODE_32;
				//if (rm == 0x04){p.reg = NULL;	p.type = ParamTInvalid;	}
				if (rm == 0x04){p.reg = RegisterByID[REG_EAX];	p.disp = DISP_MODE_8_SIB;	} // eax = provisoric
				else
					p.reg = RegisterByID[GetModRMRegister(rm, SIZE_32)];
			}
		}else if (mod == 0xc0){
			p.type = PARAMT_REGISTER;
			p.deref = false;
			if (state.ExtendModRMBase)	rm |= 0x08;
			p.reg = RegisterByID[GetModRMRegister(rm, p.size)];
		}
	}
}

inline void TryGetSIB(InstructionParam &p, char *&cur)
{
	if ((p.disp == DISP_MODE_SIB) || (p.disp == DISP_MODE_8_SIB)){
		bool disp8 = (p.disp == DISP_MODE_8_SIB);
		char sib = *cur;
		cur++;
		unsigned char ss = (sib & 0xc0); // bits 7, 6
		unsigned char index = (sib & 0x38); // bits 5, 4, 3
		unsigned char base = (sib & 0x07); // bits 2, 1, 0
		/*msg_error("SIB");
		msg_write(ss);
		msg_write(index);
		msg_write(base);*/

		// direct?
		//if (p.disp == DispModeSIB){
			if (ss == 0x00){ // scale factor 1
				p.deref = true;
				p.disp = disp8 ? DISP_MODE_8_REG2 : DISP_MODE_REG2;
				if (base == 0x00)		p.reg = RegisterByID[REG_EAX];
				else if (base == 0x01)	p.reg = RegisterByID[REG_ECX];
				else if (base == 0x02)	p.reg = RegisterByID[REG_EDX];
				else if (base == 0x03)	p.reg = RegisterByID[REG_EBX];
				else if (base == 0x04)	p.reg = RegisterByID[REG_ESP];
				else p.disp = DISP_MODE_SIB; // ...
				if (index == 0x00)		p.reg2 = RegisterByID[REG_EAX];
				else if (index == 0x08)	p.reg2 = RegisterByID[REG_ECX];
				else if (index == 0x10)	p.reg2 = RegisterByID[REG_EDX];
				else if (index == 0x18)	p.reg2 = RegisterByID[REG_EBX];
				else if (index == 0x28)	p.reg2 = RegisterByID[REG_EBP];
				else if (index == 0x30)	p.reg2 = RegisterByID[REG_ESI];
				else if (index == 0x38)	p.reg2 = RegisterByID[REG_EDI];
				else p.disp = disp8 ? DISP_MODE_8 : DISP_MODE_NONE;
			}
		//}
	}
}

inline void ReadParamData(char *&cur, InstructionParam &p, bool has_modrm)
{
	msg_db_f("ReadParamData", 2+ASM_DB_LEVEL);
	//char *o = cur;
	p.value = 0;
	if (p.type == PARAMT_IMMEDIATE){
		if (p.deref){
			int size = has_modrm ? state.AddrSize : state.FullRegisterSize; // Ov/Mv...
			memcpy(&p.value, cur, size);
			cur += size;
		}else{
			memcpy(&p.value, cur, p.size);
			cur += p.size;
		}
	/*}else if (p.type == ParamTImmediateExt){
		if (state.ParamSize == Size16){ // addr?
			*(short*)&p.value = *(short*)cur;	cur += 2;	((short*)&p.value)[2] = *(short*)cur;	cur += 2;
		}else{
			memcpy(&p.value, cur, 6);		cur += 6;
		}*/
	}else if (p.type == PARAMT_REGISTER){
		if ((p.disp == DISP_MODE_8) || (p.disp == DISP_MODE_8_REG2) || (p.disp == DISP_MODE_8_SIB)){
			*(char*)&p.value = *cur;		cur ++;
		}else if (p.disp == DISP_MODE_16){
			*(short*)&p.value = *(short*)cur;		cur += 2;
		}else if (p.disp == DISP_MODE_32){
			*(int*)&p.value = *(int*)cur;		cur += 4;
		}
	}
	//msg_write((long)cur - (long)o);
}

string show_reg(int r)
{
	return format("r%d", r);
}

int ARMDataInstructions[16] =
{
	inst_and,
	inst_eor,
	inst_sub,
	inst_rsb,
	inst_add,
	inst_adc,
	inst_sbc,
	inst_rsc,
	inst_tst,
	inst_teq,
	inst_cmp,
	inst_cmn,
	inst_orr,
	inst_mov,
	inst_bic,
	inst_mvn
};

int arm_decode_imm(int imm)
{
	int r = ((imm >> 8) & 0xf);
	int n = (imm & 0xff);
	return n >> (r*2) | n << (32 - r*2);
}

InstructionParam disarm_shift_reg(int code)
{
	InstructionParam p = param_reg(REG_R0 + (code & 0xf));
	bool by_reg = (code >> 4) & 0x1;
	int r = ((code >> 7) & 0x1f);
	if (!by_reg and r == 0)
		return p;
	/*if (((code >> 5) & 0x3) == 0)
		s += "<<";
	else
		s += ">>";
	if (by_reg){
		s += show_reg((code >> 8) & 0xf);
	}else{
		s += format("%d", r);
	}*/
	return p;
}

InstructionWithParams disarm_data_opcode(int code)
{
	InstructionWithParams i;
	i.inst = ARMDataInstructions[(code >> 21) & 15];
	if (((code >> 20) & 1) and (i.inst != inst_cmp) and (i.inst != inst_cmn) and (i.inst != inst_teq) and (i.inst != inst_tst))
		msg_write(GetInstructionName(i.inst) + "[S]");
	i.p[0] = param_reg(REG_R0 + ((code >> 12) & 15));
	i.p[1] = param_reg(REG_R0 + ((code >> 16) & 15));
	if ((code >> 25) & 1)
		i.p[2] = param_imm(arm_decode_imm(code & 0xfff), SIZE_32);
	else
		i.p[2] = disarm_shift_reg(code & 0xfff);
	if ((i.inst == inst_cmp) or (i.inst == inst_cmn) or (i.inst == inst_tst) or (i.inst == inst_teq) or (i.inst == inst_mov)){
		if ((i.inst == inst_cmp) or (i.inst == inst_cmn))
			i.p[0] = i.p[1];
		i.p[1] = i.p[2];
		i.p[2] = param_none;
	}
	return i;
}

InstructionWithParams disarm_data_opcode_mul(int code)
{
	InstructionWithParams i;
	i.inst = inst_mul;
	if ((code >> 20) & 1)
		msg_write(" [S]");
	i.p[0] = param_reg(REG_R0 + ((code >> 16) & 15));
	i.p[1] = param_reg(REG_R0 + ((code >> 0) & 15));
	i.p[2] = param_reg(REG_R0 + ((code >> 8) & 15));
	return i;
}

InstructionWithParams disarm_branch(int code)
{
	InstructionWithParams i;
	if ((code >> 24) & 1)
		i.inst = inst_bl;
	else
		i.inst = inst_b;
	i.p[0] = param_imm(code & 0x00ffffff, SIZE_32);
	i.p[1] = param_none;
	i.p[2] = param_none;
	return i;
}

InstructionWithParams disarm_data_transfer(int code)
{
	InstructionWithParams i;
	bool bb = ((code >> 22) & 1);
	bool ll = ((code >> 20) & 1);
	if (ll)
		i.inst = bb ? inst_ldrb : inst_ldr;
	else
		i.inst = bb ? inst_strb : inst_str;
	int Rn = (code >> 16) & 0xf;
	int Rd = (code >> 12) & 0xf;
	i.p[0] = param_reg(REG_R0 + Rd);
	bool imm = ((code >> 25) & 1);
	bool pre = ((code >> 24) & 1);
	bool up = ((code >> 23) & 1);
	bool ww = ((code >> 21) & 1);
	if (imm){
		msg_write( " --shifted reg--");
	}else{
		if (code & 0xfff)
			i.p[1] = param_deref_reg_shift(REG_R0 + Rn, up ? (code & 0xfff) : (-(code & 0xfff)), bb ? SIZE_8 : SIZE_32);
		else
			i.p[1] = param_deref_reg(REG_R0 + Rn, bb ? SIZE_8 : SIZE_32);
	}
	i.p[1].write_back = ww;
	i.p[2] = param_none;
	return i;
}

InstructionWithParams disarm_data_block_transfer(int code)
{
	InstructionWithParams i;
	bool ll = ((code >> 20) & 1);
	bool pp = ((code >> 24) & 1);
	bool uu = ((code >> 23) & 1);
	bool ww = ((code >> 21) & 1);
	if (!pp and uu)
		i.inst = ll ? inst_ldmia : inst_stmia;
	else if (pp and uu)
		i.inst = ll ? inst_ldmib : inst_stmib;
	else if (!pp and !uu)
		i.inst = ll ? inst_ldmda : inst_stmda;
	else if (pp and !uu)
		i.inst = ll ? inst_ldmdb : inst_stmdb;
	int Rn = (code >> 16) & 0xf;
	i.p[0] = param_reg(REG_R0 + Rn);
	i.p[1] = param_reg_set(code & 0xffff);
	i.p[0].write_back = ww;
	i.p[2] = param_none;
	return i;
}

string DisassembleARM(void *_code_,int length,bool allow_comments)
{
	string buf;
	int *code = (int*)_code_;
	for (int ni=0; ni<length/4; ni++){
		int cur = code[ni];

		int x = (cur >> 25) & 0x7;

		buf += string((char*)&cur, 4).hex(true).substr(2, -1);
		buf += "    ";

		InstructionWithParams iwp;
		iwp.inst = inst_nop;
		iwp.p[0] = param_none;
		iwp.p[1] = param_none;
		iwp.p[2] = param_none;
		if (((cur >> 26) & 3) == 0){
			if ((cur & 0x0fe000f0) == 0x00000090)
				iwp = disarm_data_opcode_mul(cur);
			else
				iwp = disarm_data_opcode(cur);
		}else if (((cur >> 26) & 0x3) == 0b01)
			iwp = disarm_data_transfer(cur);
		else if (x == 0b100)
			iwp = disarm_data_block_transfer(cur);
		else if (x == 0b101)
			iwp = disarm_branch(cur);
		iwp.condition = (cur >> 28) & 0xf;


		buf += iwp.str() + "\n";
	}
	return buf;
}

string DisassembleX86(void *_code_,int length,bool allow_comments)
{
	char *code = (char*)_code_;

	string param;
	char *opcode;
	string bufstr;
	char *end=code+length;
	char *orig=code;
	if (length<0)	end=code+65536;

	// code points to the start of the (current) complete command (dword cs: mov ax, ...)
	// cur points to the currently processed byte
	// opcode points to the start of the instruction (mov)
	char *cur = code;
	state.init();
	state.DefaultSize = SIZE_32;


	while(code < end){
		state.reset();
		opcode = cur;
		code = cur;

		// done?
		if (code >= end)
			break;

		// special info
		if (CurrentMetaInfo){

			// labels
#if 0
			// TODO
			for (int i=0;i<CurrentMetaInfo->label.num;i++)
				if ((long)code - (long)orig == CurrentMetaInfo->label[i].pos)
					bufstr += "    " + CurrentMetaInfo->label[i].name + ":\n";
#endif

			// data blocks
			bool inserted = false;
			for (int i=0;i<CurrentMetaInfo->data.num;i++){
				//printf("%d  %d  %d  %d\n", CurrentMetaInfo->data[i].Pos, (long)code, (long)orig, (long)code - (long)orig);
				if ((long)code - (long)orig == CurrentMetaInfo->data[i].offset){
					//msg_write("data");
					if (CurrentMetaInfo->data[i].size==1){
						bufstr += "  db\t";
						bufstr += d2h(cur,1);
					}else if (CurrentMetaInfo->data[i].size==2){
						bufstr += "  dw\t";
						bufstr += d2h(cur,2);
					}else if (CurrentMetaInfo->data[i].size==4){
						bufstr += "  dd\t";
						bufstr += d2h(cur,4);
					}else{
						bufstr += "  ds \t...";
					}
					cur += CurrentMetaInfo->data[i].size;
					bufstr += "\n";
					inserted = true;
				}
			}
			if (inserted)
				continue;

			// change of bits (processor mode)
			for (int i=0;i<CurrentMetaInfo->bit_change.num;i++)
				if ((long)code-(long)orig == CurrentMetaInfo->bit_change[i].offset){
					state.DefaultSize = (CurrentMetaInfo->bit_change[i].bits == 16) ? SIZE_16 : SIZE_32;
					state.reset();
					if (state.DefaultSize == SIZE_16)
						bufstr += "   bits_16\n";
					else
						bufstr += "   bits_32\n";
				}
		}

		// code

		// prefix (size/segment register)
		Register *seg = NULL;
		if (cur[0]==0x67){
			state.AddrSize = (state.DefaultSize == SIZE_32) ? SIZE_16 : SIZE_32;
			cur++;
		}
		if (cur[0]==0x66){
			state.ParamSize = (state.DefaultSize == SIZE_32) ? SIZE_16 : SIZE_32;
			cur++;
		}
		if (InstructionSet.set == INSTRUCTION_SET_AMD64){
			if ((cur[0] & 0xf0) == 0x40){
				if ((cur[0] & 0x08) > 0)
					state.ParamSize = SIZE_64;
				state.ExtendModRMReg = ((cur[0] & 0x04) > 0);
				state.ExtendModRMIndex = ((cur[0] & 0x02) > 0);
				state.ExtendModRMBase = ((cur[0] & 0x01) > 0);
				cur++;
			}
		}
		if (cur[0]==0x2e){	seg = RegisterByID[REG_CS];	cur++;	}
		else if (cur[0]==0x36){	seg = RegisterByID[REG_SS];	cur++;	}
		else if (cur[0]==0x3e){	seg = RegisterByID[REG_DS];	cur++;	}
		else if (cur[0]==0x26){	seg = RegisterByID[REG_ES];	cur++;	}
		else if (cur[0]==0x64){	seg = RegisterByID[REG_FS];	cur++;	}
		else if (cur[0]==0x65){	seg = RegisterByID[REG_GS];	cur++;	}
		opcode=cur;

		// instruction
		CPUInstruction *inst = NULL;
		foreach(CPUInstruction &ci, CPUInstructions){
			if (ci.code_size == 0)
				continue;
			if (!ci.has_fixed_param){
				if (ci.has_small_param != (state.ParamSize == SIZE_16))
					continue;
				if (ci.has_big_param != (state.ParamSize == SIZE_64))
					continue;
			}
			// opcode correct?
			bool ok = true;
			for (int j=0;j<ci.code_size;j++)
				if (cur[j] != ((char*)&ci.code)[j])
					ok = false;
			// cap correct?
			if (ci.cap >= 0)
				ok &= ((unsigned char)ci.cap == (((unsigned)cur[ci.code_size] >> 3) & 0x07));
			if ((ok) && (ci.has_modrm)){
				InstructionParam p1, p2;
				UnfuzzyParam(p1, ci.param1);
				UnfuzzyParam(p2, ci.param2);

				// modr/m byte
				char modrm = cur[ci.code_size];
				GetFromModRM(p1, ci.param1, modrm);
				GetFromModRM(p2, ci.param2, modrm);
				if ((p1.type == PARAMT_REGISTER) && (!p1.deref) && (!ci.param1.allow_register))
					continue;
				if ((p2.type == PARAMT_REGISTER) && (!p2.deref) && (!ci.param2.allow_register))
					continue;
			}
			if (ok){
				inst = &ci;
				cur += inst->code_size;
				break;
			}
		}
		if (inst){
			InstructionParamFuzzy ip1 = inst->param1;
			InstructionParamFuzzy ip2 = inst->param2;

			
			InstructionParam p1, p2;
			UnfuzzyParam(p1, ip1);
			UnfuzzyParam(p2, ip2);

			// modr/m byte
			if (inst->has_modrm){
				//msg_write("modrm");
				char modrm = *cur;
				cur ++;
				GetFromModRM(p1, ip1, modrm);
				GetFromModRM(p2, ip2, modrm);
				TryGetSIB(p1, cur);
				TryGetSIB(p2, cur);
			}

			// immediate...
			ReadParamData(cur, p1, inst->has_modrm);
			ReadParamData(cur, p2, inst->has_modrm);



		// create asm code
			string str;

			// segment register?
			if (seg)
				str += seg->name + ": ";

			// command
			str += inst->name;

			// parameters
			if ((state.ParamSize != state.DefaultSize) && ((p1.type != PARAMT_REGISTER) || (p1.deref)) && ((p2.type != PARAMT_REGISTER) || p2.deref)){
				if (state.ParamSize == SIZE_16)
					str += " word";
				else if (state.ParamSize == SIZE_32)
					str += " dword";
				else if (state.ParamSize == SIZE_64)
					str += " qword";
			}
			bool hide_size = p2.type != PARAMT_NONE;
			if (p1.type != PARAMT_NONE)
				str += " " + p1.str(hide_size);
			if (p2.type != PARAMT_NONE)
				str += ", " + p2.str(hide_size);
			
			
			if (allow_comments){
				int l = str.num;
				str += " ";
				for (int ii=0;ii<48-l;ii++)
					str += " ";
				str += "// ";
				str += d2h(code,long(cur) - long(code), false);
			}
			//msg_write(str);
			bufstr += str;
			bufstr += "\n";

		}else{
			//msg_write(string2("????? -                          unknown         // %s\n",d2h(code,1+long(cur)-long(code),false)));
			bufstr += format("????? -                          unknown         // %s\n",d2h(code,1+long(cur)-long(code),false).c_str());
			cur ++;
		}

		// done?
		if ((length < 0) && (((unsigned char)opcode[0] == 0xc3) || ((unsigned char)opcode[0] == 0xc2)))
			break;
	}
	return bufstr;
}



// convert some opcode into (human readable) assembler language
string Disassemble(void *code, int length, bool allow_comments)
{
	msg_db_f("Disassemble", 1+ASM_DB_LEVEL);

	if (InstructionSet.set == INSTRUCTION_SET_ARM)
		return DisassembleARM(code, length, allow_comments);
	return DisassembleX86(code, length, allow_comments);
}

// skip unimportant code (whitespace/comments)
//    returns true if end of code
bool IgnoreUnimportant(int &pos)
{
	msg_db_f("IgnoreUnimportant", 4+ASM_DB_LEVEL);
	bool CommentLine = false;
	
	// ignore comments and "white space"
	for (int i=0;i<1048576;i++){
		if (code_buffer[pos] == 0){
			state.EndOfCode = true;
			state.EndOfLine = true;
			return true;
		}
		if (code_buffer[pos] == '\n'){
			state.LineNo ++;
			state.ColumnNo = 0;
			CommentLine = false;
		}
		// "white space"
		if ((code_buffer[pos] == '\n') || (code_buffer[pos] == ' ') || (code_buffer[pos] == '\t')){
			pos ++;
			state.ColumnNo ++;
			continue;
		}
		// comments
		if ((code_buffer[pos] == ';') || ((code_buffer[pos] == '/') && (code_buffer[pos] == '/'))){
			CommentLine = true;
			pos ++;
			state.ColumnNo ++;
			continue;
		}
		if (!CommentLine)
			break;
		pos ++;
		state.ColumnNo ++;
	}
	return false;
}

// returns one "word" in the source code
string FindMnemonic(int &pos)
{
	msg_db_f("GetMne", 1+ASM_DB_LEVEL);
	state.EndOfLine = false;
	char mne[128];
	strcpy(mne, "");

	if (IgnoreUnimportant(pos))
		return mne;
	
	bool in_string = false;
	for (int i=0;i<128;i++){
		mne[i] = code_buffer[pos];
		mne[i + 1] = 0;
		
		// string like stuff
		if ((mne[i] == '\'') || (mne[i] == '\"'))
			in_string =! in_string;
		// end of code
		if (code_buffer[pos] == 0){
			mne[i] = 0;
			state.EndOfCode = true;
			state.EndOfLine = true;
			break;
		}
		// end of line
		if (code_buffer[pos] == '\n'){
			mne[i] = 0;
			state.EndOfLine = true;
			break;
		}
		if (!in_string){
			// "white space" -> complete
			if ((code_buffer[pos] == ' ') || (code_buffer[pos] == '\t') || (code_buffer[pos] == ',')){
				mne[i] = 0;
				// end of line?
				for (int j=0;j<128;j++){
					if ((code_buffer[pos+j] != ' ') && (code_buffer[pos+j] != '\t') && (code_buffer[pos+j] != ',')){
						if ((code_buffer[pos + j] == 0) || (code_buffer[pos + j] == '\n'))
							state.EndOfLine = true;
						// comment ending the line
						if ((code_buffer[pos + j] == ';') || ((code_buffer[pos + j] == '/') && (code_buffer[pos + j + 1] == '/')))
							state.EndOfLine = true;
						pos += j;
						state.ColumnNo += j;
						if (code_buffer[pos] == '\n')
							state.ColumnNo = 0;
						break;
					}
				}
				break;
			}
		}
		pos ++;
		state.ColumnNo ++;
	}
	/*msg_write>Write(mne);
	if (EndOfLine)
		msg_write>Write("    eol");*/
	return mne;
}

// interpret an expression from source code as an assembler parameter
void GetParam(InstructionParam &p, const string &param, InstructionWithParamsList &list, int pn)
{
	msg_db_f("GetParam", 1+ASM_DB_LEVEL);
	p.type = PARAMT_INVALID;
	p.reg = NULL;
	p.deref = false;
	p.size = SIZE_UNKNOWN;
	p.disp = DISP_MODE_NONE;
	p.is_label = false;
	//msg_write(param);

	// none
	if (param.num == 0){
		p.type = PARAMT_NONE;

	// deref
	}else if ((param[0] == '[') && (param[param.num-1] == ']')){
		if (DebugAsm)
			printf("deref:   ");
		so("Deref:");
		//bool u16 = use_mode16;
		GetParam(p, param.substr(1, -2), list, pn);
		p.size = SIZE_UNKNOWN;
		p.deref = true;
		//use_mode16 = u16;

	// string
	}else if ((param[0] == '\"') && (param[param.num-1] == '\"')){
		if (DebugAsm)
			printf("String:   ");
		char *ps = new char[param.num - 1];
		strcpy(ps, param.substr(1, -2).c_str());
		p.value = (long)ps;
		p.type = PARAMT_IMMEDIATE;

	// complex...
	}else if (param.find("+") >= 0){
		if (DebugAsm)
			printf("complex:   ");
		InstructionParam sub;
		
		// first part (must be a register)
		string part;
		for (int i=0;i<param.num;i++)
			if ((param[i] == ' ') || (param[i] == '+'))
				break;
			else
				part.add(param[i]);
		int offset = part.num;
		GetParam(sub, part, list, pn);
		if (sub.type == PARAMT_REGISTER){
			//msg_write("reg");
			p.type = PARAMT_REGISTER;
			p.size = SIZE_32;
			p.reg = sub.reg;
		}else
			p.type = PARAMT_INVALID;

		// second part (...up till now only hex)
		for (int i=offset;i<param.num;i++)
			if ((param[i] != ' ') && (param[i] != '+')){
				offset = i;
				break;
			}
		part = param.substr(offset, -1);
		GetParam(sub, part, list, pn);
		if (sub.type == PARAMT_IMMEDIATE){
			//msg_write("c2 = im");
			if (((long)sub.value & 0xffffff00) == 0)
				p.disp = DISP_MODE_8;
			else
				p.disp = DISP_MODE_32;
			p.value = sub.value;
		}else
			p.type = PARAMT_INVALID;

		

	// hex const
	}else if ((param[0] == '0') && (param[1] == 'x')){
		p.type = PARAMT_IMMEDIATE;
		long long v = 0;
		for (int i=2;i<param.num;i++){
			if (param[i] == '.'){
			}else if ((param[i] >= 'a') && (param[i] <= 'f')){
				v *= 16;
				v += param[i] - 'a' + 10;
			}else if ((param[i] >= 'A') && (param[i] <= 'F')){
				v *= 16;
				v += param[i]-'A'+10;
			}else if ((param[i]>='0')&&(param[i]<='9')){
				v*=16;
				v+=param[i]-'0';
			/*}else if (param[i]==':'){
				InstructionParam sub;
				GetParam(sub, param.tail(param.num - i - 1), list, pn);
				if (sub.type != ParamTImmediate){
					SetError("error in hex parameter:  " + string(param));
					p.type = PKInvalid;
					return;						
				}
				p.value = (long)v;
				p.value <<= 8 * sub.size;
				p.value += sub.value;
				p.size = sub.size;
				p.type = ParamTImmediate;//Ext;
				break;*/
			}else{
				SetError("evil character in hex parameter:  \"" + param + "\"");
				p.type = PARAMT_INVALID;
				return;
			}
			p.value = (long)v;
			p.size = SIZE_8;
			if (param.num > 4)
				p.size = SIZE_16;
			if (param.num > 6)
				p.size = SIZE_32;
			if (param.num > 10)
				p.size = SIZE_48;
			if (param.num > 14)
				p.size = SIZE_64;
		}
		if (DebugAsm){
			printf("hex const:  %s\n",d2h((char*)&p.value,p.size).c_str());
		}

	// char const
	}else if ((param[0] == '\'') && (param[param.num - 1] == '\'')){
		p.value = (long)param[1];
		p.type = PARAMT_IMMEDIATE;
		p.size = SIZE_8;
		if (DebugAsm)
			printf("hex const:  %s\n",d2h((char*)&p.value,1).c_str());

	// label substitude
	}else if (param == "$"){
		p.value = list.add_label(param, true);
		p.type = PARAMT_IMMEDIATE;
		p.size = SIZE_32;
		p.is_label = true;
		so("label:  " + param + "\n");
		
	}else{
		// register
		for (int i=0;i<Registers.num;i++)
			if (Registers[i].name == param){
				p.type = PARAMT_REGISTER;
				p.reg = &Registers[i];
				p.size = Registers[i].size;
				so("Register:  " + Registers[i].name + "\n");
				return;
			}
		// existing label
		for (int i=0;i<list.label.num;i++)
			if (list.label[i].name == param){
				p.value = i;
				p.type = PARAMT_IMMEDIATE;
				p.size = SIZE_32;
				p.is_label = true;
				so("label:  " + param + "\n");
				return;
			}
		// script variable (global)
		for (int i=0;i<CurrentMetaInfo->global_var.num;i++){
			if (CurrentMetaInfo->global_var[i].name == param){
				p.value = (long)CurrentMetaInfo->global_var[i].pos;
				p.type = PARAMT_IMMEDIATE;
				p.size = CurrentMetaInfo->global_var[i].size;
				p.deref = true;
				so("global variable:  \"" + param + "\"\n");
				return;
			}
		}
		// not yet existing label...
		if (param[0]=='_'){
			so("label as param:  \"" + param + "\"\n");
			p.value = list.add_label(param, false);
			p.type = PARAMT_IMMEDIATE;
			p.is_label = true;
			p.size = SIZE_32;
			return;
		}
	}
	if (p.type == PARAMT_INVALID)
		SetError("unknown parameter:  \"" + param + "\"\n");
}

inline void insert_val(char *oc, int &ocs, long long val, int size)
{
	if (size == 1)
		oc[ocs] = (char)val;
	else if (size == 2)
		*(short*)&oc[ocs] = (short)val;
	else if (size == 4)
		*(int*)&oc[ocs] = (int)val;
	else if (size == 8)
		*(long long int*)&oc[ocs] = val;
	else
		memcpy(&oc[ocs], &val, size);
}

inline void append_val(char *oc, int &ocs, long long val, int size)
{
	insert_val(oc, ocs, val, size);
	ocs += size;
}

void OpcodeAddImmideate(char *oc, int &ocs, InstructionParam &p, CPUInstruction &inst, InstructionWithParamsList &list, int next_param_size)
{
	long long value = p.value;
	int size = 0;
	if (p.type == PARAMT_IMMEDIATE){
		size = p.size;
		if (p.deref){
			//---msg_write("deref....");
			size = state.AddrSize; // inst.has_big_addr
			if (InstructionSet.set == INSTRUCTION_SET_AMD64){
				if (inst.has_modrm)
					value -= (long)oc + ocs + size + next_param_size; // amd64 uses RIP-relative addressing!
				else
					size = SIZE_64; // Ov/Mv...
			}
		}
	//}else if (p.type == ParamTImmediateExt){
	//	size = state.ParamSize;  // bits 0-15  /  0-31
	}else if (p.type == PARAMT_REGISTER){
		if (p.disp == DISP_MODE_8)	size = SIZE_8;
		if (p.disp == DISP_MODE_16)	size = SIZE_16;
		if (p.disp == DISP_MODE_32)	size = SIZE_32;
	}else
		return;

	bool rel = ((inst.name[0] == 'j') /*&& (inst.param1._type_ != ParamTImmediateDouble)*/) || (inst.name == "call") || (inst.name.find("loop") >= 0);
	if (inst.inst == inst_jmp_far)
		rel = false;
	if (p.is_label){
		WantedLabel w;
		w.pos = ocs;
		w.size = size;
		w.label_no = (int)value;
		w.name = list.label[p.value].name;
		w.relative = rel;
		w.inst_no = list.current_inst;
		list.wanted_label.add(w);
		so("add wanted label");
	}else if (rel){
		value -= CurrentMetaInfo->code_origin + ocs + size + next_param_size; // TODO ...first byte of next opcode
	}

	//---msg_write("imm " + i2s(size));
	append_val(oc, ocs, value, size);
}

void InstructionWithParamsList::LinkWantedLabels(void *oc)
{
	foreachib(WantedLabel &w, wanted_label, i){
		Label &l = label[w.label_no];
		if (l.value == -1)
			continue;
		so("linking label");

		int value = l.value;
		if (w.relative)
			value -= CurrentMetaInfo->code_origin + w.pos + w.size; // TODO first byte after command

		insert_val((char*)oc, w.pos, value, w.size);


		wanted_label.erase(i);
		_foreach_it_.update();
	}
}

void add_data_inst(InstructionWithParamsList *l, int size)
{
	AsmData d;
	d.cmd_pos = l->num;
	d.size = size;
	CurrentMetaInfo->data.add(d);
}

void InstructionWithParamsList::AppendFromSource(const string &_code)
{
	msg_db_f("AppendFromSource", 1+ASM_DB_LEVEL);

	const char *code = _code.c_str();

	if (!CurrentMetaInfo)
		SetError("no CurrentMetaInfo");

	state.LineNo = CurrentMetaInfo->line_offset;
	state.ColumnNo = 0;

	// CurrentMetaInfo->CurrentOpcodePos // Anfang aktuelle Zeile im gesammten Opcode
	code_buffer = code; // Asm-Source-Puffer

	int pos = 0;
	InstructionParam p1, p2, p3;
	state.DefaultSize = SIZE_32;
	if (CurrentMetaInfo)
		if (CurrentMetaInfo->mode16)
			state.DefaultSize = SIZE_16;
	state.EndOfCode = false;
	while(pos < _code.num - 2){

		string cmd, param1, param2, param3;

		//msg_write("..");
		state.reset();


	// interpret asm code (1 line)
		// find command
		cmd = FindMnemonic(pos);
		current_line = state.LineNo;
		current_col = state.ColumnNo;
		//msg_write(cmd);
		if (cmd.num == 0)
			break;
		// find parameters
		if (!state.EndOfLine){
			param1 = FindMnemonic(pos);
			if ((param1 == "dword") || (param1 == "word") || (param1 == "qword")){
				if (param1 == "word")
					state.ParamSize = SIZE_16;
				else if (param1 == "dword")
					state.ParamSize = SIZE_32;
				else if (param1 == "qword")
					state.ParamSize = SIZE_64;
				if (!state.EndOfLine)
					param1 = FindMnemonic(pos);
			}
		}
		if (!state.EndOfLine)
			param2 = FindMnemonic(pos);
		if (!state.EndOfLine)
			param3 = FindMnemonic(pos);
		//msg_write(string2("----: %s %s%s %s", cmd, param1, (strlen(param2)>0)?",":"", param2));
		if (state.EndOfCode)
			break;
		so("------------------------------");
		so(cmd);
		so(param1);
		so(param2);
		so(param3);
		so("------");

		// parameters
		GetParam(p1, param1, *this, 0);
		GetParam(p2, param2, *this, 1);
		GetParam(p3, param3, *this, 1);
		if ((p1.type == PARAMT_INVALID) || (p2.type == PARAMT_INVALID) || (p3.type == PARAMT_INVALID))
			return;

	// special stuff
		if (cmd == "bits_16"){
			so("16 bit Modus!");
			state.DefaultSize = SIZE_16;
			state.reset();
			if (CurrentMetaInfo){
				CurrentMetaInfo->mode16 = true;
				BitChange b;
				b.cmd_pos = num;
				b.bits = 16;
				CurrentMetaInfo->bit_change.add(b);
			}
			continue;
		}else if (cmd == "bits_32"){
			so("32 bit Modus!");
			state.DefaultSize = SIZE_32;
			state.reset();
			if (CurrentMetaInfo){
				CurrentMetaInfo->mode16 = false;
				BitChange b;
				b.cmd_pos = num;
				b.bits = 32;
				CurrentMetaInfo->bit_change.add(b);
			}
			continue;

		}else if (cmd == "db"){
			so("Daten:   1 byte");
			add_data_inst(this, 1);
		}else if (cmd == "dw"){
			so("Daten:   2 byte");
			add_data_inst(this, 2);
		}else if (cmd == "dd"){
			so("Daten:   4 byte");
			add_data_inst(this, 4);
		}/*else if ((cmd == "ds") || (cmd == "dz")){
			so("Daten:   String");
			char *s = (char*)p1.value;
			int l=strlen(s);
			if (cmd == "dz")
				l ++;
			if (CurrentMetaInfo){
				AsmData d;
				d.cmd_pos = num;
				d.size = l;
				d.data = new char[l];
				memcpy(d.data, s, l);
				CurrentMetaInfo->data.add(d);
			}
			//memcpy(&buffer[CodeLength], s, l);
			//CodeLength += l;
			continue;
		}*/else if (cmd[cmd.num - 1] == ':'){
			so("Label");
			cmd.resize(cmd.num - 1);
			so(cmd);
			add_label(cmd, true);

			continue;
		}


		InstructionWithParams iwp;
		iwp.condition = ARM_COND_ALWAYS;

		if (cmd.find(":") >= 0){
			iwp.condition = -1;
			Array<string> l = cmd.explode(":");
			for (int i=0; i<16; i++)
				if (l[0] == ARMConditions[i])
					iwp.condition = i;
			if (iwp.condition < 0)
				SetError("unknown condition: " + l[0]);
			cmd = l[1];
		}

		// command
		int inst = -1;
		for (int i=0;i<NUM_INSTRUCTION_NAMES;i++)
			if (InstructionNames[i].name == cmd)
				inst = InstructionNames[i].inst;
		if (inst < 0)
			SetError("unknown instruction:  " + cmd);
		// prefix
		if (state.ParamSize != state.DefaultSize){
			//buffer[CodeLength ++] = 0x66;
			SetError("prefix unhandled:  " + cmd);
		}
		iwp.inst = inst;
		iwp.p[0] = p1;
		iwp.p[1] = p2;
		iwp.p[2] = p3;
		iwp.line = current_line;
		iwp.col = current_col;
		add(iwp);


		if (state.EndOfCode)
			break;
	}
}


// convert human readable asm code into opcode
bool Assemble(const char *code, char *oc, int &ocs)
{
	msg_db_f("Assemble", 1+ASM_DB_LEVEL);
	/*if (!Instruction)
		SetInstructionSet(InstructionSetDefault);*/

	InstructionWithParamsList list = InstructionWithParamsList(CurrentMetaInfo->line_offset);

	list.AppendFromSource(code);

	list.Optimize(oc, ocs);

	// compile commands
	list.Compile(oc, ocs);

	return true;
}

inline bool _size_match_(InstructionParamFuzzy &inst_p, InstructionParam &wanted_p)
{
	if (inst_p.size == wanted_p.size)
		return true;
	if ((inst_p.size == SIZE_UNKNOWN) || (wanted_p.size == SIZE_UNKNOWN))
		return true;
/*	if ((inst_p.size == SizeVariable) && ((wanted_p.size == Size16) || (wanted_p.size == Size32)))
		return true;*/
	return false;
}

inline bool _deref_match_(InstructionParamFuzzy &inst_p, InstructionParam &wanted_p)
{
	if (wanted_p.deref)
		return (inst_p.allow_memory_address) || (inst_p.allow_memory_indirect);
	return true;
}

bool InstructionParamFuzzy::match(InstructionParam &wanted_p)
{
	//ParamFuzzyOut(&inst_p);
	
	// none
	if ((wanted_p.type == PARAMT_NONE) || (!used))
		return (wanted_p.type == PARAMT_NONE) && (!used);

	if ((size != SIZE_UNKNOWN) && (wanted_p.size != SIZE_UNKNOWN))
		if (size != wanted_p.size)
			return false;

	// immediate
	if (wanted_p.type == PARAMT_IMMEDIATE){
		if ((allow_memory_address) && (wanted_p.deref))
			return true;
		if ((allow_immediate) && (!wanted_p.deref)){
			//msg_write("imm " + SizeOut(inst_p.size) + " " + SizeOut(wanted_p.size));
			return (size == wanted_p.size);
		}
		return false;
	}

	// immediate double
	/*if (wanted_p.type == ParamTImmediateExt){
		msg_write("imx");
		if (allow_memory_address)
			return (size == wanted_p.size);
	}*/

	// reg
	if (wanted_p.type == PARAMT_REGISTER){
		// direct match
		if ((allow_register) && (reg)){
			if (wanted_p.reg)
				if ((reg->id >= REG_RAX) && (reg->id <= REG_RBP) && (wanted_p.reg->id == reg->id + REG_R8 - REG_RAX))
					return true;
			return ((reg == wanted_p.reg) && (_deref_match_(*this, wanted_p)));
		}
		// fuzzy match
		/*if (inst_p.allow_register){
			msg_write("r2");
			
			return ((inst_p.reg_group == wanted_p.reg->group) && (_size_match_(inst_p, wanted_p)) && (_deref_match_(inst_p, wanted_p)));
		}*/
		// very fuzzy match
		if ((allow_register) || (allow_memory_indirect)){
			if (wanted_p.deref){
				if (allow_memory_indirect)
					return ((reg_group == wanted_p.reg->group) && (_deref_match_(*this, wanted_p)));
			}else if (allow_register)
				return ((reg_group == wanted_p.reg->group) && (_size_match_(*this, wanted_p))); // FIXME (correct?)
		}
	}

	return false;
}


int GetModRMReg(Register *r)
{
	int id = r->id;
	if ((id == REG_R8)  || (id == REG_R8D)  || (id == REG_RAX) || (id == REG_EAX) || (id == REG_AX) || (id == REG_AL))	return 0x00;
	if ((id == REG_R9)  || (id == REG_R9D)  || (id == REG_RCX) || (id == REG_ECX) || (id == REG_CX) || (id == REG_CL))	return 0x01;
	if ((id == REG_R10) || (id == REG_R10D) || (id == REG_RDX) || (id == REG_EDX) || (id == REG_DX) || (id == REG_DL))	return 0x02;
	if ((id == REG_R11) || (id == REG_R11D) || (id == REG_RBX) || (id == REG_EBX) || (id == REG_BX) || (id == REG_BL))	return 0x03;
	if ((id == REG_R12) || (id == REG_R12D) || (id == REG_RSP) || (id == REG_ESP) || (id == REG_SP) || (id == REG_AH))	return 0x04;
	if ((id == REG_R13) || (id == REG_R13D) || (id == REG_RBP) || (id == REG_EBP) || (id == REG_BP) || (id == REG_CH))	return 0x05;
	if ((id == REG_R14) || (id == REG_R14D) || (id == REG_RSI) || (id == REG_ESI) || (id == REG_SI) || (id == REG_DH))	return 0x06;
	if ((id == REG_R15) || (id == REG_R15D) || (id == REG_RDI) || (id == REG_EDI) || (id == REG_DI) || (id == REG_BH))	return 0x07;
	if ((id >= REG_XMM0) && (id <= REG_XMM7))	return (id - REG_XMM0);
	SetError("GetModRMReg: register not allowed: " + r->name);
	return 0;
}

inline int CreatePartialModRMByte(InstructionParamFuzzy &pf, InstructionParam &p)
{
	int r = -1;
	if (p.reg)
		r = p.reg->id;
	if (pf.mrm_mode == MRM_REG){
		if (r == REG_ES)	return 0x00;
		if (r == REG_CS)	return 0x08;
		if (r == REG_SS)	return 0x10;
		if (r == REG_DS)	return 0x18;
		if (r == REG_FS)	return 0x20;
		if (r == REG_GS)	return 0x28;
		if (r == REG_CR0)	return 0x00;
		if (r == REG_CR1)	return 0x08;
		if (r == REG_RC2)	return 0x10;
		if (r == REG_CR3)	return 0x18;
		int mrm = GetModRMReg(p.reg) << 3;
		if (p.reg->extend_mod_rm)
			mrm += 0x0400; // REXR
		return mrm;
	}else if (pf.mrm_mode == MRM_MOD_RM){
		if (p.deref){
			if (state.AddrSize == SIZE_16){
				if ((p.type == PARAMT_IMMEDIATE) && (p.deref))	return 0x06;
			}else{
				if ((r == REG_EAX) || (r == REG_RAX))	return (p.disp == DISP_MODE_NONE) ? 0x00 : ((p.disp == DISP_MODE_8) ? 0x40 : 0x80); // default = DispMode32
				if ((r == REG_ECX) || (r == REG_RCX))	return (p.disp == DISP_MODE_NONE) ? 0x01 : ((p.disp == DISP_MODE_8) ? 0x41 : 0x81);
				if ((r == REG_EDX) || (r == REG_RDX))	return (p.disp == DISP_MODE_NONE) ? 0x02 : ((p.disp == DISP_MODE_8) ? 0x42 : 0x82);
				if ((r == REG_EBX) || (r == REG_RBX))	return (p.disp == DISP_MODE_NONE) ? 0x03 : ((p.disp == DISP_MODE_8) ? 0x43 : 0x83);
				// sib			return 4;
				// disp32		return 5;
				if ((p.type == PARAMT_IMMEDIATE) && (p.deref))	return 0x05;
				if ((r == REG_EBP) || (r == REG_RBP))	return (p.disp == DISP_MODE_8) ? 0x45 : 0x85;
				if ((r == REG_ESI) || (r == REG_RSI))	return (p.disp == DISP_MODE_NONE) ? 0x06 : ((p.disp == DISP_MODE_8) ? 0x46 : 0x86);
				if ((r == REG_EDI) || (r == REG_RDI))	return (p.disp == DISP_MODE_NONE) ? 0x07 : ((p.disp == DISP_MODE_8) ? 0x47 : 0x87);
			}
		}else{
			int mrm = GetModRMReg(p.reg) | 0xc0;
			if (p.reg->extend_mod_rm)
				mrm += 0x0100; // REXB
			return mrm;
		}
	}
	if (pf.mrm_mode != MRM_NONE)
		SetError(format("unhandled modrm %d %d %s %d %s", pf.mrm_mode, p.type, (p.reg?p.reg->name.c_str():""), p.deref, SizeOut(pf.size).c_str()));
	return 0x00;
}

int CreateModRMByte(CPUInstruction &inst, InstructionParam &p1, InstructionParam &p2)
{
	int mrm = CreatePartialModRMByte(inst.param1, p1) | CreatePartialModRMByte(inst.param2, p2);
	if (inst.cap >= 0)
		mrm |= (inst.cap << 3);
	return mrm;
}

void OpcodeAddInstruction(char *oc, int &ocs, CPUInstruction &inst, InstructionParam &p1, InstructionParam &p2, InstructionWithParamsList &list)
{
	msg_db_f("OpcodeAddInstruction", 1+ASM_DB_LEVEL);
	//---msg_write("add inst " + inst.name);

	// 16/32 bit toggle prefix
	if ((!inst.has_fixed_param) && (inst.has_small_param != (state.DefaultSize == SIZE_16)))
		append_val(oc, ocs, 0x66, 1);

	int mod_rm = 0;
	if (inst.has_modrm)
		mod_rm = CreateModRMByte(inst, p1, p2);

	// REX prefix
	char rex = mod_rm >> 8;
	if ((inst.param1.reg) && (p1.reg))
		if ((inst.param1.reg->id >= REG_RAX) && (inst.param1.reg->id <= REG_RBP) && (inst.param1.reg->id == p1.reg->id + REG_RAX - REG_R8))
			rex = 0x01;
	if (inst.has_big_param)//state.ParamSize == Size64)
		rex |= 0x08;
	if (rex != 0)
		append_val(oc, ocs, 0x40 | rex, 1);

	// add opcode
	*(int*)&oc[ocs] = inst.code;
	ocs += inst.code_size;

	// create mod/rm-byte
	if (inst.has_modrm)
		oc[ocs ++] = mod_rm;

	OCParam = ocs;

	int param2_size = 0;
	if (p2.type == PARAMT_IMMEDIATE)
		param2_size = p2.size;

	OpcodeAddImmideate(oc, ocs, p1, inst, list, param2_size);
	OpcodeAddImmideate(oc, ocs, p2, inst, list, 0);
}

void InstructionWithParamsList::AddInstruction(char *oc, int &ocs, int n)
{
	msg_db_f("AsmAddInstructionLow", 1+ASM_DB_LEVEL);

	int ocs0 = ocs;
	InstructionWithParams &iwp = (*this)[n];
	current_inst = n;
	state.reset();

	// test if any instruction matches our wishes
	int ninst = -1;
	bool has_mod_rm = false;
	foreachi(CPUInstruction &c, CPUInstructions, i)
		if ((!c.ignore) && (c.match(iwp))){
			if (((!c.has_modrm) && (has_mod_rm)) || (ninst < 0)){
				has_mod_rm = c.has_modrm;
				ninst = i;
			}
		}

/*	// try again with REX prefix?
 // now done automatically...!
	if ((ninst < 0) && (InstructionSet.set == InstructionSetAMD64)){
		state.ParamSize = Size64;

		for (int i=0;i<CPUInstructions.num;i++)
			if (CPUInstructions[i].match(iwp)){
				if (((!CPUInstructions[i].has_modrm) && (has_mod_rm)) || (ninst < 0)){
					has_mod_rm = CPUInstructions[i].has_modrm;
					ninst = i;
				}
			}

	}*/

	// none found?
	if (ninst < 0){
		state.LineNo = iwp.line;
		for (int i=0;i<NUM_INSTRUCTION_NAMES;i++)
			if (InstructionNames[i].inst == iwp.inst)
				SetError("command not compatible with its parameters\n" + iwp.str());
		SetError(format("instruction unknown: %d", iwp.inst));
	}


	if (DebugAsm)
		CPUInstructions[ninst].print();

	// compile
	OpcodeAddInstruction(oc, ocs, CPUInstructions[ninst], iwp.p[0], iwp.p[1], *this);
	iwp.size = ocs - ocs0;

	//msg_write(d2h(&oc[ocs0], ocs - ocs0, false));
}

int arm_reg_no(Register *r)
{
	if (r)
		if ((r->id >= REG_R0) and (r->id <= REG_R15))
			return r->id - REG_R0;
	SetError("ARM: invalid register: " + r->name);
	return -1;
}

int arm_encode_imm(unsigned int value)
{
	for (int ex=0; ex<=30; ex+=2){
		unsigned int mask = (0xffffff00 >> ex) | (0xffffff00 << (32-ex));
		if ((value & mask) == 0){
			unsigned int mant = (value << ex) | (value >> (32 - ex));
			return mant | (ex << (8-1));
		}
	}
	SetError("ARM: immediate value not representable: " + i2s(value));
	return 0;
}

bool inline arm_is_load_store_reg(int inst)
{
	return (inst == inst_ldr) or (inst == inst_ldrb) or (inst == inst_str) or (inst == inst_strb);
}

bool inline arm_is_load_store_multi(int inst)
{
	if ((inst == inst_ldmia) or (inst == inst_ldmib) or (inst == inst_ldmda) or (inst == inst_ldmdb))
		return true;
	if ((inst == inst_stmia) or (inst == inst_stmib) or (inst == inst_stmda) or (inst == inst_stmdb))
		return true;
	return false;
}

void arm_expect(InstructionWithParams &c, int type0 = PARAMT_NONE, int type1 = PARAMT_NONE, int type2 = PARAMT_NONE)
{
	int t[3] = {type0, type1, type2};
	for (int i=0; i<3; i++)
		if (c.p[i].type != t[i])
			SetError(format("param #%d expected to be %s: ", i+1, "???") + c.str());
}

void InstructionWithParamsList::AddInstructionARM(char *oc, int &ocs, int n)
{
	msg_db_f("AsmAddInstructionLowARM", 1+ASM_DB_LEVEL);

	InstructionWithParams &iwp = (*this)[n];
	current_inst = n;
	state.reset();

	int code = 0;

	code = iwp.condition << 28;
	int nn = -1;
	for (int i=0; i<16; i++)
		if (iwp.inst == ARMDataInstructions[i])
			nn = i;
	if (nn >= 0){
		if ((iwp.inst == inst_cmp) or (iwp.inst == inst_cmn) or (iwp.inst == inst_tst) or (iwp.inst == inst_teq) or (iwp.inst == inst_mov)){
			iwp.p[2] = iwp.p[1];
			if ((iwp.inst == inst_cmp) or (iwp.inst == inst_cmn)){
				iwp.p[1] = iwp.p[0];
				iwp.p[0] = param_reg(REG_R0);
			}else{
				iwp.p[1] = param_reg(REG_R0);
			}
		}
		bool ss = (iwp.inst == inst_cmp) or (iwp.inst == inst_cmn) or (iwp.inst == inst_tst) or (iwp.inst == inst_teq);
		code |= 0x0 << 26;
		code |= (nn << 21);
		if (ss)
			code |= 1 << 20;
		if (iwp.p[2].type == PARAMT_REGISTER){
			arm_expect(iwp, PARAMT_REGISTER, PARAMT_REGISTER, PARAMT_REGISTER);
			code |= arm_reg_no(iwp.p[2].reg) << 0;
			if (iwp.p[2].disp != DISP_MODE_NONE)
				SetError("p3.disp != DISP_MODE_NONE");
		}else if (iwp.p[2].type == PARAMT_IMMEDIATE){
			arm_expect(iwp, PARAMT_REGISTER, PARAMT_REGISTER, PARAMT_IMMEDIATE);
			code |= arm_encode_imm(iwp.p[2].value) << 0;
			code |= 1 << 25;
		}/*else if (iwp.p[2].type == PARAMT_REGISTER_SHIFT){
			msg_write("TODO reg shift");
			code |= arm_reg_no(iwp.p[2].reg) << 0;
			code |= (iwp.p[2].value & 0xff) << 4;
		}*/ else{
			SetError("unhandled param #3 in " + iwp.str());
		}
		code |= arm_reg_no(iwp.p[0].reg) << 12;
		code |= arm_reg_no(iwp.p[1].reg) << 16;
	}else if (arm_is_load_store_reg(iwp.inst)){
		if (iwp.inst == inst_ldr)
			code |= 0x04100000;
		else if (iwp.inst == inst_ldrb)
			code |= 0x04500000;
		else if (iwp.inst == inst_str)
			code |= 0x04000000;
		else if (iwp.inst == inst_strb)
			code |= 0x04400000;

		code |= arm_reg_no(iwp.p[0].reg) << 12;
		code |= arm_reg_no(iwp.p[1].reg) << 16;

		if ((iwp.p[1].disp == DISP_MODE_8) or (iwp.p[1].disp == DISP_MODE_32)){
			if (iwp.p[1].value >= 0)
				code |= 0x01800000 | iwp.p[1].value;
			else
				code |= 0x01000000 | (-iwp.p[1].value);
		}else if (iwp.p[1].disp == DISP_MODE_REG2){
			if (iwp.p[1].value >= 0)
				code |= 0x03800000;
			else
				code |= 0x03000000;
			code |= arm_reg_no(iwp.p[1].reg2);
		}
	}else if (arm_is_load_store_multi(iwp.inst)){
		arm_expect(iwp, PARAMT_REGISTER, PARAMT_IMMEDIATE);
		bool ll = ((iwp.inst == inst_ldmia) or (iwp.inst == inst_ldmib) or (iwp.inst == inst_ldmda) or (iwp.inst == inst_ldmdb));
		bool uu = ((iwp.inst == inst_ldmia) or (iwp.inst == inst_ldmib) or (iwp.inst == inst_stmia) or (iwp.inst == inst_stmib));
		bool pp = ((iwp.inst == inst_ldmib) or (iwp.inst == inst_ldmdb) or (iwp.inst == inst_stmib) or (iwp.inst == inst_stmdb));
		bool ww = true;
		if (ll)
			code |= 0x08100000;
		else
			code |= 0x08000000;
		if (uu)
			code |= 0x00800000;
		if (pp)
			code |= 0x01000000;
		if (ww)
			code |= 0x00200000;
		code |= arm_reg_no(iwp.p[0].reg) << 16;
		code |= iwp.p[1].value & 0xffff;
	}else if (iwp.inst == inst_mul){
		code |= 0x00000090;
		arm_expect(iwp, PARAMT_REGISTER, PARAMT_REGISTER, PARAMT_REGISTER);
		code |= arm_reg_no(iwp.p[0].reg) << 16;
		code |= arm_reg_no(iwp.p[1].reg);
		code |= arm_reg_no(iwp.p[2].reg) << 8;
	}else if (iwp.inst == inst_call){
		arm_expect(iwp, PARAMT_IMMEDIATE);
		// bl
		code |= 0x0b000000;
		int value = (iwp.p[0].value - (long)&oc[ocs] - 8) >> 2;
		code |= (value & 0x00ffffff);
	}else if ((iwp.inst == inst_bl) or (iwp.inst == inst_b) or (iwp.inst == inst_jmp)){
		arm_expect(iwp, PARAMT_IMMEDIATE);
		if (iwp.inst == inst_bl)
			code |= 0x0b000000;
		else
			code |= 0x0a000000;
		code |= (iwp.p[0].value & 0x00ffffff);
	}else if (iwp.inst == inst_dd){
		arm_expect(iwp, PARAMT_IMMEDIATE);
		code = iwp.p[0].value;
	}else{
		SetError("cannot assemble instruction: " + iwp.str());
	}

	*(int*)&oc[ocs] = code;
	ocs += 4;
}

void InstructionWithParamsList::ShrinkJumps(void *oc, int ocs)
{
	// first pass compilation (we need real jump distances)
	int _ocs = ocs;
	Compile(oc, _ocs);
	wanted_label.clear();

	// try shrinking
	foreachi(InstructionWithParams &iwp, *this, i){
		if ((iwp.inst == inst_jmp) || (iwp.inst == inst_jz) || (iwp.inst == inst_jnz) || (iwp.inst == inst_jl) || (iwp.inst == inst_jnl) || (iwp.inst == inst_jle) || (iwp.inst == inst_jnle)){
			if (iwp.p[0].is_label){
				int target = label[(int)iwp.p[0].value].inst_no;

				// jump distance
				int dist = 0;
				for (int j=i+1;j<target;j++)
					dist += (*this)[j].size;
				for (int j=target;j<=i;j++)
					dist += (*this)[j].size;
				//msg_write(format("%d %d   %d", i, target, dist));

				if (dist < 127){
					so("really shrink");
					iwp.p[0].size = SIZE_8;
				}
			}
		}
	}
}

void InstructionWithParamsList::Optimize(void *oc, int ocs)
{
	if (InstructionSet.set != INSTRUCTION_SET_ARM)
		ShrinkJumps(oc, ocs);
}

void InstructionWithParamsList::Compile(void *oc, int &ocs)
{
	state.DefaultSize = SIZE_32;
	state.reset();
	if (!CurrentMetaInfo){
		DummyMetaInfo.code_origin = (long)oc;
		CurrentMetaInfo = &DummyMetaInfo;
	}

	for (int i=0;i<num+1;i++){
		// bit change
		foreach(BitChange &b, CurrentMetaInfo->bit_change)
			if (b.cmd_pos == i){
				state.DefaultSize = SIZE_32;
				if (b.bits == 16)
					state.DefaultSize = SIZE_16;
				state.reset();
				b.offset = ocs;
			}

		// data?
		foreach(AsmData &d, CurrentMetaInfo->data)
			if (d.cmd_pos == i)
				d.offset = ocs;

		// defining a label?
		for (int j=0;j<label.num;j++)
			if (i == label[j].inst_no){
				so("defining found: " + label[j].name);
				label[j].value = CurrentMetaInfo->code_origin + ocs;
			}
		if (i >= num)
			break;

		// opcode
		if (InstructionSet.set == INSTRUCTION_SET_ARM)
			AddInstructionARM((char*)oc, ocs, i);
		else
			AddInstruction((char*)oc, ocs, i);
	}

	LinkWantedLabels(oc);

	foreach(WantedLabel &l, wanted_label){
		if (l.name.head(10) == "kaba-func:")
			continue;
		state.LineNo = (*this)[l.inst_no].line;
		state.ColumnNo = (*this)[l.inst_no].col;
		SetError("undeclared label used: " + l.name);
	}
}

void AddInstruction(char *oc, int &ocs, int inst, const InstructionParam &p1, const InstructionParam &p2, const InstructionParam &p3)
{
	msg_db_f("AsmAddInstruction", 1+ASM_DB_LEVEL);
	/*if (!CPUInstructions)
		SetInstructionSet(InstructionSetDefault);*/
	state.DefaultSize = SIZE_32;
	state.reset();
	/*msg_write("--------");
	for (int i=0;i<NUM_INSTRUCTION_NAMES;i++)
		if (InstructionName[i].inst == inst)
			printf("%s\n", InstructionName[i].name);*/

	OCParam = ocs;
	InstructionWithParamsList list = InstructionWithParamsList(0);
	InstructionWithParams iwp;
	iwp.inst = inst;
	iwp.p[0] = p1;
	iwp.p[1] = p2;
	iwp.p[2] = p3;
	iwp.line = -1;
	list.add(iwp);
	list.AddInstruction(oc, ocs, 0);
}

bool ImmediateAllowed(int inst)
{
	for (int i=0;i<CPUInstructions.num;i++)
		if (CPUInstructions[i].inst == inst)
			if ((CPUInstructions[i].param1.allow_immediate) || (CPUInstructions[i].param2.allow_immediate))
				return true;
	return false;
}

};
