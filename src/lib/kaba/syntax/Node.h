/*
 * Node.h
 *
 *  Created on: 18.02.2019
 *      Author: michi
 */

#ifndef SRC_LIB_KABA_SYNTAX_NODE_H_
#define SRC_LIB_KABA_SYNTAX_NODE_H_


#include "../../base/base.h"

namespace Kaba{

class Class;
class Block;
class SyntaxTree;
class Script;
class Function;
class Variable;
class Constant;
class Operator;


enum
{
	KIND_UNKNOWN,
	// data
	KIND_VAR_LOCAL,
	KIND_VAR_GLOBAL,
	KIND_VAR_FUNCTION,
	KIND_CONSTANT,
	// execution
	KIND_FUNCTION,           // = real function call
	KIND_VIRTUAL_FUNCTION,   // = virtual function call
	KIND_INLINE_FUNCTION,    // = function defined inside the compiler...
	KIND_STATEMENT,          // = if/while/break/...
	KIND_BLOCK,              // = block of commands {...}
	KIND_OPERATOR,
	KIND_PRIMITIVE_OPERATOR, // tentative...
	// data altering
	KIND_ADDRESS_SHIFT,      // = . "struct"
	KIND_ARRAY,              // = []
	KIND_POINTER_AS_ARRAY,   // = []
	KIND_REFERENCE,          // = &
	KIND_DEREFERENCE,        // = *
	KIND_DEREF_ADDRESS_SHIFT,// = ->
	KIND_REF_TO_LOCAL,
	KIND_REF_TO_GLOBAL,
	KIND_REF_TO_CONST,
	KIND_ADDRESS,            // &global (for pre processing address shifts)
	KIND_MEMORY,             // global (but LinkNr = address)
	KIND_LOCAL_ADDRESS,      // &local (for pre processing address shifts)
	KIND_LOCAL_MEMORY,       // local (but LinkNr = address)
	// special
	KIND_TYPE,
	KIND_ARRAY_BUILDER,
	KIND_CONSTRUCTOR_AS_FUNCTION,
	// compilation
	KIND_VAR_TEMP,
	KIND_DEREF_VAR_TEMP,
	KIND_DEREF_VAR_LOCAL,
	KIND_REGISTER,
	KIND_DEREF_REGISTER,
	KIND_MARKER,
	KIND_DEREF_MARKER,
	KIND_IMMEDIATE,
	KIND_GLOBAL_LOOKUP,       // ARM
	KIND_DEREF_GLOBAL_LOOKUP, // ARM
};

struct Node;

// {...}-block
struct Block
{
	Block(Function *f, Block *parent);
	~Block();
	Array<Node*> nodes;
	Array<int> vars;
	Function *function;
	Block *parent;
	void *_start, *_end; // opcode range
	int level;
	void add(Node *c);
	void set(int index, Node *c);

	int get_var(const string &name);
	int add_var(const string &name, Class *type);
};
// single operand/command
struct Node
{
	int kind;
	int64 link_no;
	Script *script;
	int ref_count;
	// parameters
	Array<Node*> params;
	// linking of class function instances
	Node *instance;
	// return value
	Class *type;
	Node(int kind, int64 link_no, Script *script, Class *type);
	~Node();
	Block *as_block() const;
	Function *as_func() const;
	Class *as_class() const;
	Constant *as_const() const;
	Operator *as_op() const;
	void *as_func_p() const;
	void *as_const_p() const;
	void *as_global_p() const;
	Variable *as_global() const;
	Variable *as_local(Function *f) const;
	void set_num_params(int n);
	void set_param(int index, Node *p);
	void set_instance(Node *p);
};
void clear_nodes(Array<Node*> &nodes);
void clear_nodes(Array<Node*> &nodes, Node *keep);


}


#endif /* SRC_LIB_KABA_SYNTAX_NODE_H_ */
