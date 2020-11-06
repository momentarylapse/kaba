/*
 * BackendAmd64.h
 *
 *  Created on: Nov 4, 2020
 *      Author: michi
 */

#ifndef SRC_LIB_KABA_COMPILER_BACKENDAMD64_H_
#define SRC_LIB_KABA_COMPILER_BACKENDAMD64_H_

#include "../kaba.h"
#include "serializer.h"

namespace kaba {

class Serializer;
class SerialNode;



struct TempVar;
struct SerialNodeParam;
struct VirtualRegister;

class BackendAmd64 {
public:
	BackendAmd64(Serializer *serializer);
	virtual ~BackendAmd64();

	void correct();

	int fc_begin(Function *__f, const Array<SerialNodeParam> &_params, const SerialNodeParam &ret);
	void fc_end(int push_size, const Array<SerialNodeParam> &params, const SerialNodeParam &ret);
	void add_function_call(Function *f, const Array<SerialNodeParam> &params, const SerialNodeParam &ret);

	/*void map();
	void assemble();

	void map_referenced_temp_vars_to_stack();*/

	Script *script;
	Array<SerialNode> &cmd;
	Asm::InstructionWithParamsList *list;
	Serializer *serializer;


	Array<int> map_reg_root;
	//Array<VirtualRegister> virtual_reg;


	int add_virtual_reg(int preg);
	void set_virtual_reg(int v, int first, int last);
	void use_virtual_reg(int v, int first, int last);

	bool is_reg_root_used_in_interval(int reg_root, int first, int last);
	int find_unused_reg(int first, int last, int size, int exclude);

	SerialNodeParam p_eax, p_eax_int, p_deref_eax;
	SerialNodeParam p_rax;
	SerialNodeParam p_ax, p_al, p_al_bool, p_al_char;
	SerialNodeParam p_st0, p_st1, p_xmm0, p_xmm1;
	static const SerialNodeParam p_none;


	SerialNodeParam param_vreg(const Class *type, int vreg, int preg = -1);
	SerialNodeParam param_deref_vreg(const Class *type, int vreg, int preg = -1);

	static int reg_resize(int reg, int size);
	void _resolve_deref_reg_shift_(SerialNodeParam &p, int i);

	//static int get_reg(int root, int size);

	void next_cmd_target(int index);
	void insert_cmd(int inst, const SerialNodeParam &p1 = p_none, const SerialNodeParam &p2 = p_none, const SerialNodeParam &p3 = p_none);
	void remove_cmd(int index);
	SerialNodeParam insert_reference(const SerialNodeParam &param, const Class *type = nullptr);


	void add_function_outro(Function *f);
	void add_function_intro_params(Function *f);
};

}

#endif /* SRC_LIB_KABA_COMPILER_BACKENDAMD64_H_ */
