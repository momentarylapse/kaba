/*
 * implicit_list.cpp
 *
 *  Created on: 12 Feb 2023
 *      Author: michi
 */

#include "../kaba.h"
#include "implicit.h"
#include "../parser/Parser.h"

namespace kaba {

extern const Class* TypeDynamicArray;
string class_name_might_need_parantheses(const Class *t);

static shared<Node> sa_num(shared<Node> node) {
	return node->shift(config.target.pointer_size, TypeInt32);
}

/*static shared<Node> sa_data(shared<Node> node) {
	return node->shift(config.pointer_size, TypeInt32);
}*/

void AutoImplementer::implement_list_constructor(Function *f, const Class *t) {
	auto self = add_node_local(f->__get_var(Identifier::SELF));

	auto te = t->get_array_element();
	auto ff = t->get_member_func("__mem_init__", TypeVoid, {TypeInt32});
	f->block->add(add_node_member_call(ff,
			self, -1,
			{const_int(te->size)}));
}

void AutoImplementer::implement_list_destructor(Function *f, const Class *t) {
	auto self = add_node_local(f->__get_var(Identifier::SELF));

	if (auto f_clear = t->get_member_func("clear", TypeVoid, {}))
		f->block->add(add_node_member_call(f_clear, self));
	else
		do_error_implicit(f, "clear() missing");
}

void AutoImplementer::implement_list_assign(Function *f, const Class *t) {
	if (!f)
		return;
	auto t_el = t->get_array_element();
	auto n_other = add_node_local(f->__get_var("other"));
	auto n_self = add_node_local(f->__get_var(Identifier::SELF));

	if (auto f_resize = t->get_member_func("resize", TypeVoid, {TypeInt32})) {
		// self.resize(other.num)
		auto n_other_num = sa_num(n_other);

		auto n_resize = add_node_member_call(f_resize, n_self);
		n_resize->set_num_params(2);
		n_resize->set_param(1, n_other_num);
		f->block->add(n_resize);
	} else {
		do_error_implicit(f, format("no %s.resize(int) found", t->long_name()));
	}

	{
		// for i=>el in self
		//    el = other[i]

		auto *v_el = f->block->add_var("el", tree->type_ref(t_el));
		auto *v_i = f->block->add_var("i", TypeInt32);

		Block *b = new Block(f, f->block.get());

		// other[i]
		auto n_other_el = add_node_dyn_array(n_other, add_node_local(v_i));

		b->add(add_assign(f, "", add_node_local(v_el)->deref(), n_other_el));

		auto n_for = add_node_statement(StatementID::FOR_CONTAINER);
		// [VAR, INDEX, ARRAY, BLOCK]
		n_for->set_param(0, add_node_local(v_el));
		n_for->set_param(1, add_node_local(v_i));
		n_for->set_param(2, n_self);
		n_for->set_param(3, b);
		f->block->add(n_for);
	}
}

void AutoImplementer::implement_list_clear(Function *f, const Class *t) {
	auto te = t->get_array_element();

	auto self = add_node_local(f->__get_var(Identifier::SELF));

// delete...
	if (auto f_del = te->get_destructor()) {

		auto *var_i = f->block->add_var("i", TypeInt32);
		auto *var_el = f->block->add_var("el", tree->type_ref(t->get_array_element()));

		Block *b = new Block(f, f->block.get());

		// __delete__
		auto cmd_delete = add_node_member_call(f_del, add_node_local(var_el)->deref());
		b->add(cmd_delete);

		auto cmd_for = add_node_statement(StatementID::FOR_CONTAINER);
		cmd_for->set_param(0, add_node_local(var_el));
		cmd_for->set_param(1, add_node_local(var_i));
		cmd_for->set_param(2, self);
		cmd_for->set_param(3, b);

		f->block->add(cmd_for);
	} else if (te->needs_destructor()) {
		do_error_implicit(f, "element destructor missing");
	}

	{
		// clear
		auto cmd_clear = add_node_member_call(t->get_member_func("__mem_clear__", TypeVoid, {}), self);
		f->block->add(cmd_clear);
	}
}

void AutoImplementer::implement_list_resize(Function *f, const Class *t) {
	if (!f)
		return;
	auto te = t->get_array_element();
	auto *var = f->block->add_var("i", TypeInt32);
	f->block->add_var("num_old", TypeInt32);

	auto num = add_node_local(f->__get_var("num"));

	auto self = add_node_local(f->__get_var(Identifier::SELF));

	auto self_num = sa_num(self);

	auto num_old = add_node_local(f->__get_var("num_old"));

	{
		// num_old = self.num
		f->block->add(add_node_operator_by_inline(InlineID::INT32_ASSIGN, num_old, self_num));
	}

// delete...
	if (auto f_del = te->get_destructor()) {
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
		cmd_for->set_param(3, const_int(1));
		cmd_for->set_param(4, b);
		f->block->add(cmd_for);

	} else if (te->needs_destructor()) {
		do_error_implicit(f, "element destructor missing");
	}

	{
		// resize
		auto c_resize = add_node_member_call(t->get_member_func("__mem_resize__", TypeVoid, {TypeInt32}), self);
		c_resize->set_param(1, num);
		f->block->add(c_resize);
	}

	// new...
	if (auto f_init = te->get_default_constructor()) {
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
		cmd_for->set_param(3, const_int(1));
		cmd_for->set_param(4, b);
		f->block->add(cmd_for);

	} else if (te->needs_constructor()) {
		do_error_implicit(f, "element default constructor missing");
	}
}


void AutoImplementer::implement_list_remove(Function *f, const Class *t) {
	if (!f)
		return;
	auto te = t->get_array_element();
	auto index = add_node_local(f->__get_var("index"));
	auto self = add_node_local(f->__get_var(Identifier::SELF));

	// delete...
	if (auto f_del = te->get_destructor()) {

		// el := self[index]
		auto cmd_el = add_node_dyn_array(self, index);

		// __delete__
		auto cmd_delete = add_node_member_call(f_del, cmd_el);
		f->block->params.add(cmd_delete);
	} else if (te->needs_destructor()) {
		do_error_implicit(f, "element destructor missing");
	}

	{
		// resize
		auto c_remove = add_node_member_call(t->get_member_func("__mem_remove__", TypeVoid, {TypeInt32}), self);
		c_remove->set_param(1, index);
		f->block->params.add(c_remove);
	}
}

void AutoImplementer::implement_list_add(Function *f, const Class *t) {
	if (!f)
		return;
	auto te = t->get_array_element();
	Block *b = f->block.get();
	auto item = add_node_local(b->get_var("x"));

	auto self = add_node_local(b->get_var(Identifier::SELF));

	{
		// resize(self.num + 1)
		auto cmd_add = add_node_operator_by_inline(InlineID::INT32_ADD, sa_num(self), const_int(1));
		auto cmd_resize = add_node_member_call(t->get_member_func("resize", TypeVoid, {TypeInt32}), self);
		cmd_resize->set_param(1, cmd_add);
		b->add(cmd_resize);
	}

	{
		// el := self.data[self.num - 1]
		auto cmd_sub = add_node_operator_by_inline(InlineID::INT32_SUBTRACT, sa_num(self), const_int(1));
		auto cmd_el = add_node_dyn_array(self, cmd_sub);

		b->add(add_assign(f, "", format("no operator %s = %s for elements found", te->long_name(), te->long_name()), cmd_el, item));
	}
}

void AutoImplementer::implement_list_equal(Function *f, const Class *t) {
	if (!f)
		return;
	auto te = t->get_array_element();
	auto other = add_node_local(f->__get_var("other"));
	auto self = add_node_local(f->__get_var(Identifier::SELF));

	{
		// if self.num != other.num
		//     return false
		auto n_eq = add_node_operator_by_inline(InlineID::INT32_NOT_EQUAL,  sa_num(self), sa_num(other));
		f->block->add(node_if(n_eq, node_return(node_false())));
	}

	{
		// for i=>e in self
		//     if e != other[i]
		//         return false
		auto *v_el = f->block->add_var("el", tree->type_ref(t->get_array_element()));
		auto *v_i = f->block->add_var("i", TypeInt32);

		Block *b = new Block(f, f->block.get());

		// other[i]
		auto n_other_el = add_node_dyn_array(other, add_node_local(v_i));

		auto n_if = add_node_statement(StatementID::IF);
		n_if->set_num_params(2);
		n_if->set_param(1, node_return(node_false()));
		b->add(n_if);

		if (auto n_neq = parser->con.link_operator_id(OperatorID::NOT_EQUAL, add_node_local(v_el)->deref(), n_other_el)) {
			n_if->set_param(0, n_neq);
		} else if (auto n_eq = parser->con.link_operator_id(OperatorID::EQUAL, add_node_local(v_el)->deref(), n_other_el)) {
			n_if->set_param(0, add_node_operator_by_inline(InlineID::BOOL_NOT, n_eq, nullptr));
		} else {
			do_error_implicit(f, format("neither operator %s != %s nor == found", te->long_name(), te->long_name()));
		}


		auto n_for = add_node_statement(StatementID::FOR_CONTAINER);
		// [VAR, INDEX, ARRAY, BLOCK]
		n_for->set_param(0, add_node_local(v_el));
		n_for->set_param(1, add_node_local(v_i));
		n_for->set_param(2, self);
		n_for->set_param(3, b);
		f->block->add(n_for);
	}

	{
		// return true
		f->block->add(node_return(node_true()));
	}
}

void AutoImplementer::implement_list_give(Function *f, const Class *t) {
	auto t_el = t->get_array_element();
	auto t_xfer = tree->request_implicit_class_xfer(t_el->param[0], -1);
	auto t_xfer_list = tree->request_implicit_class_list(t_xfer, -1);
	auto self = add_node_local(f->__get_var(Identifier::SELF));
	auto temp = add_node_local(f->block->add_var("temp", t_xfer_list));

	{
		// memcpy(temp, self)
		f->block->add(add_node_operator_by_inline(InlineID::CHUNK_ASSIGN, temp, self));
	}

	{
		// self.forget()
		f->block->add(add_node_member_call(t->get_member_func("__mem_forget__", TypeVoid, {}), self));
	}

	{
		// return temp
		f->block->add(node_return(temp));
	}
}

void AutoImplementer::_implement_functions_for_list(const Class *t) {
	implement_list_constructor(prepare_auto_impl(t, t->get_default_constructor()), t);
	implement_list_destructor(prepare_auto_impl(t, t->get_destructor()), t);
	implement_list_clear(prepare_auto_impl(t, t->get_member_func("clear", TypeVoid, {})), t);
	implement_list_resize(prepare_auto_impl(t, t->get_member_func("resize", TypeVoid, {TypeInt32})), t);
	implement_list_remove(prepare_auto_impl(t, t->get_member_func("remove", TypeVoid, {TypeInt32})), t);
	implement_list_add(prepare_auto_impl(t, t->get_member_func("add", TypeVoid, {nullptr})), t);
	if (t->param[0]->is_pointer_owned() or t->param[0]->is_pointer_owned_not_null()) {
		auto t_xfer = tree->request_implicit_class_xfer(t->param[0]->param[0], -1);
		auto t_xfer_list = tree->request_implicit_class_list(t_xfer, -1);
		implement_list_give(prepare_auto_impl(t, t->get_member_func(Identifier::Func::OWNED_GIVE, t_xfer_list, {})), t);
		implement_list_assign(prepare_auto_impl(t, t->get_member_func(Identifier::Func::ASSIGN, TypeVoid, {t_xfer_list})), t);
	}
	implement_list_assign(prepare_auto_impl(t, t->get_assign()), t);
	implement_list_equal(prepare_auto_impl(t, t->get_member_func(Identifier::Func::EQUAL, TypeBool, {t})), t);
}



Class* TemplateClassInstantiatorList::declare_new_instance(SyntaxTree *tree, const Array<const Class*> &params, int array_size, int token_id) {
	return create_raw_class(tree, class_name_might_need_parantheses(params[0]) + "[]", Class::Type::LIST, config.target.dynamic_array_size, config.target.pointer_size, -1, TypeDynamicArray, params, token_id);
}

void TemplateClassInstantiatorList::add_function_headers(Class* c) {
	//ai.complete_type(c, array_size, token_id);

	c->derive_from(TypeDynamicArray); // we already set its size!
	if (!class_can_default_construct(c->param[0]))
		c->owner->do_error(format("can not create a dynamic array from type '%s', missing default constructor", c->param[0]->long_name()), c->token_id);
	AutoImplementerInternal ai(nullptr, c->owner);
	//ai.add_missing_function_headers_for_class(c);


	ai.add_func_header(c, Identifier::Func::INIT, TypeVoid, {}, {}, nullptr, Flags::MUTABLE);
	ai.add_func_header(c, Identifier::Func::DELETE, TypeVoid, {}, {}, nullptr, Flags::MUTABLE);
	ai.add_func_header(c, "clear", TypeVoid, {}, {}, nullptr, Flags::MUTABLE);
	ai.add_func_header(c, "resize", TypeVoid, {TypeInt32}, {"num"}, nullptr, Flags::MUTABLE);
	if (c->param[0]->is_pointer_owned() or c->param[0]->is_pointer_owned_not_null()) {
		auto t_xfer = c->owner->request_implicit_class_xfer(c->param[0]->param[0], -1);
		auto t_xfer_list = c->owner->request_implicit_class_list(t_xfer, -1);
		ai.add_func_header(c, "add", TypeVoid, {t_xfer}, {"x"}, nullptr, Flags::MUTABLE);
		ai.add_func_header(c, Identifier::Func::OWNED_GIVE, t_xfer_list, {}, {}, nullptr, Flags::MUTABLE);
		//ai.add_func_header(c, Identifier::Func::ASSIGN, TypeVoid, {t_xfer_list}, {"other"});
		ai.add_func_header(c, Identifier::Func::ASSIGN, TypeVoid, {t_xfer_list}, {"other"}, nullptr, Flags::MUTABLE);
	} else if (c->param[0]->is_pointer_xfer_not_null()) {
		//	ai.add_func_header(c, "add", TypeVoid, {c->param[0]}, {"x"});
		ai.add_func_header(c, Identifier::Func::ASSIGN, TypeVoid, {c}, {"other"}, nullptr, Flags::MUTABLE);
	} else if (c->param[0]->is_reference()) {
		ai.add_func_header(c, "add", TypeVoid, {c->param[0]}, {"x"});
		ai.add_func_header(c, Identifier::Func::ASSIGN, TypeVoid, {c}, {"other"}, nullptr, Flags::MUTABLE);
	} else {
		ai.add_func_header(c, "add", TypeVoid, {c->param[0]}, {"x"}, nullptr, Flags::MUTABLE);
		if (class_can_assign(c->param[0]))
			ai.add_func_header(c, Identifier::Func::ASSIGN, TypeVoid, {c}, {"other"}, nullptr, Flags::MUTABLE);
	}
	ai.add_func_header(c, "remove", TypeVoid, {TypeInt32}, {"index"}, nullptr, Flags::MUTABLE);
	if (class_can_equal(c->param[0]))
		ai.add_func_header(c, Identifier::Func::EQUAL, TypeBool, {c}, {"other"}, nullptr, Flags::PURE);
}

}


