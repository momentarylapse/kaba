#if !defined(ASM_H__INCLUDED_)
#define ASM_H__INCLUDED_


namespace Asm
{

// instruction sets
enum{
	INSTRUCTION_SET_X86,
	INSTRUCTION_SET_AMD64,
	INSTRUCTION_SET_ARM
};

struct InstructionSetData
{
	int set;
	int pointer_size;
};

extern InstructionSetData InstructionSet;

// single registers
enum{
	REG_EAX, REG_ECX, REG_EDX, REG_EBX, REG_ESP, REG_ESI, REG_EDI, REG_EBP, // 4 byte
	REG_AX, REG_CX, REG_DX, REG_BX, REG_BP, REG_SP, REG_SI, REG_DI, // 2 byte
	REG_AL, REG_CL, REG_DL, REG_BL, REG_AH, REG_CH, REG_DH, REG_BH, // 1 byte
	REG_CS, REG_DS, REG_SS, REG_ES, REG_FS, REG_GS, // segment
	REG_CR0, REG_CR1, REG_RC2, REG_CR3,
	REG_ST0, REG_ST1, REG_ST2, REG_ST3, REG_ST4, REG_ST5, REG_ST6, REG_ST7,
	REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RSI, REG_RDI, REG_RBP, // 8 byte
	REG_R0, REG_R1, REG_R2, REG_R3, REG_R4, REG_R5, REG_R6, REG_R7, // ARM
	REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15, // ARM 4 byte / AMD64 8 byte
	REG_R8D, REG_R9D, REG_R10D, REG_R11D, REG_R12D, REG_R13D, REG_R14D, REG_R15D,
	REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3, REG_XMM4, REG_XMM5, REG_XMM6, REG_XMM7, // 16 byte
	NUM_REGISTERS
};

const int NUM_REG_ROOTS = 40;
const int MAX_REG_SIZE = 16;

extern int RegRoot[];
extern int RegResize[NUM_REG_ROOTS][MAX_REG_SIZE + 1];
string GetRegName(int reg);



enum{
	inst_db, // data instructions
	inst_dw,
	inst_dd,
	inst_ds,
	inst_dz,

	inst_add,
	inst_adc,	   // add with carry
	inst_sub,
	inst_sbb,	   // subtract with borrow
	inst_inc,
	inst_dec,
	inst_mul,
	inst_imul,
	inst_div,
	inst_idiv,
	inst_mov,
	inst_movzx,
	inst_movsx,
	inst_and,
	inst_or,
	inst_xor,
	inst_not,
	inst_neg,
	inst_pop,
	inst_popa,
	inst_push,
	inst_pusha,
	
	inst_jo,
	inst_jno,
	inst_jb,
	inst_jnb,
	inst_jz,
	inst_jnz,
	inst_jbe,
	inst_jnbe,
	inst_js,
	inst_jns,
	inst_jp,
	inst_jnp,
	inst_jl,
	inst_jnl,
	inst_jle,
	inst_jnle,
	
	inst_cmp,
	
	inst_seto,
	inst_setno,
	inst_setb,
	inst_setnb,
	inst_setz,
	inst_setnz,
	inst_setbe,
	inst_setnbe,
	inst_sets,
	inst_setns,
	inst_setp,
	inst_setnp,
	inst_setl,
	inst_setnl,
	inst_setle,
	inst_setnle,
	
	inst_sldt,
	inst_str,
	inst_lldt,
	inst_ltr,
	inst_verr,
	inst_verw,
	inst_sgdt,
	inst_sidt,
	inst_lgdt,
	inst_lidt,
	inst_smsw,
	inst_lmsw,
	
