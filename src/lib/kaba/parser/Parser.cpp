#include "../kaba.h"
#include "../asm/asm.h"
#include "../../os/msg.h"
#include "../../base/set.h"
#include "../../base/algo.h"
#include "../../base/iter.h"
#include "Parser.h"
#include "import.h"
#include "../template/template.h"


namespace kaba {

void test_node_recursion(shared<Node> root, const Class *ns, const string &message);

shared<Module> get_import_module(Parser *parser, const string &name, int token);

void add_enum_label(const Class *type, int value, const string &label);

shared<Node> build_abstract_tuple(const Array<shared<Node>> &el);

void crash() {
	int *p = nullptr;
	*p = 4;
}





#if 0
bool is_function_pointer(const Class *c) {
	if (c ==  common_types.functionP)
		return true;
	return is_typed_function_pointer(c);
}
#endif

int64 s2i2(const string &str) {
	if ((str.num > 1) and (str[0]=='0') and (str[1]=='x')) {
		int64 r=0;
		for (int i=2;i<str.num;i++) {
			r *= 16;
			if ((str[i]>='0') and (str[i]<='9'))
				r+=str[i]-48;
			if ((str[i]>='a') and (str[i]<='f'))
				r+=str[i]-'a'+10;
			if ((str[i]>='A') and (str[i]<='F'))
				r+=str[i]-'A'+10;
		}
		return r;
	} else
		return	str.i64();
}

Parser::Parser(SyntaxTree *t) :
	Exp(t->expressions),
	con(t->module->context, this, t),
	auto_implementer(this, t)
{
	context = t->module->context;
	tree = t;
	cur_func = nullptr;
	Exp.cur_line = nullptr;
	parser_loop_depth = 0;
	found_dynamic_param = false;
}


void Parser::parse_buffer(const string &buffer, bool just_analyse) {
	Exp.analyse(tree, buffer);

	parse_legacy_macros(just_analyse);

	parse();

	if (config.verbose)
		tree->show("parse:a");

}

// find the type of a (potential) constant
//  "1.2" -> float
const Class *Parser::get_constant_type(const string &str) {
	// character '...'
	if ((str[0] == '\'') and (str.back() == '\''))
		return common_types.u8;

	// string "..."
	if ((str[0] == '"') and (str.back() == '"'))
		return tree->flag_string_const_as_cstring ? common_types.cstring : common_types.string;

	// numerical (int/float)
	const Class *type = common_types.i32;
	bool hex = (str.num > 1) and (str[0] == '0') and (str[1] == 'x');
	char last = 0;
	for (int ic=0;ic<str.num;ic++) {
		char c = str[ic];
		if ((c < '0') or (c > '9')) {
			if (hex) {
				if ((ic >= 2) and (c < 'a') and (c > 'f'))
					return common_types.unknown;
			} else if (c == '.') {
				type = common_types.f32;
			} else {
				if ((ic != 0) or (c != '-')) { // allow sign
					if ((c != 'e') and (c != 'E'))
						if (((c != '+') and (c != '-')) or ((last != 'e') and (last != 'E')))
							return common_types.unknown;
				}
			}
		}
		last = c;
	}
	if (type == common_types.i32) {
		if (hex) {
			if (str.num == 4)
				type = common_types.u8;
			if (str.num > 10)
				type = common_types.i64;
		} else {
			if ((s2i2(str) >= 0x80000000) or (-s2i2(str) > 0x80000000))
				type = common_types.i64;
		}
	}
	return type;
}

void Parser::get_constant_value(const string &str, Value &value) {
	value.init(get_constant_type(str));
// literal
	if (value.type == common_types.u8) {
		if (str[0] == '\'') // 'bla'
			value.as_int() = str.unescape()[1];
		else // 0x12
			value.as_int() = (int)s2i2(str);
	} else if (value.type == common_types.string) {
		value.as_string() = str.sub(1, -1).unescape();
	} else if (value.type == common_types.cstring) {
		strcpy((char*)value.p(), str.sub(1, -1).unescape().c_str());
	} else if (value.type == common_types.i32) {
		value.as_int() = (int)s2i2(str);
	} else if (value.type == common_types.i64) {
		value.as_int64() = s2i2(str);
	} else if (value.type == common_types.f32) {
		value.as_float() = str._float();
	} else if (value.type == common_types.f64) {
		value.as_float64() = str._float();
	}
}

void Parser::do_error(const string &str, shared<Node> node) {
	do_error_exp(str, node->token_id);
}

void Parser::do_error(const string &str, int token_id) {
	do_error_exp(str, token_id);
}

void Parser::do_error_exp(const string &str, int override_token_id) {
	if (Exp.lines.num == 0)
		throw Exception(str, "", 0, 0, tree->module);

	// what data do we have?
	int token_id = Exp.cur_token();

	// override?
	if (override_token_id >= 0)
		token_id = override_token_id;

	int physical_line = Exp.token_physical_line_no(token_id);
	int pos = Exp.token_line_offset(token_id);
	string expr = Exp.get_token(token_id);

#ifdef CPU_ARM
	msg_error(str);
#endif
	throw Exception(str, expr, physical_line, pos, tree->module);
}

shared<Node> Parser::parse_abstract_operand_extension_element(shared<Node> operand) {
	Exp.next(); // .

	auto el = new Node(NodeKind::AbstractElement, 0, common_types.unknown);
	el->token_id = Exp.cur_token();
	el->set_num_params(2);
	el->set_param(0, operand);
	el->set_param(1, parse_abstract_token());
	return el;
}

shared<Node> Parser::parse_abstract_operand_extension_definitely(shared<Node> operand) {
	auto node = new Node(NodeKind::Definitely, 0, common_types.unknown);
	node->token_id = Exp.consume_token(); // "!"
	node->set_num_params(1);
	node->set_param(0, operand);
	return node;
}

shared<Node> Parser::parse_abstract_operand_extension_dict(shared<Node> operand) {
	Exp.next(); // "{"

	auto node = new Node(NodeKind::AbstractTypeDict, 0, common_types.unknown);
	node->token_id = Exp.cur_token();
	node->set_num_params(1);
	node->set_param(0, operand);

	expect_identifier("}", "'}' expected after dict 'class{'");
	return node;
}

shared<Node> Parser::parse_abstract_operand_extension_optional(shared<Node> operand) {
	Exp.next(); // "?"

	auto node = new Node(NodeKind::AbstractTypeOptional, 0, common_types.unknown);
	node->token_id = Exp.cur_token();
	node->set_num_params(1);
	node->set_param(0, operand);
	return node;
}

shared<Node> Parser::parse_abstract_operand_extension_callable(shared<Node> operand) {
	Exp.next(); // "->"

	auto node = new Node(NodeKind::AbstractTypeCallable, 0, common_types.unknown);
	node->token_id = Exp.cur_token();
	node->set_num_params(2);
	node->set_param(0, operand);
	node->set_param(1, parse_abstract_operand());
	return node;
}

shared<Node> Parser::parse_abstract_operand_extension_pointer(shared<Node> operand) {
	auto node = new Node(NodeKind::AbstractTypeStar, 0, common_types.unknown);
	node->token_id = Exp.consume_token(); // "*"
	node->set_num_params(1);
	node->set_param(0, operand);
	return node;
}

shared<Node> Parser::parse_abstract_operand_extension_reference(shared<Node> operand) {
	auto node = new Node(NodeKind::AbstractTypeReference, 0, common_types.unknown);
	node->token_id = Exp.consume_token(); // "&"
	node->set_num_params(1);
	node->set_param(0, operand);
	return node;
}

shared<Node> Parser::parse_abstract_operand_extension_array(shared<Node> operand) {
	int token0 = Exp.consume_token();
	// array index...


	if (try_consume("]")) {
		auto node = new Node(NodeKind::AbstractTypeList, 0, common_types.unknown, Flags::None, token0);
		node->set_num_params(1);
		node->set_param(0, operand);
		return node;
	}

	auto parse_value_or_slice = [this] () {
		shared<Node> index;
		if (Exp.cur == ":") {
			index = add_node_const(tree->add_constant_int(0));
		} else {
			index = parse_abstract_operand_greedy();
		}
		if (try_consume(":")) {
			shared<Node> index2;
			if (Exp.cur == "]" or Exp.cur == ",") {
				index2 = add_node_const(tree->add_constant_int(DynamicArray::MAGIC_END_INDEX));
				// magic value (-_-)'
			} else {
				index2 = parse_abstract_operand_greedy();
			}
			index = add_node_slice(index, index2);
		}
		return index;
	};

	shared<Node> index = parse_value_or_slice();
	if (try_consume(",")) {
		// TODO ...more
		auto index_b = parse_abstract_operand_greedy();
		index = build_abstract_tuple({index, index_b});
	}
	expect_identifier("]", "']' expected after array index");

	return add_node_array(operand, index, common_types.unknown);
}

shared<Node> Parser::parse_abstract_operand_extension_call(shared<Node> link) {

	// parse all parameters
	auto params = parse_abstract_call_parameters();

	auto node = new Node(NodeKind::AbstractCall, 0, common_types.unknown, Flags::None, link->token_id);
	node->set_num_params(params.num + 1);
	node->set_param(0, link);
	for (auto&& [i,p]: enumerate(params))
		node->set_param(i + 1, p);

	return node;
}


// find any ".", or "[...]"'s    or operators?
shared<Node> Parser::parse_abstract_operand_extension(shared<Node> operand, bool prefer_class) {



#if 0
	// special
	if (/*(operands[0]->kind == NodeKind::CLASS) and*/ ((Exp.cur == "*") or (Exp.cur == "[") or (Exp.cur == "{") or (Exp.cur == "->"))) {

		if (Exp.cur == "*") {
			operand = parse_type_extension_pointer(operand);
		} else if (Exp.cur == "[") {
			t = parse_type_extension_array(t);
		} else if (Exp.cur == "{") {
			t = parse_type_extension_dict(t);
		} else if (Exp.cur == "->") {
			t = parse_type_extension_func(t, block->name_space());
		} else if (Exp.cur == ".") {
			t = parse_type_extension_child(t);
		}

		return parse_operand_extension({add_node_class(t)}, block, prefer_type);
	}
#endif

	auto no_identifier_after = [this] {
		if (Exp.almost_end_of_line())
			return true;
		string next = Exp.peek_next();
		if ((next == ",") or (next == "=") or /*(next == "[") or (next == "{") or*/ (next == "->") or (next == ")") or (next == "*"))
			return true;
		return false;
	};
	/*auto might_declare_pointer_variable = [this] {
		// a line of "int *p = ..."
		if (Exp._cur_exp != 1)
			return false;
		if (is_number(Exp.cur_line->tokens[0].name[0]))
			return false;
		return true;
	};*/

	if (Exp.cur == ".") {
		// element?
		return parse_abstract_operand_extension(parse_abstract_operand_extension_element(operand), prefer_class);
	} else if (Exp.cur == "[") {
		// array?
		return parse_abstract_operand_extension(parse_abstract_operand_extension_array(operand), prefer_class);
	} else if (Exp.cur == "(") {
		// call?
		return parse_abstract_operand_extension(parse_abstract_operand_extension_call(operand), prefer_class);
	} else if (Exp.cur == "{") {
		// dict?
		return parse_abstract_operand_extension(parse_abstract_operand_extension_dict(operand), prefer_class);
	} else if (Exp.cur == "->") {
		// A->B?
		return parse_abstract_operand_extension(parse_abstract_operand_extension_callable(operand), true);
	} else if (Exp.cur == "!") {
		// definitely?
		return parse_abstract_operand_extension(parse_abstract_operand_extension_definitely(operand), prefer_class);
	} else if (Exp.cur == "?") {
		// optional?
		return parse_abstract_operand_extension(parse_abstract_operand_extension_optional(operand), true);
	/*} else if (Exp.cur == Identifier::SHARED or Exp.cur == Identifier::OWNED) {
		auto sub = operand;
		if (Exp.cur == Identifier::SHARED) {
			operand = new Node(NodeKind::ABSTRACT_TYPE_SHARED, 0, common_types.unknown);
		} else { //if (pre == Identifier::OWNED)
			operand = new Node(NodeKind::ABSTRACT_TYPE_OWNED, 0, common_types.unknown);
		}
		operand->token_id = Exp.consume_token();
		operand->set_num_params(1);
		operand->set_param(0, sub);
		return parse_abstract_operand_extension(operand, true);*/
	} else {

		if (Exp.cur == "*" and (prefer_class or no_identifier_after())) {
			// FIXME: false positives for "{{pi * 10}}"
			return parse_abstract_operand_extension(parse_abstract_operand_extension_pointer(operand), true);
		}
		if (Exp.cur == "&" and (prefer_class or no_identifier_after())) {
			return parse_abstract_operand_extension(parse_abstract_operand_extension_reference(operand), true);
		}
		// unary operator? (++,--)

		if (auto op = parse_abstract_operator(OperatorFlags::UnaryLeft)) {
			op->set_num_params(1);
			op->set_param(0, operand);
			return parse_abstract_operand_extension(op, prefer_class);
		}
		return operand;
	}

	// recursion
	return parse_abstract_operand_extension(operand, prefer_class);
}

shared_array<Node> Parser::parse_abstract_call_parameters() {
	expect_identifier("(", "'(' expected in front of function parameter list");

	shared_array<Node> params;

	// list of parameters
	if (try_consume(")"))
		return params;

	bool has_named = false;
	while (true) {

		// find parameter
		if (Exp.peek_next() == "=") {
			// names parameter:  name=...
			int name_token = Exp.cur_token();
			Exp.next();
			Exp.next(); // =
			params.add(add_node_named_parameter(tree, name_token, parse_abstract_operand_greedy()));
			has_named = true;
		} else {
			if (has_named)
				do_error("can not mix named and unnamed parameters", Exp.cur_token());
			params.add(parse_abstract_operand_greedy());
		}

		if (!try_consume(",")) {
			expect_identifier(")", "',' or ')' expected after parameter for function");
			break;
		}
	}
	return params;
}



shared<Node> build_abstract_list(const Array<shared<Node>> &el) {
	auto c = new Node(NodeKind::ArrayBuilder, 0, common_types.unknown);
	c->set_num_params(el.num);
	for (int i=0; i<el.num; i++)
		c->set_param(i, el[i]);
	return c;
}

shared<Node> build_abstract_dict(const Array<shared<Node>> &el) {
	auto c = new Node(NodeKind::DictBuilder, 0, common_types.unknown);
	c->set_num_params(el.num);
	for (int i=0; i<el.num; i++)
		c->set_param(i, el[i]);
	return c;
}

shared<Node> build_abstract_tuple(const Array<shared<Node>> &el) {
	auto c = new Node(NodeKind::Tuple, 0, common_types.unknown);
	c->set_num_params(el.num);
	for (int i=0; i<el.num; i++)
		c->set_param(i, el[i]);
	return c;
}

shared<Node> Parser::parse_abstract_set_builder() {
	//Exp.next(); // [
	auto n_for = parse_abstract_for_header();

	auto n_exp = parse_abstract_operand_greedy();
	
	shared<Node> n_cmp;
	if (try_consume(Identifier::If))
		n_cmp = parse_abstract_operand_greedy();

	expect_identifier("]", "] expected");

	auto n = new Node(NodeKind::ArrayBuilderFor, 0, common_types.unknown);
	n->set_num_params(3);
	n->set_param(0, n_for);
	n->set_param(1, n_exp);
	n->set_param(2, n_cmp);
	return n;

}


shared<Node> Parser::apply_format(shared<Node> n, const string &fmt) {
	auto f = n->type->get_member_func(Identifier::func::Format, common_types.string, {common_types.string});
	if (!f)
		do_error(format("format string: no '%s.%s(string)' function found", n->type->long_name(), Identifier::func::Format), n);
	auto *c = tree->add_constant(common_types.string);
	c->as_string() = fmt;
	auto nf = add_node_call(f, n->token_id);
	nf->set_instance(n);
	nf->set_param(1, add_node_const(c, n->token_id));
	return nf;
}

shared<Node> Parser::try_parse_format_string(Block *block, Value &v, int token_id) {
	string s = v.as_string();
	
	shared_array<Node> parts;
	int pos = 0;
	
	while (pos < s.num) {
	
		int p0 = s.find("{{", pos);
		
		// constant part before the next {{insert}}
		int pe = (p0 < 0) ? s.num : p0;
		if (pe > pos) {
			auto *c = tree->add_constant(common_types.string);
			c->as_string() = s.sub(pos, pe);
			parts.add(add_node_const(c, token_id));
		}
		if (p0 < 0)
			break;
			
		int p1 = s.find("}}", p0);
		if (p1 < 0)
			do_error("string interpolation '{{' not ending with '}}'", token_id);
			
		string xx = s.sub(p0+2, p1);

		// "expr|format" ?
		string fmt;
		int pp = xx.find("|");
		if (pp >= 0) {
			fmt = xx.sub(pp + 1);
			xx = xx.head(pp);
		}

		//msg_write("format:  " + xx);
		ExpressionBuffer ee;
		ee.analyse(tree, xx);
		ee.lines[0].physical_line = Exp.token_logical_line(token_id)->physical_line;
		for (auto& t: ee.lines[0].tokens)
			t.pos += Exp.token_line_offset(token_id) + p0 + 2;
		
		int token0 = Exp.cur_token();
		//int cl = Exp.get_line_no();
		//int ce = Exp.cur_exp;
		Exp.lines.add(ee.lines[0]);
		Exp.update_meta_data();
		Exp.jump(Exp.lines.back().token_ids[0]);
		
		//try {
			auto n = parse_operand_greedy(block, false);
			n = con.deref_if_reference(n);

			if (fmt != "") {
				n = apply_format(n, fmt);
			} else {
				n = con.check_param_link(n, common_types.string_auto_cast, "", 0, 1);
			}
			//n->show();
			parts.add(n);
		/*} catch (Exception &e) {
			msg_write(e.line);
			msg_write(e.column);
			throw;
			//e.line += cl;
			//e.column += Exp.
			
			// not perfect (e has physical line-no etc and e.text has filenames baked in)
			do_error(e.text, token_id);
		}*/
		
		Exp.lines.pop();
		Exp.update_meta_data();
		Exp.jump(token0);
		
		pos = p1 + 2;
	
	}
	
	// empty???
	if (parts.num == 0) {
		auto c = tree->add_constant(common_types.string);
		return add_node_const(c, token_id);
	}
	
	// glue
	while (parts.num > 1) {
		auto b = parts.pop();
		auto a = parts.pop();
		auto n = con.link_operator_id(OperatorID::Add, a, b, token_id);
		parts.add(n);
	}
	//parts[0]->show();
	return parts[0];
}

shared<Node> Parser::parse_abstract_list() {
	shared_array<Node> el;
	if (!try_consume("]"))
		while (true) {
			el.add(parse_abstract_operand_greedy());
			if (try_consume("]"))
				break;
			expect_identifier(",", "',' or ']' expected");
		}
	return build_abstract_list(el);
}

shared<Node> Parser::parse_abstract_dict() {
	Exp.next(); // {
	shared_array<Node> el;
	if (!try_consume("}"))
		while (true) {
			el.add(parse_abstract_operand_greedy()); // key
			expect_identifier(":", "':' after key expected");
			el.add(parse_abstract_operand_greedy()); // value
			if (try_consume("}"))
				break;
			expect_identifier(",", "',' or '}' expected in dict");
		}
	return build_abstract_dict(el);
}

const Class *merge_type_tuple_into_product(SyntaxTree *tree, const Array<const Class*> &classes, int token_id) {
	return tree->module->context->template_manager->request_product(tree, classes, token_id);
}

shared<Node> Parser::parse_abstract_token() {
	return add_node_token(tree, Exp.consume_token());
}

shared<Node> Parser::parse_abstract_type() {
	return parse_abstract_operand(true);
}

// minimal operand
// but with A[...], A(...) etc
shared<Node> Parser::parse_abstract_operand(bool prefer_class) {
	shared<Node> operand;

	// ( -> one level down and combine commands
	if (try_consume("(")) {
		operand = parse_abstract_operand_greedy(true);
		expect_identifier(")", "')' expected");
	} else if (try_consume("&")) { // & -> address operator
		int token = Exp.cur_token();
		operand = parse_abstract_operand()->ref(common_types.unknown);
		operand->token_id = token;
	} else if (try_consume("*")) { // * -> dereference
		int token = Exp.cur_token();
		operand = parse_abstract_operand()->deref(common_types.unknown);
		operand->token_id = token;
	} else if (try_consume("[")) {
		if (Exp.cur == "for") {
			operand = parse_abstract_set_builder();
		} else {
			operand = parse_abstract_list();
		}
	} else if (Exp.cur == "{") {
		operand = parse_abstract_dict();
	} else if ([[maybe_unused]] auto s = which_statement(Exp.cur)) {
		operand = parse_abstract_statement();
	//} else if (auto s = which_special_function(Exp.cur)) {
	//	operand = parse_abstract_special_function(block, s);
	} else if (auto w = which_abstract_operator(Exp.cur, OperatorFlags::UnaryRight)) { // negate/not...
		operand = new Node(NodeKind::AbstractOperator, (int_p)w, common_types.unknown, Flags::None, Exp.cur_token());
		Exp.next();
		operand->set_num_params(1);
		operand->set_param(0, parse_abstract_operand_greedy(false, 13)); // allow '*', don't allow '+'
	} else {
		operand = parse_abstract_token();
	}

	if (Exp.end_of_line())
		return operand;

	//return operand;
	// resolve arrays, structures, calls...
	return parse_abstract_operand_extension(operand, prefer_class);
}

// no type information
shared<Node> Parser::parse_abstract_operator(OperatorFlags param_flags, int min_op_level) {
	auto op = which_abstract_operator(Exp.cur, param_flags);
	if (!op or op->level < min_op_level)
		return nullptr;

	auto cmd = new Node(NodeKind::AbstractOperator, (int_p)op, common_types.unknown);
	cmd->token_id = Exp.consume_token();

	return cmd;
}

/*inline int find_operator(int primitive_id, Type *param_type1, Type *param_type2) {
	for (int i=0;i<PreOperator.num;i++)
		if (PreOperator[i].PrimitiveID == primitive_id)
			if ((PreOperator[i].ParamType1 == param_type1) and (PreOperator[i].ParamType2 == param_type2))
				return i;
	//_do_error_("");
	return 0;
}*/

void get_comma_range(shared_array<Node> &_operators, int mio, int &first, int &last) {
	first = mio;
	last = mio+1;
	for (int i=mio; i>=0; i--) {
		if (_operators[i]->as_abstract_op()->id == OperatorID::Comma)
			first = i;
		else
			break;
	}
	for (int i=mio; i<_operators.num; i++) {
		if (_operators[i]->as_abstract_op()->id == OperatorID::Comma)
			last = i+1;
		else
			break;
	}
}

void analyse_func(Function *f) {
	msg_write("----------------------------");
	msg_write(f->signature());
	msg_write(f->num_params);
	msg_write(b2s(f->is_member()));
	for (auto p: f->literal_param_type)
		msg_write("  LPT: " + p->long_name());
	for (int i=0; i<f->num_params; i++)
		if (auto p = f->abstract_param_type(i))
			msg_write("  APT: " + p->str());
		else
			msg_write("  APT: <nil>");
}

// A+B*C  ->  +(A, *(B, C))
shared<Node> digest_operator_list_to_tree(shared_array<Node> &operands, shared_array<Node> &operators) {
	while (operators.num > 0) {

		// find the most important operator (mio)
		int mio = 0;
		int level_max = -10000;
		for (int i=0;i<operators.num;i++) {
			if (operators[i]->as_abstract_op()->level > level_max) {
				mio = i;
				level_max = operators[i]->as_abstract_op()->level;
			}
		}
		auto op_no = operators[mio]->as_abstract_op();

		if (op_no->id == OperatorID::Comma) {
			int first = mio, last = mio;
			get_comma_range(operators, mio, first, last);
			auto n = build_abstract_tuple(operands.sub_ref(first, last + 1));
			operands[first] = n;
			for (int i=last-1; i>=first; i--) {
				operators.erase(i);
				//op_tokens.erase(i);
				operands.erase(i + 1);
			}
			continue;
		}

		// link it
		operators[mio]->set_num_params(2);
		operators[mio]->set_param(0, operands[mio]);
		operators[mio]->set_param(1, operands[mio+1]);

		// remove from list
		operands[mio] = operators[mio];
		operators.erase(mio);
		//op_tokens.erase(mio);
		operands.erase(mio + 1);
	}
	return operands[0];
}

// greedily parse AxBxC...(operand, operator)
shared<Node> Parser::parse_operand_greedy(Block *block, bool allow_tuples) {
	auto tree = parse_abstract_operand_greedy(allow_tuples);
	if (config.verbose)
		tree->show();
	return con.concretify_node(tree, block, block->name_space());
}

// greedily parse AxBxC...(operand, operator)
shared<Node> Parser::parse_abstract_operand_greedy(bool allow_tuples, int min_op_level) {
	shared_array<Node> operands;
	shared_array<Node> operators;

	// find the first operand
	auto first_operand = parse_abstract_operand();
	if (config.verbose) {
		msg_write("---first:");
		first_operand->show();
	}
	operands.add(first_operand);

	// find pairs of operators and operands
	while (true) {
		if (!allow_tuples and Exp.cur == ",")
			break;
		if (auto op = parse_abstract_operator(OperatorFlags::Binary, min_op_level)) {
			operators.add(op);
			expect_no_new_line("unexpected end of line after operator");
			operands.add(parse_abstract_operand());
		} else {
			break;
		}
	}

	return digest_operator_list_to_tree(operands, operators);
}

// greedily parse AxBxC...(operand, operator)
shared<Node> Parser::parse_operand_super_greedy(Block *block) {
	return parse_operand_greedy(block, true);
}

// TODO later...
//  J make Node=Block
//  J for with p[0]=set init
//  * for_var in for "Block"

// Node structure
//  p = [VAR, START, STOP, STEP]
//  p = [REF_VAR, KEY, ARRAY]
shared<Node> Parser::parse_abstract_for_header() {

	// variable name
	int token0 = Exp.consume_token(); // for
	auto flags = parse_flags(Flags::None);
	auto var = parse_abstract_token();

	// index
	shared<Node> key;
	if (try_consume("=>")) {
		// key => value
		key = var;
		var = parse_abstract_token();
	}


	expect_identifier(Identifier::In, "'in' expected after variable in 'for ...'");

	// first value/array
	auto val0 = parse_abstract_operand_greedy();


	if (try_consume(":")) {
		// range

		if (key)
			do_error("no key=>value allowed in START:END for loop", key);

		auto val1 = parse_abstract_operand_greedy();

		shared<Node> val_step;
		if (try_consume(":"))
			val_step = parse_abstract_operand_greedy();

		auto cmd_for = add_node_statement(StatementID::ForRange, token0, common_types.unknown);
		cmd_for->set_param(0, var);
		cmd_for->set_param(1, val0);
		cmd_for->set_param(2, val1);
		cmd_for->set_param(3, val_step);
		//cmd_for->set_uparam(4, loop_block);

		return cmd_for;

	} else {
		// array

		auto array = val0;


		auto cmd_for = add_node_statement(StatementID::ForContainer, token0, common_types.unknown);
		// [REF_VAR (token), KEY? (token), ARRAY, BLOCK]

		cmd_for->set_param(0, var);
		cmd_for->set_param(1, key);
		cmd_for->set_param(2, array);
		//cmd_for->set_uparam(3, loop_block);

		flags_set(cmd_for->flags, flags);
		return cmd_for;
	}
}

void Parser::post_process_for(shared<Node> cmd_for) {
	auto *n_var = cmd_for->params[0].get();
	auto *var = n_var->as_local();

	/*if (cmd_for->as_statement()->id == StatementID::FOR_CONTAINER) {
		auto *loop_block = cmd_for->params[3].get();

	// ref.
		var->type = tree->get_pointer(var->type);
		n_var->type = var->type;
		tree->transform_node(loop_block, [this, var] (shared<Node> n) {
			return tree->conv_cbr(n, var);
		});
	}*/

	// force for_var out of scope...
	var->name = ":" + var->name;
	if (cmd_for->as_statement()->id == StatementID::ForContainer) {
		auto *index = cmd_for->params[1]->as_local();
		index->name = ":" + index->name;
	}
}



// Node structure
shared<Node> Parser::parse_abstract_statement_for() {
	int ind0 = Exp.cur_line->indent;

	auto cmd_for = parse_abstract_for_header();

	// ...block
	expect_new_line_with_indent();
	Exp.next_line();
	parser_loop_depth ++;
	auto loop_block = parse_abstract_block();
	parser_loop_depth --;

	cmd_for->set_param(cmd_for->params.num - 1, loop_block);

	// else?
	int token_id = Exp.cur_token();
	Exp.next_line();
	if (!Exp.end_of_file() and (Exp.cur == Identifier::Else) and (Exp.cur_line->indent >= ind0)) {
		Exp.next();
		// ...block
		expect_new_line_with_indent();
		Exp.next_line();
		cmd_for->params.add(parse_abstract_block());
	} else {
		Exp.jump(token_id);
	}

	//post_process_for(cmd_for);

	return cmd_for;
}

// Node structure
//  p[0]: test
//  p[1]: loop block
shared<Node> Parser::parse_abstract_statement_while() {
	int token0 = Exp.consume_token(); // "while"
	auto cmd_cmp = parse_abstract_operand_greedy();

	auto cmd_while = add_node_statement(StatementID::While, token0, common_types.unknown);
	cmd_while->set_param(0, cmd_cmp);

	// ...block
	expect_new_line_with_indent();
	Exp.next_line();
	parser_loop_depth ++;
	cmd_while->set_num_params(2);
	cmd_while->set_param(1, parse_abstract_block());
	parser_loop_depth --;
	return cmd_while;
}

// [INPUT, PATTERN1, RESULT1, ...]
shared<Node> Parser::parse_abstract_statement_match() {
	int token0 = Exp.consume_token(); // "match"
	auto cmd_input = parse_abstract_operand_greedy();

	auto cmd_match = add_node_statement(StatementID::Match, token0, common_types.unknown);
	cmd_match->set_param(0, cmd_input);

	expect_new_line_with_indent();
	int indent = Exp.next_line_indent();
	Exp.next_line();
	for (int i=0;; i++) {

		// pattern
		shared<Node> pattern;
		if (try_consume(Identifier::Else) or try_consume("*")) {
			if (i == 0)
				do_error("'match' must not begin with the default pattern", Exp.cur_token() - 1);
			pattern = add_node_statement(StatementID::Pass);
		} else {
			pattern = parse_abstract_operand();
		}

		// =>
		expect_identifier("=>", "'=>' expected after 'match' pattern");

		// result
		shared<Node> result;
		if (Exp.end_of_line()) {
			Exp.next_line();
			// indented block
			result = parse_abstract_block();
		} else {
			// single expression
			result = parse_abstract_operand_greedy();
		}

		cmd_match->set_num_params(3 + 2*i);
		cmd_match->set_param(1 + 2*i, pattern);
		cmd_match->set_param(2 + 2*i, result);

		if (Exp.next_line_indent() < indent)
			break;
		Exp.next_line();
		if (pattern->kind == NodeKind::Statement)
			do_error("no more 'match' patterns allowed after default pattern", Exp.cur_token());
	}

	return cmd_match;
}

shared<Node> Parser::parse_abstract_statement_break() {
	if (parser_loop_depth == 0)
		do_error_exp("'break' only allowed inside a loop");
	int token0 = Exp.consume_token(); // "break"
	return add_node_statement(StatementID::Break, token0);
}

shared<Node> Parser::parse_abstract_statement_continue() {
	if (parser_loop_depth == 0)
		do_error_exp("'continue' only allowed inside a loop");
	int token0 = Exp.consume_token(); // "continue"
	return add_node_statement(StatementID::Continue, token0);
}

// Node structure
//  p[0]: value (if not void)
shared<Node> Parser::parse_abstract_statement_return() {
	int token0 = Exp.consume_token(); // "return"
	auto cmd = add_node_statement(StatementID::Return, token0, common_types.unknown);
	if (Exp.end_of_line()) {
		cmd->set_num_params(0);
	} else {
		cmd->set_num_params(1);
		cmd->set_param(0, parse_abstract_operand_greedy(true));
	}
	expect_new_line();
	return cmd;
}

// IGNORE!!! raise() is a function :P
shared<Node> Parser::parse_abstract_statement_raise() {
	throw "jhhhh";
#if 0
	Exp.next();
	auto cmd = add_node_statement(StatementID::RAISE);

	auto cmd_ex = check_param_link(parse_operand_greedy(block), TypeExceptionP, Identifier::RAISE, 0);
	cmd->set_num_params(1);
	cmd->set_param(0, cmd_ex);

	/*if (block->function->return_type == common_types._void) {
		cmd->set_num_params(0);
	} else {
		auto cmd_value = CheckParamLink(GetCommand(block), block->function->return_type, Identifier::RETURN, 0);
		cmd->set_num_params(1);
		cmd->set_param(0, cmd_value);
	}*/
	expect_new_line();
	return cmd;
#endif
	return nullptr;
}

// Node structure
//  p[0]: try block
//  p[1]: statement except (with type of Exception filter...)
//  p[2]: except block
shared<Node> Parser::parse_abstract_statement_try() {
	int ind = Exp.cur_line->indent;
	int token0 = Exp.consume_token(); // "try"
	auto cmd_try = add_node_statement(StatementID::Try, token0, common_types.unknown);
	cmd_try->set_num_params(1);
	// ...block
	expect_new_line_with_indent();
	Exp.next_line();
	cmd_try->set_param(0, parse_abstract_block());
	token0 = Exp.cur_token();
	Exp.next_line();

	int num_excepts = 0;

	// except?
	while (!Exp.end_of_file() and (Exp.cur == Identifier::Except) and (Exp.cur_line->indent == ind)) {
		int token1 = Exp.consume_token(); // "except"

		auto cmd_ex = add_node_statement(StatementID::Except, token1, common_types.unknown);

		if (!Exp.end_of_line()) {
			auto ex_type = parse_abstract_operand(true); // type
			if (!ex_type)
				do_error_exp("Exception class expected");
			cmd_ex->params.add(ex_type);
			if (!Exp.end_of_line()) {
				expect_identifier(Identifier::As, "'as' expected");
				cmd_ex->params.add(parse_abstract_token()); // var name
			}
		}

		// ...block
		expect_new_line_with_indent();
		Exp.next_line();

		auto cmd_ex_block = parse_abstract_block();

		num_excepts ++;
		cmd_try->set_num_params(1 + num_excepts * 2);
		cmd_try->set_param(num_excepts*2 - 1, cmd_ex);
		cmd_try->set_param(num_excepts*2, cmd_ex_block);

		token0 = Exp.cur_token();
		Exp.next_line();
	}

	Exp.jump(token0); // undo next_line()
	return cmd_try;
}

// Node structure (IF):
//  p[0]: test
//  p[1]: true block
// [p[2]: false block]
shared<Node> Parser::parse_abstract_statement_if() {
	int ind0 = Exp.cur_line->indent;
	int token0 = Exp.consume_token(); // "if"

	bool is_compiletime = try_consume(Identifier::Compiletime);

	auto cmd_cmp = parse_abstract_operand_greedy();

	auto cmd_if = add_node_statement(is_compiletime ? StatementID::IfCompiletime : StatementID::If, token0, common_types.unknown);
	cmd_if->set_param(0, cmd_cmp);
	// ...block
	expect_new_line_with_indent();
	Exp.next_line();
	cmd_if->set_param(1, parse_abstract_block());

	// else?
	int token_id = Exp.cur_token();
	Exp.next_line();
	if (!Exp.end_of_file() and (Exp.cur == Identifier::Else) and (Exp.cur_line->indent >= ind0)) {
		cmd_if->set_num_params(3);
		Exp.next();
		// iterative if
		if (Exp.cur == Identifier::If) {
			// sub-if's in a new block
			auto cmd_block = add_node_block(nullptr, common_types.unknown);
			cmd_if->set_param(2, cmd_block);
			// parse the next if
			parse_abstract_complete_command_into_block(cmd_block.get());
			return cmd_if;
		}
		// ...block
		expect_new_line_with_indent();
		Exp.next_line();
		cmd_if->set_param(2, parse_abstract_block());
	} else {
		Exp.jump(token_id);
	}
	return cmd_if;
}

shared<Node> Parser::parse_abstract_statement_pass() {
	int token0 = Exp.consume_token(); // "pass"
	expect_new_line();

	return add_node_statement(StatementID::Pass, token0);
}

// Node structure
//  type: class
//  p[0]: call to constructor (optional)
shared<Node> Parser::parse_abstract_statement_new() {
	const int token0 = Exp.consume_token(); // "new"
	const auto flags = parse_flags();
	auto cmd = add_node_statement(StatementID::New, token0, common_types.unknown);
	cmd->set_param(0, parse_abstract_operand());
	flags_set(cmd->flags, flags);
	return cmd;
}

// Node structure
//  p[0]: operand
shared<Node> Parser::parse_abstract_statement_delete() {
	int token0 = Exp.consume_token(); // "del"
	auto cmd = add_node_statement(StatementID::Delete, token0, common_types.unknown);
	cmd->set_param(0, parse_abstract_operand());
	return cmd;
}

shared<Node> Parser::parse_abstract_single_func_param() {
	string func_name = Exp.get_token(Exp.cur_token() - 1);
	expect_identifier("(", format("'(' expected after '%s'", func_name));
	auto n = parse_abstract_operand_greedy();
	expect_identifier(")", format("')' expected after parameter of '%s'", func_name));
	return n;
}

// local (variable) definitions...
shared<Node> Parser::parse_abstract_statement_let() {
	do_error_exp("'let' is deprecated, will change it's meaning soon...");
	return nullptr;
}

Array<string> parse_comma_sep_list(Parser *p) {
	Array<string> names;

	names.add(p->Exp.consume());

	while (p->try_consume(","))
		names.add(p->Exp.consume());

	return names;
}

shared_array<Node> parse_comma_sep_token_list(Parser *p) {
	shared_array<Node> names;

	names.add(p->parse_abstract_token());

	while (p->try_consume(","))
		names.add(p->parse_abstract_token());

	return names;
}

// local (variable) definitions...
shared<Node> Parser::parse_abstract_statement_var() {
	auto flags = Flags::Mutable;
	if (Exp.cur == Identifier::Let)
		flags = Flags::None;
	Exp.next(); // "var"/"let"

	// tuple "var (x,y) = ..."
	if (try_consume("(")) {
		auto names = parse_comma_sep_token_list(this);
		expect_identifier(")", "')' expected after tuple declaration");

		expect_identifier("=", "'=' expected after tuple declaration", false);

		auto tuple = build_abstract_tuple(names);

		auto assign = parse_abstract_operator(OperatorFlags::Binary);

		auto rhs = parse_abstract_operand_greedy(true);

		assign->set_num_params(2);
		assign->set_param(0, tuple);
		assign->set_param(1, rhs);

		auto node = new Node(NodeKind::AbstractVar, 0, common_types.unknown, flags);
		node->set_num_params(3);
		//node->set_param(0, type); // no type
		node->set_param(1, cp_node(tuple));
		node->set_param(2, assign);
		expect_new_line();
		return node;
	}

	auto names = parse_comma_sep_token_list(this);
	shared<Node> type;

	// explicit type?
	if (try_consume(":")) {
		type = parse_abstract_operand(true);
	} else {
		expect_identifier("=", "':' or '=' expected after 'var' declaration", false);
	}

	if (Exp.cur == "=") {
		if (names.num != 1)
			do_error_exp(format("'var' declaration with '=' only allowed with a single variable name, %d given", names.num));

		auto assign = parse_abstract_operator(OperatorFlags::Binary);

		auto rhs = parse_abstract_operand_greedy(true);

		assign->set_num_params(2);
		assign->set_param(0, names[0]);
		assign->set_param(1, rhs);

		auto node = new Node(NodeKind::AbstractVar, 0, common_types.unknown, flags);
		node->set_num_params(3);
		node->set_param(0, type); // type
		node->set_param(1, names[0]->shallow_copy()); // name
		node->set_param(2, assign);
		expect_new_line();
		return node;
	}

	expect_new_line();

	/*if (names.num > 1)
		do_error("FIXME allow multi var statement", Exp.cur_token());*/

	auto group = new Node(NodeKind::Group, 0, common_types.unknown);

	for (auto &n: names) {
		auto node = new Node(NodeKind::AbstractVar, 0, common_types.unknown, flags);
		node->set_num_params(2);
		node->set_param(0, type); // type
		node->set_param(1, n); // name
		group->add(node);
		//return node;
	}
	return group;//add_node_statement(StatementID::Pass);
}

shared<Node> Parser::parse_abstract_statement_lambda() {
	auto n = parse_abstract_function_header(Flags::Static);
	auto f = realize_function_header(n, common_types.unknown, tree->base_class);

	// lambda body
	if (Exp.end_of_line()) {
		// indented block
		Exp.next_line();
		f->block_node = parse_abstract_block();
		f->block_node->link_no = (int_p)f->block;
	} else {
		// single expression
		auto cmd = parse_abstract_operand_greedy();
		f->block_node->add(cmd);
	}

	auto node = add_node_statement(StatementID::Lambda, f->token_id, common_types.unknown);
	node->set_num_params(1);
	node->set_param(0, add_node_func_name(f));
	return node;
}

shared<Node> Parser::parse_abstract_statement_raw_function_pointer() {
	int token0 = Exp.consume_token(); // "raw_function_pointer"
	auto node = add_node_statement(StatementID::RawFunctionPointer, token0, common_types.unknown);
	node->set_param(0, parse_abstract_single_func_param());
	return node;
}

shared<Node> Parser::parse_abstract_statement_trust_me() {
	[[maybe_unused]] int token0 = Exp.consume_token(); // "trust_me"
	auto node = add_node_statement(StatementID::TrustMe, token0, common_types.unknown);
	// ...block
	expect_new_line_with_indent();
	Exp.next_line();
	node->set_param(0, parse_abstract_block());
	//flags_set(node->flags, Flags::TrustMe);
	return node;

	/*expect_new_line_with_indent();
	Exp.next_line();
	auto b = parse_abstract_block();
	flags_set(b->flags, Flags::TrustMe);
	return b;*/
}

shared<Node> Parser::parse_abstract_statement() {
	if (Exp.cur == Identifier::For) {
		return parse_abstract_statement_for();
	} else if (Exp.cur == Identifier::While) {
		return parse_abstract_statement_while();
 	} else if (Exp.cur == Identifier::Break) {
 		return parse_abstract_statement_break();
	} else if (Exp.cur == Identifier::Continue) {
		return parse_abstract_statement_continue();
	} else if (Exp.cur == Identifier::Return) {
		return parse_abstract_statement_return();
	//} else if (Exp.cur == Identifier::RAISE) {
	//	ParseStatementRaise();
	} else if (Exp.cur == Identifier::Try) {
		return parse_abstract_statement_try();
	} else if (Exp.cur == Identifier::If) {
		return parse_abstract_statement_if();
	} else if (Exp.cur == Identifier::Pass) {
		return parse_abstract_statement_pass();
	} else if (Exp.cur == Identifier::New) {
		return parse_abstract_statement_new();
	} else if (Exp.cur == Identifier::Delete) {
		return parse_abstract_statement_delete();
	} else if (Exp.cur == Identifier::Let or Exp.cur == Identifier::Var) {
		return parse_abstract_statement_var();
	} else if (Exp.cur == Identifier::Lambda or Exp.cur == Identifier::Func) {
		return parse_abstract_statement_lambda();
	} else if (Exp.cur == Identifier::RawFunctionPointer) {
		return parse_abstract_statement_raw_function_pointer();
	} else if (Exp.cur == Identifier::TrustMe) {
		return parse_abstract_statement_trust_me();
	} else if (Exp.cur == Identifier::Match) {
		return parse_abstract_statement_match();
	}
	do_error_exp("unhandled statement: " + Exp.cur);
	return nullptr;
}

// unused
shared<Node> Parser::parse_abstract_special_function(SpecialFunction *s) {
	int token0 = Exp.consume_token(); // name

	// no call, just the name
	if (Exp.cur != "(") {
		auto node = add_node_special_function_name(s->id, token0, common_types.special_function_ref);
		node->set_num_params(0);
		return node;
	}

	auto node = add_node_special_function_call(s->id, token0, common_types.unknown);
	auto params = parse_abstract_call_parameters();
	node->params = params;
	if (params.num < s->min_params or params.num > s->max_params) {
		if (s->min_params == s->max_params)
			do_error_exp(format("%s() expects %d parameters", s->name, s->min_params));
		else
			do_error_exp(format("%s() expects %d-%d parameters", s->name, s->min_params, s->max_params));
	}
	/*node->set_param(0, params[0]);
	if (params.num >= 2) {
		node->set_param(1, params[1]);
	} else {
		// empty string
		node->set_param(1, add_node_const(tree->add_constant(common_types.string)));
	}*/
	return node;
}

shared<Node> Parser::parse_abstract_block() {
	int indent0 = Exp.cur_line->indent;

	auto block = add_node_block(nullptr, common_types.unknown);

	while (!Exp.end_of_file()) {

		parse_abstract_complete_command_into_block(block.get());

		if (Exp.next_line_indent() < indent0)
			break;
		Exp.next_line();
	}

	return block;
}

// we already are in the line to analyse ...indentation for a new block should compare to the last line
void Parser::parse_abstract_complete_command_into_block(Node *block) {
	// beginning of a line!

	//bool is_type = tree->find_root_type_by_name(Exp.cur, block->name_space(), true);

	// assembler block
	if (try_consume("-asm-")) {
		auto a = add_node_statement(StatementID::Asm);
		a->params.add(auto_implementer.const_int(tree->asm_blocks[next_asm_block ++].uuid));
		block->add(a);
	} else {
		// commands (the actual code!)
		block->add( parse_abstract_operand_greedy(true));
	}

	expect_new_line();
}

void Parser::parse_import() {
	Exp.next(); // 'use'

	[[maybe_unused]] bool also_export = false;
	if (try_consume("@export") or try_consume("out"))
		also_export = true;

	// parse source name (a.b.c)
	Array<string> name = {Exp.cur};
	int token = Exp.consume_token();
	bool recursively = false;
	while (!Exp.end_of_line()) {
		if (!try_consume("."))
			break;
		expect_no_new_line();
		if (try_consume("*")) {
			recursively = true;
			break;
		} else {
			name.add(Exp.consume());
		}
	}

	// alias
	string as_name;
	if (try_consume(Identifier::As)) {
		expect_no_new_line("name expected after 'as'");
		if (recursively)
			do_error_exp("'as' not allowed after 'use module.*'");
		as_name = Exp.cur;
	}

	// resolve
	auto source = resolve_import_source(this, name, token);

	if (as_name == "")
		as_name = name.back();
	if (recursively)
		tree->import_data_all(source._class, token);
	else
		tree->import_data_selective(source._class, source.func, source.var, source._const, as_name, token);
}


void Parser::parse_enum(Class *_namespace) {
	Exp.next(); // 'enum'

	expect_no_new_line("name expected (anonymous enum is deprecated)");

	// class name
	int token0 = Exp.cur_token();
	auto _class = tree->create_new_class(Exp.consume(), common_types.enum_t, sizeof(int), -1, nullptr, {}, _namespace, token0);

	// as shared|@noauto
	if (try_consume(Identifier::As))
		_class->flags = parse_flags(_class->flags);

	context->template_manager->request_class_instance(tree, common_types.enum_t, {_class}, token0);

	expect_new_line_with_indent();
	Exp.next_line();
	int indent0 = Exp.cur_line->indent;

	int next_value = 0;

	while (!Exp.end_of_file()) {
		while (!Exp.end_of_line()) {
			auto *c = tree->add_constant(_class, _class);
			c->name = Exp.consume();

			// explicit value
			if (try_consume("=")) {
				expect_no_new_line();

				auto cv = parse_and_eval_const(tree->root_of_all_evil->block, common_types.i32);
				next_value = cv->as_const()->as_int();
			} else {
				// linked from host program?
				next_value = context->external->process_class_offset(_class->cname(_namespace), c->name, next_value);
			}
			c->as_int() = (next_value ++);

			if (try_consume(Identifier::As)) {
				expect_no_new_line();

				auto cn = parse_and_eval_const(tree->root_of_all_evil->block, common_types.string);
				auto label = cn->as_const()->as_string();
				add_enum_label(_class, c->as_int(), label);
			}

			if (Exp.end_of_line())
				break;
			expect_identifier(",", "',' or newline expected after enum definition");
			expect_no_new_line();
		}
		if (Exp.next_line_indent() < indent0)
			break;
		Exp.next_line();
	}

	flags_set(_class->flags, Flags::FullyParsed);
}


bool is_same_kind_of_pointer(const Class *a, const Class *b);

void parser_class_add_element(Parser *p, Class *_class, const string &name, const Class *type, Flags flags, int64 &_offset, int token_id) {

	// override?
	ClassElement *orig = base::find_by_element(_class->elements, &ClassElement::name, name);

	bool override = flags_has(flags, Flags::Override);
	if (override and ! orig)
		p->do_error(format("can not override element '%s', no previous definition", name), token_id);
	if (!override and orig)
		p->do_error(format("element '%s' is already defined, use '%s' to override", name, Identifier::Override), token_id);
	if (override) {
		if (is_same_kind_of_pointer(orig->type, type))
			orig->type = type;
		else
			p->do_error("can only override pointer elements with other pointer type", token_id);
		return;
	}

	// check parsing dependencies
	if (!type->is_size_known())
		p->do_error(format("size of type '%s' is not known at this point", type->long_name()), token_id);


	// add element
	if (flags_has(flags, Flags::Static)) {
		auto v = new Variable(name, type);
		flags_set(v->flags, flags);
		_class->static_variables.add(v);
	} else {
		_offset = mem_align(_offset, type->alignment);
		_offset = p->context->external->process_class_offset(_class->cname(p->tree->base_class), name, _offset);
		auto el = ClassElement(name, type, _offset);
		_class->elements.add(el);
		_offset += (int)type->size;
	}
}

const Class* parse_class_type(const string& e) {
	if (e == Identifier::Interface)
		return common_types.interface_t;
	if (e == Identifier::Namespace)
		return common_types.namespace_t;
	if (e == Identifier::Struct)
		return common_types.struct_t;
	return nullptr;
}

// [METACLASS, NAME, PARENT, TEMPLATEPARAM] + flags
shared<Node> Parser::parse_abstract_class_header() {
	auto node = new Node(NodeKind::AbstractClass, 0, common_types.unknown);
	node->set_num_params(4);

	// metaclass
	node->set_param(0, parse_abstract_token()); // class/struct/interface

	if (try_consume(Identifier::Override))
		flags_set(node->flags, Flags::Override);

	// name
	string name = Exp.cur;
	// TODO check name validity
	node->set_param(1, parse_abstract_token());

	// template?
	Array<string> template_param_names;
	if (try_consume("[")) {
		flags_set(node->flags, Flags::Template);
		while (true) {
			node->set_param(3, parse_abstract_token());
			if (!try_consume(","))
				break;
		}
		expect_identifier("]", "']' expected after template parameter");
		Exp.next();
	}

	// parent class / interface
	if (try_consume(Identifier::Extends))
		node->set_param(2, parse_abstract_type());
	else if (try_consume(Identifier::Implements))
		node->set_param(2, parse_abstract_type());

	// as shared|@noauto
	Flags explicit_flags = Flags::None;
	if (try_consume(Identifier::As))
		flags_set(node->flags, parse_flags(explicit_flags));

	expect_new_line();

	return node;
}

Class *Parser::realize_class_header(shared<Node> node, Class* _namespace, int64& var_offset0) {
	var_offset0 = 0;

	string name = node->params[1]->as_token();

	Class *_class = nullptr;
	if (flags_has(node->flags, Flags::Override)) {
		// class override X
		_class = const_cast<Class*>(con.concretify_as_type(node->params[1], tree->root_of_all_evil->block, _namespace));
		var_offset0 = _class->size;
		restore_namespace_mapping.add({_class, _class->name_space});
		_class->name_space = _namespace;
	} else {
		// create class
		_class = const_cast<Class*>(tree->find_root_type_by_name(name, _namespace, false));
		// already created...
		if (!_class)
			tree->module->do_error_internal("class declaration ...not found " + name);
		_class->token_id = node->token_id;
		_class->from_template = parse_class_type(node->params[0]->as_token()); // class/struct/interface;
	}

	// template?
	/*Array<string> template_param_names;
	if (try_consume("[")) {
		while (true) {
			template_param_names.add(Exp.consume());
			if (!try_consume(","))
				break;
		}
		expect_identifier("]", "']' expected after template parameter");
		flags_set(_class->flags, Flags::Template);
		Exp.next();
		context->template_manager->add_class_template(_class, template_param_names, [] (SyntaxTree* tree, const Array<const Class*>&, int) -> Class* {
			tree->do_error("TEMPLATE INSTANCE...", -1);
			return nullptr;
		});
	}*/

	// parent class
	if (node->params[2]) {
		auto parent = con.concretify_as_type(node->params[2], tree->root_of_all_evil->block, _namespace); // force
		if (!parent->fully_parsed())
			return nullptr;
			//do_error(format("parent class '%s' not fully parsed yet", parent->long_name()));
		_class->derive_from(parent, DeriveFlags::SET_SIZE | DeriveFlags::KEEP_CONSTRUCTORS | DeriveFlags::COPY_VTABLE);
		_class->flags = parent->flags;
		var_offset0 = _class->size;
	}

	/*if (try_consume(Identifier::Implements)) {
		auto parent = parse_type(_namespace); // force
		if (!parent->fully_parsed())
			return nullptr;
		_class->derive_from(parent, DeriveFlags::SET_SIZE | DeriveFlags::KEEP_CONSTRUCTORS | DeriveFlags::COPY_VTABLE);
		var_offset0 = _class->size;
	}*/

	// as shared|@noauto
	/*Flags explicit_flags = Flags::None;
	if (try_consume(Identifier::As)) {
		explicit_flags = parse_flags(explicit_flags);
		flags_set(_class->flags, explicit_flags);
	}

	expect_new_line();*/

	flags_set(_class->flags, node->flags);

	if (flags_has(node->flags, Flags::Shared)) {
		parser_class_add_element(this, _class, Identifier::SharedCount, common_types.i32, Flags::None, var_offset0, _class->token_id);
	}

	return _class;
}

shared<Node> Parser::parse_abstract_class(Class *_namespace, bool* finished) {
	int indent0 = Exp.cur_line->indent;
	int64 var_offset = 0;
	*finished = false;


	auto node = parse_abstract_class_header();

	auto _class = realize_class_header(node, _namespace, var_offset);
	if (!_class) // in case, not fully parsed
		return node;

	if (_class->is_template()) // parse later...
		return node;

	Array<int> sub_class_token_ids;

	// body
	while (!Exp.end_of_file()) {
		Exp.next_line();
		if (Exp.cur_line->indent <= indent0) //(unindented)
			break;
		if (Exp.end_of_file())
			break;

		if (Exp.cur == Identifier::Enum) {
			parse_enum(_class);
		} else if ((Exp.cur == Identifier::Class) or (Exp.cur == Identifier::Struct) or (Exp.cur == Identifier::Interface) or (Exp.cur == Identifier::Namespace)) {
			int cur_token = Exp.cur_token();
			bool _finished;
			node->add(parse_abstract_class(_class, &_finished));
			if (!_finished) {
				sub_class_token_ids.add(cur_token);
				skip_parse_class();
			}
		} else if (Exp.cur == Identifier::Func) {
			auto flags = Flags::None;
			if (_class->is_interface())
				flags_set(flags, Flags::Virtual);
			if (_class->is_namespace())
				flags_set(flags, Flags::Static);
			node->add(parse_abstract_function(_class, flags));
		} else if ((Exp.cur == Identifier::Const) or (Exp.cur == Identifier::Let)) {
			parse_named_const(_class, tree->root_of_all_evil->block);
		} else if (try_consume(Identifier::Var)) {
			auto nodes = parse_abstract_variable_declaration();
			for (auto n: weak(nodes)) {
				node->add(n);
				realize_class_variable_declaration(n, _class, tree->root_of_all_evil->block, var_offset);
			}
		} else if (Exp.cur == Identifier::Use) {
			parse_class_use_statement(_class);
		} else {
			auto nodes = parse_abstract_variable_declaration();
			for (auto n: weak(nodes)) {
				node->add(n);
				realize_class_variable_declaration(n, _class, tree->root_of_all_evil->block, var_offset);
			}
		}
	}

	post_process_newly_parsed_class(_class, (int)var_offset);


	int cur_token = Exp.cur_token();

	for (int id: sub_class_token_ids) {
		Exp.jump(id);
		bool _finished;
		auto nn = parse_abstract_class(_class, &_finished);
		if (!_finished)
			do_error(format("parent class not fully parsed yet"), id);
	}

	Exp.jump(cur_token-1);
	*finished = true;
	return node;
}

void Parser::post_process_newly_parsed_class(Class *_class, int size) {
	auto external = context->external.get();

	for (const auto [c,n]: restore_namespace_mapping)
		if (c == _class)
			_class->name_space = n;

	// virtual functions?     (derived -> _class->num_virtual)
//	_class->vtable = cur_virtual_index;
	//for (ClassFunction &cf, _class->function)
	//	_class->num_virtual = max(_class->num_virtual, cf.virtual_index);
	if (_class->vtable.num > 0) {
		if (_class->parent) {
			if (_class->parent->vtable.num == 0)
				do_error("no virtual functions allowed when inheriting from class without virtual functions", _class->token_id);
			// element "-vtable-" being derived
		} else {
			for (ClassElement &e: _class->elements)
				e.offset = external->process_class_offset(_class->cname(tree->base_class), e.name, e.offset + config.target.pointer_size);

			auto el = ClassElement(Identifier::VtableVar, common_types.pointer, 0);
			_class->elements.insert(el, 0);
			size += config.target.pointer_size;

			for (auto &i: _class->initializers)
				i.element ++;
		}
	}

	int align = 1;
	if (_class->parent)
		align = _class->parent->alignment;
	for (auto &e: _class->elements)
		align = max(align, e.type->alignment);
	size = mem_align(size, align);
	_class->size = external->process_class_size(_class->cname(tree->base_class), size);
	_class->alignment = align;


	auto_implementer.add_missing_function_headers_for_class(_class);

	flags_set(_class->flags, Flags::FullyParsed);
}

void Parser::skip_parse_class() {
	int indent0 = Exp.cur_line->indent;

	// elements
	while (!Exp.end_of_file()) {
		if (Exp.next_line_indent() <= indent0)
			break;
		Exp.next_line();
	}
	Exp.jump(Exp.cur_line->token_ids.back());
}

void Parser::expect_no_new_line(const string &error_msg) {
	if (Exp.end_of_line())
		do_error_exp(error_msg.num > 0 ? error_msg : "unexpected newline");
}

void Parser::expect_new_line(const string &error_msg) {
	if (!Exp.end_of_line())
		do_error_exp(error_msg.num > 0 ? error_msg : "newline expected");
}

void Parser::expect_new_line_with_indent() {
	if (!Exp.end_of_line())
		do_error_exp("newline expected");
	if (!Exp.next_line_is_indented())
		do_error_exp("additional indent expected");
}

void Parser::expect_identifier(const string &identifier, const string &error_msg, bool consume) {
	if (Exp.cur != identifier)
		do_error_exp(error_msg);
	if (consume)
		Exp.next();
}

bool Parser::try_consume(const string &identifier) {
	if (Exp.cur == identifier) {
		Exp.next();
		return true;
	}
	return false;
}

shared<Node> Parser::eval_to_const(shared<Node> cv, Block *block, const Class *type) {
	cv = con.concretify_node(cv, block, block->name_space());
	if (type) {
		CastingDataSingle cast;
		if (con.type_match_with_cast(cv, false, type, cast)) {
			cv = con.apply_type_cast(cast, cv, type);
		} else {
			do_error(format("constant value of type '%s' expected", type->long_name()), cv);
		}
	} else {
		cv = con.force_concrete_type(cv);
		type = cv->type;
	}

	cv = tree->transform_node(cv, [this] (shared<Node> n) {
		return tree->conv_eval_const_func(tree->conv_fake_constructors(n));
	});

	if (cv->kind != NodeKind::Constant) {
		//cv->show(common_types._void);
		do_error("constant value expected, but expression can not be evaluated at compile time", cv);
	}
	return cv;
}

shared<Node> Parser::parse_and_eval_const(Block *block, const Class *type) {
	// find const value
	auto cv = parse_abstract_operand_greedy(true);
	if (config.verbose)
		cv->show();
	return eval_to_const(cv, block, type);

}

void Parser::parse_named_const(Class *name_space, Block *block) {
	Exp.next(); // 'const' / 'let'
	string name = Exp.consume();

	const Class *type = nullptr;
	if (try_consume(":"))
		type = parse_type(name_space);

	expect_identifier("=", "'=' expected after const name");

	// find const value
	auto cv = parse_and_eval_const(block, type);
	Constant *c_value = cv->as_const();

	auto *c = tree->add_constant(c_value->type.get(), name_space);
	c->set(*c_value);
	c->name = name;
}

// [[NAME, TYPE, VALUE], ...]
shared_array<Node> Parser::parse_abstract_variable_declaration(Flags flags0) {
	shared_array<Node> nodes;
	shared<Node> type, value;

	Flags flags = parse_flags(flags0);

	auto names = parse_comma_sep_token_list(this);

	// explicit type?
	if (try_consume(":")) {
		type = parse_abstract_type();
	} else {
		expect_identifier("=", "':' or '=' expected after 'var' declaration", false);
	}

	if (try_consume("=")) {

		//if (names.num != 1)
		//	do_error(format("'var' declaration with '=' only allowed with a single variable name, %d given", names.num));

		value = parse_abstract_operand_greedy();
	}

	expect_new_line();

	for (auto& name: names) {
		auto node = new Node(NodeKind::AbstractVar, 0, common_types.unknown, flags, name->token_id);
		node->set_num_params(3);
		node->set_param(0, name);
		node->set_param(1, type);
		node->set_param(2, value);

		nodes.add(node);
	}

	return nodes;
}

void Parser::realize_class_variable_declaration(shared<Node> node, const Class *ns, Block *block, int64 &_offset, Flags flags0) {
	if (ns->is_interface())
		do_error_exp("interfaces can not have data elements");

	Flags flags = parse_flags(flags0);
	flags_set(flags, node->flags);
	if (ns->is_namespace())
		flags_set(flags, Flags::Static);

	auto cc = const_cast<Class*>(ns);

	const Class *type = nullptr;

	// explicit type?
	if (node->params[1])
		type = con.concretify_as_type(node->params[1], tree->root_of_all_evil->block, ns);

	Constant *c_value = nullptr;
	if (node->params[2]) {

		//if (nodes.num != 1)
		//	do_error(format("'var' declaration with '=' only allowed with a single variable name, %d given", names.num));

		auto cv = eval_to_const(node->params[2], block, type);
		c_value = cv->as_const();
		if (!type)
			type = cv->type;
	}


	parser_class_add_element(this, cc, node->params[0]->as_token(), type, flags, _offset, node->params[0]->token_id);

	if (node->params[2]) {
		ClassInitializers init = {ns->elements.num - 1, c_value};
		cc->initializers.add(init);
	}
}

void Parser::parse_class_use_statement(const Class *c) {
	Exp.next(); // "use"
	string name = Exp.consume();
	bool found = false;
	for (auto &e: c->elements)
		if (e.name == name) {
			e.allow_indirect_use = true;
			found = true;
		}
	if (!found)
		do_error_exp(format("use: class '%s' does not have an element '%s'", c->name, name));

	expect_new_line();
}


/*const Class *Parser::parse_product_type(const Class *ns) {
	Exp.next(); // (
	Array<const Class*> types;
	types.add(parse_type(ns));

	while (Exp.cur == ",") {
		Exp.next();
		types.add(parse_type(ns));
	}
	if (Exp.cur != ")")
		do_error("',' or ')' in type list expected");
	Exp.next();
	if (types.num == 1)
		return types[0];

	int size = 0;
	string name = types[0]->name;
	for (int i=1; i<types.num; i++) {
		name += "," + types[i]->name;
	}

	auto t = tree->make_class("(" + name + ")", Class::Type::OTHER, size, -1, nullptr, nullptr, ns);
	return t;
}*/

// complicated types like "int[]*[4]" etc
// greedy
const Class *Parser::parse_type(const Class *ns) {
	auto cc = parse_abstract_operand(true);
	return con.concretify_as_type(cc, tree->root_of_all_evil->block, ns);
}

// [NAME?, RETURN?, [PARAMS]?, [TEMPLATEARGS]?, BLOCK]
shared<Node> Parser::parse_abstract_function_header(Flags flags0) {
	auto node = new Node(NodeKind::AbstractFunction, 0, common_types.unknown, flags0);
	node->token_id = Exp.cur_token();
	node->set_num_params(5);

	Exp.next(); // "func"

	node->flags = parse_flags(node->flags);

	// name?
	//string name;
	if (Exp.cur == "(" or Exp.cur == "[") {
		//static int lambda_count = 0;
		//name = format(":lambda-%d:", lambda_count ++);
	} else {
		node->set_param(0, parse_abstract_token());
		/*name = Exp.consume();
		if ((name == Identifier::func::Init) or (name == Identifier::func::Delete) or (name == Identifier::func::Assign))
			flags_set(node->flags, Flags::Mutable);*/
	}

	// template?
	Array<string> template_param_names;
	if (try_consume("[")) {
		auto tnode = new Node(NodeKind::AbstractTypeList, 0, common_types.unknown);
		tnode->params = parse_comma_sep_token_list(this);
		expect_identifier("]", "']' expected after template parameter");
		node->set_param(3, tnode);
		flags_set(node->flags, Flags::Template);
	}

	// parameter list
	expect_identifier("(", "'(' expected after function name");
	auto pnode = new Node(NodeKind::AbstractTypeList, 0, common_types.unknown);
	if (!try_consume(")")) {
		int nn = 0;
		while (true) {
			nn ++;
			pnode->params.resize(nn * 3);
			// like variable definitions

			auto param_flags = parse_flags();

			pnode->set_param(nn*3-3, parse_abstract_token());
			pnode->params[nn*3-3]->flags = param_flags;

			//expect_identifier(":", "':' expected after parameter name");

			// type of parameter variable
			if (try_consume(":"))
				pnode->set_param(nn*3-2, parse_abstract_operand(true));

			// default parameter?
			if (try_consume("="))
				pnode->set_param(nn*3-1, parse_abstract_operand_greedy(false, 1));

			if (!pnode->params[nn*3-2] and !pnode->params[nn*3-1])
				do_error("':' or '=' expected after parameter name", Exp.cur_token());

			if (try_consume(")"))
				break;

			expect_identifier(",", "',' or ')' expected after parameter");
		}
	}
	node->set_param(2, pnode);

	// return type
	if (try_consume("->"))
		node->set_param(1, parse_abstract_operand(true));

	return node;
}

Function *Parser::realize_function_header(shared<Node> node, const Class *default_type, Class *name_space) {
	// name?
	string name;
	if (!node->params[0]) {
		static int lambda_count = 0;
		name = format(":lambda-%d:", lambda_count ++);
	} else {
		name = node->params[0]->as_token();
		if ((name == Identifier::func::Init) or (name == Identifier::func::Delete) or (name == Identifier::func::Assign))
			flags_set(node->flags, Flags::Mutable);
	}

	Function *f = tree->add_function(name, default_type, name_space, node->flags);

	// template?
	Array<string> template_param_names;
	if (node->params[3]) {
		for (auto t: weak(node->params[3]->params))
			template_param_names.add(t->as_token());
		flags_set(f->flags, Flags::Template);
	}

	//if (config.verbose)
	//	msg_write("PARSE HEAD  " + f->signature());
	f->token_id = node->token_id;
	cur_func = f;

	// parameter list
	if (auto pnode = node->params[2]) {
		for (int i=0; i<pnode->params.num/3; i++) {
			[[maybe_unused]] auto v = f->add_param(pnode->params[i*3]->as_token(), common_types.unknown, pnode->params[i*3]->flags);
		}
	}

	f->abstract_node = node;

	post_process_function_header(f, template_param_names, name_space, node->flags);
	cur_func = nullptr;

	return f;
}

void Parser::post_process_function_header(Function *f, const Array<string> &template_param_names, Class *name_space, Flags flags) {
	if (f->is_template()) {
		context->template_manager->add_function_template(f, template_param_names, nullptr);
		name_space->add_template_function(tree, f, flags_has(flags, Flags::Virtual), flags_has(flags, Flags::Override));
	} else if (f->is_macro()) {
		name_space->add_function(tree, f, false, flags_has(flags, Flags::Override));
	} else {
		con.concretify_function_header(f);

		f->update_parameters_after_parsing();

		name_space->add_function(tree, f, flags_has(flags, Flags::Virtual), flags_has(flags, Flags::Override));
	}
}

void Parser::parse_abstract_function_body(Function *f) {
	parser_loop_depth = 0;

	if (Exp.next_line_is_indented()) {
		Exp.next_line();
		f->block_node = parse_abstract_block();
		f->block_node->link_no = (int_p)f->block;
	}

	if (config.verbose) {
		msg_write("ABSTRACT:");
		f->block_node->show();
	}
}

shared<Node> Parser::parse_abstract_function(Class *name_space, Flags flags0) {
	auto n = parse_abstract_function_header(flags0);
	auto f = realize_function_header(n, common_types._void, name_space);
	expect_new_line("newline expected after parameter list");

	if (!f->is_extern())
		parse_abstract_function_body(f);

	functions_to_concretify.add(f);

	return f->abstract_node;
}

void Parser::parse_all_class_names_in_block(Class *ns, int indent0) {
	while (!Exp.end_of_file()) {
		if ((Exp.cur_line->indent == indent0) and (Exp.cur_line->tokens.num >= 2)) {
			if ((Exp.cur == Identifier::Class) or (Exp.cur == Identifier::Struct) or (Exp.cur == Identifier::Interface) or (Exp.cur == Identifier::Namespace)) {
				Exp.next();
				if (Exp.cur == Identifier::Override)
					continue;
				Class *t = tree->create_new_class(Exp.cur, nullptr, 0, 0, nullptr, {}, ns, Exp.cur_token());
				flags_clear(t->flags, Flags::FullyParsed);

				Exp.next_line();
				parse_all_class_names_in_block(t, indent0 + 1);
				continue;
			}
		}
		if (Exp.end_of_file())
			break;
		if (Exp.cur_line->indent < indent0)
			break;
		Exp.next_line();
	}
}

void Parser::concretify_all_functions() {
	//for (auto *f: function_needs_parsing)   might add lambda functions...
	for (int i=0; i<functions_to_concretify.num; i++) {
		auto f = functions_to_concretify[i];
		if (!f->is_extern() and (f->token_id >= 0)) {
			if (!f->is_template() and !f->is_macro())
				con.concretify_function_body(f);
		}
	}

	cur_func = nullptr;
}

Flags Parser::parse_flags(Flags initial) {
	Flags flags = initial;

	while (true) {
		if (Exp.cur == Identifier::Static) {
			flags_set(flags, Flags::Static);
		} else if (Exp.cur == Identifier::Extern) {
			flags_set(flags, Flags::Extern);
		} else if (Exp.cur == Identifier::Const) {
			do_error("'const' deprecated", Exp.cur_token());
		} else if (Exp.cur == Identifier::Mutable) {
			flags_set(flags, Flags::Mutable);
		} else if (Exp.cur == Identifier::Virtual) {
			flags_set(flags, Flags::Virtual);
		} else if (Exp.cur == Identifier::Override) {
			flags_set(flags, Flags::Override);
		} else if (Exp.cur == Identifier::Selfref or Exp.cur == Identifier::Ref) {
			flags_set(flags, Flags::Ref);
		} else if (Exp.cur == Identifier::Shared) {
			flags_set(flags, Flags::Shared);
		} else if (Exp.cur == Identifier::Owned) {
			flags_set(flags, Flags::Owned);
		} else if (Exp.cur == Identifier::Out) {
			flags_set(flags, Flags::Out);
		} else if (Exp.cur == Identifier::Throws) {
			flags_set(flags, Flags::RaisesExceptions);
		} else if (Exp.cur == Identifier::Pure) {
			flags_set(flags, Flags::Pure);
		} else if (Exp.cur == Identifier::Noauto) {
			flags_set(flags, Flags::Noauto);
		} else if (Exp.cur == Identifier::Noframe) {
			flags_set(flags, Flags::Noframe);
		} else {
			break;
		}
		Exp.next();
	}
	return flags;
}

shared<Node> Parser::parse_abstract_top_level() {
	cur_func = nullptr;
	shared<Node> node = new Node(NodeKind::AbstractRoot, 0, common_types.unknown);

	// syntax analysis

	Exp.reset_walker();
	parse_all_class_names_in_block(tree->base_class, 0);

	Exp.reset_walker();

	// global definitions (enum, class, variables and functions)
	while (!Exp.end_of_file()) {

		if (Exp.cur == Identifier::Use) {
			parse_import();

		// enum
		} else if (Exp.cur == Identifier::Enum) {
			parse_enum(tree->base_class);

		// class
		} else if ((Exp.cur == Identifier::Class) or (Exp.cur == Identifier::Struct) or (Exp.cur == Identifier::Interface) or (Exp.cur == Identifier::Namespace)) {
			bool finished;
			node->add(parse_abstract_class(tree->base_class, &finished));
			if (!finished)
				msg_write("X");

		// func
		} else if (Exp.cur == Identifier::Func) {
			node->add(parse_abstract_function(tree->base_class, Flags::Static));

		// macro
		} else if (Exp.cur == Identifier::Macro) {
			node->add(parse_abstract_function(tree->base_class, Flags::Static | Flags::Macro));

		} else if ((Exp.cur == Identifier::Const) or (Exp.cur == Identifier::Let)) {
			parse_named_const(tree->base_class, tree->root_of_all_evil->block);

		} else if (try_consume(Identifier::Var)) {
			int64 offset = 0;
			auto nodes = parse_abstract_variable_declaration();
			for (auto n: weak(nodes)) {
				node->add(n);
				realize_class_variable_declaration(n, tree->base_class, tree->root_of_all_evil->block, offset, Flags::Static);
			}

		} else {
			do_error_exp("unknown top level definition");
		}
		if (!Exp.end_of_file())
			Exp.next_line();
	}
	//node->show();

	return node;
}

// convert text into script data
void Parser::parse() {
	Exp.reset_walker();
	Exp.do_error_endl = [this] {
		do_error_exp("unexpected newline");
	};

	tree->root_node = parse_abstract_top_level();

	concretify_all_functions();
	
	tree->show("aaa");

	for (auto *f: tree->functions)
		test_node_recursion(f->block_node.get(), tree->base_class, "a " + f->long_name());

	for (int i=0; i<tree->owned_classes.num; i++) // array might change...
		auto_implementer.implement_functions(tree->owned_classes[i]);

	for (auto *f: tree->functions)
		test_node_recursion(f->block_node.get(), tree->base_class, "b " + f->long_name());
}

}
