/*
 * implicit_future.cpp
 *
 *  Created on: 24 Sep 2023
 *      Author: michi
 */

#include "../kaba.h"
#include "implicit.h"
#include "../parser/Parser.h"

namespace kaba {

void AutoImplementer::_add_missing_function_headers_for_future(Class *t) {
	//auto t_shared_core = tree->request_implicit_class_shared_not_null(BLA, -1);
	auto t_callback = tree->request_implicit_class_callable_fp({t->param[0]}, TypeVoid, -1);
	auto t_callback_fail = tree->request_implicit_class_callable_fp({}, TypeVoid, -1);
	add_func_header(t, Identifier::Func::INIT, TypeVoid, {}, {});
	add_func_header(t, Identifier::Func::DELETE, TypeVoid, {}, {});
	add_func_header(t, "then", TypeVoid, {t_callback}, {"f"});
	add_func_header(t, "then_or_fail", TypeVoid, {t_callback, t_callback_fail}, {"f"});
	add_func_header(t, Identifier::Func::ASSIGN, TypeVoid, {t}, {"other"});
}

void AutoImplementer::implement_future_constructor(Function *f, const Class *t) {
	/*auto self = add_node_local(f->__get_var(Identifier::SELF));

	auto te = t->get_array_element();
	auto ff = t->get_member_func("__mem_init__", TypeVoid, {TypeInt});
	f->block->add(add_node_member_call(ff,
			self, -1,
			{add_node_const(tree->add_constant_int(te->size + TypeString->size))}));*/
}

void AutoImplementer::_implement_functions_for_future(const Class *t) {
	/*implement_list_constructor(prepare_auto_impl(t, t->get_default_constructor()), t);
	implement_list_destructor(prepare_auto_impl(t, t->get_destructor()), t);*/
}

}



