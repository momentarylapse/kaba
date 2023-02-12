/*
 * implicit_super_array.cpp
 *
 *  Created on: 12 Feb 2023
 *      Author: michi
 */

#include "../../kaba.h"
#include "../implicit.h"
#include "../Parser.h"

namespace kaba {

void AutoImplementer::_add_missing_function_headers_for_super_array(Class *t) {
	add_func_header(t, Identifier::Func::INIT, TypeVoid, {}, {});
	add_func_header(t, Identifier::Func::DELETE, TypeVoid, {}, {});
	add_func_header(t, "clear", TypeVoid, {}, {});
	add_func_header(t, "resize", TypeVoid, {TypeInt}, {"num"});
	add_func_header(t, "add", TypeVoid, {t->param[0]}, {"x"});
	add_func_header(t, "remove", TypeVoid, {TypeInt}, {"index"});
	add_func_header(t, Identifier::Func::ASSIGN, TypeVoid, {t}, {"other"});
}

void AutoImplementer::implement_super_array_constructor(Function *f, const Class *t) {
	auto self = add_node_local(f->__get_var(Identifier::SELF));

	auto te = t->get_array_element();
	auto ff = t->get_member_func("__mem_init__", TypeVoid, {TypeInt});
	f->block->add(add_node_member_call(ff,
			self, -1,
			{add_node_const(tree->add_constant_int(te->size))}));
}

void AutoImplementer::implement_super_array_destructor(Function *f, const Class *t) {
	auto te = t->get_array_element();
	auto self = add_node_local(f->__get_var(Identifier::SELF));

	Function *f_clear = t->get_member_func("clear", TypeVoid, {});
	if (!f_clear)
		do_error_implicit(f, "clear() missing");
	f->block->add(add_node_member_call(f_clear, self));
}

void AutoImplementer::implement_super_array_assign(Function *f, const Class *t) {
	auto te = t->get_array_element();
	auto n_other = add_node_local(f->__get_var("other"));
	auto n_self = add_node_local(f->__get_var(Identifier::SELF));

	Function *f_resize = t->get_member_func("resize", TypeVoid, {TypeInt});
	if (!f_resize)
		do_error_implicit(f, format("no %s.resize(int) found", t->long_name()));

	// self.resize(other.num)
	auto n_other_num = n_other->shift(config.pointer_size, TypeInt);

	auto n_resize = add_node_member_call(f_resize, n_self);
	n_resize->set_num_params(2);
	n_resize->set_param(1, n_other_num);
	f->block->add(n_resize);

	// for el,i in *self
	//    el = other[i]

	auto *v_el = f->block->add_var("el", tree->get_pointer(t->get_array_element()));
	auto *v_i = f->block->add_var("i", TypeInt);

	Block *b = new Block(f, f->block.get());

	// other[i]
	shared<Node> n_other_el = add_node_dyn_array(n_other, add_node_local(v_i));

	auto n_assign = parser->con.link_operator_id(OperatorID::ASSIGN, add_node_local(v_el)->deref(), n_other_el);
	if (!n_assign)
		do_error_implicit(f, format("no operator %s = %s found", te->long_name(), te->long_name()));
	b->add(n_assign);

	auto n_for = add_node_statement(StatementID::FOR_ARRAY);
	// [VAR, INDEX, ARRAY, BLOCK]
	n_for->set_param(0, add_node_local(v_el));
	n_for->set_param(1, add_node_local(v_i));
	n_for->set_param(2, n_self);
	n_for->set_param(3, b);
	f->block->add(n_for);

}

void AutoImplementer::implement_super_array_clear(Function *f, const Class *t) {
	auto te = t->get_array_element();

	auto self = add_node_local(f->__get_var(Identifier::SELF));

// delete...
	Function *f_del = te->get_destructor();
	if (f_del) {

		auto *var_i = f->block->add_var("i", TypeInt);
		auto *var_el = f->block->add_var("el", tree->get_pointer(t->get_array_element()));

		Block *b = new Block(f, f->block.get());

		// __delete__
		auto cmd_delete = add_node_member_call(f_del, add_node_local(var_el)->deref());
		b->add(cmd_delete);

		auto cmd_for = add_node_statement(StatementID::FOR_ARRAY);
		cmd_for->set_param(0, add_node_local(var_el));
		cmd_for->set_param(1, add_node_local(var_i));
		cmd_for->set_param(2, self);
		cmd_for->set_param(3, b);

		f->block->add(cmd_for);
	} else if (te->needs_destructor()) {
		do_error_implicit(f, "element destructor missing");
	}

	// clear
	auto cmd_clear = add_node_member_call(t->get_member_func("__mem_clear__", TypeVoid, {}), self);
	f->block->add(cmd_clear);
}


void AutoImplementer::implement_super_array_resize(Function *f, const Class *t) {
	auto te = t->get_array_element();
	auto *var = f->block->add_var("i", TypeInt);
	f->block->add_var("num_old", TypeInt);

	auto num = add_node_local(f->__get_var("num"));

	auto self = add_node_local(f->__get_var(Identifier::SELF));

	auto self_num = self->shift(config.pointer_size, TypeInt);

	auto num_old = add_node_local(f->__get_var("num_old"));

	// num_old = self.num
	f->block->add(add_node_operator_by_inline(InlineID::INT_ASSIGN, num_old, self_num));

// delete...
	Function *f_del = te->get_destructor();
	if (f_del) {

		Block *b = new Block(f, f->block.get());

		// el := self[i]
		auto el = add_node_dyn_array(self, add_node_local(var));

		// __delete__
		auto cmd_delete = add_node_member_call(f_del, el);
		b->add(cmd_delete);

		//  [VAR, START, STOP, STEP, BLOCK]
		auto cmd_for = add_node_statement(StatementID::FOR_RANGE);
		cmd_for->set_param(0, add_node_local(var));
		cmd_for->set_param(1, num);
		cmd_for->set_param(2, self_num);
		cmd_for->set_param(3, add_node_const(tree->add_constant_int(1)));
		cmd_for->set_param(4, b);
		f->block->add(cmd_for);

	} else if (te->needs_destructor()) {
		do_error_implicit(f, "element destructor missing");
	}

	// resize
	auto c_resize = add_node_member_call(t->get_member_func("__mem_resize__", TypeVoid, {TypeInt}), self);
	c_resize->set_param(1, num);
	f->block->add(c_resize);

	// new...
	Function *f_init = te->get_default_constructor();
	if (f_init) {

		Block *b = new Block(f, f->block.get());

		// el := self[i]
		auto el = add_node_dyn_array(self, add_node_local(var));

		// __init__
		auto cmd_init = add_node_member_call(f_init, el);
		b->add(cmd_init);

		//  [VAR, START, STOP, STEP, BLOCK]
		auto cmd_for = add_node_statement(StatementID::FOR_RANGE);
		cmd_for->set_param(0, add_node_local(var));
		cmd_for->set_param(1, num_old);
		cmd_for->set_param(2, self_num);
		cmd_for->set_param(3, add_node_const(tree->add_constant_int(1)));
		cmd_for->set_param(4, b);
		f->block->add(cmd_for);

	} else if (te->needs_constructor()) {
		do_error_implicit(f, "element default constructor missing");
	}
}


void AutoImplementer::implement_super_array_remove(Function *f, const Class *t) {
	auto te = t->get_array_element();
	auto index = add_node_local(f->__get_var("index"));
	auto self = add_node_local(f->__get_var(Identifier::SELF));

	// delete...
	Function *f_del = te->get_destructor();
	if (f_del) {

		// el := self[index]
		auto cmd_el = add_node_dyn_array(self, index);

		// __delete__
		auto cmd_delete = add_node_member_call(f_del, cmd_el);
		f->block->params.add(cmd_delete);
	} else if (te->needs_destructor()) {
		do_error_implicit(f, "element destructor missing");
	}

	// resize
	auto c_remove = add_node_member_call(t->get_member_func("__mem_remove__", TypeVoid, {TypeInt}), self);
	c_remove->set_param(1, index);
	f->block->params.add(c_remove);
}

void AutoImplementer::implement_super_array_add(Function *f, const Class *t) {
	auto te = t->get_array_element();
	Block *b = f->block.get();
	auto item = add_node_local(b->get_var("x"));

	auto self = add_node_local(b->get_var(Identifier::SELF));

	auto self_num = self->shift(config.pointer_size, TypeInt);


	// resize(self.num + 1)
	auto cmd_1 = add_node_const(tree->add_constant_int(1));
	auto cmd_add = add_node_operator_by_inline(InlineID::INT_ADD, self_num, cmd_1);
	auto cmd_resize = add_node_member_call(t->get_member_func("resize", TypeVoid, {TypeInt}), self);
	cmd_resize->set_param(1, cmd_add);
	b->add(cmd_resize);


	// el := self.data[self.num - 1]
	auto cmd_sub = add_node_operator_by_inline(InlineID::INT_SUBTRACT, self_num, cmd_1);
	auto cmd_el = add_node_dyn_array(self, cmd_sub);

	auto cmd_assign = parser->con.link_operator_id(OperatorID::ASSIGN, cmd_el, item);
	if (!cmd_assign)
		do_error_implicit(f, format("no operator %s = %s for elements found", te->long_name(), te->long_name()));
	b->add(cmd_assign);
}

}