	inst_test,
	inst_xchg,
	inst_lea,
	inst_nop,
	inst_cbw_cwde,
	inst_cgq_cwd,
	inst_movs_ds_esi_es_edi,	// mov string
	inst_movs_b_ds_esi_es_edi,
	inst_cmps_ds_esi_es_edi,	// cmp string
	inst_cmps_b_ds_esi_es_edi,
	inst_rol,
	inst_ror,
	inst_rcl,
	inst_rcr,
	inst_shl,
	inst_shr,
	inst_sar,
	inst_ret,
	inst_leave,
	inst_ret_far,
	inst_int,
	inst_iret,
	
	inst_fadd,
	inst_fmul,
	inst_fsub,
	inst_fdiv,
	inst_fld,
	inst_fld1,
	inst_fldz,
	inst_fldpi,
	inst_fxch,
	inst_fst,
	inst_fstp,
	inst_fild,
	inst_faddp,
	inst_fmulp,
	inst_fsubp,
	inst_fdivp,
	inst_fldcw,
	inst_fnstcw,
	inst_fnstsw,
	inst_fistp,
	inst_fsqrt,
	inst_fsin,
	inst_fcos,
	inst_fptan,
	inst_fpatan,
	inst_fyl2x,
	inst_fchs,
	inst_fabs,
	inst_fucompp,
	
	inst_loop,
	inst_loope,
	inst_loopne,
	inst_in,
	inst_out,
	
	inst_call,
	inst_call_far,
	inst_jmp,
	inst_jmp_far,
	inst_lock,
	inst_rep,
	inst_repne,
	inst_hlt,
	inst_cmc,
	inst_clc,
	inst_stc,
	inst_cli,
	inst_sti,
	inst_cld,
	inst_std,

	inst_movss,
	inst_movsd,
	
	// ARM
	inst_b,
	inst_bl,

	inst_ldr,
	inst_ldrb,
//	inst_str,
	inst_strb,

	inst_ldmia,
	inst_ldmib,
	inst_ldmda,
	inst_ldmdb,
	inst_stmia,
	inst_stmib,
	inst_stmda,
	inst_stmdb,

	inst_eor,
	inst_rsb,
	inst_sbc,
	inst_rsc,
	inst_tst,
	inst_teq,
	inst_cmn,
	inst_orr,
	inst_bic,
	inst_mvn,

	NUM_INSTRUCTION_NAMES
};

enum
{
	ARM_COND_EQUAL,
	ARM_COND_NOT_EQUAL,
	ARM_COND_CARRY_SET,
	ARM_COND_CARRY_CLEAR,
	ARM_COND_NEGATIVE,
	ARM_COND_POSITIVE,
	ARM_COND_OVERFLOW,
	ARM_COND_NO_OVERFLOW,
	ARM_COND_UNSIGNED_HIGHER,
	ARM_COND_UNSIGNED_LOWER_SAME,
	ARM_COND_GREATER_EQUAL,
	ARM_COND_LESS_THAN,
	ARM_COND_GREATER_THAN,
	ARM_COND_LESS_EQUAL,
	ARM_COND_ALWAYS,
	ARM_COND_UNKNOWN,
};

string GetInstructionName(int inst);

struct GlobalVar
{
	string name;
	void *pos; // points into the memory of a script
	int size;
};

struct Label
{
	string name;
	int inst_no;
	long long value;
};

struct WantedLabel
{
	string name;
	int pos; // position to fill into     relative to CodeOrigin (Opcode[0])
	int size; // number of bytes to fill
	int add; // to add to the value...
	int label_no;
	int inst_no;
	bool relative;
	bool abs;
};

struct AsmData
{
	int size; // number of bytes
	int cmd_pos;
	int offset; // relative to code_origin (Opcode[0])
	//void *data;
};

struct BitChange
{
	int cmd_pos;
	int offset; // relative to code_origin (Opcode[0])
	int bits;
};

struct MetaInfo
{
	long long code_origin; // how to interpret opcode buffer[0]
	bool mode16;
	int line_offset; // number of script lines preceding asm block (to give correct error messages)

	//Array<Label> label;
	//Array<WantedLabel> wanted_label;

