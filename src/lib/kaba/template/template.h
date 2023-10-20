/*
 * template.h
 *
 *  Created on: 22 May 2022
 *      Author: michi
 */

#ifndef SRC_LIB_KABA_PARSER_TEMPLATE_H_
#define SRC_LIB_KABA_PARSER_TEMPLATE_H_

namespace kaba {

class Function;
class Class;
class Node;
class Block;
class Parser;
class SyntaxTree;
class Context;


class ImplicitClassRegistry {
public:
	ImplicitClassRegistry(Context *c);
	void copy_from(ImplicitClassRegistry *i);
	void init();
	const Class *find(const string &name, Class::Type type, int array_size, const Array<const Class*> &params);
	void add(const Class* t);
	void clear_from_module(Module *m);

	Context *context;
	Module *module = nullptr;
	Array<const Class*> classes;
};

class TemplateManager {
public:

	TemplateManager(Context *c);
	void copy_from(TemplateManager *m);
	
	void add_template(Function *f, const Array<string> &param_names);
	Function *request_instance(SyntaxTree *tree, Function *f0, const Array<const Class*> &params, Block *block, const Class *ns, int token_id);
	Function *request_instance_matching(SyntaxTree *tree, Function *f0, const shared_array<Node> &params, Block *block, const Class *ns, int token_id);

	void add_template(Class *c, const Array<string> &param_names);
	const Class *request_instance(SyntaxTree *tree, const Class *c0, const Array<const Class*> &params, Block *block, const Class *ns, int token_id);
	const Class *request_instance(SyntaxTree *tree, const Class *c0, const Array<const Class*> &params, int array_size, Block *block, const Class *ns, int token_id);

	void clear_from_module(Module *m);

	const Class *find_implicit_legacy(const string &name, Class::Type type, int array_size, const Array<const Class*> &params);
	void add_implicit_legacy(const Class* t);
	void add_explicit(SyntaxTree *tree, const Class* t, const Class* t0, const Array<const Class*> &params, int array_size = 0);

private:
	Context *context;
	owned<ImplicitClassRegistry> implicit_class_registry;

	struct FunctionInstance {
		Function *f;
		Array<const Class*> params;
	};
	struct FunctionTemplate {
		Function *func;
		Array<string> params;
		Array<FunctionInstance> instances;
	};
	Array<FunctionTemplate> function_templates;

	struct ClassInstance {
		const Class *c;
		Array<const Class*> params;
		int array_size;
	};
	struct ClassTemplate {
		const Class *_class;
		Array<string> params;
		Array<ClassInstance> instances;
	};
	Array<ClassTemplate> class_templates;

	FunctionTemplate &get_template(SyntaxTree *tree, Function *f0, int token_id);
	ClassTemplate &get_template(SyntaxTree *tree, const Class *c0, int token_id);

	Function *full_copy(SyntaxTree *tree, Function *f0);
	shared<Node> node_replace(SyntaxTree *tree, shared<Node> n, const Array<string> &names, const Array<const Class*> &params);
	Function *instantiate(SyntaxTree *tree, FunctionTemplate &t, const Array<const Class*> &params, Block *block, const Class *ns, int token_id);
	const Class *instantiate(SyntaxTree *tree, ClassTemplate &t, const Array<const Class*> &params, int array_size, int token_id);

	void match_parameter_type(shared<Node> p, const Class *t, std::function<void(const string&, const Class*)> f);
};


}

#endif /* SRC_LIB_KABA_PARSER_TEMPLATE_H_ */
