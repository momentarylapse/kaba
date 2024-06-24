//
// Created by Michael Ankele on 2024-06-23.
//

#include "BackendArm64.h"
#include "Serializer.h"
#include "../../os/msg.h"

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

	msg_write("AAAAAAAAA");

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
	msg_write("AAAAAAAAA2");
	//list->add2(Asm::InstID::ALIGN_OPCODE);
}

} // kaba