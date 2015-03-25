
#ifndef SERIALIZER_H_
#define SERIALIZER_H_

namespace Script
{


#define max_reg			8 // >= all RegXXX used...

struct RegChannel
{
	int reg_root;
	int first, last;
};

// high level instructions
enum{
	inst_marker = 10000,
	inst_asm,
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
	Type *type;
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
	int pos;
	string str() const;
};

struct TempVar
{
	Type *type;
	int first, last, count;
	bool force_stack;
	int entangled;
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
	bool temp_var_ranges_defined;

	Array<int> map_reg_root;
	Array<RegChannel> reg_channel;

	bool reg_root_used[max_reg];
	Array<LoopData> loop;

	int stack_offset, stack_max_size, max_push_size;
	Array<TempVar> temp_var;

	Array<AddLaterData> add_later;

	Array<void*> global_refs;
	int add_global_ref(void *p);

	Asm::InstructionWithParamsList *list;

	void DoError(const string &msg);
	void DoErrorLink(const string &msg);

	void Assemble();
	void assemble_cmd(SerialCommand &c);
	void assemble_cmd_arm(SerialCommand &c);
	Asm::InstructionParam get_param(int inst, SerialCommandParam &p);

	void SerializeFunction(Function *f);
	void SerializeBlock(Block *block, int level);
	void SerializeParameter(Command *link, int level, int index, SerialCommandParam &param);
	SerialCommandParam SerializeCommand(Command *com, int level, int index);
	virtual void SerializeCompilerFunction(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret, int level, int index, int marker_before_params) = 0;
	virtual void SerializeOperator(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret) = 0;
	virtual void AddFunctionIntro(Function *f) = 0;
	virtual void AddFunctionOutro(Function *f) = 0;
	virtual void CorrectReturn(){};

	void SimplifyIfStatements();
	void SimplifyFloatStore();
	void TryMergeTempVars();

	void cmd_list_out();

	void add_reg_channel(int reg, int first, int last);
	void add_temp(Type *t, SerialCommandParam &param, bool add_constructor = true);
	void add_cmd(int cond, int inst, const SerialCommandParam &p1, const SerialCommandParam &p2, const SerialCommandParam &p3);
	void add_cmd(int inst, const SerialCommandParam &p1, const SerialCommandParam &p2, const SerialCommandParam &p3);
	void add_cmd(int inst, const SerialCommandParam &p1, const SerialCommandParam &p2);
	void add_cmd(int inst, const SerialCommandParam &p);
	void add_cmd(int inst);
	void move_last_cmd(int index);
	void remove_cmd(int index);
	void remove_temp_var(int v);
	void move_param(SerialCommandParam &p, int from, int to);
	int add_marker(int m = -1);
	int add_marker_after_command(int level, int index);
	void add_jump_after_command(int level, int index, int marker);


	Array<SerialCommandParam> inserted_constructor_func;
	Array<SerialCommandParam> inserted_constructor_temp;
	void add_cmd_constructor(SerialCommandParam &param, int modus);
	void add_cmd_destructor(SerialCommandParam &param, bool ref = true);

	virtual void DoMapping() = 0;
	void FindReferencedTempVars();
	void TryMapTempVarsRegisters();
	void MapRemainingTempVarsToStack();

	bool is_reg_root_used_in_interval(int reg_root, int first, int last);
	void MapTempVar(int vi);
	void MapTempVars();
	void MapReferencedTempVars();
	void DisentangleShiftedTempVars();
	void ResolveDerefRegShift();

	int temp_in_cmd(int c, int v);
	void ScanTempVarUsage();
	virtual void CorrectUnallowedParamCombis() = 0;
	virtual void CorrectUnallowedParamCombis2(SerialCommand &c) = 0;

	int find_unused_reg(int first, int last, int size, bool allow_eax);
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
	void AddReference(SerialCommandParam &param, Type *type, SerialCommandParam &ret);
	void AddDereference(SerialCommandParam &param, SerialCommandParam &ret, Type *force_type = NULL);

	void MapTempVarToReg(int vi, int reg);
	void add_stack_var(Type *type, int first, int last, SerialCommandParam &p);
	void MapTempVarToStack(int vi);


	void FillInDestructors(bool from_temp);
	void FillInConstructorsFunc();
};

class SerializerX86 : public Serializer
{
public:
	SerializerX86(Script *script, Asm::InstructionWithParamsList *list) : Serializer(script, list){};
	virtual ~SerializerX86(){}
	virtual void add_function_call(Script *script, int func_no);
	virtual void add_virtual_function_call(int virtual_index);
	virtual int fc_begin();
	virtual void fc_end(int push_size);
	virtual void AddFunctionIntro(Function *f);
	virtual void AddFunctionOutro(Function *f);
	virtual void SerializeCompilerFunction(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret, int level, int index, int marker_before_params);
	virtual void SerializeOperator(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret);

	virtual void DoMapping();
	virtual void CorrectUnallowedParamCombis();
	virtual void CorrectUnallowedParamCombis2(SerialCommand &c);
};

class SerializerAMD64 : public SerializerX86
{
public:
	SerializerAMD64(Script *script, Asm::InstructionWithParamsList *list) : SerializerX86(script, list){};
	virtual ~SerializerAMD64(){}
	virtual void add_function_call(Script *script, int func_no);
	virtual void add_virtual_function_call(int virtual_index);
	virtual int fc_begin();
	virtual void fc_end(int push_size);
	virtual void AddFunctionIntro(Function *f);
	virtual void AddFunctionOutro(Function *f);
	virtual void CorrectUnallowedParamCombis2(SerialCommand &c);
};

class SerializerARM : public Serializer
{
public:
	SerializerARM(Script *script, Asm::InstructionWithParamsList *list) : Serializer(script, list){};
	virtual ~SerializerARM(){}
	virtual void add_function_call(Script *script, int func_no);
	virtual void add_virtual_function_call(int virtual_index);
	virtual int fc_begin();
	virtual void fc_end(int push_size);
	virtual void AddFunctionIntro(Function *f);
	virtual void AddFunctionOutro(Function *f);
	virtual void SerializeCompilerFunction(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret, int level, int index, int marker_before_params);
	virtual void SerializeOperator(Command *com, Array<SerialCommandParam> &param, SerialCommandParam &ret);

	virtual void DoMapping();
	void ConvertGlobalLookups();
	virtual void CorrectUnallowedParamCombis();
	virtual void CorrectUnallowedParamCombis2(SerialCommand &c);
	virtual void CorrectReturn();
};

};

#endif /* SERIALIZER_H_ */
