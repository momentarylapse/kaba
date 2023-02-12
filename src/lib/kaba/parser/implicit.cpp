#include "../kaba.h"
#include "implicit.h"
#include "../asm/asm.h"
#include "Parser.h"
#include "../syntax/SyntaxTree.h"
#include <stdio.h>
#include "../../os/msg.h"
#include "../../base/iter.h"

namespace kaba {


const int FULL_CONSTRUCTOR_MAX_PARAMS = 4;


AutoImplementer::AutoImplementer(Parser *p, SyntaxTree *t) {
	parser = p;
	tree = t;
}

shared<Node> AutoImplementer::node_false() {
	auto c_false = tree->add_constant(TypeBool);
	c_false->as_int() = 0;
	return add_node_const(c_false);
}

shared<Node> AutoImplementer::node_true() {
	auto c_false = tree->add_constant(TypeBool);
	c_false->as_int() = 1;
	return add_node_const(c_false);
}

void AutoImplementer::db_add_print_node(shared<Block> block, shared<Node> node) {
	auto ff = tree->required_func_global("print");
	auto cmd = add_node_call(ff);
	cmd->set_param(0, parser->con.add_converter_str(node, false));
	block->add(cmd);
}

void AutoImplementer::db_add_print_label(shared<Block> block, const string &s) {
	auto c = tree->add_constant(TypeString);
	c->as_string() = s;
	db_add_print_node(block, add_node_const(c));
}

void AutoImplementer::db_add_print_label_node(shared<Block> block, const string &s, shared<Node> node) {
	auto c = tree->add_constant(TypeString);
	c->as_string() = s;

	auto ff = tree->required_func_global("print");
	auto cmd = add_node_call(ff);
	cmd->set_param(0, parser->con.link_operator_id(OperatorID::ADD, add_node_const(c), parser->con.add_converter_str(node, false)));
	block->add(cmd);
}



void AutoImplementer::do_error_implicit(Function *f, const string &str) {
	int token_id = max(f->token_id, f->name_space->token_id);
	tree->do_error(format("[auto generating %s] : %s", f->signature(), str), token_id);
}





// skip the "self" parameter!
Function *AutoImplementer::add_func_header(Class *t, const string &name, const Class *return_type, const Array<const Class*> &param_types, const Array<string> &param_names, Function *cf, Flags flags, const shared_array<Node> &def_params) {
	Function *f = tree->add_function(name, return_type, t, flags); // always member-function??? no...?
	f->auto_declared = true;
	f->token_id = t->token_id;
	for (auto&& [i,p]: enumerate(param_types)) {
		f->literal_param_type.add(p);
		auto v = f->block->add_var(param_names[i], p);
		flags_set(v->flags, Flags::CONST);
		f->num_params ++;
	}
	f->default_parameters = def_params;
	f->update_parameters_after_parsing();
	if (config.verbose)
		msg_write("ADD HEADER " + f->signature(TypeVoid));
	bool override = cf;
	t->add_function(tree, f, false, override);
	return f;
}

bool AutoImplementer::needs_new(Function *f) {
	if (!f)
		return true;
	return f->needs_overriding();
}

Array<string> AutoImplementer::class_func_param_names(Function *cf) {
	Array<string> names;
	auto *f = cf;
	for (int i=0; i<f->num_params; i++)
		names.add(f->var[i]->name);
	return names;
}

bool AutoImplementer::has_user_constructors(const Class *t) {
	for (auto *cc: t->get_constructors())
		if (!cc->needs_overriding())
			return true;
	return false;
}

void AutoImplementer::remove_inherited_constructors(Class *t) {
	for (int i=t->functions.num-1; i>=0; i--)
		if (t->functions[i]->name == Identifier::Func::INIT and t->functions[i]->needs_overriding())
			t->functions.erase(i);
}

void AutoImplementer::redefine_inherited_constructors(Class *t) {
	for (auto *pcc: t->parent->get_constructors()) {
		auto c = t->get_same_func(Identifier::Func::INIT, pcc);
		if (needs_new(c)) {
			auto ff = add_func_header(t, Identifier::Func::INIT, TypeVoid, pcc->literal_param_type, class_func_param_names(pcc), c, Flags::NONE, pcc->default_parameters);
		}
	}
}

void AutoImplementer::add_full_constructor(Class *t) {
	Array<string> names;
	Array<const Class*> types;
	for (auto &e: t->elements) {
		if (!e.hidden()) {
			names.add(e.name);
			types.add(e.type);
		}
	}
	auto f = add_func_header(t, Identifier::Func::INIT, TypeVoid, types, names);
	flags_set(f->flags, Flags::__INIT_FILL_ALL_PARAMS);
}

bool AutoImplementer::can_fully_construct(const Class *t) {
	if (t->vtable.num > 0)
		return false;
	if (t->elements.num > FULL_CONSTRUCTOR_MAX_PARAMS)
		return false;
	int num_el = 0;
	for (auto &e: t->elements) {
		if (e.hidden())
			continue;
		if (!e.type->get_assign() and e.type->uses_call_by_reference()) {
			msg_write(format("class %s auto constructor prevented by element %s %s", t->name, e.name, e.type->name));
			return false;
		}
		num_el ++;
	}
	return num_el > 0;
}

bool AutoImplementer::class_can_assign(const Class *t) {
	if (t->is_pointer())
		return true;
	if (t->get_assign())
		return true;
	return false;
}

void AutoImplementer::add_missing_function_headers_for_class(Class *t) {
	if (t->owner != tree)
		return;
	if (t->is_pointer())
		return;

	if (t->is_super_array()) {
		_add_missing_function_headers_for_super_array(t);
	} else if (t->is_array()) {
		_add_missing_function_headers_for_array(t);
	} else if (t->is_dict()) {
		_add_missing_function_headers_for_dict(t);
	} else if (t->is_pointer_shared()) {
		_add_missing_function_headers_for_shared(t);
	} else if (t->is_pointer_owned()) {
		_add_missing_function_headers_for_owned(t);
	} else if (t->is_product()) {
		_add_missing_function_headers_for_product(t);
	} else if (t->is_callable_fp()) {
		_add_missing_function_headers_for_callable_fp(t);
	} else if (t->is_callable_bind()) {
		_add_missing_function_headers_for_callable_bind(t);
	} else if (t->is_enum()) {
		_add_missing_function_headers_for_enum(t);
	} else if (t->is_optional()) {
		_add_missing_function_headers_for_optional(t);
	} else { // regular classes
		_add_missing_function_headers_for_regular(t);
	}
}

Function* class_get_member_func(const Class *t, const string &name, const Class *return_type, const Array<const Class*> &params) {
	Function *cf = t->get_member_func(name, return_type, params);
	if (cf) {
		Function *f = cf;
		flags_clear(f->flags, Flags::NEEDS_OVERRIDE); // we're about to implement....
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
		flags_clear(f->flags, Flags::NEEDS_OVERRIDE); // we're about to implement....
		return f;
	}
	return nullptr;
	t->owner->module->do_error_internal("prepare class func..." + f->signature());
	return f;
}

// completely create and implement
void AutoImplementer::implement_functions(const Class *t) {
	if (t->owner != tree)
		return;
	if (t->is_pointer())
		return;

	auto sub_classes = t->classes; // might change

	if (t->is_super_array()) {
		implement_super_array_constructor(prepare_auto_impl(t, t->get_default_constructor()), t);
		implement_super_array_destructor(prepare_auto_impl(t, t->get_destructor()), t);
		implement_super_array_clear(prepare_auto_impl(t, t->get_member_func("clear", TypeVoid, {})), t);
		implement_super_array_resize(prepare_auto_impl(t, t->get_member_func("resize", TypeVoid, {TypeInt})), t);
		implement_super_array_remove(prepare_auto_impl(t, t->get_member_func("remove", TypeVoid, {TypeInt})), t);
		implement_super_array_add(class_get_member_func(t, "add", TypeVoid, {nullptr}), t);
		implement_super_array_assign(prepare_auto_impl(t, t->get_assign()), t);
	} else if (t->is_array()) {
		implement_array_constructor(prepare_auto_impl(t, t->get_default_constructor()), t);
		implement_array_destructor(prepare_auto_impl(t, t->get_destructor()), t);
		implement_array_assign(prepare_auto_impl(t, t->get_assign()), t);
	} else if (t->is_dict()) {
		implement_super_array_constructor(prepare_auto_impl(t, t->get_default_constructor()), t);
		implement_super_array_destructor(prepare_auto_impl(t, t->get_destructor()), t);
	} else if (t->is_pointer_shared()) {
		implement_shared_constructor(prepare_auto_impl(t, t->get_default_constructor()), t);
		implement_shared_destructor(prepare_auto_impl(t, t->get_destructor()), t);
		implement_shared_clear(prepare_auto_impl(t, t->get_member_func(Identifier::Func::SHARED_CLEAR, TypeVoid, {})), t);
		implement_shared_assign(prepare_auto_impl(t, t->get_member_func(Identifier::Func::ASSIGN, TypeVoid, {tree->get_pointer(t->param[0])})), t);
		implement_shared_assign(prepare_auto_impl(t, t->get_member_func(Identifier::Func::ASSIGN, TypeVoid, {t})), t);
		implement_shared_create(prepare_auto_impl(t, t->get_func(Identifier::Func::SHARED_CREATE, t, {tree->get_pointer(t->param[0])})), t);
	} else if (t->is_pointer_owned()) {
		implement_shared_constructor(prepare_auto_impl(t, t->get_default_constructor()), t);
		implement_shared_destructor(prepare_auto_impl(t, t->get_destructor()), t);
		implement_owned_clear(prepare_auto_impl(t, t->get_member_func(Identifier::Func::SHARED_CLEAR, TypeVoid, {})), t);
		implement_owned_assign(prepare_auto_impl(t, t->get_member_func(Identifier::Func::ASSIGN, TypeVoid, {tree->get_pointer(t->param[0])})), t);
		//implement_shared_assign(prepare_auto_impl(t, t->get_func(Identifier::Func::ASSIGN, TypeVoid, {nullptr, t})), t);
		//implement_shared_create(prepare_auto_impl(t, t->get_func(Identifier::Func::SHARED_CREATE, t, {nullptr, tree->get_pointer(t->param[0])})), t);
	} else if (t->is_callable_fp()) {
		for (auto *cf: t->get_constructors())
			implement_callable_constructor(prepare_auto_impl(t, cf), t);
		implement_callable_fp_call(prepare_auto_impl(t, t->get_call()), t);
	} else if (t->is_callable_bind()) {
		for (auto *cf: t->get_constructors())
			implement_callable_constructor(prepare_auto_impl(t, cf), t);
		implement_callable_bind_call(prepare_auto_impl(t, t->get_call()), t);
	} else if (t->is_optional()) {
		implement_optional_constructor(prepare_auto_impl(t, t->get_default_constructor()), t);
		implement_optional_constructor(prepare_auto_impl(t, t->get_member_func(Identifier::Func::INIT, TypeVoid, {TypePointer})), t);
		implement_optional_constructor_wrap(prepare_auto_impl(t, t->get_member_func(Identifier::Func::INIT, TypeVoid, {t->param[0]})), t);
		implement_optional_destructor(prepare_auto_impl(t, t->get_destructor()), t);
		implement_optional_assign(prepare_auto_impl(t, t->get_member_func(Identifier::Func::ASSIGN, TypeVoid, {t})), t);
		implement_optional_assign_raw(prepare_auto_impl(t, t->get_member_func(Identifier::Func::ASSIGN, TypeVoid, {t->param[0]})), t);
		implement_optional_assign_null(prepare_auto_impl(t, t->get_member_func(Identifier::Func::ASSIGN, TypeVoid, {TypePointer})), t);
		implement_optional_has_value(prepare_auto_impl(t, t->get_member_func(Identifier::Func::OPTIONAL_HAS_VALUE, TypeBool, {})), t);
		implement_optional_has_value(prepare_auto_impl(t, t->get_member_func("__bool__", TypeBool, {})), t);
		implement_optional_value(prepare_auto_impl(t, t->get_member_func(Identifier::Func::CALL, t->param[0], {})), t);
		implement_optional_equal(prepare_auto_impl(t, t->get_member_func(Identifier::Func::EQUAL, TypeBool, {t})), t);
		implement_optional_equal_raw(prepare_auto_impl(t, t->get_member_func(Identifier::Func::EQUAL, TypeBool, {t->param[0]})), t);
	} else if (t->is_product()) {
		for (auto *cf: t->get_constructors())
			implement_regular_constructor(prepare_auto_impl(t, cf), t, true);
		implement_regular_destructor(prepare_auto_impl(t, t->get_destructor()), t); // if exists...
		implement_regular_assign(prepare_auto_impl(t, t->get_assign()), t); // if exists...
		implement_product_equal(prepare_auto_impl(t, t->get_member_func(Identifier::Func::EQUAL, TypeBool, {t})), t); // if exists...
	} else {
		// regular
		for (auto *cf: t->get_constructors())
			implement_regular_constructor(prepare_auto_impl(t, cf), t, true);
		implement_regular_destructor(prepare_auto_impl(t, t->get_destructor()), t); // if exists...
		implement_regular_assign(prepare_auto_impl(t, t->get_assign()), t); // if exists...
	}

	// recursion
	//for (auto *c: sub_classes)
	//	implement_functions(c);
}


}
