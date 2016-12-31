
#ifndef SERIALIZER_H_
#define SERIALIZER_H_

namespace Kaba
{


#define max_reg			8 // >= all RegXXX used...

// represents a register
// (or rather the data inside, since many VirtualRegisters might be mapped to the same physical register)
struct VirtualRegister
{
	int reg;
	int reg_root;
	int first, last;
};

// high level instructions
enum{
	INST_MARKER = 10000,
	INST_ASM,
};

struct LoopData
{
	int marker_continue, marker_break;
	int level, index;
};


struct SerialCommandParam
{
	int kind;
	long long p;
	int virt; // virtual register (if p represents a physical register)
	Class *type;
	int shift;
	//int c_id, v_id;
	bool operator == (const SerialCommandParam &param) const
	{	return (kind == param.kind) && (p == param.p) && (type == param.type) && (shift == param.shift);	}
	string str() const;
};

#define SERIAL_COMMAND_NUM_PARAMS	3

struct SerialCommand
{
	int inst;
	int cond;
	SerialCommandParam p[SERIAL_COMMAND_NUM_PARAMS];
	int index;
	string str() const;
};

struct TempVar
{
	Class *type;
	int first, last, usage_count;
	bool mapped;
	bool referenced;
	bool force_stack;
	int stack_offset;
	int entangled;
	void use(int first, int last);
};

struct AddLaterData
{
	int kind, label, level, index;
};

enum{
	STUFF_KIND_MARKER,
	STUFF_KIND_JUMP,
};


class Serializer
{
public:
	Serializer(Script *script, Asm::InstructionWithParamsList *list);
	virtual ~Serializer();

	Array<SerialCommand> cmd;
	int num_markers;
	Script *script;
	SyntaxTree *syntax_tree;
	Function *cur_func;
	int cur_func_index;
	bool call_used;
	Command *next_command;
	int next_cmd_index;

	Array<int> map_reg_root;
	Array<VirtualRegister> virtual_reg;

	bool reg_root_used[max_reg];
	Array<LoopData> loop;

	int stack_offset, stack_max_size, max_push_size;
	Array<TempVar> temp_var;

	Array<AddLaterData> add_later;

	struct GlobalRef
	{
		int label;
		void *p;
	};
	Array<GlobalRef> global_refs;
	int add_global_ref(void *p);

	Asm::InstructionWithParamsList *list;

	void DoError(const string &msg);
	void DoErrorLink(const string &msg);

	void Assemble();
	void assemble_cmd(SerialCommand &c);
	void assemble_cmd_arm(SerialCommand &c);
	Asm::InstructionParam get_param(int inst, SerialCommandParam &p);

	void SerializeFunction(Function *f);
	void SerializeBlock(Block *block);
	virtual SerialCommandParam SerializeParameter(Command *link, Block *block, int index) = 0;
	SerialCommandParam SerializeCommand(Command *com, Block *block, int index);
	virtual void SerializeStatement(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret, Block *block, int index, int marker_before_params) = 0;
	virtual void SerializeInlineFunction(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret) = 0;
	virtual void SerializeOperator(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret) = 0;
	virtual void AddFunctionIntro(Function *f) = 0;
	virtual void AddFunctionOutro(Function *f) = 0;
	virtual void CorrectReturn(){};

	void SimplifyIfStatements();
	void SimplifyFloatStore();
	void TryMergeTempVars();

	void cmd_list_out(const string &message);
	void vr_list_out();

	//void add_reg_channel(int reg, int first, int last);
	int add_virtual_reg(int reg);
	void set_virtual_reg(int v, int first, int last);
	void use_virtual_reg(int v, int first, int last);
	SerialCommandParam add_temp(Class *t, bool add_constructor = true);
	void add_cmd(int cond, int inst, const SerialCommandParam &p1, const SerialCommandParam &p2, const SerialCommandParam &p3);
	void add_cmd(int inst, const SerialCommandParam &p1, const SerialCommandParam &p2, const SerialCommandParam &p3);
	void add_cmd(int inst, const SerialCommandParam &p1, const SerialCommandParam &p2);
	void add_cmd(int inst, const SerialCommandParam &p);
	void add_cmd(int inst);
	void set_cmd_param(SerialCommand &c, int param_index, const SerialCommandParam &p);
	void next_cmd_target(int index);
	void remove_cmd(int index);
	void remove_temp_var(int v);
	void move_param(SerialCommandParam &p, int from, int to);
	int add_marker(int m = -1);
	int add_marker_after_command(int level, int index);
	void add_jump_after_command(int level, int index, int marker);


