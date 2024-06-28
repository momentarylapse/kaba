//
// Created by Michael Ankele on 2024-06-23.
//

#include "BackendArm64.h"
#include "Serializer.h"
#include "../asm/asm.h"
#include "../../os/msg.h"

namespace Asm{
	extern Asm::RegID r_reg(int i);
	extern Asm::RegID w_reg(int i);
	extern Asm::RegID s_reg(int i);
};

namespace kaba {

BackendArm64::BackendArm64(Serializer* serializer) : BackendARM(serializer) {
}

void BackendArm64::process(Function *f, int index) {
	cur_func = f;
	cur_func_index = index;
	//call_used = false;
	stack_offset = f->_var_size;
	stack_max_size = f->_var_size;

	//do_mapping();
	correct();
}

void BackendArm64::add_function_intro_params(Function *f) {
	// return, instance, params
	Array<Variable*> param;
	if (f->effective_return_type->uses_return_by_memory()) {
		for (Variable *v: weak(f->var))
			if (v->name == Identifier::RETURN_VAR) {
				param.add(v);
				break;
			}
	}
	if (!f->is_static()) {
		for (Variable *v: weak(f->var))
			if (v->name == Identifier::SELF) {
				param.add(v);
				break;
			}
	}
	for (int i=0;i<f->num_params;i++)
		param.add(f->var[i].get());

	// map params...
	Array<Variable*> reg_param;
	Array<Variable*> stack_param;
	Array<Variable*> float_param;
	for (Variable *p: param) {
		if ((p->type == TypeInt) or (p->type == TypeInt64) or (p->type == TypeInt8) or (p->type == TypeBool) or p->type->is_some_pointer()) {
			if (reg_param.num < 8) {
				reg_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else if (p->type == TypeFloat32) {
			if (float_param.num < 8) {
				float_param.add(p);
			} else {
				stack_param.add(p);
			}
		} else {
			do_error("parameter type currently not supported: " + p->type->name);
		}
	}

	// s0-7
	foreachib(Variable *p, float_param, i) {
		int reg = cmd.add_virtual_reg(Asm::s_reg(i));
		_from_register_float(reg, param_local(p->type, p->_offset), 0);
	}

	// r0-7
	foreachib(Variable *p, reg_param, i) {
		if (p->type->size > 4) {
			int reg = cmd.add_virtual_reg(Asm::r_reg(i));
			_from_register_32(reg, param_local(p->type, p->_offset), 0);
			cmd.set_virtual_reg(reg, cmd.cmd.num - 1, cmd.cmd.num - 1);
		} else {
			int reg = cmd.add_virtual_reg(Asm::w_reg(i));
			_from_register_32(reg, param_local(p->type, p->_offset), 0);
			cmd.set_virtual_reg(reg, cmd.cmd.num - 1, cmd.cmd.num - 1);
		}
	}

	// get parameters from stack
	foreachb([[maybe_unused]] Variable *p, stack_param) {
		do_error("func with stack...");
		/*int s = 8;
		cmd.add_cmd(Asm::inst_push, p);
		push_size += s;*/
	}
}

void BackendArm64::correct() {
	cmd.next_cmd_target(0);

	serializer->cmd_list_out("x:a", "paramtrafo");

	correct_implement_commands();

	cmd.next_cmd_target(0);
	add_function_intro_params(cur_func);
	serializer->cmd_list_out("x:b", "post paramtrafo");
}

void BackendArm64::correct_implement_commands() {
}

void BackendArm64::assemble() {
	// intro + allocate stack memory

	/*foreachi(GlobalRef &g, global_refs, i) {
		g.label = list->create_label(format("_kaba_ref_%d_%d", cur_func_index, i));
		list->insert_location_label(g.label);
		list->add2(Asm::InstID::DD, Asm::param_imm((int_p)g.p, 4));
	}*/

	list->insert_location_label(cur_func->_label);

	//if (!flags_has(cur_func->flags, Flags::NOFRAME))
	//	add_function_intro_frame(stack_max_size);

	//	do_error("new ARM assemble() not yet implemented");
	for (int i=0;i<cmd.cmd.num;i++) {
		if (cmd.cmd[i].inst == Asm::InstID::LABEL) {
			list->insert_location_label(cmd.cmd[i].p[0].p);
		} else if (cmd.cmd[i].inst == Asm::InstID::ASM) {
			add_asm_block(cmd.cmd[i].p[0].p);
		} else {
			assemble_cmd_arm(cmd.cmd[i]);
		}
	}

	list->show();
	//list->add2(Asm::InstID::ALIGN_OPCODE);
}

} // kaba