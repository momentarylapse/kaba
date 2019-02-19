/*
 * Function.h
 *
 *  Created on: 18.02.2019
 *      Author: michi
 */

#ifndef SRC_LIB_KABA_SYNTAX_FUNCTION_H_
#define SRC_LIB_KABA_SYNTAX_FUNCTION_H_

#include "../../base/base.h"

namespace Kaba{

class Class;
class Block;
class SyntaxTree;


struct Variable
{
	Variable(const string &name, Class *type);
	~Variable();
	Class *type; // for creating instances
	string name;
	int64 _offset; // for compilation
	void *memory;
	bool memory_owner;
	bool is_extern;
	bool dont_add_constructor;
};

// user defined functions
struct Function
{
	SyntaxTree *tree;

	string name;
	// parameters (linked to intern variables)
	int num_params;
	// block of code
	Block *block;
	// local variables
	Array<Variable*> var;
	Array<Class*> literal_param_type;
	Class *_class;
	Class *return_type;
	Class *literal_return_type;
	bool is_extern, auto_declared;
	bool is_pure;
	bool throws_exceptions; // for external
	int inline_no;
	int num_slightly_hidden_vars;
	// for compilation...
	int64 _var_size, _param_size;
	int _logical_line_no;
	int _exp_no;
	void *address;
	int _label;
	Function(SyntaxTree *tree, const string &name, Class *return_type);
	~Function();
	Variable *__get_var(const string &name) const;
	string create_slightly_hidden_name();
	void update(Class *class_type);
	string signature(bool include_class = false) const;
	Array<Block*> all_blocks();
};



}

#endif /* SRC_LIB_KABA_SYNTAX_FUNCTION_H_ */
