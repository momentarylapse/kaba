/*
 * BackendAmd64.h
 *
 *  Created on: Nov 4, 2020
 *      Author: michi
 */

#ifndef SRC_LIB_KABA_COMPILER_BACKENDAMD64_H_
#define SRC_LIB_KABA_COMPILER_BACKENDAMD64_H_

#include "../kaba.h"

namespace kaba {

class Serializer;
class SerialNode;



struct TempVar;

class BackendAmd64 {
public:
	BackendAmd64(Serializer *serializer);
	virtual ~BackendAmd64();

	void correct();

	/*void map();
	void assemble();

	void map_referenced_temp_vars_to_stack();*/

	Script *script;
	Array<SerialNode> &cmd;
	Asm::InstructionWithParamsList *list;
	Serializer *serializer;
};

}

#endif /* SRC_LIB_KABA_COMPILER_BACKENDAMD64_H_ */
