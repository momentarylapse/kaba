/*
 * template.cpp
 *
 *  Created on: 22 May 2022
 *      Author: michi
 */

#include "../kaba.h"
#include "../parser/Parser.h"
#include "../parser/Concretifier.h"
#include "template.h"
#include "../../os/msg.h"
#include "../../base/iter.h"
#include "../../base/algo.h"
#include "../../base/future.h"

namespace kaba {

TemplateManager::TemplateManager(Context *c) {
	context = c;
	implicit_class_registry = new ImplicitClassRegistry(c);
}

void TemplateManager::copy_from(TemplateManager *t) {
	function_templates = t->function_templates;
	class_templates = t->class_templates;
	implicit_class_registry->copy_from(t->implicit_class_registry.get());
}


void TemplateManager::add_template(Function *f, const Array<string> &param_names) {
	if (config.verbose)
		msg_write("ADD TEMPLATE");
	FunctionTemplate t;
	t.func = f;
	t.params = param_names;
	function_templates.add(t);
}


void TemplateManager::add_template(Class *c, const Array<string> &param_names) {
	if (config.verbose)
		msg_write("ADD TEMPLATE");
	//msg_write("add class template  " + c->long_name());
	flags_set(c->flags, Flags::TEMPLATE);
	ClassTemplate t;
	t._class = c;
	t.params = param_names;
	class_templates.add(t);
}

void TemplateManager::clear_from_module(Module *m) {
	implicit_class_registry->clear_from_module(m);
}


void show_node_details(shared<Node> n) {
	if (n->kind == NodeKind::BLOCK) {
		msg_write("block " + p2s(n.get()) + "  ->  " + p2s(n->as_block()->function));
		msg_right();
		for (auto v: n->as_block()->vars)
			msg_write(v->name + ": " + v->type->name + "  " + p2s(v));
	}
	for (auto p: weak(n->params))
		show_node_details(p);
	if (n->kind == NodeKind::BLOCK) {
		msg_left();
	}
}

void show_func_details(Function *f) {
	msg_write("DETAILS:    " + f->signature() + "  " + p2s(f));
	show_node_details(f->block.get());
}

Function *TemplateManager::full_copy(SyntaxTree *tree, Function *f0) {
	//msg_error("FULL COPY");
	auto f = f0->create_dummy_clone(f0->name_space);
	f->block = cp_node(f0->block.get())->as_block();
	flags_clear(f->flags, Flags::NEEDS_OVERRIDE);

	auto convert = [f,tree](shared<Node> n) {
		if (n->kind != NodeKind::BLOCK)
			return n;
		auto b = n->as_block();
		//msg_write("block " + p2s(b));
		for (auto&& [vi,v]: enumerate(b->vars)) {
			int i = weak(b->function->var).find(v);
			b->vars[vi] = f->var[i].get();
		}
		b->function = f;
		return n;
	};

	//convert(f->block.get())->as_block();
	tree->transform_node(f->block.get(), convert);

	//show_func_details(f0);
	//show_func_details(f);

	//parser->do_error("x", -1);

	return f;
}

Function *TemplateManager::request_instance(SyntaxTree *tree, Function *f0, const Array<const Class*> &params, Block *block, const Class *ns, int token_id) {
	auto &t = get_template(tree, f0, token_id);
	
	// already instanciated?
	for (auto &i: t.instances)
		if (i.params == params)
			return i.f;
	
	// new
	FunctionInstance ii;
	ii.f = instantiate(tree, t, params, block, ns, token_id);
	ii.params = params;
	t.instances.add(ii);
	return ii.f;
}

const Class *TemplateManager::request_instance(SyntaxTree *tree, const Class *c0, const Array<const Class*> &params, Block *block, const Class *ns, int token_id) {
	auto &t = get_template(tree, c0, token_id);

	// already instanciated?
	for (auto &i: t.instances)
		if (i.params == params)
			return i.c;

	// new
	ClassInstance ii;
	ii.c = instantiate(tree, t, params, token_id);
	ii.params = params;
	t.instances.add(ii);
	return ii.c;
}

void TemplateManager::match_parameter_type(shared<Node> p, const Class *t, std::function<void(const string&, const Class*)> f) {
	if (p->kind == NodeKind::ABSTRACT_TOKEN) {
		// direct
		string token = p->as_token();
		f(token, t);
	} else if (p->kind == NodeKind::ABSTRACT_TYPE_LIST) {
		if (t->is_list())
			match_parameter_type(p->params[0], t->get_array_element(), f);
	} else if (p->kind == NodeKind::ABSTRACT_TYPE_STAR) {
		if (t->is_pointer_raw())
			match_parameter_type(p->params[0], t->param[0], f);
	} else if (p->kind == NodeKind::ABSTRACT_TYPE_REFERENCE) {
		if (t->is_reference())
			match_parameter_type(p->params[0], t->param[0], f);
	} /*else if (p->kind == NodeKind::ABSTRACT_TYPE_SHARED) {
		if (t->is_pointer_shared())
			match_parameter_type(p->params[0], t->param[0], f);
	} else if (p->kind == NodeKind::ABSTRACT_TYPE_SHARED_NOT_NULL) {
		if (t->is_pointer_shared_not_null())
			match_parameter_type(p->params[0], t->param[0], f);
	} else if (p->kind == NodeKind::ARRAY) {
		if (p->params[0]->kind == NodeKind::ABSTRACT_TYPE_POINTER and t->is_pointer_raw())
			match_parameter_type(p->params[1], t->param[0], f);
		else if (p->params[0]->kind == NodeKind::ABSTRACT_TYPE_SHARED and t->is_pointer_shared())
			match_parameter_type(p->params[1], t->param[0], f);
		else if (p->params[0]->kind == NodeKind::ABSTRACT_TYPE_SHARED_NOT_NULL and t->is_pointer_shared_not_null())
			match_parameter_type(p->params[1], t->param[0], f);
	}*/
}

Function *TemplateManager::request_instance_matching(SyntaxTree *tree, Function *f0, const shared_array<Node> &params, Block *block, const Class *ns, int token_id) {
	if (config.verbose)
		msg_write("____MATCHING");
	auto &t = get_template(tree, f0, token_id);

	Array<const Class*> arg_types;
	arg_types.resize(t.params.num);

	auto set_match_type = [&arg_types, &t, tree, token_id] (const string &token, const Class *type) {
		for (int j=0; j<t.params.num; j++)
			if (token == t.params[j]) {
				if (arg_types[j])
					if (arg_types[j] != type)
						tree->do_error(format("inconsistent template parameter: %s = %s  vs  %s", token, arg_types[j]->long_name(), type->long_name()), token_id);
				arg_types[j] = type;
				if (config.verbose)
					msg_error("FOUND: " + token + " = " + arg_types[j]->name);
			}
	};

	if (params.num != f0->abstract_param_types.num)
		tree->do_error(format("not able to match all template parameters: %d parameters given, %d expected", params.num, f0->abstract_param_types.num), token_id);


	for (auto&& [i,p]: enumerate(weak(params))) {
		if (p->type == TypeUnknown)
			tree->do_error(format("parameter #%d '%s' has undecided type when trying to match template arguments", i+1, f0->var[i]->name), token_id);
		//f0->abstract_param_types[i]->show();
		match_parameter_type(f0->abstract_param_types[i], p->type, set_match_type);
	}

	for (auto t: arg_types)
		if (!t)
			tree->do_error("not able to match all template parameters", token_id);


	return request_instance(tree, f0, arg_types, block, ns, token_id);
}

TemplateManager::FunctionTemplate &TemplateManager::get_template(SyntaxTree *tree, Function *f0, int token_id) {
	for (auto &t: function_templates)
		if (t.func == f0)
			return t;

	tree->do_error("INTERNAL: can not find template...", token_id);
	return function_templates[0];
}

TemplateManager::ClassTemplate &TemplateManager::get_template(SyntaxTree *tree, const Class *c0, int token_id) {
	for (auto &t: class_templates)
		if (t._class == c0)
			return t;
	msg_write(class_templates.num);

	tree->do_error("INTERNAL: can not find template...", token_id);
	return class_templates[0];
}

/*static const Class *concretify_type(shared<Node> n, SyntaxTree *tree, Block *block, const Class *ns) {
	return parser->concretify_as_type(n, block, ns);
}*/

shared<Node> TemplateManager::node_replace(SyntaxTree *tree, shared<Node> n, const Array<string> &names, const Array<const Class*> &params) {
	//return parser->concretify_as_type(n, block, ns);
	return tree->transform_node(n, [tree, &names, &params](shared<Node> nn) {
		if (nn->kind == NodeKind::ABSTRACT_TOKEN) {
			string token = nn->as_token();
			for (int i=0; i<names.num; i++)
				if (token == names[i])
					return add_node_class(params[i], nn->token_id);
		}
		return nn;
	});
}

Function *TemplateManager::instantiate(SyntaxTree *tree, FunctionTemplate &t, const Array<const Class*> &params, Block *block, const Class *ns, int token_id) {
	if (config.verbose)
		msg_write("INSTANTIATE TEMPLATE");
	Function *f0 = t.func;
	auto f = full_copy(tree, f0);
	f->name += format("[%s]", params[0]->long_name());

	// replace in parameters/return type
	for (int i=0; i<f->num_params; i++) {
		f->abstract_param_types[i] = node_replace(tree, f->abstract_param_types[i], t.params, params);
		//f->abstract_param_types[i]->show();
	}
	if (f->abstract_return_type)
		f->abstract_return_type = node_replace(tree, f->abstract_return_type, t.params, params);

	// replace in body
	for (int i=0; i<f->block->params.num; i++)
		f->block->params[i] = node_replace(tree, f->block->params[i], t.params, params);
	
	//f->show();

	// concretify
	try {

		tree->parser->con.concretify_function_header(f);

		f->update_parameters_after_parsing();

		tree->parser->con.concretify_function_body(f);

		if (config.verbose)
			f->block->show();

		auto __ns = const_cast<Class*>(f0->name_space);
		__ns->add_function(tree, f, false, false);

		tree->functions.add(f);

	} catch (kaba::Exception &e) {
		//msg_write(e.message());
		tree->do_error(format("failed to instantiate template %s: %s", f->name, e.message()), token_id);
	}

	return f;
}


extern const Class *TypeRawT;
extern const Class *TypeReferenceT;
extern const Class *TypeXferT;
extern const Class *TypeAliasT;
extern const Class *TypeSharedT;
extern const Class *TypeSharedNotNullT;
extern const Class *TypeOwnedT;
extern const Class *TypeOwnedNotNullT;
extern const Class *TypeListT;
extern const Class *TypeDictT;
extern const Class *TypeOptionalT;
extern const Class *TypeFutureT;
extern const Class *TypeDynamicArray;
extern const Class *TypeDictBase;
string class_name_might_need_parantheses(const Class *t);


const Class *TemplateManager::instantiate(SyntaxTree *tree, ClassTemplate &t, const Array<const Class*> &params, int token_id) {
	if (config.verbose)
		msg_write("INSTANTIATE TEMPLATE CLASS  " + t._class->name + " ... " + params[0]->name);
	const Class *c0 = t._class;

	auto create_class = [this, tree] (const string &name, Class::Type type, int size, int array_size, const Class *parent, const Array<const Class*> &params, int token_id) {
		/*msg_write("CREATE " + name);
		msg_write(p2s(tree));
		msg_write(p2s(tree->implicit_symbols.get()));*/

		auto ns = tree->implicit_symbols.get();

		Class *t = new Class(type, name, size, tree, parent, params);
		t->token_id = token_id;
		tree->owned_classes.add(t);

		// link namespace
		ns->classes.add(t);
		t->name_space = ns;

		AutoImplementer ai(nullptr, tree);
		ai.complete_type(t, array_size, token_id);
		return t;
	};

	Class *c = nullptr;
	if (c0 == TypeRawT)
		c = create_class(class_name_might_need_parantheses(params[0]) + "*", Class::Type::POINTER_RAW, config.target.pointer_size, 0, nullptr, params, token_id);
//		c = create_class(format("%s[%s]", Identifier::RAW_POINTER, params[0]->name), Class::Type::POINTER_RAW, config.target.pointer_size, 0, nullptr, params, token_id);
	else if (c0 == TypeReferenceT)
		c = create_class(class_name_might_need_parantheses(params[0]) + "&", Class::Type::REFERENCE, config.target.pointer_size, 0, nullptr, params, token_id);
	else if (c0 == TypeSharedT)
		c = create_class(format("%s[%s]", Identifier::SHARED, params[0]->name), Class::Type::POINTER_SHARED, config.target.pointer_size, 0, nullptr, params, token_id);
	else if (c0 == TypeSharedNotNullT)
		c = create_class(format("%s![%s]", Identifier::SHARED, params[0]->name), Class::Type::POINTER_SHARED_NOT_NULL, config.target.pointer_size, 0, nullptr, params, token_id);
	else if (c0 == TypeOwnedT)
		c = create_class(format("%s[%s]", Identifier::OWNED, params[0]->name), Class::Type::POINTER_OWNED, config.target.pointer_size, 0, nullptr, params, token_id);
	else if (c0 == TypeOwnedNotNullT)
		c = create_class(format("%s![%s]", Identifier::OWNED, params[0]->name), Class::Type::POINTER_OWNED_NOT_NULL, config.target.pointer_size, 0, nullptr, params, token_id);
	else if (c0 == TypeXferT)
		c = create_class(format("%s[%s]", Identifier::XFER, params[0]->name), Class::Type::POINTER_XFER, config.target.pointer_size, 0, nullptr, params, token_id);
	else if (c0 == TypeAliasT)
		c = create_class(format("%s[%s]", Identifier::ALIAS, params[0]->name), Class::Type::POINTER_ALIAS, config.target.pointer_size, 0, nullptr, params, token_id);
	else if (c0 == TypeListT)
		c = create_class(class_name_might_need_parantheses(params[0]) + "[]", Class::Type::LIST, config.target.dynamic_array_size, -1, TypeDynamicArray, params, token_id);
	else if (c0 == TypeDictT)
		c = create_class(class_name_might_need_parantheses(params[0]) + "{}", Class::Type::DICT, config.target.dynamic_array_size, -1, TypeDictBase, params, token_id);
	else if (c0 == TypeOptionalT)
		c = create_class(class_name_might_need_parantheses(params[0]) + "?", Class::Type::OPTIONAL, params[0]->size + 1, 0, nullptr, params, token_id);
	else if (c0 == TypeFutureT)
		c = create_class(format("%s[%s]", Identifier::FUTURE, params[0]->name), Class::Type::FUTURE, sizeof(base::future<void>), 0, nullptr, params, token_id);
	else
		tree->do_error("can not instantiate template " + c0->name, token_id);

	//add_implicit(c);
	return c;
}

const Class *TemplateManager::find_implicit(const string &name, Class::Type type, int array_size, const Array<const Class*> &params) {
	return implicit_class_registry->find(name, type, array_size, params);
}
void TemplateManager::add_implicit_legacy(const Class* t) {
	implicit_class_registry->add(t);
}

void TemplateManager::add_explicit(SyntaxTree *tree, const Class* c, const Class* c0, const Array<const Class*> &params) {
	auto &t = get_template(tree, c0, -1);

	ClassInstance ii;
	ii.c = c;
	ii.params = params;
	t.instances.add(ii);
}

ImplicitClassRegistry::ImplicitClassRegistry(Context *c) {
	context = c;
}

void ImplicitClassRegistry::copy_from(ImplicitClassRegistry *i) {
	classes = i->classes;
}

void ImplicitClassRegistry::init() {
	module = new Module(context, "<implicit-class-owner>");
}

const Class *ImplicitClassRegistry::find(const string &name, Class::Type type, int array_size, const Array<const Class*> &params) {
	for (auto t: classes) {
		if (t->type != type)
			continue;
		if (t->param != params)
			continue;
		if (type == Class::Type::ARRAY)
			if (t->array_length != array_size)
				continue;
		//if (t->name != name)
		//	continue;
		return t;
	}
	return nullptr;
}

void ImplicitClassRegistry::add(const Class* t) {
	//msg_write("ADD  " + p2s(this) + "  " + t->long_name());
	//if (!module)
	//	init();
	//module->syntax->owned_classes.add(t);
	classes.add(t);
}

// TODO track which module requests what
// TODO implement as templates INSIDE base module
void ImplicitClassRegistry::clear_from_module(Module *m) {
	remove_if(classes, [m] (const Class *c) {
		return c->owner->module == m;
	});
}



}

