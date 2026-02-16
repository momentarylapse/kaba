/*
 * Parser.h
 *
 *  Created on: 18.02.2019
 *      Author: michi
 */

#ifndef SRC_LIB_KABA_PARSER_PARSER_H_
#define SRC_LIB_KABA_PARSER_PARSER_H_

#include "lexical.h"
#include "AbstractParser.h"
#include "Concretifier.h"
#include "../template/implicit.h"
#include "../syntax/SyntaxTree.h"

namespace kaba {

class Class;
class Function;
class Block;
class SyntaxTree;
class Statement;
class SpecialFunction;
class AbstractOperator;
enum class Flags;
struct CastingData;
class Context;

class Parser : public AbstractParser {
public:
	explicit Parser(SyntaxTree *syntax);

	void parse_buffer(const string &buffer, bool just_analyse);

	void parse_legacy_macros(bool just_analyse);
	void handle_legacy_macro(int &line_no, int &NumIfDefs, bool *IfDefed, bool just_analyse);

	shared<Node> parse_abstract_list();
	shared<Node> parse_abstract_dict();

	const Class *get_constant_type(const string &str);
	void get_constant_value(const string &str, Value &value);

	void parse();
	shared<Node> parse_abstract_top_level();
	void parse_all_class_names_in_block(Class *ns, int indent0);
	void concretify_all_functions();
	Flags parse_flags(Flags initial = Flags::None);
	void parse_import();
	shared<Node> parse_abstract_enum();
	void realize_enum(shared<Node> node, Class *_namespace);
	shared<Node> parse_abstract_class();
	shared<Node> parse_abstract_class_header();
	Class *realize_class(shared<Node>, Class *name_space);
	Class *realize_class_header(shared<Node>, Class* _namespace, int64& var_offset0);
	void post_process_newly_parsed_class(Class *c, int size);
	void skip_parse_class();
	shared<Node> parse_abstract_function(Flags flags0);
	shared<Node> parse_abstract_function_header(Flags flags0);
	Function *realize_function_header(shared<Node> node, const Class *default_type, Class *name_space);
	void realize_function(shared<Node> node, Class* name_space);
	void post_process_function_header(Function *f, const Array<string> &template_param_names, Class *name_space, Flags flags);
	Function* realize_lambda(shared<Node> node, Class* name_space);
	shared_array<Node> parse_abstract_variable_declaration(Flags flags0 = Flags::None);
	void realize_class_variable_declaration(shared<Node> node, const Class *ns, Block *block, int64 &_offset, Flags flags0 = Flags::None);
	shared<Node> parse_abstract_class_use_statement();
	void realize_class_use_statement(shared<Node> node, const Class *c);
	shared<Node> parse_abstract_named_const();
	void realize_named_const(shared<Node> node, Class *name_space, Block *block);
	shared<Node> parse_and_eval_const(Block *block, const Class *type);
	shared<Node> eval_to_const(shared<Node> node, Block *block, const Class *type);
	static AbstractOperator *which_abstract_operator(const string &name, OperatorFlags param_flags = OperatorFlags::Binary);
	static Statement *which_statement(const string &name);
	static SpecialFunction *which_special_function(const string &name);


	shared<Node> parse_abstract_operand_extension(shared<Node> operands, bool prefer_class);
	shared<Node> parse_abstract_operand_extension_element(shared<Node> operand);
	shared<Node> parse_abstract_operand_extension_definitely(shared<Node> operand);
	shared<Node> parse_abstract_operand_extension_array(shared<Node> operand);
	shared<Node> parse_abstract_operand_extension_pointer(shared<Node> operand);
	shared<Node> parse_abstract_operand_extension_reference(shared<Node> operand);
	shared<Node> parse_abstract_operand_extension_dict(shared<Node> operand);
	shared<Node> parse_abstract_operand_extension_optional(shared<Node> operand);
	shared<Node> parse_abstract_operand_extension_callable(shared<Node> operand);
	shared<Node> parse_abstract_operand_extension_call(shared<Node> operand);

	shared<Node> parse_operand_extension(const shared_array<Node> &operands, Block *block, bool prefer_type);
	shared_array<Node> parse_operand_extension_element(shared<Node> operand);
	shared<Node> parse_operand_extension_array(shared<Node> operand, Block *block);
	shared<Node> parse_operand_extension_call(const shared_array<Node> &operands, Block *block);

	shared<Node> parse_abstract_single_func_param();
	void parse_abstract_complete_command_into_block(Node* block);
	shared<Node> parse_abstract_block();
	shared<Node> parse_abstract_operand(bool prefer_class = false);
	shared<Node> parse_operand_greedy(Block *block, bool allow_tuples = false);
	shared<Node> parse_abstract_operand_greedy(bool allow_tuples = false, int min_op_level = -999);
	shared<Node> parse_abstract_set_builder();
	shared<Node> parse_abstract_token();
	shared<Node> parse_abstract_type();

	shared<Node> parse_abstract_operator(OperatorFlags param_flags, int min_op_level = -999);
	shared_array<Node> parse_abstract_call_parameters();
	shared<Node> try_parse_format_string(Block *block, Value &v, int token_id);
	shared<Node> apply_format(shared<Node> n, const string &fmt);
	void post_process_for(shared<Node> n);

	shared<Node> parse_abstract_statement();
	shared<Node> parse_abstract_for_header();
	shared<Node> parse_abstract_statement_for();
	shared<Node> parse_abstract_statement_while();
	shared<Node> parse_abstract_statement_break();
	shared<Node> parse_abstract_statement_continue();
	shared<Node> parse_abstract_statement_return();
	shared<Node> parse_abstract_statement_raise();
	shared<Node> parse_abstract_statement_try();
	shared<Node> parse_abstract_statement_if();
	shared<Node> parse_abstract_statement_pass();
	shared<Node> parse_abstract_statement_new();
	shared<Node> parse_abstract_statement_delete();
	shared<Node> parse_abstract_statement_let();
	shared<Node> parse_abstract_statement_var();
	shared<Node> parse_abstract_statement_lambda();
	shared<Node> parse_abstract_statement_raw_function_pointer();
	shared<Node> parse_abstract_statement_trust_me();
	shared<Node> parse_abstract_statement_match();


	shared<Node> parse_abstract_special_function(SpecialFunction *s);


	//Context *context;
	//SyntaxTree *tree;
	Function *cur_func;
	//ExpressionBuffer &Exp;
	int next_asm_block = 0;

	Concretifier con;
	AutoImplementerInternal auto_implementer;

	int parser_loop_depth;
	bool found_dynamic_param;

	Array<Function*> functions_to_concretify;

	struct NamespaceFix {
		const Class *_class, *_namespace;
	};
	Array<NamespaceFix> restore_namespace_mapping;
};

} /* namespace kaba */

#endif /* SRC_LIB_KABA_SYNTAX_PARSER_H_ */