	Array<SerialCommandParam> inserted_temp;
	void add_cmd_constructor(SerialCommandParam &param, int modus);
	void add_cmd_destructor(SerialCommandParam &param, bool needs_ref = true);

	virtual void DoMapping() = 0;
	void MapReferencedTempVarsToStack();
	void TryMapTempVarsRegisters();
	void MapRemainingTempVarsToStack();

	bool is_reg_root_used_in_interval(int reg_root, int first, int last);
	void MapTempVar(int vi);
	void MapTempVars();
	void DisentangleShiftedTempVars();
	void ResolveDerefRegShift();

	int temp_in_cmd(int c, int v);
	void ScanTempVarUsage();

	int find_unused_reg(int first, int last, int size, int exclude = -1);
	void solve_deref_temp_local(int c, int np, bool is_local);
	void ResolveDerefTempAndLocal();
	bool ParamUntouchedInInterval(SerialCommandParam &p, int first, int last);
	void SimplifyFPUStack();
	void SimplifyMovs();
	void RemoveUnusedTempVars();

	void AddFunctionCall(Script *script, int func_no);
	void AddClassFunctionCall(ClassFunction *cf);
	virtual void add_function_call(Script *script, int func_no) = 0;
	virtual void add_virtual_function_call(int virtual_index) = 0;
	virtual int fc_begin() = 0;
	virtual void fc_end(int push_size) = 0;
	SerialCommandParam AddReference(SerialCommandParam &param, Class *force_type = NULL);
	SerialCommandParam AddDereference(SerialCommandParam &param, Class *force_type = NULL);

	void MapTempVarToReg(int vi, int reg);
	void add_stack_var(TempVar &v, SerialCommandParam &p);
	void MapTempVarToStack(int vi);


	void FillInDestructorsBlock(Block *b, bool recursive = false);
	void FillInDestructorsTemp();
	void FillInConstructorsBlock(Block *b);

	void InsertAddedStuffIfNeeded(Block *b, int index);



	Array<SerialCommandParam> CompilerFunctionParam;
	SerialCommandParam CompilerFunctionReturn;
	SerialCommandParam CompilerFunctionInstance;

	SerialCommandParam p_eax, p_eax_int, p_deref_eax;
	SerialCommandParam p_rax;
	SerialCommandParam p_ax, p_al, p_al_bool, p_al_char;
	SerialCommandParam p_st0, p_st1, p_xmm0, p_xmm1;
	static const SerialCommandParam p_none;

	void AddFuncParam(const SerialCommandParam &p);
	void AddFuncReturn(const SerialCommandParam &r);
	void AddFuncInstance(const SerialCommandParam &inst);


	static SerialCommandParam param_shift(const SerialCommandParam &param, int shift, Class *t);
	static SerialCommandParam param_global(Class *type, void *v);
	static SerialCommandParam param_local(Class *type, int offset);
	static SerialCommandParam param_const(Class *type, long c);
	static SerialCommandParam param_marker(int m);
	static SerialCommandParam param_deref_marker(Class *type, int m);
	SerialCommandParam param_vreg(Class *type, int vreg, int preg = -1);
	static SerialCommandParam param_preg(Class *type, int reg);
	SerialCommandParam param_deref_vreg(Class *type, int vreg, int preg = -1);
	static SerialCommandParam param_deref_preg(Class *type, int reg);
	static SerialCommandParam param_lookup(Class *type, int ref);
	static SerialCommandParam param_deref_lookup(Class *type, int ref);

	static int reg_resize(int reg, int size);
	void _resolve_deref_reg_shift_(SerialCommandParam &p, int i);

	static int get_reg(int root, int size);
};


};

#endif /* SERIALIZER_H_ */