	Array<AsmData> data;
	Array<BitChange> bit_change;
	Array<GlobalVar> global_var;

	MetaInfo();
};


struct Register;

// a real parameter (usable)
struct InstructionParam
{
	InstructionParam();
	int type;
	int disp;
	Register *reg, *reg2;
	bool deref;
	int size;
	long long value; // disp or immediate
	bool is_label;
	bool write_back;
	string str(bool hide_size = false);
};

struct InstructionWithParams
{
	int inst;
	int condition; // ARM
	InstructionParam p[3];
	int line, col;
	int size;
	int addr_size;
	int param_size;
	string str(bool hide_size = false);
};


enum
{
	SIZE_8 = 1,
	SIZE_16 = 2,
	SIZE_24 = 3,
	SIZE_32 = 4,
	SIZE_48 = 6,
	SIZE_64 = 8,
	SIZE_128 = 16,
	SIZE_8L4 = -13,
	/*SIZE_VARIABLE = -5,
	SIZE_32OR48 = -6,*/
	SIZE_UNKNOWN = -7,
};

extern InstructionParam param_none;
InstructionParam param_reg(int reg);
InstructionParam param_reg_set(int set);
InstructionParam param_deref_reg(int reg, int size);
InstructionParam param_deref_reg_shift(int reg, int shift, int size);
InstructionParam param_deref_reg_shift_reg(int reg, int reg2, int size);
InstructionParam param_imm(long long value, int size);
InstructionParam param_deref_imm(long long value, int size);
InstructionParam param_label(long long value, int size);

struct InstructionWithParamsList : public Array<InstructionWithParams>
{
	InstructionWithParamsList(int line_offset);
	~InstructionWithParamsList();

//	void add_easy(int inst, int param1_type = PK_NONE, int param1_size = -1, void *param1 = NULL, int param2_type = PK_NONE, int param2_size = -1, void *param2 = NULL);
	void add2(int inst, const InstructionParam &p1 = param_none, const InstructionParam &p2 = param_none);
	void add_arm(int cond, int inst, const InstructionParam &p1, const InstructionParam &p2 = param_none, const InstructionParam &p3 = param_none);

	int add_label(const string &name);
	int get_label(const string &name);
	void *get_label_value(const string &name);

	void add_wanted_label(int pos, int label_no, int inst_no, bool rel, bool abs, int size);

	void add_func_intro(int stack_alloc_size);
	void add_func_return(int return_size);

	void AppendFromSource(const string &code);
	void ShrinkJumps(void *oc, int ocs);
	void Optimize(void *oc, int ocs);
	void Compile(void *oc, int &ocs);
	void LinkWantedLabels(void *oc);
	void AddInstruction(char *oc, int &ocs, int n);
	void AddInstructionARM(char *oc, int &ocs, int n);

	void show();

	Array<Label> label;
	Array<WantedLabel> wanted_label;
	int current_line;
	int current_col;
	int current_inst;
};

void Init(int instruction_set = -1);
int QueryLocalInstructionSet();
bool Assemble(const char *code, char *oc, int &ocs);
string Disassemble(void *code, int length = -1, bool allow_comments = true);

class Exception
{
public:
	Exception(const string &message, const string &expression, int line, int column);
	virtual ~Exception();
	void print() const;
	string message;
	int line, column;
};

void AddInstruction(char *oc, int &ocs, int inst, const InstructionParam &p1, const InstructionParam &p2 = param_none, const InstructionParam &p3 = param_none);
void SetInstructionSet(int set);
bool ImmediateAllowed(int inst);
extern int OCParam;
extern MetaInfo *CurrentMetaInfo;

void GetInstructionParamFlags(int inst, bool &p1_read, bool &p1_write, bool &p2_read, bool &p2_write);
bool GetInstructionAllowConst(int inst);
bool GetInstructionAllowGenReg(int inst);

};

#endif
