#if !defined(ASM_H__INCLUDED_)
#define ASM_H__INCLUDED_


namespace Asm
{

// instruction sets
enum class InstructionSet {
	NATIVE = -1,
	X86,
	AMD64,
	ARM
};

struct InstructionSetData {
	InstructionSet set;
	int pointer_size;
};

extern InstructionSetData instruction_set;

// single registers
enum class RegID {
	INVALID = -1,
	EAX, ECX, EDX, EBX, ESP, ESI, EDI, EBP, // 4 byte
	AX, CX, DX, BX, BP, SP, SI, DI, // 2 byte
	AL, CL, DL, BL, AH, CH, DH, BH, // 1 byte
	CS, DS, SS, ES, FS, GS, // segment
	CR0, CR1, RC2, CR3, CR4,
	ST0, ST1, ST2, ST3, ST4, ST5, ST6, ST7,
	RAX, RCX, RDX, RBX, RSP, RSI, RDI, RBP, // 8 byte
	R0, R1, R2, R3, R4, R5, R6, R7, // ARM
	R8, R9, R10, R11, R12, R13, R14, R15, // ARM 4 byte / AMD64 8 byte
	R8D, R9D, R10D, R11D, R12D, R13D, R14D, R15D,
	XMM0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, // 16 byte
	S0,  S1,  S2,  S3,  S4,  S5,  S6,  S7, // ARM float
	S8,  S9,  S10, S11, S12, S13, S14, S15,
	S16, S17, S18, S19, S20, S21, S22, S23,
	S24, S25, S26, S27, S28, S29, S30, S31,
	COUNT
};

const int NUM_REG_ROOTS = 40;
const int MAX_REG_SIZE = 16;

extern int RegRoot[];
extern RegID RegResize[NUM_REG_ROOTS][MAX_REG_SIZE + 1];
string get_reg_name(RegID reg);



enum {
	// data instructions
	INST_DB,
	INST_DW,
	INST_DD,
	INST_DS,
	INST_DZ,
	INST_ALIGN_OPCODE,

	INST_ADD,
	INST_ADC,	   // add with carry
	INST_SUB,
	INST_SBB,	   // subtract with borrow
	INST_INC,
	INST_DEC,
	INST_MUL,
	INST_IMUL,
	INST_DIV,
	INST_IDIV,
	INST_MOV,
	INST_MOVZX,
	INST_MOVSX,
	INST_MOVSXD,
	INST_AND,
	INST_OR,
	INST_XOR,
	INST_NOT,
	INST_NEG,
	INST_POP,
	INST_POPA,
	INST_PUSH,
	INST_PUSHA,
	
	INST_JO,
	INST_JNO,
	INST_JB,
	INST_JNB,
	INST_JZ,
	INST_JNZ,
	INST_JBE,
	INST_JNBE,
	INST_JS,
	INST_JNS,
	INST_JP,
	INST_JNP,
	INST_JL,
	INST_JNL,
	INST_JLE,
	INST_JNLE,
	
	INST_CMP,
	
	INST_SETO,
	INST_SETNO,
	INST_SETB,
	INST_SETNB,
	INST_SETZ,
	INST_SETNZ,
	INST_SETBE,
	INST_SETNBE,
	INST_SETS,
	INST_SETNS,
	INST_SETP,
	INST_SETNP,
	INST_SETL,
	INST_SETNL,
	INST_SETLE,
	INST_SETNLE,
	
	INST_SLDT,
	INST_STR,
	INST_LLDT,
	INST_LTR,
	INST_VERR,
	INST_VERW,
	INST_SGDT,
	INST_SIDT,
	INST_LGDT,
	INST_LIDT,
	INST_SMSW,
	INST_LMSW,
	
	INST_TEST,
	INST_XCHG,
	INST_LEA,
	INST_NOP,
	INST_CBW_CWDE,
	INST_CGQ_CWD,
	INST_MOVS_DS_ESI_ES_EDI,	// mov string
	INST_MOVS_B_DS_ESI_ES_EDI,
	INST_CMPS_DS_ESI_ES_EDI,	// cmp string
	INST_CMPS_B_DS_ESI_ES_EDI,
	INST_ROL,
	INST_ROR,
	INST_RCL,
	INST_RCR,
	INST_SHL,
	INST_SHR,
	INST_SAR,
	INST_RET,
	INST_LEAVE,
	INST_RET_FAR,
	INST_INT,
	INST_IRET,
	
