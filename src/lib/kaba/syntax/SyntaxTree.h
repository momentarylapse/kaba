#if !defined(SYNTAX_TREE_H__INCLUDED_)
#define SYNTAX_TREE_H__INCLUDED_


#include "lexical.h"
#include <functional>
#include "Class.h"
#include "Constant.h"
#include "Function.h"
#include "Node.h"


class complex;

namespace Asm{
	struct MetaInfo;
};

namespace Kaba{

class Script;
class SyntaxTree;
class Operator;
class Function;
class Variable;
class Node;
class Constant;
class Block;

#define MAX_STRING_CONST_LENGTH	2048

// macros
struct Define
{
	string source;
	Array<string> dest;
};


struct Operator
{
	int primitive_id;
	Class *return_type, *param_type_1, *param_type_2;

	SyntaxTree *owner;
	int func_index;
	int class_func_index;
	int inline_index;

	void *func; // temporary...!

	string str() const;
};


struct AsmBlock
{
	string block;
	int line;
};



// data structures (uncompiled)
class SyntaxTree
{
public:
	SyntaxTree(Script *_script);
	~SyntaxTree();

	void ParseBuffer(const string &buffer, bool just_analyse);
	void AddIncludeData(Script *s);

	void DoError(const string &msg, int override_exp_no = -1, int override_line = -1);
	void ExpectNoNewline();
	void ExpectNewline();
	void ExpectIndent();
	
	// syntax parsing
	void Parser();
	void ParseTopLevel();
	void ParseAllClassNames();
	void ParseAllFunctionBodies();
	void ParseImport();
	void ParseEnum();
	void ParseClass();
	Function *ParseFunctionHeader(Class *class_type, bool as_extern);
	void SkipParsingFunctionBody();
	void ParseFunctionBody(Function *f);
	void ParseClassFunctionHeader(Class *t, bool as_extern, bool as_virtual, bool override);
	bool ParseFunctionCommand(Function *f, ExpressionBuffer::Line *this_line);
	Class *ParseType();
	void ParseVariableDef(bool single, Block *block);
	void ParseGlobalConst(const string &name, Class *type);
	int WhichPrimitiveOperator(const string &name);
	int WhichStatement(const string &name);
	int WhichType(const string &name);
	void AddType();

	// pre compiler
	void PreCompiler(bool just_analyse);
	void HandleMacro(int &line_no, int &NumIfDefs, bool *IfDefed, bool just_analyse);
	void AutoImplementAddVirtualTable(Node *self, Function *f, Class *t);
	void AutoImplementAddChildConstructors(Node *self, Function *f, Class *t);
	void AutoImplementConstructor(Function *f, Class *t, bool allow_parent_constructor);
	void AutoImplementDestructor(Function *f, Class *t);
	void AutoImplementAssign(Function *f, Class *t);
	void AutoImplementArrayClear(Function *f, Class *t);
	void AutoImplementArrayResize(Function *f, Class *t);
	void AutoImplementArrayAdd(Function *f, Class *t);
	void AutoImplementArrayRemove(Function *f, Class *t);
	void AutoImplementFunctions(Class *t);
	void AddMissingFunctionHeadersForClass(Class *t);

