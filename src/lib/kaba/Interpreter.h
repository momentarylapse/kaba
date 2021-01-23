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
class SerializerX;
class CommandList;
class SerialNode;

class Interpreter {
public:
	Interpreter(Script *s);
	~Interpreter();

	void add_function(Function *f, SerializerX *ser);

	Script *script;
	//Asm::InstructionWithParamsList *list
	string stack;
	Array<int> stack_pointer;
	Array<int64> call_params;

	struct IFunction {
		Function *f;
		//CommandList* cmd;
		SerializerX* ser;
	};
	Array<IFunction> functions;

	void run(const string &name);
	void run_function(Function *f, SerializerX *ser);
	void run_command(SerialNode &n, SerializerX *ser);
};


}
