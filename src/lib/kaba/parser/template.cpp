/*
 * template.cpp
 *
 *  Created on: 22 May 2022
 *      Author: michi
 */

#include "../kaba.h"
#include "Parser.h"
#include "Concretifier.h"
#include "template.h"
#include "../../os/msg.h"
#include "../../base/iter.h"

namespace kaba {

Array<TemplateManager::Template> TemplateManager::templates;


void TemplateManager::add_template(Function *f, const Array<string> &param_names) {
	if (config.verbose)
		msg_write("ADD TEMPLATE");
	Template t;
	t.func = f;
	t.params = param_names;
	templates.add(t);
}

Function *TemplateManager::full_copy(Parser *parser, Function *f0) {
	auto f = f0->create_dummy_clone(f0->name_space);
	f->block = cp_node(f0->block.get())->as_block();
	flags_clear(f->flags, Flags::NEEDS_OVERRIDE);

	auto convert = [f,parser](shared<Node> n) {
		if (n->kind != NodeKind::BLOCK)
			return n;
		auto b = n->as_block();
		for (auto&& [vi,v]: enumerate(b->vars)) {
			int i = weak(b->function->var).find(v);
			//msg_write(i);
			b->vars[vi] = f->var[i].get();
		}
		b->function = f;
		return n;
	};

	convert(f->block.get())->as_block();
	parser->tree->transform_block(f->block.get(), convert);

	return f;
}

Function *TemplateManager::get_instantiated(Parser *parser, Function *f0, const Array<const Class*> &params, Block *block, const Class *ns, int token_id) {
	auto &t = get_template(parser, f0, token_id);
	
	// already instanciated?
	for (auto &i: t.instances)
		if (i.params == params)
			return i.f;
	
	// new
	Instance ii;
	ii.f = instantiate(parser, t, params, block, ns, token_id);
	ii.params = params;
	t.instances.add(ii);
	return ii.f;
}

Function *TemplateManager::get_instantiated_matching(Parser *parser, Function *f0, const shared_array<Node> &params, Block *block, const Class *ns, int token_id) {
	auto &t = get_template(parser, f0, token_id);

	Array<const Class*> arg_types;
	arg_types.resize(t.params.num);

	auto set_match_type = [&arg_types, &t, parser, token_id] (const string &token, const Class *type) {
		for (int j=0; j<t.params.num; j++)
			if (token == t.params[j]) {
				if (arg_types[j])
					if (arg_types[j] != type)
						parser->do_error(format("inconsistent template parameter: %s = %s  vs  %s", token, arg_types[j]->long_name(), type->long_name()), token_id);
				arg_types[j] = type;
				if (config.verbose)
					msg_error("FOUND: " + token + " = " + arg_types[j]->name);
			}
	};

	for (auto&& [i,p]: enumerate(weak(params))) {
		//f0->abstract_param_types[i]->show();
		if (f0->abstract_param_types[i]->kind == NodeKind::ABSTRACT_TOKEN) {
			string token = f0->abstract_param_types[i]->as_token();
			set_match_type(token, params[i]->type);
		}
	}

	for (auto t: arg_types)
		if (!t)
			parser->do_error("not able to match all template parameters", token_id);


	return get_instantiated(parser, f0, arg_types, block, ns, token_id);
}

TemplateManager::Template &TemplateManager::get_template(Parser *parser, Function *f0, int token_id) {
	for (auto &t: templates)
		if (t.func == f0)
			return t;

	parser->do_error("INTERNAL: can not find template...", token_id);
	return templates[0];
}

/*static const Class *concretify_type(shared<Node> n, Parser *parser, Block *block, const Class *ns) {
	return parser->concretify_as_type(n, block, ns);
}*/

shared<Node> TemplateManager::node_replace(Parser *parser, shared<Node> n, const Array<string> &names, const Array<const Class*> &params) {
	//return parser->concretify_as_type(n, block, ns);
	return parser->tree->transform_node(n, [parser, &names, &params](shared<Node> nn) {
		if (nn->kind == NodeKind::ABSTRACT_TOKEN) {
			string token = nn->as_token();
			for (int i=0; i<names.num; i++)
				if (token == names[i])
					return add_node_class(params[i], nn->token_id);
		}
		return nn;
	});
}

Function *TemplateManager::instantiate(Parser *parser, Template &t, const Array<const Class*> &params, Block *block, const Class *ns, int token_id) {
	if (config.verbose)
		msg_write("INSTANTIATE TEMPLATE");
	Function *f0 = t.func;
	auto f = full_copy(parser, f0);
	f->name += format("[%s]", params[0]->long_name());

	// replace in parameters/return type
	for (int i=0; i<f->num_params; i++)
		f->abstract_param_types[i] = node_replace(parser, f->abstract_param_types[i], t.params, params);
	if (f->abstract_return_type)
		f->abstract_return_type = node_replace(parser, f->abstract_return_type, t.params, params);

	// replace in body
	for (int i=0; i<f->block->params.num; i++)
		f->block->params[i] = node_replace(parser, f->block->params[i], t.params, params);

	// concretify
	try {

		parser->con.concretify_function_header(f);

		f->update_parameters_after_parsing();

		parser->con.concretify_function_body(f);

		if (config.verbose)
			f->block->show();

		auto __ns = const_cast<Class*>(f0->name_space);
		__ns->add_function(parser->tree, f, false, false);

		parser->tree->functions.add(f);

	} catch (kaba::Exception &e) {
		parser->do_error(format("failed to instantiate template %s: %s", f->name, e.message()), token_id);
	}

	return f;
}


}

