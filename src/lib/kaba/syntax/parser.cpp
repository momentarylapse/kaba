#include "../kaba.h"
#include "../asm/asm.h"
#include "../../file/file.h"
#include <stdio.h>

namespace Kaba{


Node *conv_cbr(SyntaxTree *ps, Node *c, Variable *var);

extern bool next_extern;
extern bool next_const;

const int TYPE_CAST_OWN_STRING = 4096;

bool type_match(Class *given, Class *wanted);
bool _type_match(Class *given, bool same_chunk, Class *wanted);
bool type_match_with_cast(Class *type, bool same_chunk, bool is_modifiable, Class *wanted, int &penalty, int &cast);
Node exlink_make_func_class(SyntaxTree *ps, Function *f, ClassFunction &cf);


int64 s2i2(const string &str)
{
	if ((str.num > 1) and (str[0]=='0') and (str[1]=='x')){
		int64 r=0;
		for (int i=2;i<str.num;i++){
			r *= 16;
			if ((str[i]>='0') and (str[i]<='9'))
				r+=str[i]-48;
			if ((str[i]>='a') and (str[i]<='f'))
				r+=str[i]-'a'+10;
			if ((str[i]>='A') and (str[i]<='F'))
				r+=str[i]-'A'+10;
		}
		return r;
	}else
		return	str.i64();
}

// find the type of a (potential) constant
//  "1.2" -> float
Class *SyntaxTree::GetConstantType(const string &str)
{
	// character "..."
	if ((str[0] == '\'') and (str.back() == '\''))
		return TypeChar;

	// string "..."
	if ((str[0] == '"') and (str.back() == '"'))
		return flag_string_const_as_cstring ? TypeCString : TypeString;

	// numerical (int/float)
	Class *type = TypeInt;
	bool hex = (str.num > 1) and (str[0] == '0') and (str[1] == 'x');
	char last = 0;
	for (int ic=0;ic<str.num;ic++){
		char c = str[ic];
		if ((c < '0') or (c > '9')){
			if (hex){
				if ((ic >= 2) and (c < 'a') and (c > 'f'))
					return TypeUnknown;
			}else if (c == '.'){
				type = TypeFloat32;
			}else{
				if ((ic != 0) or (c != '-')){ // allow sign
					if ((c != 'e') and (c != 'E'))
						if (((c != '+') and (c != '-')) or ((last != 'e') and (last != 'E')))
							return TypeUnknown;
				}
			}
		}
		last = c;
	}
	if (type == TypeInt){
		if (hex){
			if ((s2i2(str) >= 0x100000000) or (-s2i2(str) > 0x00000000))
				type = TypeInt64;
		}else{
			if ((s2i2(str) >= 0x80000000) or (-s2i2(str) > 0x80000000))
				type = TypeInt64;
		}
	}

	// super array [...]
	if (str == "["){
		DoError("super array constant");
	}
	return type;
}

void SyntaxTree::GetConstantValue(const string &str, Value &value)
{
	value.init(GetConstantType(str));
// literal
	if (value.type == TypeChar){
		value.as_int() = str[1];
	}else if (value.type == TypeString){
		value.as_string() = str.substr(1, -2);
	}else if (value.type == TypeCString){
		strcpy((char*)value.p(), str.substr(1, -2).c_str());
	}else if (value.type == TypeInt){
		value.as_int() = (int)s2i2(str);
	}else if (value.type == TypeInt64){
		value.as_int64() = s2i2(str);
	}else if (value.type == TypeFloat32){
		value.as_float() = str._float();
	}else if (value.type == TypeFloat64){
		value.as_float64() = str._float();
	}
}


Node *SyntaxTree::DoClassFunction(Node *ob, Array<ClassFunction> &cfs, Block *block)
{
	// the function
	Function *ff = cfs[0].func();

	Array<Node*> links;
	for (ClassFunction &cf: cfs)
		links.add(add_node_classfunc(&cf, ob));
	return GetFunctionCall(ff->name, links, block);
}

Node *SyntaxTree::GetOperandExtensionElement(Node *operand, Block *block)
{
	Exp.next();
	Class *type = operand->type;

	// pointer -> dereference
	bool deref = false;
	if (type->is_pointer()){
		type = type->parent;
		deref = true;
	}

	// super
	if ((type->parent) and (Exp.cur == IDENTIFIER_SUPER)){
		Exp.next();
		if (deref){
			operand->type = type->parent->get_pointer();
			return operand;
		}
		return ref_node(operand, type->parent->get_pointer());
	}

	// find element
	for (ClassElement &e: type->elements)
		if (Exp.cur == e.name){
			Exp.next();
			return shift_node(operand, deref, e.offset, e.type);
		}


	if (!deref)
		operand = ref_node(operand);

	string f_name = Exp.cur;

	// class function?
	Array<Node*> links;
	for (ClassFunction &cf: type->functions)
		if (f_name == cf.name)
			links.add(add_node_classfunc(&cf, operand));
	if (links.num > 0){
		Exp.next();
		return GetFunctionCall(f_name, links, block);
	}

	DoError("unknown element of " + type->name);
	return nullptr;
}

Node *SyntaxTree::GetOperandExtensionArray(Node *Operand, Block *block)
{
	// array index...
	Exp.next();
	Node *index = nullptr;
	Node *index2 = nullptr;
	if (Exp.cur == ":"){
		index = add_node_const(AddConstant(TypeInt));
		index->as_const()->as_int() = 0;
	}else{
		index = GetCommand(block);
	}
	if (Exp.cur == ":"){
		Exp.next();
		if (Exp.cur == "]"){
			index2 = add_node_const(AddConstant(TypeInt));
			index2->as_const()->as_int() = -1;

		}else{
			index2 = GetCommand(block);
		}
	}
	if (Exp.cur != "]")
		DoError("\"]\" expected after array index");
	Exp.next();

	// subarray() ?
	if (index2){
		auto *cf = Operand->type->get_func("subarray", Operand->type, 2, index->type);
		if (cf){
			Node *f = add_node_classfunc(cf, ref_node(Operand));
			f->set_param(0, index);
			f->set_param(1, index2);
			return f;
		}
	}

	// __get__() ?
	auto *cf = Operand->type->get_get(index->type);
	if (cf){
		Node *f = add_node_classfunc(cf, ref_node(Operand));
		f->set_param(0, index);
		return f;
	}

	// allowed?
	bool allowed = ((Operand->type->is_array()) or (Operand->type->usable_as_super_array()));
	bool pparray = false;
	if (!allowed)
		if (Operand->type->is_pointer()){
			if ((!Operand->type->parent->is_array()) and (!Operand->type->parent->usable_as_super_array()))
				DoError(format("using pointer type \"%s\" as an array (like in C) is not allowed any more", Operand->type->name.c_str()));
			allowed = true;
			pparray = (Operand->type->parent->usable_as_super_array());
		}
	if (!allowed)
		DoError(format("type \"%s\" is neither an array nor a pointer to an array nor does it have a function __get__(%s)", Operand->type->name.c_str(), index->type->name.c_str()));

	Node *array;

	// pointer?
	if (pparray){
		DoError("test... anscheinend gibt es [] auf * super array");
		//array = cp_command(this, Operand);
/*		Operand->kind = KindPointerAsArray;
		Operand->type = t->type->parent;
		deref_command_old(this, Operand);
		array = Operand->param[0];*/
	}else if (Operand->type->usable_as_super_array()){
		array = add_node_parray(shift_node(Operand, false, 0, Operand->type->get_pointer()),
		                           index, Operand->type->get_array_element());
	}else if (Operand->type->is_pointer()){
		array = add_node_parray(Operand, index, Operand->type->parent->parent);
	}else{
		array = AddNode(KIND_ARRAY, 0, Operand->type->parent);
		array->set_num_params(2);
		array->set_param(0, Operand);
		array->set_param(1, index);
	}

	if (index->type != TypeInt){
		Exp.rewind();
		DoError(format("type of index for an array needs to be int, not %s", index->type->name.c_str()));
	}
	return array;
}

// find any ".", "->", or "[...]"'s    or operators?
Node *SyntaxTree::GetOperandExtension(Node *Operand, Block *block)
{
	// nothing?
	int op_no = WhichPrimitiveOperator(Exp.cur);
	if ((Exp.cur != ".") and (Exp.cur != "[") and (Exp.cur != "->") and (op_no < 0))
		return Operand;

	if (Exp.cur == "->")
		DoError("\"->\" deprecated,  use \".\" instead");

	if (Exp.cur == "."){
		// class element?

		Operand = GetOperandExtensionElement(Operand, block);

	}else if (Exp.cur == "["){
		// array?

		Operand = GetOperandExtensionArray(Operand, block);


	}else if (op_no >= 0){
		// unary operator? (++,--)

		for (auto *op: operators)
			if (op->primitive_id == op_no)
				if ((op->param_type_1 == Operand->type) and (op->param_type_2 == TypeVoid)){
					Exp.next();
					return add_node_operator(Operand, nullptr, op);
				}
		return Operand;
	}

	// recursion
	return GetOperandExtension(Operand, block);
}

void clear_nodes(Array<Node*> &nodes)
{
	for (auto *n: nodes)
		delete n;
}

void clear_nodes(Array<Node*> &nodes, Node *keep)
{
	for (auto *n: nodes)
		if (n != keep)
			delete n;
}

Node *SyntaxTree::GetSpecialFunctionCall(const string &f_name, Node *link, Block *block)
{
	// sizeof
	if (link->kind != KIND_STATEMENT)
		DoError("evil special function");

	if (link->link_no == STATEMENT_SIZEOF){
		delete link;

		Exp.next();
		Node *c = add_node_const(AddConstant(TypeInt));


		Class *type = FindType(Exp.cur);
		Array<Node*> links = GetExistence(Exp.cur, block);
		if (type){
			c->as_const()->as_int() = type->size;
		}else if ((links.num > 0) and ((links[0]->kind == KIND_VAR_GLOBAL) or (links[0]->kind == KIND_VAR_LOCAL))){
			c->as_const()->as_int() = links[0]->type->size;
		}else{
			type = GetConstantType(Exp.cur);
			if (type)
				c->as_const()->as_int() = type->size;
			else
				DoError("type-name or variable name expected in sizeof(...)");
		}
		clear_nodes(links);
		Exp.next();
		if (Exp.cur != ")")
			DoError("\")\" expected after parameter list");
		Exp.next();
		return c;
	}else if (link->link_no == STATEMENT_TYPE){
		delete link;

		Exp.next();
		Node *c = add_node_const(AddConstant(TypeClassP));


		Class *type = FindType(Exp.cur);
		Array<Node*> links = GetExistence(Exp.cur, block);
		if (type){
			c->as_const()->as_int64() = (int_p)type;
		}else if ((links.num > 0) and ((links[0]->kind == KIND_VAR_GLOBAL) or (links[0]->kind == KIND_VAR_LOCAL))){
			c->as_const()->as_int64() = (int_p)links[0]->type;
		}else{
			type = GetConstantType(Exp.cur);
			if (type)
				c->as_const()->as_int64() = (int_p)type;
			else
				DoError("type-name or variable name expected in type(...)");
		}
		clear_nodes(links);
		Exp.next();
		if (Exp.cur != ")")
			DoError("\")\" expected after parameter list");
		Exp.next();
		return c;
	}

	DoError("evil special function");
	return nullptr;
}


Array<Class*> SyntaxTree::GetFunctionWantedParams(Node *link)
{
	if (link->kind == KIND_FUNCTION){
		return link->as_func()->literal_param_type;
	}else if (link->kind == KIND_VIRTUAL_FUNCTION){
		ClassFunction *cf = link->instance->type->parent->get_virtual_function(link->link_no);
		if (!cf)
			DoError("FindFunctionSingleParameter: can't find virtual function...?!?");
		return cf->param_types;
	}else if (link->kind == KIND_TYPE){
		Class *t = link->as_class();
		for (auto *c: t->get_constructors())
			return c->param_types;
	}else
		DoError("evil function...kind="+i2s(link->kind));

	Array<Class*> dummy_types;
	return dummy_types;
}

Array<Node*> SyntaxTree::FindFunctionParameters(Block *block)
{
	if (Exp.cur != "(")
		DoError("\"(\" expected in front of function parameter list");

	Exp.next();

	Array<Node*> params;

	// list of parameters
	for (int p=0;;p++){
		if (Exp.cur == ")")
			break;

		// find parameter
		params.add(GetCommand(block));

		if (Exp.cur != ","){
			if (Exp.cur == ")")
				break;
			DoError("\",\" or \")\" expected after parameter for function");
		}
		Exp.next();
	}
	Exp.next(); // ')'
	return params;
}

Node *apply_type_cast(SyntaxTree *ps, int tc, Node *param);


// check, if the command <link> links to really has type <type>
//   ...and try to cast, if not
Node *SyntaxTree::CheckParamLink(Node *link, Class *wanted, const string &f_name, int param_no)
{
	// type cast needed and possible?
	Class *given = link->type;

	if (type_match(given, wanted))
		return link;

	if (wanted->is_pointer_silent()){
		// "silent" pointer (&)?
		if (type_match(given, wanted->parent)){

			return ref_node(link);
		}else if ((given->is_pointer()) and (type_match(given->parent, wanted->parent))){
			// silent reference & of *

			return link;
		}else{
			Exp.rewind();
			DoError(format("(c) parameter %d in command \"%s\" has type %s, %s expected", param_no + 1, f_name.c_str(), given->name.c_str(), wanted->name.c_str()));
		}

	}else{
		// normal type cast
		int pen, tc;

		if (type_match_with_cast(given, false, false, wanted, pen, tc))
			return apply_type_cast(this, tc, link);

		Exp.rewind();
		DoError(format("parameter %d in command \"%s\" has type %s, %s expected", param_no + 1, f_name.c_str(), given->name.c_str(), wanted->name.c_str()));
	}
	return link;
}

Class *get_func_type(SyntaxTree *syntax, Function *f)
{
	return TypeFunction;
	string params;
	for (int i=0; i<f->num_params; i++){
		if (i > 0)
			params += ",";
		params += f->literal_param_type[i]->name;
	}
	return syntax->CreateNewClass("func(" + params + ")->" + f->return_type->name, Class::Type::POINTER, config.pointer_size, 0, TypeVoid);

	return TypePointer;
}

bool direct_param_match(SyntaxTree *ps, Node *operand, Array<Node*> &params)
{
	Array<Class*> wanted_types = ps->GetFunctionWantedParams(operand);
	if (wanted_types.num != params.num)
		return false;
	for (int p=0; p<params.num; p++)
		if (!type_match(params[p]->type, wanted_types[p]))
			return false;
	return true;
}

bool param_match_with_cast(SyntaxTree *ps, Node *operand, Array<Node*> &params, Array<int> &casts)
{
	Array<Class*> wanted_types = ps->GetFunctionWantedParams(operand);
	if (wanted_types.num != params.num)
		return false;
	casts.resize(params.num);
	for (int p=0; p<params.num; p++){
		int penalty;
		if (!type_match_with_cast(params[p]->type, false, false, wanted_types[p], penalty, casts[p]))
			return false;
	}
	return true;
}

Node *apply_params_direct(SyntaxTree *ps, Node *operand, Array<Node*> &params)
{
	for (int p=0; p<params.num; p++)
		operand->set_param(p, params[p]);
	return operand;
}

Node *apply_params_with_cast(SyntaxTree *ps, Node *operand, Array<Node*> &params, Array<int> &casts)
{
	for (int p=0; p<params.num; p++){
		if (casts[p] >= 0)
			params[p] = apply_type_cast(ps, casts[p], params[p]);
		operand->set_param(p, params[p]);
	}
	return operand;
}

// creates <Operand> to be the function call
//  on entry <Operand> only contains information from GetExistence (Kind, Nr, Type, NumParams)
Node *SyntaxTree::GetFunctionCall(const string &f_name, Array<Node*> &links, Block *block)
{
	// function as a variable?
	if (Exp.cur_exp >= 2)
	if ((Exp.get_name(Exp.cur_exp - 2) == "&") and (Exp.cur != "(")){
		if (links[0]->kind == KIND_FUNCTION){
			// can't be const because the function might not be compiled yet!
			Node *c = new Node(KIND_VAR_FUNCTION, links[0]->link_no, links[0]->script, get_func_type(this, links[0]->as_func()));
			clear_nodes(links);
			return c;
		}else{
			Exp.rewind();
			//DoError("\"(\" expected in front of parameter list");
			DoError("only functions can be referenced");
		}
	}


	// "special" functions
    if (links[0]->kind == KIND_STATEMENT)
	    if ((links[0]->link_no == STATEMENT_SIZEOF) or (links[0]->link_no == STATEMENT_TYPE)){
			return GetSpecialFunctionCall(f_name, links[0], block);
	    }

    // parse all parameters
    Array<Node*> params = FindFunctionParameters(block);


	// find (and provisional link) the parameters in the source

	/*bool needs_brackets = ((Operand->type != TypeVoid) or (Operand->param.num != 1));
	if (needs_brackets){
		FindFunctionParameters(wanted_type, block, Operand);

	}else{
		wanted_type.add(TypeUnknown);
		FindFunctionSingleParameter(0, wanted_type, block, Operand);
	}*/

	// direct match...
	for (Node *operand: links){
		if (!direct_param_match(this, operand, params))
			continue;

		clear_nodes(links, operand);
		return apply_params_direct(this, operand, params);
	}


	// advanced match...
	for (Node *operand: links){
		Array<int> casts;
		if (!param_match_with_cast(this, operand, params, casts))
			continue;

		clear_nodes(links, operand);
		return apply_params_with_cast(this, operand, params, casts);
	}


	// error message

	string found = "(";
	foreachi(Node *p, params, i){
		if (i > 0)
			found += ", ";
		found += p->type->name;
	}
	found += ")";
	string available;
	foreachi(Node *link, links, li){
		if (li > 0)
			available += "\n";
		Array<Class*> wanted_types = GetFunctionWantedParams(link);
		available += f_name + "(";
		foreachi(Class *p, wanted_types, i){
			if (i > 0)
				available += ", ";
			available += p->name;
		}
		available += ")";
	}
	DoError("invalid function parameters: " + found + ", valid:\n" + available);
	return nullptr;

	/*if (links.num > 1){

	}else{
	DoError("...wrong function params...");
	}*/


/*
	// test compatibility
	if (wanted_type.num != Operand->param.num){
		Exp.rewind();
		DoError(format("function \"%s\" expects %d parameters, %d were found",f_name.c_str(), Operand->param.num, wanted_type.num));
	}
	for (int p=0;p<wanted_type.num;p++){
		Operand->set_param(p, CheckParamLink(Operand->param[p], wanted_type[p], f_name, p));
	}
	return Operand;*/
}

Node *build_list(SyntaxTree *ps, Array<Node*> &el)
{
	if (el.num == 0)
		ps->DoError("empty arrays not supported yet");
//	if (el.num > SCRIPT_MAX_PARAMS)
//		ps->DoError(format("only %d elements in auto arrays supported yet", SCRIPT_MAX_PARAMS));
	Class *t = ps->CreateArrayClass(el[0]->type, -1);
	Node *c = ps->AddNode(KIND_ARRAY_BUILDER, 0, t);
	c->set_num_params(el.num);
	for (int i=0; i<el.num; i++){
		if (el[i]->type != el[0]->type)
			ps->DoError(format("inhomogenous array types %s/%s", el[i]->type->name.c_str(), el[0]->type->name.c_str()));
		c->set_param(i, el[i]);
	}
	return c;
}

Node *SyntaxTree::GetOperand(Block *block)
{
	Node *operand = nullptr;

	// ( -> one level down and combine commands
	if (Exp.cur == "("){
		Exp.next();
		operand = GetCommand(block);
		if (Exp.cur != ")")
			DoError("\")\" expected");
		Exp.next();
	}else if (Exp.cur == "&"){ // & -> address operator
		Exp.next();
		operand = ref_node(GetOperand(block));
	}else if (Exp.cur == "*"){ // * -> dereference
		Exp.next();
		operand = GetOperand(block);
		if (!operand->type->is_pointer()){
			Exp.rewind();
			DoError("only pointers can be dereferenced using \"*\"");
		}
		operand = deref_node(operand);
	}else if (Exp.cur == "["){
		Exp.next();
		Array<Node*> el;
		while(true){
			el.add(GetCommand(block));
			if ((Exp.cur != ",") and (Exp.cur != "]"))
				DoError("\",\" or \"]\" expected");
			if (Exp.cur == "]")
				break;
			Exp.next();
		}
		operand = build_list(this, el);
		Exp.next();
	}else if (Exp.cur == "new"){ // new operator
		Exp.next();
		Class *t = ParseType();
		operand = add_node_statement(STATEMENT_NEW);
		operand->type = t->get_pointer();
		if (Exp.cur == "("){
			Array<ClassFunction> cfs;
			for (ClassFunction &cf: t->functions)
				if (cf.name == IDENTIFIER_FUNC_INIT and cf.param_types.num > 0)
					cfs.add(cf);
			if (cfs.num == 0)
				DoError(format("class \"%s\" does not have a constructor with parameters", t->name.c_str()));
			operand->set_num_params(1);
			operand->set_param(0, DoClassFunction(nullptr, cfs, block));
		}
	}else if (Exp.cur == "delete"){ // delete operator
		Exp.next();
		operand = add_node_statement(STATEMENT_DELETE);
		operand->set_param(0, GetOperand(block));
		if (!operand->params[0]->type->is_pointer())
			DoError("pointer expected after delete");
	}else{
		// direct operand
		Array<Node*> links = GetExistence(Exp.cur, block);
		if (links.num > 0){
			string f_name =  Exp.cur;
			Exp.next();
			// variables get linked directly...

			// operand is executable
			if ((links[0]->kind == KIND_FUNCTION) or (links[0]->kind == KIND_VIRTUAL_FUNCTION) or (links[0]->kind == KIND_STATEMENT)){
				operand = GetFunctionCall(f_name, links, block);

			}else if (links[0]->kind == KIND_TYPE){
				if (Exp.cur == "("){
					Class *t = links[0]->as_class();
					clear_nodes(links);
					auto *vv = block->add_var(block->function->create_slightly_hidden_name(), t);
					vv->dont_add_constructor = true;
					Node *dummy = add_node_local_var(vv);
					links = {};
					for (auto *cf: t->get_constructors())
						links.add(add_node_classfunc(cf, ref_node(dummy)));
						//links.add(exlink_make_func_class(this, block->function, *cf));

					operand = GetFunctionCall(f_name, links, block);
					operand->kind = KIND_CONSTRUCTOR_AS_FUNCTION;
					operand->type = t;
					//DoError("type as function");
				}else{
					// just the type...
					operand = links.pop();
					clear_nodes(links);
				}
			}else if (links[0]->kind == KIND_PRIMITIVE_OPERATOR){
				// unary operator
				int _ie = Exp.cur_exp - 1;
				int po = links[0]->link_no;
				Operator *op = nullptr;
				clear_nodes(links);
				Node *sub_command = GetOperand(block);
				//Class *r = TypeVoid;
				Class *p2 = sub_command->type;

				// exact match?
				bool ok=false;
				for (auto *_op: operators)
					if (po == _op->primitive_id)
						if ((_op->param_type_1 == TypeVoid) and (type_match(p2, _op->param_type_2))){
							op = _op;
							ok = true;
							break;
						}


				// needs type casting?
				if (!ok){
					int pen2;
					int c2, c2_best;
					int pen_min = 100;
					for (auto *_op: operators)
						if (po == _op->primitive_id)
							if ((_op->param_type_1 == TypeVoid) and (type_match_with_cast(p2, false, false, _op->param_type_2, pen2, c2))){
								ok = true;
								if (pen2 < pen_min){
									op = _op;
									pen_min = pen2;
									c2_best = c2;
								}
						}
					// cast
					if (ok){
						sub_command = apply_type_cast(this, c2_best, sub_command);
					}
				}


				if (!ok)
					DoError("unknown unitary operator " + PrimitiveOperators[po].name + " " + p2->name, _ie);
				return add_node_operator(sub_command, nullptr, op);
			}else{

				// variables etc...
				operand = links.pop();
				clear_nodes(links);
			}
		}else{
			Class *t = GetConstantType(Exp.cur);
			if (t == TypeUnknown)
				DoError("unknown operand");

			operand = add_node_const(AddConstant(t));
			// constant for parameter (via variable)
			GetConstantValue(Exp.cur, *operand->as_const());
			Exp.next();
		}

	}

	// resolve arrays, and structures...
	operand = GetOperandExtension(operand,block);

	return operand;
}

// only "primitive" operator -> no type information
Node *SyntaxTree::GetPrimitiveOperator(Block *block)
{
	int op = WhichPrimitiveOperator(Exp.cur);
	if (op < 0)
		return nullptr;

	// command from operator
	Node *cmd = AddNode(KIND_PRIMITIVE_OPERATOR, op, TypeUnknown);
	// only provisional (only operator sign, parameters and their types by GetCommand!!!)

	Exp.next();
	return cmd;
}

/*inline int find_operator(int primitive_id, Type *param_type1, Type *param_type2)
{
	for (int i=0;i<PreOperator.num;i++)
		if (PreOperator[i].PrimitiveID == primitive_id)
			if ((PreOperator[i].ParamType1 == param_type1) and (PreOperator[i].ParamType2 == param_type2))
				return i;
	//_do_error_("");
	return 0;
}*/

bool type_match_with_cast(Class *given, bool same_chunk, bool is_modifiable, Class *wanted, int &penalty, int &cast)
{
	penalty = 0;
	cast = -1;
	if (_type_match(given, same_chunk, wanted))
	    return true;
	if (is_modifiable) // is a variable getting assigned.... better not cast
		return false;
	if (wanted == TypeString){
		ClassFunction *cf = given->get_func("str", TypeString, 0);
		if (cf){
			penalty = 50;
			cast = TYPE_CAST_OWN_STRING;
			return true;
		}
	}
	for (int i=0;i<TypeCasts.num;i++)
		if ((type_match(given, TypeCasts[i].source)) and (type_match(TypeCasts[i].dest, wanted))){ // type_match()?
			penalty = TypeCasts[i].penalty;
			cast = i;
			return true;
		}
	return false;
}

Node *apply_type_cast(SyntaxTree *ps, int tc, Node *param)
{
	if (tc < 0)
		return param;
	if (tc == TYPE_CAST_OWN_STRING){
		ClassFunction *cf = param->type->get_func("str", TypeString, 0);
		if (cf)
			return ps->add_node_classfunc(cf, ps->ref_node(param));
		ps->DoError("automatic .str() not implemented yet");
		return param;
	}
	if (param->kind == KIND_CONSTANT){
		Node *c_new = ps->add_node_const(ps->AddConstant(TypeCasts[tc].dest));
		TypeCasts[tc].func(*c_new->as_const(), *param->as_const());

		// relink node
		return c_new;
	}else{
		Node *c = ps->add_node_func(TypeCasts[tc].script, TypeCasts[tc].func_no, TypeCasts[tc].dest);
		c->set_param(0, param);
		return c;
	}
	return param;
}

Node *LinkSpecialOperatorIs(SyntaxTree *tree, Node *param1, Node *param2)
{
	if (param2->kind != KIND_TYPE)
		tree->DoError("class name expexted after 'is'");
	Class *t2 = param2->as_class();
	if (t2->vtable.num == 0)
		tree->DoError("class after 'is' needs to have virtual functions: '" + t2->name + "'");

	Class *t1 = param1->type;
	if (t1->is_pointer()){
		param1 = tree->deref_node(param1);
		t1 = t1->parent;
	}
	if (!t2->is_derived_from(t1))
		tree->DoError("'is': class '" + t2->name + "' is not derived from '" + t1->name + "'");

	// vtable2
	Node *vtable2 = tree->add_node_const(tree->AddConstant(TypePointer));
	vtable2->as_const()->as_int64() = (int_p)t2->_vtable_location_compiler_;

	// vtable1
	param1->type = TypePointer;

	return tree->add_node_operator_by_inline(param1, vtable2, INLINE_POINTER_EQUAL);
}

Node *SyntaxTree::LinkOperator(int op_no, Node *param1, Node *param2)
{
	bool left_modifiable = PrimitiveOperators[op_no].left_modifiable;
	string op_func_name = PrimitiveOperators[op_no].function_name;
	Node *op = nullptr;

	if (PrimitiveOperators[op_no].id == OPERATOR_IS)
		return LinkSpecialOperatorIs(this, param1, param2);

	Class *p1 = param1->type;
	Class *p2 = param2->type;
	bool equal_classes = false;
	if (p1 == p2)
		if (!p1->is_super_array())
			if (p1->elements.num > 0)
				equal_classes = true;

	Class *pp1 = p1;
	if (pp1->is_pointer())
		pp1 = p1->parent;

	// exact match as class function?
	for (ClassFunction &f: pp1->functions)
		if (f.name == op_func_name){
			// exact match as class function but missing a "&"?
			if (f.param_types[0]->is_pointer() and f.param_types[0]->is_pointer_silent()){
				if (type_match(p2, f.param_types[0]->parent)){
					Node *inst = ref_node(param1);
					if (p1 == pp1)
						op = add_node_classfunc(&f, inst);
					else
						op = add_node_classfunc(&f, deref_node(inst));
					op->set_num_params(1);
					op->set_param(0, ref_node(param2));
					return op;
				}
			}else if (_type_match(p2, equal_classes, f.param_types[0])){
				Node *inst = ref_node(param1);
				if (p1 == pp1)
					op = add_node_classfunc(&f, inst);
				else
					op = add_node_classfunc(&f, deref_node(inst));
				op->set_num_params(1);
				op->set_param(0, param2);
				return op;
			}
		}

	// exact (operator) match?
	for (auto *op: operators)
		if (op_no == op->primitive_id)
			if (_type_match(p1, equal_classes, op->param_type_1) and _type_match(p2, equal_classes, op->param_type_2)){
				return add_node_operator(param1, param2, op);
			}


	// needs type casting?
	int pen1, pen2;
	int c1, c2, c1_best, c2_best;
	int pen_min = 2000;
	Operator *op_found = nullptr;
	ClassFunction *op_cf_found = nullptr;
	for (auto *op: operators)
		if (op_no == op->primitive_id)
			if (type_match_with_cast(p1, equal_classes, left_modifiable, op->param_type_1, pen1, c1) and type_match_with_cast(p2, equal_classes, false, op->param_type_2, pen2, c2))
				if (pen1 + pen2 < pen_min){
					op_found = op;
					pen_min = pen1 + pen2;
					c1_best = c1;
					c2_best = c2;
				}
	for (ClassFunction &cf: p1->functions)
		if (cf.name == op_func_name)
			if (type_match_with_cast(p2, equal_classes, false, cf.param_types[0], pen2, c2))
				if (pen2 < pen_min){
					op_cf_found = &cf;
					pen_min = pen2;
					c1_best = -1;
					c2_best = c2;
				}
	// cast
	if (op_found or op_cf_found){
		param1 = apply_type_cast(this, c1_best, param1);
		param2 = apply_type_cast(this, c2_best, param2);
		if (op_cf_found){
			Node *inst = ref_node(param1);
			op = add_node_classfunc(op_cf_found, inst);
			op->set_num_params(1);
			op->set_param(0, param2);
		}else{
			return add_node_operator(param1, param2, op_found);
		}
		return op;
	}

	return nullptr;
}

void SyntaxTree::LinkMostImportantOperator(Array<Node*> &Operand, Array<Node*> &Operator, Array<int> &op_exp)
{
// find the most important operator (mio)
	int mio = 0;
	for (int i=0;i<Operator.num;i++){
		if (PrimitiveOperators[Operator[i]->link_no].level > PrimitiveOperators[Operator[mio]->link_no].level)
			mio = i;
	}

// link it
	Node *param1 = Operand[mio];
	Node *param2 = Operand[mio + 1];
	int op_no = Operator[mio]->link_no;
	Operator[mio] = LinkOperator(op_no, param1, param2);
	if (!Operator[mio])
		DoError(format("no operator found: %s %s %s", param1->type->name.c_str(), PrimitiveOperators[op_no].name.c_str(), param2->type->name.c_str()), op_exp[mio]);

// remove from list
	Operand[mio] = Operator[mio];
	Operator.erase(mio);
	op_exp.erase(mio);
	Operand.erase(mio + 1);
}

Node *SyntaxTree::GetCommand(Block *block)
{
	Array<Node*> Operand;
	Array<Node*> Operator;
	Array<int> op_exp;

	// find the first operand
	Operand.add(GetOperand(block));

	// find pairs of operators and operands
	for (int i=0;true;i++){
		op_exp.add(Exp.cur_exp);
		Node *op = GetPrimitiveOperator(block);
		if (!op)
			break;
		Operator.add(op);
		if (Exp.end_of_line()){
			//Exp.rewind();
			DoError("unexpected end of line after operator");
		}
		Operand.add(GetOperand(block));
	}


	// in each step remove/link the most important operator
	while(Operator.num > 0)
		LinkMostImportantOperator(Operand, Operator, op_exp);

	// complete command is now collected in Operand[0]
	return Operand[0];
}

// TODO later...
//  * make Node=Block
//  * for with p[0]=set init
//  * for_var in for "Block"
Node *SyntaxTree::ParseStatementFor(Block *block)
{
	// variable name
	Exp.next();
	string var_name = Exp.cur;
	Exp.next();

	if (Exp.cur != "in")
		DoError("\"in\" expected after variable in for");
	Exp.next();

	// first value
	Node *val0 = GetCommand(block);


	// last value
	if (Exp.cur != ":")
		DoError("\":\" expected after first value in for");
	Exp.next();
	Node *val1 = GetCommand(block);

	Node *val_step = nullptr;
	if (Exp.cur == ":"){
		Exp.next();
		val_step = GetCommand(block);
	}

	// type?
	Class *t = val0->type;
	if (val1->type == TypeFloat32)
		t = val1->type;
	if (val_step)
		if (val_step->type == TypeFloat32)
			t = val_step->type;
	val0 = CheckParamLink(val0, t, "for", 1);
	val1 = CheckParamLink(val1, t, "for", 1);
	if (val_step)
		val_step = CheckParamLink(val_step, t, "for", 1);

	Node *cmd_for = add_node_statement(STATEMENT_FOR);

	// variable
	Node *for_var;
	auto *var_no = block->add_var(var_name, t);
	for_var = add_node_local_var(var_no);

	// implement
	// for_var = val0
	Node *cmd_assign = add_node_operator_by_inline(for_var, val0, INLINE_INT_ASSIGN);
	cmd_for->set_param(0, cmd_assign);

	// while(for_var < val1)
	Node *cmd_cmp = add_node_operator_by_inline(for_var, val1, INLINE_INT_SMALLER);
	cmd_for->set_param(1, cmd_cmp);

	ExpectNewline();
	// ...block
	Exp.next_line();
	ExpectIndent();
	parser_loop_depth ++;
	cmd_for->set_param(2, ParseBlock(block));
	parser_loop_depth --;

	// ...for_var += 1
	Node *cmd_inc;
	if (for_var->type == TypeInt){
		if (val_step)
			cmd_inc = add_node_operator_by_inline(for_var, val_step, INLINE_INT_ADD_ASSIGN);
		else
			cmd_inc = add_node_operator_by_inline(for_var, val1 /*dummy*/, INLINE_INT_INCREASE);
	}else{
		if (!val_step){
			val_step = add_node_const(AddConstant(TypeFloat32));
			val_step->as_const()->as_float() = 1.0f;
		}
		cmd_inc = add_node_operator_by_inline(for_var, val_step, INLINE_FLOAT_ADD_ASSIGN);
	}
	cmd_for->set_param(3, cmd_inc); // add to loop-block

	// <for_var> declared internally?
	// -> force it out of scope...
	for_var->as_local()->name = "-out-of-scope-";
	// TODO  FIXME

	return cmd_for;
}

Node *SyntaxTree::ParseStatementForall(Block *block)
{
	// variable
	Exp.next();
	string var_name = Exp.cur;
	Exp.next();

	// index
	string index_name = format("-for_index_%d-", for_index_count ++);
	if (Exp.cur == ","){
		Exp.next();
		index_name = Exp.cur;
		Exp.next();
	}

	// for index
	auto *var_no_index = block->add_var(index_name, TypeInt);
	Node *for_index = add_node_local_var(var_no_index);

	// super array
	if (Exp.cur != "in")
		DoError("\"in\" expected after variable in \"for . in .\"");
	Exp.next();
	Node *for_array = GetOperand(block);
	if ((!for_array->type->usable_as_super_array()) and (!for_array->type->is_array()))
		DoError("array or list expected as second parameter in \"for . in .\"");
	//Exp.next();

	Node *cmd_for = add_node_statement(STATEMENT_FOR);

	// variable...
	Class *var_type = for_array->type->get_array_element();
	auto *var = block->add_var(var_name, var_type);
	Node *for_var = add_node_local_var(var);

	// 0
	Node *val0 = add_node_const(AddConstant(TypeInt));
	val0->as_const()->as_int() = 0;

	// implement
	// for_index = 0
	Node *cmd_assign = add_node_operator_by_inline(for_index, val0, INLINE_INT_ASSIGN);
	cmd_for->set_param(0, cmd_assign);

	Node *val1;
	if (for_array->type->usable_as_super_array()){
		// array.num
		val1 = AddNode(KIND_ADDRESS_SHIFT, config.pointer_size, TypeInt);
		val1->set_num_params(1);
		val1->set_param(0, for_array);
	}else{
		// array.size
		val1 = add_node_const(AddConstant(TypeInt));
		val1->as_const()->as_int() = for_array->type->array_length;
	}

	// while(for_index < val1)
	Node *cmd_cmp = add_node_operator_by_inline(for_index, val1, INLINE_INT_SMALLER);
	cmd_for->set_param(1, cmd_cmp);
	ExpectNewline();
	// ...block
	Exp.next_line();
	ExpectIndent();
	parser_loop_depth ++;
	Node *loop_block = ParseBlock(block);
	cmd_for->set_param(2, loop_block);
	parser_loop_depth --;

	// ...for_index += 1
	Node *cmd_inc = add_node_operator_by_inline(for_index, val1 /*dummy*/, INLINE_INT_INCREASE);
	cmd_for->set_param(3, cmd_inc);

	// &for_var
	Node *for_var_ref = ref_node(for_var);

	Node *array_el;
	if (for_array->type->usable_as_super_array()){
		// &array.data[for_index]
		array_el = add_node_parray(shift_node(cp_node(for_array), false, 0, var_type->get_pointer()),
	                                       	   for_index, var_type);
	}else{
		// &array[for_index]
		array_el = add_node_parray(ref_node(for_array),
	                                       	   for_index, var_type);
	}
	Node *array_el_ref = ref_node(array_el);

	// &for_var = &array[for_index]
	Node *cmd_var_assign = add_node_operator_by_inline(for_var_ref, array_el_ref, INLINE_POINTER_ASSIGN);
	loop_block->as_block()->params.insert(cmd_var_assign, 0);

	// ref...
	var->type = var_type->get_pointer();
	transform_node(loop_block, [&](Node *n){ return conv_cbr(this, n, var); });

	// force for_var out of scope...
	for_var->as_local()->name = "-out-of-scope-";
	for_index->as_local()->name = "-out-of-scope-";

	return cmd_for;
}

Node *SyntaxTree::ParseStatementWhile(Block *block)
{
	Exp.next();
	Node *cmd_cmp = CheckParamLink(GetCommand(block), TypeBool, "while", 0);
	ExpectNewline();

	Node *cmd_while = add_node_statement(STATEMENT_WHILE);
	cmd_while->set_param(0, cmd_cmp);

	// ...block
	Exp.next_line();
	ExpectIndent();
	parser_loop_depth ++;
	cmd_while->params.resize(2);
	cmd_while->set_param(1, ParseBlock(block));
	parser_loop_depth --;
	return cmd_while;
}

Node *SyntaxTree::ParseStatementBreak(Block *block)
{
	if (parser_loop_depth == 0)
		DoError("'break' only allowed inside a loop");
	Exp.next();
	return add_node_statement(STATEMENT_BREAK);
}

Node *SyntaxTree::ParseStatementContinue(Block *block)
{
	if (parser_loop_depth == 0)
		DoError("'continue' only allowed inside a loop");
	Exp.next();
	return add_node_statement(STATEMENT_CONTINUE);
}

Node *SyntaxTree::ParseStatementReturn(Block *block)
{
	Exp.next();
	Node *cmd = add_node_statement(STATEMENT_RETURN);
	if (block->function->return_type == TypeVoid){
		cmd->set_num_params(0);
	}else{
		Node *cmd_value = CheckParamLink(GetCommand(block), block->function->return_type, IDENTIFIER_RETURN, 0);
		cmd->set_num_params(1);
		cmd->set_param(0, cmd_value);
	}
	ExpectNewline();
	return cmd;
}

// IGNORE!!! raise() is a function :P
Node *SyntaxTree::ParseStatementRaise(Block *block)
{
	throw "jhhhh";
	Exp.next();
	Node *cmd = add_node_statement(STATEMENT_RAISE);

	Node *cmd_ex = CheckParamLink(GetCommand(block), TypeExceptionP, IDENTIFIER_RAISE, 0);
	cmd->set_num_params(1);
	cmd->set_param(0, cmd_ex);

	/*if (block->function->return_type == TypeVoid){
		cmd->set_num_params(0);
	}else{
		Node *cmd_value = CheckParamLink(GetCommand(block), block->function->return_type, IDENTIFIER_RETURN, 0);
		cmd->set_num_params(1);
		cmd->set_param(0, cmd_value);
	}*/
	ExpectNewline();
	return cmd;
}

Node *SyntaxTree::ParseStatementTry(Block *block)
{
	int ind = Exp.cur_line->indent;
	Exp.next();
	Node *cmd_try = add_node_statement(STATEMENT_TRY);
	cmd_try->params.resize(3);
	ExpectNewline();
	// ...block
	Exp.next_line();
	ExpectIndent();
	cmd_try->set_param(0, ParseBlock(block));
	Exp.next_line();

	if (Exp.cur != IDENTIFIER_EXCEPT)
		DoError("except after try expected");
	if (Exp.cur_line->indent != ind)
		DoError("wrong indentation for except");
	Exp.next();

	Node *cmd_ex = add_node_statement(STATEMENT_EXCEPT);
	cmd_try->set_param(1, cmd_ex);

	Block *except_block = new Block(block->function, block);

	if (!Exp.end_of_line()){
		Class *ex_type = FindType(Exp.cur);
		if (!ex_type)
			DoError("Exception class expected");
		if (!ex_type->is_derived_from(TypeException))
			DoError("Exception class expected");
		cmd_ex->type = ex_type;
		ex_type = ex_type->get_pointer();
		Exp.next();
		if (!Exp.end_of_line()){
			if (Exp.cur != "as")
				DoError("'as' expected");
			Exp.next();
			string ex_name = Exp.cur;
			auto *v = except_block->add_var(ex_name, ex_type);
			cmd_ex->params.add(add_node_local_var(v));
			Exp.next();
		}
	}

	int last_indent = Exp.indent_0;

	ExpectNewline();
	// ...block
	Exp.next_line();
	ExpectIndent();
	//ParseCompleteCommand(block);
	//Exp.next_line();

	//auto n = block->nodes.back();
	//n->as_block()->

	cmd_try->set_param(2, ParseBlock(block, except_block));

	//Node *cmd_ex_block = add_node_block(new_block);
	/*block->nodes.add(cmd_ex_block);

	Exp.indented = false;

	for (int i=0;true;i++){
		if (((i > 0) and (Exp.cur_line->indent <= last_indent)) or (Exp.end_of_file()))
			break;

		ParseCompleteCommand(new_block);
		Exp.next_line();
	}
	Exp.cur_line --;
	Exp.indent_0 = Exp.cur_line->indent;
	Exp.indented = false;
	Exp.cur_exp = Exp.cur_line->exp.num - 1;
	Exp.cur = Exp.cur_line->exp[Exp.cur_exp].name;*/
	return cmd_try;
}

Node *SyntaxTree::ParseStatementIf(Block *block)
{
	int ind = Exp.cur_line->indent;
	Exp.next();
	Node *cmd_cmp = CheckParamLink(GetCommand(block), TypeBool, IDENTIFIER_IF, 0);
	ExpectNewline();

	Node *cmd_if = add_node_statement(STATEMENT_IF);
	cmd_if->set_param(0, cmd_cmp);
	// ...block
	Exp.next_line();
	ExpectIndent();
	//block->nodes.add(ParseBlock(block));
	cmd_if->set_param(1, ParseBlock(block));
	Exp.next_line();

	// else?
	if ((!Exp.end_of_file()) and (Exp.cur == IDENTIFIER_ELSE) and (Exp.cur_line->indent >= ind)){
		cmd_if->link_no = STATEMENT_IF_ELSE;
		cmd_if->params.resize(3);
		Exp.next();
		// iterative if
		if (Exp.cur == IDENTIFIER_IF){
			//DoError("else if...");
			// sub-if's in a new block
			Block *cmd_block = new Block(block->function, block);
			cmd_if->set_param(2, cmd_block);
			// parse the next if
			ParseCompleteCommand(cmd_block);
			return cmd_if;
		}
		ExpectNewline();
		// ...block
		Exp.next_line();
		ExpectIndent();
		cmd_if->set_param(2, ParseBlock(block));
		//Exp.next_line();
	}else{
		int line = Exp.get_line_no() - 1;
		Exp.set(Exp.line[line].exp.num - 1, line);
	}
	return cmd_if;
}

Node *SyntaxTree::ParseStatementPass(Block *block)
{
	Exp.next(); // pass
	ExpectNewline();

	return add_node_statement(STATEMENT_PASS);
}

Node *SyntaxTree::ParseStatement(Block *block)
{
	bool has_colon = false;
	for (auto &e: Exp.cur_line->exp)
		if (e.name == ":")
			has_colon = true;

	// special commands...
	if (Exp.cur == IDENTIFIER_FOR and !has_colon){
		return ParseStatementForall(block);
	}else if (Exp.cur == IDENTIFIER_FOR){
		return ParseStatementFor(block);
	}else if (Exp.cur == IDENTIFIER_WHILE){
		return ParseStatementWhile(block);
 	}else if (Exp.cur == IDENTIFIER_BREAK){
 		return ParseStatementBreak(block);
	}else if (Exp.cur == IDENTIFIER_CONTINUE){
		return ParseStatementContinue(block);
	}else if (Exp.cur == IDENTIFIER_RETURN){
		return ParseStatementReturn(block);
	//}else if (Exp.cur == IDENTIFIER_RAISE){
	//	ParseStatementRaise(block);
	}else if (Exp.cur == IDENTIFIER_TRY){
		return ParseStatementTry(block);
	}else if (Exp.cur == IDENTIFIER_IF){
		return ParseStatementIf(block);
	}else if (Exp.cur == IDENTIFIER_PASS){
		return ParseStatementPass(block);
	}
	return nullptr;
}

Node *SyntaxTree::ParseBlock(Block *parent, Block *block)
{
	int last_indent = Exp.indent_0;

	Exp.indented = false;
	Exp.set(0); // bad hack...
	if (!block)
		block = new Block(parent->function, parent);

	for (int i=0;true;i++){
		if (((i > 0) and (Exp.cur_line->indent < last_indent)) or (Exp.end_of_file()))
			break;


		ParseCompleteCommand(block);
		Exp.next_line();
	}
	Exp.cur_line --;
	Exp.indent_0 = Exp.cur_line->indent;
	Exp.indented = false;
	Exp.cur_exp = Exp.cur_line->exp.num - 1;
	Exp.cur = Exp.cur_line->exp[Exp.cur_exp].name;

	return block;
}

// local (variable) definitions...
void SyntaxTree::ParseLocalDefinition(Block *block)
{
	// type of variable
	Class *type = ParseType();
	for (int l=0;!Exp.end_of_line();l++){
		// name
		block->add_var(Exp.cur, type);
		Exp.next();

		// assignment?
		if (Exp.cur == "="){
			Exp.rewind();
			// parse assignment
			block->add(GetCommand(block));
		}
		if (Exp.end_of_line())
			break;
		if ((Exp.cur != ",") and (!Exp.end_of_line()))
			DoError("\",\", \"=\" or newline expected after declaration of local variable");
		Exp.next();
	}
}

// we already are in the line to analyse ...indentation for a new block should compare to the last line
void SyntaxTree::ParseCompleteCommand(Block *block)
{
	// cur_exp = 0!

	bool is_type = FindType(Exp.cur);

	// block?  <- indent
	if (Exp.indented){
		block->add(ParseBlock(block));

	// assembler block
	}else if (Exp.cur == "-asm-"){
		Exp.next();
		block->add(add_node_statement(STATEMENT_ASM));

	}else if (is_type){

		ParseLocalDefinition(block);

	}else{


	// commands (the actual code!)
		//if (WhichStatement(Exp.cur) >= 0){
		if ((Exp.cur == IDENTIFIER_FOR) or (Exp.cur == IDENTIFIER_WHILE) or (Exp.cur == IDENTIFIER_BREAK) or (Exp.cur == IDENTIFIER_CONTINUE) or (Exp.cur == IDENTIFIER_RETURN) or /*(Exp.cur == IDENTIFIER_RAISE) or*/ (Exp.cur == IDENTIFIER_TRY) or (Exp.cur == IDENTIFIER_IF) or (Exp.cur == IDENTIFIER_PASS)){
			block->add(ParseStatement(block));
			// new/delete/sizeof/type... are operands...

		}else{

			// normal commands
			block->add(GetCommand(block));
		}
	}

	ExpectNewline();
}

extern Array<Script*> loading_script_stack;

void SyntaxTree::ParseImport()
{
	Exp.next(); // 'use' / 'import'

	string name = Exp.cur;
	if (name.find(".kaba") >= 0){

		string filename = script->filename.dirname() + name.substr(1, name.num - 2); // remove ""
		filename = filename.no_recursion();



		for (Script *ss: loading_script_stack)
			if (ss->filename == filename.sys_filename())
				DoError("recursive include");

		msg_right();
		Script *include;
		try{
			include = Load(filename, script->just_analyse or config.compile_os);
			// os-includes will be appended to syntax_tree... so don't compile yet
		}catch(Exception &e){

			int logical_line = Exp.get_line_no();
			int exp_no = Exp.cur_exp;
			int physical_line = Exp.line[logical_line].physical_line;
			int pos = Exp.line[logical_line].exp[exp_no].pos;
			string expr = Exp.line[logical_line].exp[exp_no].name;
			e.line = physical_line;
			e.column = pos;
			e.text += "\n...imported from:\nline " + i2s(physical_line) + ", " + script->filename;
			throw e;
			//msg_write(e.message);
			//msg_write("...");
			string msg = e.message() + "\nimported file:";
			//string msg = "in imported file:\n\"" + e.message + "\"";
			DoError(msg);
		}

		msg_left();
		AddIncludeData(include);
	}else{
		for (Package &p: Packages)
			if (p.name == name){
				AddIncludeData(p.script);
				return;
			}
		DoError("unknown package: " + name);
	}
}


void SyntaxTree::ParseEnum()
{
	Exp.next(); // 'enum'
	ExpectNewline();
	Exp.next_line();
	ExpectIndent();

	int value = 0;

	for (int i=0;!Exp.end_of_file();i++){
		for (int j=0;!Exp.end_of_line();j++){
			auto *c = AddConstant(TypeInt);
			c->name = Exp.cur;
			Exp.next();

			// explicit value
			if (Exp.cur == "="){
				Exp.next();
				ExpectNoNewline();
				Value v;
				GetConstantValue(Exp.cur, v);
				if (v.type == TypeInt)
					value = v.as_int();
				else
					DoError("integer constant expected after \"=\" for explicit value of enum");
				Exp.next();
			}
			c->as_int() = (value ++);

			if (Exp.end_of_line())
				break;
			if ((Exp.cur != ","))
				DoError("\",\" or newline expected after enum definition");
			Exp.next();
			ExpectNoNewline();
		}
		Exp.next_line();
		if (Exp.unindented)
			break;
	}
	Exp.cur_line --;
}

void SyntaxTree::ParseClassFunctionHeader(Class *t, bool as_extern, bool as_virtual, bool override)
{
	Function *f = ParseFunctionHeader(t, as_extern);
	int n = -1;
	foreachi(Function *g, functions, i)
		if (f == g)
			n = i;

	t->add_function(this, n, as_virtual, override);

	SkipParsingFunctionBody();
}

inline bool type_needs_alignment(Class *t)
{
	if (t->is_array())
		return type_needs_alignment(t->parent);
	return (t->size >= 4);
}

int class_count_virtual_functions(SyntaxTree *ps)
{
	ExpressionBuffer::Line *l = ps->Exp.cur_line;
	int count = 0;
	l ++;
	while(l != &ps->Exp.line[ps->Exp.line.num - 1]){
		if (l->indent == 0)
			break;
		if ((l->indent == 1) and (l->exp[0].name == IDENTIFIER_VIRTUAL))
			count ++;
		else if ((l->indent == 1) and (l->exp[0].name == IDENTIFIER_EXTERN) and (l->exp[1].name == IDENTIFIER_VIRTUAL))
			count ++;
		l ++;
	}
	return count;
}

void SyntaxTree::ParseClass()
{
	int indent0 = Exp.cur_line->indent;
	int _offset = 0;
	Exp.next(); // 'class'
	string name = Exp.cur;
	Exp.next();

	// create class and type
	Class *_class = CreateNewClass(name, Class::Type::OTHER, 0, 0, nullptr);

	// parent class
	if (Exp.cur == IDENTIFIER_EXTENDS){
		Exp.next();
		Class *parent = ParseType(); // force
		if (!_class->derive_from(parent, true))
			DoError(format("parental type in class definition after \"%s\" has to be a class, but %s is not", IDENTIFIER_EXTENDS.c_str(), parent->name.c_str()));
		_offset = parent->size;
	}
	ExpectNewline();

	// elements
	while(!Exp.end_of_file()){
		Exp.next_line();
		if (Exp.cur_line->indent <= indent0) //(unindented)
			break;
		if (Exp.end_of_file())
			break;

		// extern?
		next_extern = false;
		if (Exp.cur == IDENTIFIER_EXTERN){
			next_extern = true;
			Exp.next();
		}

		// virtual?
		bool next_virtual = false;
		bool override = false;
		if (Exp.cur == IDENTIFIER_VIRTUAL){
			next_virtual = true;
			Exp.next();
		}else if (Exp.cur == IDENTIFIER_OVERRIDE){
			override = true;
			Exp.next();
		}
		int ie = Exp.cur_exp;

		Class *type = ParseType(); // force
		while(!Exp.end_of_line()){
			//int indent = Exp.cur_line->indent;

			ClassElement el;
			el.type = type;
			el.name = Exp.cur;
			Exp.next();

			// is a function?
			bool is_function = false;
			if (Exp.cur == "(")
			    is_function = true;
			if (is_function){
				Exp.set(ie);
				ParseClassFunctionHeader(_class, next_extern, next_virtual, override);

				break;
			}

			// override?
			ClassElement *orig = nullptr;
			for (ClassElement &e: _class->elements)
				if (e.name == el.name) //and e.type->is_pointer and el.type->is_pointer)
					orig = &e;
			if (override and ! orig)
				DoError(format("can not override element '%s', no previous definition", el.name.c_str()));
			if (!override and orig)
				DoError(format("element '%s' is already defined, use '%s' to override", el.name.c_str(), IDENTIFIER_OVERRIDE.c_str()));
			if (override){
				if (orig->type->is_pointer() and el.type->is_pointer())
					orig->type = el.type;
				else
					DoError("can only override pointer elements with other pointer type");
				continue;
			}

			// check parsing dependencies
			if (!type->is_size_known())
				DoError("size of type " + type->name + " is not known at this point");


			// add element
			if (type_needs_alignment(type))
				_offset = mem_align(_offset, 4);
			_offset = ProcessClassOffset(_class->name, el.name, _offset);
			if ((Exp.cur != ",") and (!Exp.end_of_line()))
				DoError("\",\" or newline expected after class element");
			el.offset = _offset;
			_offset += type->size;
			_class->elements.add(el);
			if (Exp.end_of_line())
				break;
			Exp.next();
		}
	}



	// virtual functions?     (derived -> _class->num_virtual)
//	_class->vtable = cur_virtual_index;
	//foreach(ClassFunction &cf, _class->function)
	//	_class->num_virtual = max(_class->num_virtual, cf.virtual_index);
	if (_class->vtable.num > 0){
		if (_class->parent){
			if (_class->parent->vtable.num == 0)
				DoError("no virtual functions allowed when inheriting from class without virtual functions");
			// element "-vtable-" being derived
		}else{
			for (ClassElement &e: _class->elements)
				e.offset = ProcessClassOffset(_class->name, e.name, e.offset + config.pointer_size);

			ClassElement el;
			el.name = IDENTIFIER_VTABLE_VAR;
			el.type = TypePointer;
			el.offset = 0;
			el.hidden = true;
			_class->elements.insert(el, 0);
			_offset += config.pointer_size;
		}
	}

	for (ClassElement &e: _class->elements)
		if (type_needs_alignment(e.type))
			_offset = mem_align(_offset, 4);
	_class->size = ProcessClassSize(_class->name, _offset);


	AddMissingFunctionHeadersForClass(_class);

	_class->fully_parsed = true;

	Exp.cur_line --;
}

void SyntaxTree::ExpectNoNewline()
{
	if (Exp.end_of_line())
		DoError("unexpected newline");
}

void SyntaxTree::ExpectNewline()
{
	if (!Exp.end_of_line())
		DoError("newline expected");
}

void SyntaxTree::ExpectIndent()
{
	if (!Exp.indented)
		DoError("additional indent expected");
}

void SyntaxTree::ParseGlobalConst(const string &name, Class *type)
{
	if (Exp.cur != "=")
		DoError("\"=\" expected after const name");
	Exp.next();

	// find const value
	Node *cv = transform_node(GetCommand(root_of_all_evil.block), [&](Node *n){ return PreProcessNode(n); });

	if ((cv->kind != KIND_CONSTANT) or (cv->type != type))
		DoError(format("only constants of type \"%s\" allowed as value for this constant", type->name.c_str()));
	Constant *c_orig = cv->as_const();

	auto *c = AddConstant(type);
	c->set(*c_orig);
	c->name = name;

	// give our const the name
	//c_orig->name = name;
}

void SyntaxTree::ParseVariableDef(bool single, Block *block)
{
	Class *type = ParseType(); // force

	for (int j=0;true;j++){
		ExpectNoNewline();

		// name
		string name = Exp.cur;
		Exp.next();

		if (next_const){
			ParseGlobalConst(name, type);
		}else
			block->add_var(name, type);

		if ((Exp.cur != ",") and (!Exp.end_of_line()))
			DoError("\",\" or newline expected after definition of a global variable");

		// last one?
		if (Exp.end_of_line())
			break;

		Exp.next(); // ','
	}
}

bool peek_commands_super(ExpressionBuffer &Exp)
{
	ExpressionBuffer::Line *l = Exp.cur_line + 1;
	if (l->exp.num < 3)
		return false;
	if ((l->exp[0].name == IDENTIFIER_SUPER) and (l->exp[1].name == ".") and (l->exp[2].name == IDENTIFIER_FUNC_INIT))
		return true;
	return false;
}

bool SyntaxTree::ParseFunctionCommand(Function *f, ExpressionBuffer::Line *this_line)
{
	if (Exp.end_of_file())
		return false;

	Exp.next_line();
	Exp.indented = false;

	// end of file
	if (Exp.end_of_file())
		return false;

	// end of function
	if (Exp.cur_line->indent <= this_line->indent)
		return false;

	// command or local definition
	ParseCompleteCommand(f->block);
	return true;
}

void Function::update(Class *class_type)
{
	// save "original" param types (Var[].Type gets altered for call by reference)
	for (int i=literal_param_type.num;i<num_params;i++)
		literal_param_type.add(var[i]->type);
	// but only, if not existing yet...

	// return by memory
	if (return_type->uses_return_by_memory())
		block->add_var(IDENTIFIER_RETURN_VAR, return_type->get_pointer());

	// class function
	_class = class_type;
	if (class_type){
		if (!__get_var(IDENTIFIER_SELF))
			block->add_var(IDENTIFIER_SELF, class_type->get_pointer());

		// convert name to Class.Function
		name = class_type->name + "." +  name;
	}
}


Class *SyntaxTree::ParseType()
{
	// base type
	Class *t = FindType(Exp.cur);
	if (!t)
		DoError("unknown type");
	Exp.next();

	while (true){

		// pointer?
		if (Exp.cur == "*"){
			Exp.next();
			t = t->get_pointer();
		}else if (Exp.cur == "["){
			Exp.next();

			// no index -> super array
			if (Exp.cur == "]"){
				t = CreateArrayClass(t, -1);
			}else{

				// find array index
				Node *c = transform_node(GetCommand(root_of_all_evil.block), [&](Node *n){ return PreProcessNode(n); });

				if ((c->kind != KIND_CONSTANT) or (c->type != TypeInt))
					DoError("only constants of type \"int\" allowed for size of arrays");
				int array_size = c->as_const()->as_int();
				//Exp.next();
				if (Exp.cur != "]")
					DoError("\"]\" expected after array size");
				t = CreateArrayClass(t, array_size);
			}

			Exp.next();
		}else if (Exp.cur == "{"){
			Exp.next();

			if (Exp.cur != "}")
				DoError("\"}\" expected after dict{");

			Exp.next();

			t = CreateDictClass(t);
		}else{
			break;
		}
	}

	return t;
}

Function *SyntaxTree::ParseFunctionHeader(Class *class_type, bool as_extern)
{
// return type
	Class *return_type = ParseType(); // force...

	Function *f = AddFunction(Exp.cur, return_type);
	f->_logical_line_no = Exp.get_line_no();
	f->_exp_no = Exp.cur_exp;
	cur_func = f;
	next_extern = false;

	Exp.next();
	Exp.next(); // '('

// parameter list

	if (Exp.cur != ")")
		for (int k=0;;k++){
			// like variable definitions

			// type of parameter variable
			Class *param_type = ParseType(); // force
			f->block->add_var(Exp.cur, param_type);
			f->literal_param_type.add(param_type);
			Exp.next();
			f->num_params ++;

			if (Exp.cur == ")")
				break;

			if (Exp.cur != ",")
				DoError("\",\" or \")\" expected after parameter");
			Exp.next(); // ','
		}
	Exp.next(); // ')'

	if (!Exp.end_of_line())
		DoError("newline expected after parameter list");

	f->update(class_type);

	f->is_extern = as_extern;
	cur_func = nullptr;

	return f;
}

void SyntaxTree::SkipParsingFunctionBody()
{
	int indent0 = Exp.cur_line->indent;
	while (!Exp.end_of_file()){
		if (Exp.cur_line[1].indent <= indent0)
			break;
		Exp.next_line();
	}
}

void SyntaxTree::ParseFunctionBody(Function *f)
{
	Exp.cur_line = &Exp.line[f->_logical_line_no];

	ExpressionBuffer::Line *this_line = Exp.cur_line;
	bool more_to_parse = true;

	// auto implement constructor?
	if (f->name.tail(IDENTIFIER_FUNC_INIT.num + 1) == "." + IDENTIFIER_FUNC_INIT){
		if (peek_commands_super(Exp)){
			more_to_parse = ParseFunctionCommand(f, this_line);

			AutoImplementConstructor(f, f->_class, false);
		}else
			AutoImplementConstructor(f, f->_class, true);
	}

	parser_loop_depth = 0;

// instructions
	while(more_to_parse){
		more_to_parse = ParseFunctionCommand(f, this_line);
	}

	// auto implement destructor?
	if (f->name.tail(IDENTIFIER_FUNC_DELETE.num + 1) == "." + IDENTIFIER_FUNC_DELETE)
		AutoImplementDestructor(f, f->_class);
	cur_func = nullptr;

	Exp.cur_line --;
}

void SyntaxTree::ParseAllClassNames()
{
	Exp.reset_parser();
	while (!Exp.end_of_file()){
		if ((Exp.cur_line->indent == 0) and (Exp.cur_line->exp.num >= 2)){
			if (Exp.cur == IDENTIFIER_CLASS){
				Exp.next();
				int nt0 = classes.num;
				Class *t = CreateNewClass(Exp.cur, Class::Type::OTHER, 0, 0, nullptr);
				if (nt0 == classes.num)
					DoError("class already exists");
				t->fully_parsed = false;
			}
		}
		Exp.next_line();
	}
}

void SyntaxTree::ParseAllFunctionBodies()
{
	//for (auto *f: functions)   NO... might encounter new classes creating new functions!
	for (int i=0; i<functions.num; i++){
		auto *f = functions[i];
		if ((!f->is_extern) and (f->_logical_line_no >= 0))
			ParseFunctionBody(f);
	}
}

void SyntaxTree::ParseTopLevel()
{
	root_of_all_evil.name = "RootOfAllEvil";
	cur_func = nullptr;

	// syntax analysis

	ParseAllClassNames();

	Exp.reset_parser();

	// global definitions (enum, class, variables and functions)
	while (!Exp.end_of_file()){
		next_extern = false;
		next_const = false;

		// extern?
		if (Exp.cur == IDENTIFIER_EXTERN){
			next_extern = true;
			Exp.next();
		}

		// const?
		if (Exp.cur == IDENTIFIER_CONST){
			next_const = true;
			Exp.next();
		}


		/*if ((Exp.cur == "import") or (Exp.cur == "use")){
			ParseImport();

		// enum
		}else*/ if (Exp.cur == IDENTIFIER_ENUM){
			ParseEnum();

		// class
		}else if (Exp.cur == IDENTIFIER_CLASS){
			ParseClass();

		}else{

			// type of definition
			bool is_function = false;
			for (int j=1;j<Exp.cur_line->exp.num-1;j++)
				if (Exp.cur_line->exp[j].name == "(")
				    is_function = true;

			// function?
			if (is_function){
				ParseFunctionHeader(nullptr, next_extern);
				SkipParsingFunctionBody();

			// global variables
			}else{
				ParseVariableDef(false, root_of_all_evil.block);
			}
		}
		if (!Exp.end_of_file())
			Exp.next_line();
	}
}

// convert text into script data
void SyntaxTree::Parser()
{
	ParseTopLevel();

	ParseAllFunctionBodies();

	for (int i=0; i<classes.num; i++)
		AutoImplementFunctions(classes[i]);
}

}