	// x87
	INST_FADD,
	INST_FMUL,
	INST_FSUB,
	INST_FDIV,
	INST_FLD,
	INST_FLD1,
	INST_FLDZ,
	INST_FLDPI,
	INST_FXCH,
	INST_FST,
	INST_FSTP,
	INST_FILD,
	INST_FADDP,
	INST_FMULP,
	INST_FSUBP,
	INST_FDIVP,
	INST_FLDCW,
	INST_FNSTCW,
	INST_FNSTSW,
	INST_FISTP,
	INST_FSQRT,
	INST_FSIN,
	INST_FCOS,
	INST_FPTAN,
	INST_FPATAN,
	INST_FYL2X,
	INST_FCHS,
	INST_FABS,
	INST_FUCOMPP,
	
	INST_LOOP,
	INST_LOOPE,
	INST_LOOPNE,
	INST_IN,
	INST_OUT,
	
	INST_CALL,
	INST_CALL_FAR,
	INST_JMP,
	INST_JMP_FAR,
	INST_LOCK,
	INST_REP,
	INST_REPNE,
	INST_HLT,
	INST_CMC,
	INST_CLC,
	INST_STC,
	INST_CLI,
	INST_STI,
	INST_CLD,
	INST_STD,

	// sse
	INST_MOVSS,
	INST_MOVSD,
	INST_MOVUPS,
	INST_MOVAPS,
	INST_MOVLPS,
	INST_MOVHPS,
	INST_ADDSS,
	INST_ADDSD,
	INST_ADDPS,
	INST_SUBSS,
	INST_SUBSD,
	INST_MULSS,
	INST_MULSD,
	INST_DIVSS,
	INST_DIVSD,
	INST_SQRTSS,
	INST_SQRTSD,
	INST_MINSS,
	INST_MINSD,
	INST_MAXSS,
	INST_MAXSD,
	INST_CVTSS2SD,
	INST_CVTSD2SS,
	INST_CVTTSS2SI,
	INST_CVTTSD2SI,
	INST_CVTSI2SS,
	INST_CVTSI2SD,
	INST_COMISS,
	INST_COMISD,
	INST_UCOMISS,
	INST_UCOMISD,

	INST_SYSCALL,
	INST_SYSRET,
	INST_SYSENTER,
	INST_SYSEXIT,

	
	// ARM
	INST_B,
	INST_BL,
	INST_BLX,

	INST_MULS,
	INST_ADDS,
	INST_SUBS,
	INST_RSBS,
	INST_ADCS,
	INST_SBCS,
	INST_RSCS,
	INST_ANDS,
	INST_BICS,
	INST_XORS,
	INST_ORS,
	INST_MOVS,
	INST_MVNS,

	INST_LDR,
	INST_LDRB,
//	INST_STR,
	INST_STRB,

	INST_LDMIA,
	INST_LDMIB,
	INST_LDMDA,
	INST_LDMDB,
	INST_STMIA,
	INST_STMIB,
	INST_STMDA,
	INST_STMDB,

	INST_RSB,
	INST_SBC,
	INST_RSC,
	INST_TST,
	INST_TEQ,
	INST_CMN,
	INST_BIC,
	INST_MVN,

	// ARM float
	INST_FMACS,
	INST_FNMACS,
	INST_FMSCS,
	INST_FNMSCS,
	INST_FMULS,
	INST_FNMULS,
	INST_FADDS,
	INST_FSUBS,
	INST_FDIVS,
	INST_FCPYS,
	INST_FABSS,
	INST_FNEGS,
	INST_FSQRTS,
	INST_FCMPS,
	INST_FCMPES,
	INST_FCMPZS,
	INST_FCMPEZS,
	INST_CVTDS,
	INST_FTOUIS,
	INST_FTOUIZS,
	INST_FTOSIS,
	INST_FTOSIZS,
	INST_FUITOS,
	INST_FSITOS,
	INST_FMRS,
	INST_FMSR,
	INST_FLDS,
	INST_FSTS,

	// fake
	INST_MODULO,

