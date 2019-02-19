/*
 * Node.cpp
 *
 *  Created on: 18.02.2019
 *      Author: michi
 */
#include "../kaba.h"
#include "../asm/asm.h"
#include "../../file/file.h"
#include <stdio.h>

namespace Kaba{

extern bool next_extern;

string kind2str(int kind)
{
	if (kind == KIND_VAR_LOCAL)			return "local";
	if (kind == KIND_VAR_GLOBAL)			return "global";
	if (kind == KIND_VAR_FUNCTION)		return "function as variable";
	if (kind == KIND_CONSTANT)			return "constant";
	if (kind == KIND_REF_TO_CONST)			return "reference to const";
	if (kind == KIND_FUNCTION)			return "function";
	if (kind == KIND_INLINE_FUNCTION)			return "inline";
	if (kind == KIND_VIRTUAL_FUNCTION)	return "virtual function";
	if (kind == KIND_STATEMENT)			return "statement";
	if (kind == KIND_OPERATOR)			return "operator";
	if (kind == KIND_PRIMITIVE_OPERATOR)	return "PRIMITIVE operator";
	if (kind == KIND_BLOCK)				return "block";
	if (kind == KIND_ADDRESS_SHIFT)		return "address shift";
	if (kind == KIND_ARRAY)				return "array element";
	if (kind == KIND_POINTER_AS_ARRAY)		return "pointer as array element";
	if (kind == KIND_REFERENCE)			return "address operator";
	if (kind == KIND_DEREFERENCE)		return "dereferencing";
	if (kind == KIND_DEREF_ADDRESS_SHIFT)	return "deref address shift";
	if (kind == KIND_TYPE)				return "type";
	if (kind == KIND_ARRAY_BUILDER)		return "array builder";
	if (kind == KIND_CONSTRUCTOR_AS_FUNCTION)		return "constructor function";
	if (kind == KIND_VAR_TEMP)			return "temp";
	if (kind == KIND_DEREF_VAR_TEMP)		return "deref temp";
	if (kind == KIND_REGISTER)			return "register";
	if (kind == KIND_ADDRESS)			return "address";
	if (kind == KIND_MEMORY)				return "memory";
	if (kind == KIND_LOCAL_ADDRESS)		return "local address";
	if (kind == KIND_LOCAL_MEMORY)		return "local memory";
	if (kind == KIND_DEREF_REGISTER)		return "deref register";
	if (kind == KIND_MARKER)				return "marker";
	if (kind == KIND_DEREF_MARKER)		return "deref marker";
	if (kind == KIND_GLOBAL_LOOKUP)		return "global lookup";
	if (kind == KIND_DEREF_GLOBAL_LOOKUP)	return "deref global lookup";
	if (kind == KIND_IMMEDIATE)			return "immediate";
	if (kind == KIND_REF_TO_LOCAL)			return "ref to local";
	if (kind == KIND_REF_TO_GLOBAL)		return "ref to global";
	if (kind == KIND_REF_TO_CONST)			return "ref to const";
	if (kind == KIND_DEREF_VAR_LOCAL)		return "deref local";
	return format("UNKNOWN KIND: %d", kind);
}


string op_sig(const Operator *op)
{
	return "(" + op->param_type_1->name + ") " + PrimitiveOperators[op->primitive_id].name + " (" + op->param_type_2->name + ")";
}

string node_sig(SyntaxTree *s, Function *f, Node *n)
{
	string t = n->type->name + " ";
	if (n->kind == KIND_VAR_LOCAL)			return t + n->as_local(f)->name;
	if (n->kind == KIND_VAR_GLOBAL)			return t + n->as_global()->name;
	if (n->kind == KIND_VAR_FUNCTION)		return t + n->as_func()->name;
	if (n->kind == KIND_CONSTANT)			return t + n->as_const()->str();
	if (n->kind == KIND_FUNCTION)			return n->as_func()->signature();
	if (n->kind == KIND_INLINE_FUNCTION)	return n->as_func()->signature();
	if (n->kind == KIND_VIRTUAL_FUNCTION)	return t + i2s(n->link_no);//s->Functions[nr]->name;
	if (n->kind == KIND_STATEMENT)	return t + Statements[n->link_no].name;
	if (n->kind == KIND_OPERATOR)			return op_sig(n->as_op());
	if (n->kind == KIND_PRIMITIVE_OPERATOR)	return PrimitiveOperators[n->link_no].name;
	if (n->kind == KIND_BLOCK)				return "";//p2s(n->as_block());
	if (n->kind == KIND_ADDRESS_SHIFT)		return t + i2s(n->link_no);
	if (n->kind == KIND_ARRAY)				return t;
	if (n->kind == KIND_POINTER_AS_ARRAY)		return t;
	if (n->kind == KIND_REFERENCE)			return t;
	if (n->kind == KIND_DEREFERENCE)		return t;
	if (n->kind == KIND_DEREF_ADDRESS_SHIFT)	return t + i2s(n->link_no);
	if (n->kind == KIND_TYPE)				return n->as_class()->name;
	if (n->kind == KIND_REGISTER)			return t + Asm::GetRegName(n->link_no);
	if (n->kind == KIND_ADDRESS)			return t + d2h(&n->link_no, config.pointer_size);
	if (n->kind == KIND_MEMORY)				return t + d2h(&n->link_no, config.pointer_size);
	if (n->kind == KIND_LOCAL_ADDRESS)		return t + d2h(&n->link_no, config.pointer_size);
	if (n->kind == KIND_LOCAL_MEMORY)		return t + d2h(&n->link_no, config.pointer_size);
	return t + i2s(n->link_no);
}

string node2str(SyntaxTree *s, Function *f, Node *n)
{
	return "<" + kind2str(n->kind) + ">  " + node_sig(s, f, n);
}





Block::Block(Function *f, Block *_parent) :
	Node(KIND_BLOCK, (int_p)this, f->tree->script, TypeVoid)
{
	level = 0;
	function = f;
	parent = _parent;
	if (parent)
		level = parent->level + 1;
	_start = _end = nullptr;
}

Block::~Block()
{
	/*for (Node *n: params){
		if (n->kind == KIND_BLOCK)
			delete (n->as_block());
		//delete n;
	}*/
}


inline void set_command(Node *&a, Node *b)
{
	if (a == b)
		return;
	if (a)
		a->ref_count --;
	if (b){
		if (b->ref_count > 0){
			//msg_write(">> " + kindsStr(b->kind));
		}
		b->ref_count ++;
	}
	a = b;
}

void Block::add(Node *c)
{
	params.add(c);
	c->ref_count ++;
}

void Block::set(int index, Node *c)
{
	set_command(params[index], c);
}

int Block::add_var(const string &name, Class *type)
{
	if (get_var(name) >= 0)
		function->tree->DoError(format("variable '%s' already declared in this context", name.c_str()));
	Variable *v = new Variable(name, type);
	v->is_extern = next_extern;
	function->var.add(v);
	int n = function->var.num - 1;
	vars.add(n);
	return n;
}

int Block::get_var(const string &name)
{
	for (int i: vars)
		if (function->var[i]->name == name)
			return i;
	if (parent)
		return parent->get_var(name);
	return -1;
}


Node::Node(int _kind, long long _link_no, Script *_script, Class *_type)
{
	type = _type;
	kind = _kind;
	link_no = _link_no;
	instance = nullptr;
	script = _script;
	ref_count = 0;
}

Node::~Node()
{
	/*if (instance)
		delete instance;
	for (auto &p: params)
		if (p)
			delete p;*/
	// TODO later
}

Block *Node::as_block() const
{
	return (Block*)this;//(int_p)link_no;
}

Function *Node::as_func() const
{
	return script->syntax->functions[link_no];
}

Class *Node::as_class() const
{
	return script->syntax->classes[link_no];
}

Constant *Node::as_const() const
{
	return script->syntax->constants[link_no];
}

Operator *Node::as_op() const
{
	return &script->syntax->operators[link_no];
}
void *Node::as_func_p() const
{
	return (void*)script->func[link_no];
}

// will be the address at runtime...(not the current location...)
void *Node::as_const_p() const
{
	return as_const()->address;
	//return script->cnst[link_no];
}

void *Node::as_global_p() const
{
	return script->syntax->root_of_all_evil.var[link_no]->memory;
}

Variable *Node::as_global() const
{
	return script->syntax->root_of_all_evil.var[link_no];
}

Variable *Node::as_local(Function *f) const
{
	return f->var[link_no];
}

void Node::set_instance(Node *p)
{
	set_command(instance, p);
}

void Node::set_num_params(int n)
{
	params.resize(n);
}

void Node::set_param(int index, Node *p)
{
	if ((index < 0) or (index >= params.num)){
		this->script->syntax->ShowNode(this, this->script->cur_func);
		script->DoErrorInternal(format("Command.set_param...  %d %d", index, params.num));
	}
	set_command(params[index], p);
}

}

