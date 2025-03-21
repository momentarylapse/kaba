/*
 * type_matching.cpp
 *
 *  Created on: 27 Feb 2023
 *      Author: michi
 */

#include "Concretifier.h"
#include "Parser.h"
#include "../template/template.h"
#include "../Context.h"
#include "../lib/lib.h"
#include "../../base/set.h"
#include "../../base/iter.h"
#include "../../os/msg.h"


namespace kaba {

extern const Class *TypeDynamicArray;
extern const Class *TypeStringAutoCast;
extern const Class *TypeNone;



bool type_match_up(const Class *given, const Class *wanted);


shared<Node> Concretifier::deref_if_reference(shared<Node> node) {
	if (node->type->is_some_pointer_not_null())
		return node->deref();
	return node;
}

bool func_pointer_match_up(const Class *given, const Class *wanted) {
	auto g = given->param[0];
	auto w = wanted->param[0];
	//msg_write(format("%s   vs   %s", g->name, w->name));
	if (g->param.num != w->param.num) {
		//msg_error(format("%s   vs   %s     ###", g->name, w->name));
		return false;
	}
	// hmmm, let's be pedantic for return values
	if (g->param.back() != w->param.back()) {
		//msg_error(format("%s   vs   %s     return", g->name, w->name));
		return false;
	}
	// allow down-cast for parameters
	// ACTUALLY, this is the wrong way!!!!
	//    but we can't know user types when creating the standard library...
	for (int i=0; i<g->param.num-1; i++)
		if (!type_match_up(g->param[i], w->param[i])) {
			//msg_error(format("%s   vs   %s     param", g->name, w->name, i));
			return false;
		}
	return true;
}

bool is_same_kind_of_pointer(const Class *a, const Class *b) {
	return (a->is_some_pointer() and (a->from_template == b->from_template));
}

// can be re-interpreted as...?
bool type_match_generic_pointer(const Class *given, const Class *wanted) {
	// allow any non-owning pointer?
	if ((wanted == TypePointer) and (given->is_pointer_raw() or given->is_reference()))
		return true;

	// any reference
	if ((wanted == TypeReference) and given->is_reference())
		return true;

	// any xfer[..]?
	if ((wanted->is_pointer_xfer_not_null() and wanted->param[0] == TypeVoid) and given->is_pointer_xfer_not_null())
		return true;

	return false;
}

// can be re-interpreted as...?
bool type_match_up(const Class *given, const Class *wanted) {
	// exact match?
	if (given == wanted)
		return true;

	if (type_match_generic_pointer(given, wanted))
		return true;

	// nil  ->  any raw pointer
	if ((given == TypeNone) and wanted->is_pointer_raw())
		return true;

	// compatible pointers (of same or derived class)
	if (is_same_kind_of_pointer(given, wanted)) {
		// MAYBE: return type_match_up(given->param[0], wanted->param[0]);
		if (given->param[0]->is_derived_from(wanted->param[0]))
			return true;
	}

	// shared not-null  ->  shared
	if (given->is_pointer_shared_not_null() and wanted->is_pointer_shared())
		if (given->param[0]->is_derived_from(wanted->param[0]))
			return true;

	//msg_write(given->long_name() + "  ->  " + wanted->long_name());

	// ref/owned/owned!  ->  raw
	if ((given->is_reference() or given->is_pointer_owned() or given->is_pointer_owned_not_null()) and wanted->is_pointer_raw())
		if (type_match_up(given->param[0], wanted->param[0]))
			return true;

	// ref/owned!  ->  ref
	if ((given->is_reference() or given->is_pointer_owned_not_null()) and wanted->is_reference())
		if (type_match_up(given->param[0], wanted->param[0]))
			return true;

	// callable
	if (given->is_callable() and wanted->is_callable())
		return func_pointer_match_up(given, wanted);

	// lists
	if (wanted->is_list() and given->is_list())
		if (type_match_up(given->param[0], wanted->param[0]) and (given->param[0]->size == wanted->param[0]->size))
			return true;

	if (wanted == TypeStringAutoCast and given == TypeString)
		return true;

	if (wanted == TypeDynamic)
		return true;

	return given->is_derived_from(wanted);
}

shared<Node> Concretifier::explicit_cast(shared<Node> node, const Class *wanted) {
	auto type = node->type;
	if (type == wanted)
		return node;

	CastingDataSingle cast;
	if (type_match_with_cast(node, false, wanted, cast)) {
		if (cast.cast == TypeCastId::NONE) {
			auto c = node->shallow_copy();
			c->type = wanted;
			return c;
		}
		return apply_type_cast(cast, node, wanted);
	}

// explicit pointer cast (we might restrict this later...)
	if (wanted->is_pointer_raw() and type->is_some_pointer()) {
		node->type = wanted;
		return node;
	}
	if (is_same_kind_of_pointer(type, wanted)) {
		node->type = wanted;
		return node;
	}
	if (wanted->is_pointer_xfer_not_null() and type->is_reference()) {
		node->type = wanted;
		return node;
	}

	if (wanted == TypeString)
		return add_converter_str(node, false);

	if (type->get_member_func("__" + wanted->name + "__", wanted, {})) {
		auto rrr = turn_class_into_constructor(wanted, {node}, node->token_id);
		if (rrr.num > 0) {
			rrr[0]->set_param(0, node);
			return rrr[0];
		}
	}

	if (wanted->is_reference() and type->is_pointer_raw())
		do_error(format("can not cast raw pointer type '%s' to reference type '%s'", node->type->long_name(), wanted->long_name()), node);

	do_error(format("can not cast expression of type '%s' to type '%s'", node->type->long_name(), wanted->long_name()), node);
	return nullptr;
}


bool Concretifier::type_match_tuple_as_contructor(shared<Node> node, Function *f_constructor, int &penalty) {
	if (f_constructor->literal_param_type.num != node->params.num + 1)
		return false;

	penalty = 20;
	for (auto&& [i,e]: enumerate(weak(node->params))) {
		CastingDataSingle cast;
		if (!type_match_with_cast(e, false, f_constructor->literal_param_type[i+1], cast))
			return false;
		penalty += cast.penalty;
	}
	return true;
}

bool Concretifier::type_match_with_cast(shared<Node> node, bool is_modifiable, const Class *wanted, CastingDataSingle &cd) {
	cd.penalty = 0;
	cd.cast = TypeCastId::NONE;
	cd.pre_deref_count = 0;
	auto given = node->type;

	//msg_write("?   " + given->long_name() + " -> " + wanted->long_name());


	if (given->is_some_pointer_not_null()) {
		CastingDataSingle cd_sub;
		if (type_match_with_cast(node->deref(), is_modifiable, wanted, cd_sub)) {
			cd = cd_sub;
			cd.pre_deref_count ++;
			cd.penalty += 10;
			return true;
		}
	}

	if (is_modifiable) {
		// is a variable getting assigned.... better not cast
		if (given == wanted)
			return true;

		// allow any raw pointer
		if (given->is_pointer_raw() and wanted == TypePointer) {
			cd.penalty = 10;
			return true;
		}

		return false;
	}
	if (type_match_up(given, wanted))
		return true;

	/*if (type_match_generic_pointer(given, wanted)) {
		msg_error(" -> GENERIC");
		cd.penalty = 10;
		return true;
	}*/

	/*if (given->is_some_pointer()) {
		if (type_match_up(given->param[0], wanted)) {
			cd.penalty = 10;
			cd.cast = TYPE_CAST_DEREFERENCE;
			return true;
		}
	}*/


	// xfer  ->  shared(!)
	/*auto can_sharify = [] (const Class *w, const Class *g) {
		if ((wanted->is_pointer_shared() or wanted->is_pointer_shared_not_null()) and given->is_pointer_xfer_not_null())
			return true;
		if ((wanted->is_pointer_shared() or wanted->is_pointer_shared_not_null()) and given->is_pointer_raw_not_null())
			return true;
		return false;
	};*/
	if ((wanted->is_pointer_shared() or wanted->is_pointer_shared_not_null())
			and given->is_pointer_xfer_not_null()) {
		if (type_match_up(given->param[0], wanted->param[0])) {
			auto t_xfer = tree->request_implicit_class_xfer(wanted->param[0], -1);
			cd.penalty = 10;
			cd.cast = TypeCastId::MAKE_SHARED;
			cd.f = wanted->get_func(Identifier::func::SharedCreate, wanted, {t_xfer});
			return true;
		}
	}

	// ???  ->  raw
	if (given->is_some_pointer() and wanted->is_pointer_raw()) {
		if (type_match_up(given->param[0], wanted->param[0])) {
			cd.penalty = 10;
			cd.cast = TypeCastId::WEAK_POINTER;
			return true;
		}
	}
	if (node->kind == NodeKind::ArrayBuilder and given == TypeUnknown) {
		if (wanted->is_list()) {
			auto t = wanted->get_array_element();
			CastingDataSingle cast;
			for (auto *e: weak(node->params)) {
				if (!type_match_with_cast(e, false, t, cast))
					return false;
				cd.penalty += cast.penalty;
			}
			cd.cast = TypeCastId::ABSTRACT_LIST;
			return true;
		}
		if (wanted == TypeDynamicArray) {
			cd.cast = TypeCastId::ABSTRACT_LIST;
			return true;
		}
		// TODO: only for tuples
		for (auto *f: wanted->get_constructors()) {
			if (type_match_tuple_as_contructor(node, f, cd.penalty)) {
				cd.cast = TypeCastId::TUPLE_AS_CONSTRUCTOR;
				cd.f = f;
				return true;
			}
		}
	}
	if ((node->kind == NodeKind::Tuple) and (given == TypeUnknown)) {
		for (auto *f: wanted->get_constructors()) {
			if (type_match_tuple_as_contructor(node, f, cd.penalty)) {
				cd.cast = TypeCastId::TUPLE_AS_CONSTRUCTOR;
				cd.f = f;
				return true;
			}
		}

		// FIXME this probably doesn't make sense... we should already know the wanted type!
		if (wanted->is_product()) {
			if (wanted->param.num != node->params.num)
				return false;
			for (int i=0; i<node->params.num; i++)
				if (!type_match_up(node->params[i]->type, wanted->param[i]))
					return false;
			msg_error("product");
			cd.cast = TypeCastId::ABSTRACT_TUPLE;
			return true;
		}
	}
	if (node->kind == NodeKind::DictBuilder and given == TypeUnknown) {
		if (wanted->is_dict()) {
			auto t = wanted->get_array_element();
			CastingDataSingle cast;
			for (auto&& [i,e]: enumerate(node->params)) {
				if ((i % 2) == 1) {
					if (!type_match_with_cast(e, false, t, cast))
						return false;
					cd.penalty += cast.penalty;
				}
			}
			cd.cast = TypeCastId::ABSTRACT_DICT;
			return true;
		}
	}
	if (wanted->is_callable() and (given == TypeUnknown)) {
		if (node->kind == NodeKind::Function) {
			auto ft = make_effective_class_callable(node);
			if (type_match_up(ft, wanted)) {
				cd.cast = TypeCastId::FUNCTION_AS_CALLABLE;
				return true;
			}
		}
	}
	if (wanted == TypeStringAutoCast) {
		//Function *cf = given->get_func(Identifier::Func::STR, TypeString, {});
		//if (cf) {
			cd.penalty = 50;
			cd.cast = TypeCastId::OWN_STRING;
			return true;
		//}
	}

	// single parameter auto-constructor?
	for (auto *f: wanted->get_constructors())
		if (f->num_params == 2 and flags_has(f->flags, Flags::AutoCast)) {
			CastingDataSingle c;
			if (type_match_with_cast(node, false, f->literal_param_type[1], c)) {
				cd.cast = TypeCastId::AUTO_CONSTRUCTOR;
				cd.f = f;
				return true;
			}
		}

	for (const auto& [i,c]: enumerate(context->type_casts))
		if (type_match_up(given, c.source) and type_match_up(c.dest, wanted)) {
			cd.penalty = c.penalty;
			cd.cast = i;
			return true;
		}
	return false;
}

shared<Node> Concretifier::apply_type_cast_basic(const CastingDataSingle &cast, shared<Node> node, const Class *wanted) {
	if (cast.cast == TypeCastId::NONE)
		return node;
	if (cast.cast == TypeCastId::DEREFERENCE)
		return node->deref();
	if (cast.cast == TypeCastId::REFERENCE)
		return node->ref(tree);
	if (cast.cast == TypeCastId::OWN_STRING)
		return add_converter_str(node, false);
	if (cast.cast == TypeCastId::ABSTRACT_LIST) {
		if (wanted == TypeDynamicArray) // whatever, anything is ok!
			return force_concrete_type(node);
		CastingDataSingle cd2;
		for (auto&& [i,e]: enumerate(node->params)) {
			if (!type_match_with_cast(e, false, wanted->get_array_element(), cd2)) {
				do_error("nope????", node);
			}
			node->params[i] = apply_type_cast(cd2, e, wanted->get_array_element());
		}
		node->type = wanted;
		return node;
	}
	if (cast.cast == TypeCastId::ABSTRACT_TUPLE) {
		msg_error("AUTO TUPLE");
		node->type = wanted;
		return node;
	}
	if (cast.cast == TypeCastId::ABSTRACT_DICT) {
		//if (wanted == TypeDict)
		//	return force_concrete_type(node);
		CastingDataSingle cd2;
		for (auto&& [i,e]: enumerate(node->params)) {
			if ((i % 2) == 1) {
				if (!type_match_with_cast(e, false, wanted->get_array_element(), cd2)) {
					do_error("nope????", node);
				}
				node->params[i] = apply_type_cast(cd2, e, wanted->get_array_element());
			}
		}
		node->type = wanted;
		return node;
	}
	if (cast.cast == TypeCastId::FUNCTION_AS_CALLABLE) {
		return force_concrete_type(node);
	}

	if (cast.cast == TypeCastId::TUPLE_AS_CONSTRUCTOR) {
		CastingDataCall c;
		c.params.resize(node->params.num);
		auto f = cast.f;

		for (auto&& [i,e]: enumerate(node->params))
			if (!type_match_with_cast(e, false, f->literal_param_type[i+1], c.params[i])) {
				do_error("tuple as constructor...mismatch", e);
			}
		auto cmd = add_node_constructor(f);
		c.wanted = f->literal_param_type.sub_ref(1);
		return apply_params_with_cast(cmd, node->params, c, 1);
	}
	if (cast.cast == TypeCastId::AUTO_CONSTRUCTOR) {
		auto f = cast.f;
		CastingDataSingle c;
		if (!type_match_with_cast(node, false, f->literal_param_type[1], c)) {
			do_error("auto constructor...mismatch", node);
		}
		auto cmd = add_node_constructor(f);
		CastingDataCall cc;
		cc.params = {c};
		cc.wanted = f->literal_param_type.sub_ref(1);
		return apply_params_with_cast(cmd, {node}, cc, 1);
	}
	if ((cast.cast == TypeCastId::MAKE_SHARED) or (cast.cast == TypeCastId::MAKE_OWNED)) {
		if (!cast.f)
			do_error(format("internal: make shared... %s.%s() missing...", wanted->name, Identifier::func::SharedCreate), node);
		auto nn = add_node_call(cast.f, node->token_id);
		nn->set_param(0, node);
		return nn;
	}
	if (cast.cast == TypeCastId::WEAK_POINTER) {
		return node->change_type(wanted, node->token_id);
	}

	auto c = add_node_call(context->type_casts[cast.cast].f, node->token_id);
	c->type = context->type_casts[cast.cast].dest;
	c->set_param(0, node);
	return c;
}

shared<Node> Concretifier::apply_type_cast(const CastingDataSingle &cast, shared<Node> node, const Class *wanted) {
	if (cast.pre_deref_count > 0) {
		for (int i=0; i<cast.pre_deref_count; i++)
			node = node->deref();
	}
	return apply_type_cast_basic(cast, node, wanted);
}

}