	NUM_INSTRUCTION_NAMES
};

enum {
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

const string GetInstructionName(int inst);

struct GlobalVar {
	string name;
	void *pos; // points into the memory of a script
	int size;
};

struct Label {
	string name;
	int inst_no;
	int64 value;
};

struct WantedLabel {
	string name;
	int pos; // position to fill into     relative to CodeOrigin (Opcode[0])
	int size; // number of bytes to fill
	int add; // to add to the value...
	int label_no;
	int inst_no;
	bool relative;
	bool abs;
};

struct AsmData {
	int size; // number of bytes
	int cmd_pos;
	int offset; // relative to code_origin (Opcode[0])
	//void *data;
};

struct BitChange {
	int cmd_pos;
	int offset; // relative to code_origin (Opcode[0])
	int bits;
};

struct MetaInfo {
	int64 code_origin; // how to interpret opcode buffer[0]
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
struct InstructionParam {
	InstructionParam();
	int type;
	int disp;
	Register *reg, *reg2;
	bool deref;
	int size;
	int64 value; // disp or immediate
	bool is_label;
	bool write_back;
	string str(bool hide_size = false);
};

struct InstructionWithParams {
	int inst;
	int condition; // ARM
	InstructionParam p[3];
	int line, col;
	int size;
	int addr_size;
	int param_size;
	string str(bool hide_size = false);
};


enum {
	SIZE_8 = 1,
	SIZE_16 = 2,
	SIZE_24 = 3,
	SIZE_32 = 4,
	SIZE_48 = 6,
	SIZE_64 = 8,
	SIZE_128 = 16,
	SIZE_8L4 = -13,
	SIZE_12 = -14,
	SIZE_8S2 = -15,
	/*SIZE_VARIABLE = -5,
	SIZE_32OR48 = -6,*/
	SIZE_UNKNOWN = -7,
};

extern InstructionParam param_none;
InstructionParam param_reg(RegID reg);
InstructionParam param_reg_set(int set);
InstructionParam param_deref_reg(RegID reg, int size);
InstructionParam param_deref_reg_shift(RegID reg, int shift, int size);
InstructionParam param_deref_reg_shift_reg(RegID reg, RegID reg2, int size);
InstructionParam param_imm(int64 value, int size);
InstructionParam param_deref_imm(int64 value, int size);
InstructionParam param_label(int64 value, int size);
InstructionParam param_deref_label(int64 value, int size);

struct InstructionWithParamsList : public Array<InstructionWithParams> {
	InstructionWithParamsList(int line_offset);
	~InstructionWithParamsList();

//	void add_easy(int inst, int param1_type = PK_NONE, int param1_size = -1, void *param1 = NULL, int param2_type = PK_NONE, int param2_size = -1, void *param2 = NULL);
	void add2(int inst, const InstructionParam &p1 = param_none, const InstructionParam &p2 = param_none);
	void add_arm(int cond, int inst, const InstructionParam &p1, const InstructionParam &p2 = param_none, const InstructionParam &p3 = param_none);


	// new label system
	int create_label(const string &name);
	int _find_label(const string &name);
	void insert_label(int index);
	int64 _label_value(int index);


	int add_label(const string &name);
	int get_label(const string &name);
	void *get_label_value(const string &name);

	void add_wanted_label(int pos, int label_no, int inst_no, bool rel, bool abs, int size);

	void append_from_source(const string &code);
	void shrink_jumps(void *oc, int ocs);
	void optimize(void *oc, int ocs);
	void compile(void *oc, int &ocs);
	void link_wanted_labels(void *oc);
	void add_instruction(char *oc, int &ocs, int n);
	void add_instruction_arm(char *oc, int &ocs, int n);

	void show();

	Array<Label> label;
	Array<WantedLabel> wanted_label;
	int current_line;
	int current_col;
	int current_inst;
};

void init(InstructionSet instruction_set = InstructionSet::NATIVE);
InstructionSet guess_native_instruction_set();
bool assemble(const char *code, char *oc, int &ocs);
string disassemble(void *code, int length = -1, bool allow_comments = true);

class Exception : public ::Exception {
public:
	Exception(const string &message, const string &expression, int line, int column);
	~Exception() override;
	void print() const;
	int line, column;
};

void add_instruction(char *oc, int &ocs, int inst, const InstructionParam &p1, const InstructionParam &p2 = param_none, const InstructionParam &p3 = param_none);
void set_instructionSet(int set);
bool immediate_allowed(int inst);
extern int OCParam;
extern MetaInfo *CurrentMetaInfo;

void get_instruction_param_flags(int inst, bool &p1_read, bool &p1_write, bool &p2_read, bool &p2_write);
bool get_instruction_allow_const(int inst);
bool get_instruction_allow_gen_reg(int inst);

};

#endif