	// syntax analysis
	Class *GetConstantType(const string &str);
	void GetConstantValue(const string &str, Value &value);
	Class *FindType(const string &name);
	Class *AddClass(Class *type);
	Class *CreateNewClass(const string &name, Class::Type type, int size, int array_size, Class *sub);
	Class *CreateArrayClass(Class *element_type, int num_elements);
	Class *CreateDictClass(Class *element_type);
	Array<Node*> GetExistence(const string &name, Block *block);
	Array<Node*> GetExistenceShared(const string &name);
	void LinkMostImportantOperator(Array<Node*> &operand, Array<Node*> &_operator, Array<int> &op_exp);
	Node *LinkOperator(int op_no, Node *param1, Node *param2);
	Node *GetOperandExtension(Node *operand, Block *block);
	Node *GetOperandExtensionElement(Node *operand, Block *block);
	Node *GetOperandExtensionArray(Node *operand, Block *block);
	Node *GetCommand(Block *block);
	void ParseCompleteCommand(Block *block);
	void ParseLocalDefinition(Block *block);
	Node *ParseBlock(Block *parent, Block *block = nullptr);
	Node *GetOperand(Block *block);
	Node *GetPrimitiveOperator(Block *block);
	Array<Node*> FindFunctionParameters(Block *block);
	//void FindFunctionSingleParameter(int p, Array<Type*> &wanted_type, Block *block, Node *cmd);
	Array<Class*> GetFunctionWantedParams(Node *link);
	Node *GetFunctionCall(const string &f_name, Array<Node*> &links, Block *block);
	Node *DoClassFunction(Node *ob, Array<ClassFunction> &cfs, Block *block);
	Node *GetSpecialFunctionCall(const string &f_name, Node *link, Block *block);
	Node *CheckParamLink(Node *link, Class *type, const string &f_name = "", int param_no = -1);
	Node *ParseStatement(Block *block);
	Node *ParseStatementFor(Block *block);
	Node *ParseStatementForall(Block *block);
	Node *ParseStatementWhile(Block *block);
	Node *ParseStatementBreak(Block *block);
	Node *ParseStatementContinue(Block *block);
	Node *ParseStatementReturn(Block *block);
	Node *ParseStatementRaise(Block *block);
	Node *ParseStatementTry(Block *block);
	Node *ParseStatementIf(Block *block);
	Node *ParseStatementPass(Block *block);

	void CreateAsmMetaInfo();

	// neccessary conversions
	void ConvertCallByReference();
	void BreakDownComplicatedCommands();
	Node *BreakDownComplicatedCommand(Node *c);
	void MakeFunctionsInline();
	void MapLocalVariablesToStack();

	void transform(std::function<Node*(Node*)> F);
	static void transform_block(Block *block, std::function<Node*(Node*)> F);
	static Node* transform_node(Node *n, std::function<Node*(Node*)> F);

	// data creation
	int AddConstant(Class *type);
	Function *AddFunction(const string &name, Class *type);

	// nodes
	Node *AddNode(int kind, int64 link_no, Class *type);
	Node *AddNode(int kind, int64 link_no, Class *type, Script *s);
	Node *add_node_statement(int index);
	Node *add_node_classfunc(ClassFunction *f, Node *inst, bool force_non_virtual = false);
	Node *add_node_func(Script *script, int no, Class *return_type);
	Node *add_node_const(int nc);
	Node *add_node_operator_by_index(Node *p1, Node *p2, int op);
	Node *add_node_operator_by_inline(Node *p1, Node *p2, int inline_index);
	Node *add_node_local_var(int no, Class *type);
	Node *add_node_parray(Node *p, Node *index, Class *type);
	//Node *add_node_block(Block *b);
	Node *cp_node(Node *c);
	Node *ref_node(Node *sub, Class *override_type = nullptr);
	Node *deref_node(Node *sub, Class *override_type = nullptr);
	Node *shift_node(Node *sub, bool deref, int shift, Class *type);

	// pre processor
	Node *PreProcessNode(Node *c);
	void PreProcessor();
	Node *PreProcessNodeAddresses(Node *c);
	void PreProcessorAddresses();
	void SimplifyRefDeref();
	void SimplifyShiftDeref();

	// debug displaying
	void ShowNode(Node *c, Function *f);
	void ShowFunction(Function *f, const string &stage = "");
	void ShowBlock(Block *b);
	void Show(const string &stage);

// data

	ExpressionBuffer Exp;

	// compiler options
	bool flag_immortal;
	bool flag_string_const_as_cstring;

	Array<Class*> classes;
	Array<Script*> includes;
	Array<Define> defines;
	Asm::MetaInfo *asm_meta_info;
	Array<AsmBlock> asm_blocks;
	Array<Constant*> constants;
	Array<Operator> operators;
	Array<Function*> functions;
	Array<Node*> nodes;

	Function root_of_all_evil;

	Script *script;
	Function *cur_func;
	int for_index_count;

	int parser_loop_depth;
};

string kind2str(int kind);
string node2str(SyntaxTree *s, Function *f, Node *n);




};

#endif
