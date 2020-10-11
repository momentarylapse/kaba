#include "../kaba.h"
#include "../lib/common.h"
#include "../asm/asm.h"
#include "Parser.h"
#include "SyntaxTree.h"
#include <stdio.h>
#include "../../file/file.h"

namespace Kaba {



void Parser::do_error_implicit(Function *f, const string &str) {
	int line = max(f->_logical_line_no, f->name_space->_logical_line_no);
	int ex = max(f->_exp_no, f->name_space->_exp_no);
	do_error(format("[auto generating %s] : %s", f->signature(), str), ex, line);
}

void Parser::auto_implement_add_virtual_table(shared<Node> self, Function *f, const Class *t) {
	if (t->vtable.num > 0) {
		auto p = tree->shift_node(self, false, 0, TypePointer);
		auto *c = tree->add_constant_pointer(TypePointer, t->_vtable_location_target_);
		auto n_0 = tree->add_node_const(c);
		auto n_assign = tree->add_node_operator_by_inline(p, n_0, InlineID::POINTER_ASSIGN);
		f->block->add(n_assign);
	}
}

void Parser::auto_implement_add_child_constructors(shared<Node> n_self, Function *f, const Class *t) {
	int i0 = t->parent ? t->parent->elements.num : 0;
	foreachi(ClassElement &e, t->elements, i) {
		if (i < i0)
			continue;
		Function *ff = e.type->get_default_constructor();
		if (e.type->needs_constructor() and !ff)
			do_error_implicit(f, format("missing default constructor for element %s", e.name));
		if (!ff)
			continue;
		auto p = tree->shift_node(tree->cp_node(n_self), false, e.offset, e.type);
		auto c = tree->add_node_member_call(ff, p);
		f->block->add(c);
	}

	if (flags_has(t->flags, Flags::SHARED)) {
		for (auto &e: t->elements)
			if (e.name == IDENTIFIER_SHARED_COUNT and e.type == TypeInt) {
				auto p = tree->shift_node(tree->cp_node(n_self), false, e.offset, e.type);
				auto zero = tree->add_node_const(tree->add_constant_int(0));
				auto c = tree->add_node_operator_by_inline(p, zero, InlineID::INT_ASSIGN);
				f->block->add(c);
			}
	}
}

void Parser::auto_implement_constructor(Function *f, const Class *t, bool allow_parent_constructor) {
	if (!f)
		return;
	auto n_self = tree->add_node_local(f->__get_var(IDENTIFIER_SELF));

	if (t->is_super_array()) {
		auto n_el_size = tree->add_node_const(tree->add_constant_int(t->param->size));
		auto n_mem_init = tree->add_node_member_call(t->get_func("__mem_init__", TypeVoid, {TypeInt}), n_self);
		n_mem_init->set_param(1, n_el_size);
		f->block->add(n_mem_init);
	}else if (t->is_dict()) {
		auto n_el_size = tree->add_node_const(tree->add_constant_int(t->param->size + TypeString->size));
		auto n_mem_init = tree->add_node_member_call(t->get_func("__mem_init__", TypeVoid, {TypeInt}), n_self);
		n_mem_init->set_param(1, n_el_size);
		f->block->add(n_mem_init);
	}else if (t->is_array()) {
		auto *pc_el_init = t->param->get_default_constructor();
		if (t->param->needs_constructor() and !pc_el_init)
			do_error_implicit(f, format("missing default constructor for %s", t->param->long_name()));
		if (pc_el_init) {
			for (int i=0; i<t->array_length; i++) {
				auto n_el = tree->shift_node(tree->cp_node(n_self), false, t->param->size * i, t->param);
				auto n_init_el = tree->add_node_member_call(pc_el_init, n_el);
				f->block->add(n_init_el);
			}
		}
	} else if (t->is_pointer_shared()) {
		auto n_null = tree->add_node_const(tree->add_constant_pointer(t->param->get_pointer(), nullptr));
		auto n_op = tree->add_node_operator_by_inline(tree->shift_node(n_self, false, 0, TypePointer), n_null, InlineID::POINTER_ASSIGN);
		f->block->add(n_op);
	} else {

		// parent constructor
		if (t->parent and allow_parent_constructor) {
			Function *pc_same = t->parent->get_same_func(IDENTIFIER_FUNC_INIT, f);
			Function *pc_def = t->parent->get_default_constructor();
			if (pc_same) {
				// first, try same signature
				auto n_init_parent = tree->add_node_member_call(pc_same, tree->cp_node(n_self));
				for (int i=0; i<pc_same->num_params; i++)
					n_init_parent->set_param(i+1, tree->add_node_local(f->var[i].get()));
				f->block->add(n_init_parent);
			} else if (pc_def) {
				// then, try default constructor
				f->block->add(tree->add_node_member_call(pc_def, tree->cp_node(n_self)));
			} else if (t->parent->needs_constructor()) {
				do_error_implicit(f, "parent class does not have a default constructor or one with matching signature. Use super.__init__(...)");
			}
		}

		// call child constructors for elements
		auto_implement_add_child_constructors(n_self, f, t);

		// add vtable reference
		// after child constructor (otherwise would get overwritten)
		if (t->vtable.num > 0)
			auto_implement_add_virtual_table(tree->cp_node(n_self), f, t);
	}
}

void Parser::auto_implement_destructor(Function *f, const Class *t) {
	if (!f)
		return;
	auto n_self = tree->add_node_local(f->__get_var(IDENTIFIER_SELF));

	if (t->is_super_array() or t->is_dict()) {
		Function *f_clear = t->get_func("clear", TypeVoid, {});
		if (!f_clear)
			do_error_implicit(f, "clear() missing");
		f->block->add(tree->add_node_member_call(f_clear, n_self));
	} else if (t->is_array()) {
		auto *pc_el_init = t->param->get_destructor();
		if (pc_el_init) {
			for (int i=0; i<t->array_length; i++){
				auto p = tree->shift_node(tree->cp_node(n_self), false, t->param->size * i, t->param);
				auto c = tree->add_node_member_call(pc_el_init, p);
				f->block->add(c);
			}
		} else if (t->param->needs_destructor()) {
			do_error_implicit(f, "element desctructor missing");
		}
	} else if (t->is_pointer_shared()) {
		// call clear()
		auto f_clear = t->get_func(IDENTIFIER_FUNC_SHARED_CLEAR, TypeVoid, {});
		if (!f_clear)
			do_error_implicit(f, IDENTIFIER_FUNC_SHARED_CLEAR + "() missing");
		auto call_clear = tree->add_node_member_call(f_clear, n_self);
		f->block->add(call_clear);
	} else {

		// call child destructors
		int i0 = t->parent ? t->parent->elements.num : 0;
		foreachi(ClassElement &e, t->elements, i) {
			if (i < i0)
				continue;
			Function *ff = e.type->get_destructor();
			if (!ff and e.type->needs_destructor())
				do_error_implicit(f, format("missing destructor for element %s", e.name));
			if (!ff)
				continue;
			auto p = tree->shift_node(tree->cp_node(n_self), false, e.offset, e.type);
			f->block->add(tree->add_node_member_call(ff, p));
		}

		// parent destructor
		if (t->parent) {
			Function *ff = t->parent->get_destructor();
			if (ff)
				f->block->add(tree->add_node_member_call(ff, tree->cp_node(n_self), true));
			else if (t->parent->needs_destructor())
				do_error_implicit(f, "parent desctructor missing");
		}
	}
}

void Parser::auto_implement_assign(Function *f, const Class *t) {
	if (!f)
		return;
	auto n_other = tree->add_node_local(f->__get_var("other"));
	auto n_self = tree->add_node_local(f->__get_var(IDENTIFIER_SELF));

	if (t->is_super_array() or t->is_array()){

		if (t->is_super_array()) {
			Function *f_resize = t->get_func("resize", TypeVoid, {TypeInt});
			if (!f_resize)
				do_error_implicit(f, format("no %s.resize(int) found", t->long_name()));

			// self.resize(other.num)
			auto n_other_num = tree->shift_node(n_other, false, config.pointer_size, TypeInt);

			auto n_resize = tree->add_node_member_call(f_resize, n_self);
			n_resize->set_num_params(2);
			n_resize->set_param(1, n_other_num);
			f->block->add(n_resize);
		}

		// for el,i in *self
		//    el = other[i]

		auto *v_el = f->block->add_var("el", t->get_array_element()->get_pointer());
		auto *v_i = f->block->add_var("i", TypeInt);

		Block *b = new Block(f, f->block.get());

		// other[i]
		shared<Node> n_other_el;
		if (t->is_array())
			n_other_el = tree->add_node_array(tree->cp_node(n_other), tree->add_node_local(v_i));
		else
			n_other_el = tree->add_node_dyn_array(tree->cp_node(n_other), tree->add_node_local(v_i));

		auto n_assign = link_operator_id(OperatorID::ASSIGN, tree->deref_node(tree->add_node_local(v_el)), n_other_el);
		if (!n_assign)
			do_error_implicit(f, format("no %s.__assign__() found", t->param->long_name()));
		b->add(n_assign);

		auto n_for = tree->add_node_statement(StatementID::FOR_ARRAY);
		// [VAR, INDEX, ARRAY, BLOCK]
		n_for->set_param(0, tree->add_node_local(v_el));
		n_for->set_param(1, tree->add_node_local(v_i));
		n_for->set_param(2, tree->cp_node(n_self));
		n_for->set_param(3, b);
		f->block->add(n_for);

	} else {

		// parent assignment
		if (t->parent) {
			auto p = tree->cp_node(n_self);
			auto o = tree->cp_node(n_other);
			p->type = o->type = t->parent;

			auto cmd_assign = link_operator_id(OperatorID::ASSIGN, p, o);
			if (!cmd_assign)
				do_error_implicit(f, "missing parent default constructor");
			f->block->add(cmd_assign);
		}

		// call child assignment
		int i0 = t->parent ? t->parent->elements.num : 0;
		foreachi(ClassElement &e, t->elements, i) {
			if (i < i0)
				continue;
			auto p = tree->shift_node(tree->cp_node(n_self), false, e.offset, e.type);
			auto o = tree->shift_node(tree->cp_node(n_other), false, e.offset, e.type); // needed for call-by-ref conversion!

			auto n_assign = link_operator_id(OperatorID::ASSIGN, p, o);
			if (!n_assign)
				do_error_implicit(f, format("no %s.__assign__ for element \"%s\"", e.type->long_name(), e.name));
			f->block->add(n_assign);
		}
	}
}


void Parser::auto_implement_array_clear(Function *f, const Class *t) {
	if (!f)
		return;

	auto self = tree->add_node_local(f->__get_var(IDENTIFIER_SELF));

// delete...
	Function *f_del = t->param->get_destructor();
	if (f_del) {

		auto *var_i = f->block->add_var("i", TypeInt);
		auto *var_el = f->block->add_var("el", t->get_array_element()->get_pointer());

		Block *b = new Block(f, f->block.get());

		// __delete__
		auto cmd_delete = tree->add_node_member_call(f_del, tree->deref_node(tree->add_node_local(var_el)));
		b->add(cmd_delete);

		auto cmd_for = tree->add_node_statement(StatementID::FOR_ARRAY);
		cmd_for->set_param(0, tree->add_node_local(var_el));
		cmd_for->set_param(1, tree->add_node_local(var_i));
		cmd_for->set_param(2, tree->cp_node(self));
		cmd_for->set_param(3, b);

		f->block->add(cmd_for);
	} else if (t->param->needs_destructor()) {
		do_error_implicit(f, "element destructor missing");
	}

	// clear
	auto cmd_clear = tree->add_node_member_call(t->get_func("__mem_clear__", TypeVoid, {}), self);
	f->block->add(cmd_clear);
}


void Parser::auto_implement_array_resize(Function *f, const Class *t) {
	if (!f)
		return;
	auto *var = f->block->add_var("i", TypeInt);
	f->block->add_var("num_old", TypeInt);

	auto num = tree->add_node_local(f->__get_var("num"));

	auto self = tree->add_node_local(f->__get_var(IDENTIFIER_SELF));

	auto self_num = tree->shift_node(self, false, config.pointer_size, TypeInt);

	auto num_old = tree->add_node_local(f->__get_var("num_old"));

	// num_old = self.num
	f->block->add(tree->add_node_operator_by_inline(num_old, self_num, InlineID::INT_ASSIGN));

// delete...
	Function *f_del = t->param->get_destructor();
	if (f_del) {

		Block *b = new Block(f, f->block.get());

		// el := self[i]
		auto el = tree->add_node_dyn_array(tree->cp_node(self), tree->add_node_local(var));

		// __delete__
		auto cmd_delete = tree->add_node_member_call(f_del, el);
		b->add(cmd_delete);

		//  [VAR, START, STOP, STEP, BLOCK]
		auto cmd_for = tree->add_node_statement(StatementID::FOR_RANGE);
		cmd_for->set_param(0, tree->add_node_local(var));
		cmd_for->set_param(1, tree->cp_node(num));
		cmd_for->set_param(2, tree->cp_node(self_num));
		cmd_for->set_param(3, tree->add_node_const(tree->add_constant_int(1)));
		cmd_for->set_param(4, b);
		f->block->add(cmd_for);

	} else if (t->param->needs_destructor()) {
		do_error_implicit(f, "element destructor missing");
	}

	// resize
	auto c_resize = tree->add_node_member_call(t->get_func("__mem_resize__", TypeVoid, {TypeInt}), tree->cp_node(self));
	c_resize->set_param(1, num);
	f->block->add(c_resize);

	// new...
	Function *f_init = t->param->get_default_constructor();
	if (f_init) {

		Block *b = new Block(f, f->block.get());

		// el := self[i]
		auto el = tree->add_node_dyn_array(tree->cp_node(self), tree->add_node_local(var));

		// __init__
		auto cmd_init = tree->add_node_member_call(f_init, el);
		b->add(cmd_init);

		//  [VAR, START, STOP, STEP, BLOCK]
		auto cmd_for = tree->add_node_statement(StatementID::FOR_RANGE);
		cmd_for->set_param(0, tree->add_node_local(var));
		cmd_for->set_param(1, tree->cp_node(num_old));
		cmd_for->set_param(2, tree->cp_node(self_num));
		cmd_for->set_param(3, tree->add_node_const(tree->add_constant_int(1)));
		cmd_for->set_param(4, b);
		f->block->add(cmd_for);

	} else if (t->param->needs_constructor()) {
		do_error_implicit(f, "element default constructor missing");
	}
}


void Parser::auto_implement_array_remove(Function *f, const Class *t) {
	if (!f)
		return;

	auto index = tree->add_node_local(f->__get_var("index"));
	auto self = tree->add_node_local(f->__get_var(IDENTIFIER_SELF));

	// delete...
	Function *f_del = t->param->get_destructor();
	if (f_del) {

		// el := self[index]
		auto cmd_el = tree->add_node_dyn_array(tree->cp_node(self), tree->cp_node(index));

		// __delete__
		auto cmd_delete = tree->add_node_member_call(f_del, cmd_el);
		f->block->params.add(cmd_delete);
	} else if (t->param->needs_destructor()) {
		do_error_implicit(f, "element destructor missing");
	}

	// resize
	auto c_remove = tree->add_node_member_call(t->get_func("__mem_remove__", TypeVoid, {TypeInt}), self);
	c_remove->set_param(1, index);
	f->block->params.add(c_remove);
}

void Parser::auto_implement_array_add(Function *f, const Class *t) {
	if (!f)
		return;
	Block *b = f->block.get();
	auto item = tree->add_node_local(b->get_var("x"));

	auto self = tree->add_node_local(b->get_var(IDENTIFIER_SELF));

	auto self_num = tree->shift_node(tree->cp_node(self), false, config.pointer_size, TypeInt);


	// resize(self.num + 1)
	auto cmd_1 = tree->add_node_const(tree->add_constant_int(1));
	auto cmd_add = tree->add_node_operator_by_inline(self_num, cmd_1, InlineID::INT_ADD);
	auto cmd_resize = tree->add_node_member_call(t->get_func("resize", TypeVoid, {TypeInt}), self);
	cmd_resize->set_param(1, cmd_add);
	b->add(cmd_resize);


	// el := self.data[self.num - 1]
	auto cmd_sub = tree->add_node_operator_by_inline(tree->cp_node(self_num), tree->cp_node(cmd_1), InlineID::INT_SUBTRACT);
	auto cmd_el = tree->add_node_dyn_array(tree->cp_node(self), cmd_sub);

	auto cmd_assign = link_operator_id(OperatorID::ASSIGN, cmd_el, item);
	if (!cmd_assign)
		do_error_implicit(f, format("no %s.%s for elements", t->param->long_name(), IDENTIFIER_FUNC_ASSIGN));
	b->add(cmd_assign);
}



void Parser::auto_implement_shared_assign(Function *f, const Class *t) {
	if (!f)
		return;
	auto p = tree->add_node_local(f->__get_var("p"));
	auto self = tree->add_node_local(f->__get_var(IDENTIFIER_SELF));

	auto self_p = tree->shift_node(tree->cp_node(self), false, 0, t->param->get_pointer());

	// call clear()
	auto f_clear = t->get_func(IDENTIFIER_FUNC_SHARED_CLEAR, TypeVoid, {});
	if (!f_clear)
		do_error_implicit(f, IDENTIFIER_FUNC_SHARED_CLEAR + "() missing");
	auto call_clear = tree->add_node_member_call(f_clear, self);
	f->block->add(call_clear);


	auto op = tree->add_node_operator_by_inline(self_p, tree->cp_node(p), InlineID::POINTER_ASSIGN);
	f->block->add(op);


	// if p
	//     p.count ++
	auto cmd_if = tree->add_node_statement(StatementID::IF);
	f->block->add(cmd_if);

	// if p
	auto ff = tree->required_func_global("p2b");
	auto cmd_cmp = tree->add_node_call(ff);
	cmd_cmp->set_param(0, tree->cp_node(p));
	cmd_if->set_param(0, cmd_cmp);

	auto b = new Block(f, f->block.get());
	cmd_if->set_param(1, b);


	auto tt = self->type->param;
	bool found = false;
	for (auto &e: tt->elements)
		if (e.name == IDENTIFIER_SHARED_COUNT and e.type == TypeInt) {
			// count ++
			auto count = tree->shift_node(tree->deref_node(tree->cp_node(self_p)), false, e.offset, e.type);
			auto inc = tree->add_node_operator_by_inline(count, nullptr, InlineID::INT_INCREASE);
			b->add(inc);
			found = true;
		}
	if (!found)
		do_error_implicit(f, format("class '%s' is not a shared class (declare with '%s class' or add an element 'int %s')", tt->long_name(), IDENTIFIER_SHARED, IDENTIFIER_SHARED_COUNT));
}

void Parser::auto_implement_shared_clear(Function *f, const Class *t) {
	if (!f)
		return;
	auto self = tree->add_node_local(f->__get_var(IDENTIFIER_SELF));
	auto self_p = tree->shift_node(tree->cp_node(self), false, 0, t->param->get_pointer());

	auto tt = t->param;

	// if self.p
	//     self.p.count --
	//     if self.p.count == 0
	//         del self.p
	//     self.p = nil

	auto cmd_if = tree->add_node_statement(StatementID::IF);
	f->block->add(cmd_if);

	// if self.p
	auto ff = tree->required_func_global("p2b");
	auto cmd_cmp = tree->add_node_call(ff);
	cmd_cmp->set_param(0, tree->cp_node(self_p));
	cmd_if->set_param(0, cmd_cmp);

	auto b = new Block(f, f->block.get());
	cmd_if->set_param(1, b);


	shared<Node> count;
	for (auto &e: tt->elements)
		if (e.name == IDENTIFIER_SHARED_COUNT and e.type == TypeInt)
			count = tree->shift_node(tree->cp_node(self_p), true, e.offset, e.type);
	if (!count)
		do_error_implicit(f, format("class '%s' is not a shared class (declare with '%s class' or add an element 'int %s')", tt->long_name(), IDENTIFIER_SHARED, IDENTIFIER_SHARED_COUNT));

	// count --
	auto dec = tree->add_node_operator_by_inline(count, nullptr, InlineID::INT_DECREASE);
	b->add(dec);


	auto cmd_if_del = tree->add_node_statement(StatementID::IF);
	b->add(cmd_if_del);

	// if count == 0
	auto zero = tree->add_node_const(tree->add_constant_int(0));
	auto cmp = tree->add_node_operator_by_inline(tree->cp_node(count), zero, InlineID::INT_EQUAL);
	cmd_if_del->set_param(0, cmp);

	auto b2 = new Block(f, b);


	// del self
	auto cmd_del = tree->add_node_statement(StatementID::DELETE);
	cmd_del->set_param(0, tree->cp_node(self_p));
	b2->add(cmd_del);
	cmd_if_del->set_param(1, b2);


	// self = nil
	auto n_null = tree->add_node_const(tree->add_constant_pointer(t->param->get_pointer(), nullptr));
	auto n_op = tree->add_node_operator_by_inline(self_p, n_null, InlineID::POINTER_ASSIGN);
	b->add(n_op);
}


void Parser::auto_implement_shared_create(Function *f, const Class *t) {
	if (!f)
		return;
	auto p = tree->add_node_local(f->__get_var("p"));
	auto r = tree->add_node_local(f->__get_var(IDENTIFIER_RETURN_VAR));


	// r = p
	auto f_assign = t->get_func(IDENTIFIER_FUNC_ASSIGN, TypeVoid, {p->type});
	if (!f_assign)
		do_error_implicit(f, "= missing...");
	auto call_assign = tree->add_node_member_call(f_assign, tree->cp_node(r));
	call_assign->set_param(1, p);
	f->block->add(call_assign);

	// return r
	auto ret = tree->add_node_statement(StatementID::RETURN);
	ret->set_num_params(1);
	ret->set_param(0, r);
	f->block->add(ret);
}


void SyntaxTree::add_func_header(Class *t, const string &name, const Class *return_type, const Array<const Class*> &param_types, const Array<string> &param_names, Function *cf, Flags flags) {
	Function *f = add_function(name, return_type, t, flags); // always member-function??? no...?
	f->auto_declared = true;
	foreachi (auto &p, param_types, i) {
		f->literal_param_type.add(p);
		auto v = f->block->add_var(param_names[i], p);
		v->is_const = true;
		f->num_params ++;
	}
	f->update_parameters_after_parsing();
	bool override = cf;
	t->add_function(this, f, false, override);
}

bool needs_new(Function *f) {
	if (!f)
		return true;
	return f->needs_overriding;
}

Array<string> class_func_param_names(Function *cf) {
	Array<string> names;
	auto *f = cf;
	for (int i=0; i<f->num_params; i++)
		names.add(f->var[i]->name);
	return names;
}

bool has_user_constructors(const Class *t) {
	for (auto *cc: t->get_constructors())
		if (!cc->needs_overriding)
			return true;
	return false;
}

void remove_inherited_constructors(Class *t) {
	for (int i=t->functions.num-1; i>=0; i--)
		if (t->functions[i]->name == IDENTIFIER_FUNC_INIT and t->functions[i]->needs_overriding)
			t->functions.erase(i);
}

void redefine_inherited_constructors(Class *t, SyntaxTree *tree) {
	for (auto *pcc: t->parent->get_constructors()) {
		auto c = t->get_same_func(IDENTIFIER_FUNC_INIT, pcc);
		if (needs_new(c))
			tree->add_func_header(t, IDENTIFIER_FUNC_INIT, TypeVoid, pcc->literal_param_type, class_func_param_names(pcc), c);
	}
}

void SyntaxTree::add_missing_function_headers_for_class(Class *t) {
	if (t->owner != this)
		return;
	if (t->is_pointer())
		return;

	if (t->is_super_array()) {
		add_func_header(t, IDENTIFIER_FUNC_INIT, TypeVoid, {}, {});
		add_func_header(t, IDENTIFIER_FUNC_DELETE, TypeVoid, {}, {});
		add_func_header(t, "clear", TypeVoid, {}, {});
		add_func_header(t, "resize", TypeVoid, {TypeInt}, {"num"});
		add_func_header(t, "add", TypeVoid, {t->param}, {"x"});
		add_func_header(t, "remove", TypeVoid, {TypeInt}, {"index"});
		add_func_header(t, IDENTIFIER_FUNC_ASSIGN, TypeVoid, {t}, {"other"});
	} else if (t->is_array()) {
		if (t->needs_constructor())
			add_func_header(t, IDENTIFIER_FUNC_INIT, TypeVoid, {}, {});
		if (t->needs_destructor())
			add_func_header(t, IDENTIFIER_FUNC_DELETE, TypeVoid, {}, {});
		add_func_header(t, IDENTIFIER_FUNC_ASSIGN, TypeVoid, {t}, {"other"});
	} else if (t->is_dict()) {
		add_func_header(t, IDENTIFIER_FUNC_INIT, TypeVoid, {}, {});
		add_func_header(t, IDENTIFIER_FUNC_DELETE, TypeVoid, {}, {});
		add_func_header(t, "clear", TypeVoid, {}, {});
		add_func_header(t, "add", TypeVoid, {TypeString, t->param}, {"key", "x"});
		add_func_header(t, IDENTIFIER_FUNC_GET, t->param, {TypeString}, {"key"});
		add_func_header(t, IDENTIFIER_FUNC_ASSIGN, TypeVoid, {t}, {"other"});
	} else if (t->is_pointer_shared()) {
		add_func_header(t, IDENTIFIER_FUNC_INIT, TypeVoid, {}, {});
		add_func_header(t, IDENTIFIER_FUNC_DELETE, TypeVoid, {}, {});
		add_func_header(t, IDENTIFIER_FUNC_SHARED_CLEAR, TypeVoid, {}, {});
		add_func_header(t, IDENTIFIER_FUNC_ASSIGN, TypeVoid, {t->param->get_pointer()}, {"p"});
		add_func_header(t, IDENTIFIER_FUNC_ASSIGN, TypeVoid, {t}, {"p"});
		add_func_header(t, IDENTIFIER_FUNC_SHARED_CREATE, t, {t->param->get_pointer()}, {"p"}, nullptr, Flags::STATIC);
	} else { // regular classes
		if (!t->is_simple_class()) {
			if (t->parent) {
				if (has_user_constructors(t)) {
					// don't inherit constructors!
					remove_inherited_constructors(t);
				} else {
					// only auto-implement matching constructors
					redefine_inherited_constructors(t, this);
				}
			}
			if (t->needs_constructor() and t->get_constructors().num == 0)
				add_func_header(t, IDENTIFIER_FUNC_INIT, TypeVoid, {}, {}, t->get_default_constructor());
			if (needs_new(t->get_destructor()))
				add_func_header(t, IDENTIFIER_FUNC_DELETE, TypeVoid, {}, {}, t->get_destructor());
		}
		if (needs_new(t->get_assign())) {
			//add_func_header(t, NAME_FUNC_ASSIGN, TypeVoid, t, "other");
			// implement only if parent has also done so
			if (t->parent) {
				if (t->parent->get_assign())
					add_func_header(t, IDENTIFIER_FUNC_ASSIGN, TypeVoid, {t}, {"other"}, t->get_assign());
			} else {
				add_func_header(t, IDENTIFIER_FUNC_ASSIGN, TypeVoid, {t}, {"other"}, t->get_assign());
			}
		}
		if (t->get_assign() and t->is_simple_class()) {
			t->get_assign()->inline_no = InlineID::CHUNK_ASSIGN;
		}
	}
}

Function* class_get_func(const Class *t, const string &name, const Class *return_type, const Array<const Class*> &params) {
	Function *cf = t->get_func(name, return_type, params);
	if (cf) {
		Function *f = cf;
		f->needs_overriding = false; // we're about to implement....
		if (f->auto_declared) {
			return f;
		}
		return nullptr;
	}
	//t->owner->DoError("class_get_func... " + t->name + "." + name);
	return nullptr;
}

Function* prepare_auto_impl(const Class *t, Function *f) {
	if (!f)
		return nullptr;
	if (f->auto_declared) {
		f->needs_overriding = false; // we're about to implement....
		return f;
	}
	return nullptr;
	t->owner->script->do_error_internal("prepare class func..." + f->signature());
	return f;
}

// completely create and implement
void Parser::auto_implement_functions(const Class *t) {
	if (t->owner != tree)
		return;
	if (t->is_pointer())
		return;

	auto sub_classes = t->classes; // might change

	if (t->is_super_array()) {
		auto_implement_constructor(prepare_auto_impl(t, t->get_default_constructor()), t, true);
		auto_implement_destructor(prepare_auto_impl(t, t->get_destructor()), t);
		auto_implement_array_clear(prepare_auto_impl(t, t->get_func("clear", TypeVoid, {})), t);
		auto_implement_array_resize(prepare_auto_impl(t, t->get_func("resize", TypeVoid, {TypeInt})), t);
		auto_implement_array_remove(prepare_auto_impl(t, t->get_func("remove", TypeVoid, {TypeInt})), t);
		auto_implement_array_add(class_get_func(t, "add", TypeVoid, {nullptr}), t);
		auto_implement_assign(prepare_auto_impl(t, t->get_assign()), t);
	} else if (t->is_array()) {
		auto_implement_constructor(prepare_auto_impl(t, t->get_default_constructor()), t, true);
		auto_implement_destructor(prepare_auto_impl(t, t->get_destructor()), t);
		auto_implement_assign(prepare_auto_impl(t, t->get_assign()), t);
	} else if (t->is_dict()) {
		auto_implement_constructor(prepare_auto_impl(t, t->get_default_constructor()), t, true);
	} else if (t->is_pointer_shared()) {
		auto_implement_constructor(prepare_auto_impl(t, t->get_default_constructor()), t, true);
		auto_implement_destructor(prepare_auto_impl(t, t->get_destructor()), t);
		auto_implement_shared_clear(prepare_auto_impl(t, t->get_func(IDENTIFIER_FUNC_SHARED_CLEAR, TypeVoid, {})), t);
		auto_implement_shared_assign(prepare_auto_impl(t, t->get_func(IDENTIFIER_FUNC_ASSIGN, TypeVoid, {t->param->get_pointer()})), t);
		auto_implement_shared_assign(prepare_auto_impl(t, t->get_func(IDENTIFIER_FUNC_ASSIGN, TypeVoid, {t})), t);
		auto_implement_shared_create(prepare_auto_impl(t, t->get_func(IDENTIFIER_FUNC_SHARED_CREATE, t, {t->param->get_pointer()})), t);
	} else {
		for (auto *cf: t->get_constructors())
			auto_implement_constructor(prepare_auto_impl(t, cf), t, true);
		auto_implement_destructor(prepare_auto_impl(t, t->get_destructor()), t); // if exists...
		auto_implement_assign(prepare_auto_impl(t, t->get_assign()), t); // if exists...

	}

	// recursion
	//for (auto *c: sub_classes)
	//	auto_implement_functions(c);
}


}
