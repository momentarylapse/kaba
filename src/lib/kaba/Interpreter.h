/*
 * Interpreter.h
 *
 *  Created on: Jan 22, 2021
 *      Author: michi
 */

#pragma once

#include "../base/base.h"

namespace kaba {

class Script;
class SyntaxTree;
class Function;
class Serializer;
class CommandList;
class SerialNode;
class SerialNodeParam;

class Interpreter {
public:
	Interpreter(Script *s);
	~Interpreter();

	void add_function(Function *f, Serializer *ser);

	Script *script;
	//Asm::InstructionWithParamsList *list
	//string stack;
	Array<int> stack_pointer;
	Array<void*> call_params;

	struct IFunction {
		Function *f;
		//CommandList* cmd;
		Serializer* ser;
	};
	Array<IFunction> functions;

	struct Frame {
		string stack;
		int offset;
		Array<string> temps;
		//Serializer *ser;
	};

	void run(const string &name);
	void run_function(Function *f, Serializer *ser);
	int run_command(int index, SerialNode &n, Serializer *ser, Frame &f);

	void do_error(const string &s);

	//int64 get_param(SerialNodeParam &p);
};


}
