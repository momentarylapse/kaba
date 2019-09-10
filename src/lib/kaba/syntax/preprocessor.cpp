#include "../kaba.h"
#include "../asm/asm.h"
#include "../../file/file.h"
#include <type_traits>

namespace Kaba {

typedef void op_func(Value&, Value&, Value&);


void db_out(const string &s) {
	//msg_write(s);
}

// call-by-reference dummy
class CBR {};

template<class R>
void call0(void *ff, void *ret, const Array<void*> &param) {
	if (std::is_same<CBR,R>::value) {
		//msg_write("CBR return (1p)!!!");
		((void(*)(void*))ff)(ret);
	} else {
		*(R*)ret = ((R(*)())ff)();
	}
}

template<class R, class A>
void call1(void *ff, void *ret, const Array<void*> &param) {
	if (std::is_same<CBR,R>::value) {
		if (std::is_same<CBR,A>::value) {
			db_out("CBR -> CBR");
			((void(*)(void*, void*))ff)(ret, param[0]);
		} else {
			db_out("x -> CBR");
			((void(*)(void*, A))ff)(ret, *(A*)param[0]);
		}
	} else {
		if (std::is_same<CBR,A>::value) {
			db_out("CBR -> x");
			*(R*)ret = ((R(*)(void*))ff)(param[0]);
		} else {
			db_out("x -> x");
			*(R*)ret = ((R(*)(A))ff)(*(A*)param[0]);
		}
	}
}

template<class R, class A, class B>
void call2(void *ff, void *ret, const Array<void*> &param) {
	if (std::is_same<CBR,R>::value) {
		if (std::is_same<CBR,A>::value and std::is_same<CBR,B>::value) {
			//msg_write("CBR CBR -> CBR");
			((void(*)(void*, void*, void*))ff)(ret, param[0], param[1]);
		} else {
			//msg_write("x x -> CBR");
			((void(*)(void*, A, B))ff)(ret, *(A*)param[0], *(B*)param[1]);
		}

	} else {
		*(R*)ret = ((R(*)(A, B))ff)(*(A*)param[0], *(B*)param[1]);
	}
}

template<class R, class A, class B, class C>
void call3(void *ff, void *ret, const Array<void*> &param) {
	if (std::is_same<CBR,R>::value) {
		((void(*)(void*, A, B, C))ff)(ret, *(A*)param[0], *(B*)param[1], *(C*)param[2]);
	} else {
		*(R*)ret = ((R(*)(A, B, C))ff)(*(A*)param[0], *(B*)param[1], *(C*)param[2]);
	}
}

template<class R, class A, class B, class C, class D>
void call4(void *ff, void *ret, const Array<void*> &param) {
	if (std::is_same<CBR,R>::value) {
		((void(*)(void*, A, B, C, D))ff)(ret, *(A*)param[0], *(B*)param[1], *(C*)param[2], *(D*)param[3]);
	} else {
		*(R*)ret = ((R(*)(A, B, C, D))ff)(*(A*)param[0], *(B*)param[1], *(C*)param[2], *(D*)param[3]);
	}
}

bool call_function(Function *f, void *ff, void *ret, const Array<void*> &param) {
	db_out("eval: " + f->signature());
	Array<const Class*> ptype = f->literal_param_type;
	if (!f->is_static)
		ptype.insert(f->name_space, 0);


	if (ptype.num == 0) {
		if (f->return_type == TypeInt) {
			call0<int>(ff, ret, param);
			return true;
		} else if (f->return_type == TypeFloat32) {
			call0<float>(ff, ret, param);
			return true;
		} else if (f->return_type->uses_return_by_memory()) {
			call0<CBR>(ff, ret, param);
			return true;
		}
	} else if (ptype.num == 1) {
		if (f->return_type == TypeInt) {
			if (ptype[0] == TypeInt) {
				call1<int,int>(ff, ret, param);
				return true;
			} else if (ptype[0] == TypeFloat32) {
				call1<int,float>(ff, ret, param);
				return true;
			} else if (ptype[0]->uses_call_by_reference()) {
				call1<int,CBR>(ff, ret, param);
				return true;
			}
		} else if (f->return_type == TypeBool) {
			if (ptype[0] == TypeInt) {
				call1<bool,int>(ff, ret, param);
				return true;
			} else if (ptype[0] == TypeFloat32) {
				call1<bool,float>(ff, ret, param);
				return true;
			}
		} else if (f->return_type == TypeFloat32) {
			if (ptype[0] == TypeFloat32) {
				call1<float,float>(ff, ret, param);
				return true;
			} else if (ptype[0]->uses_call_by_reference()) {
				call1<float,CBR>(ff, ret, param);
				return true;
			}
		} else if (f->return_type->uses_return_by_memory()) {
			if (ptype[0] == TypeInt) {
				call1<CBR,int>(ff, ret, param);
				return true;
			} else if (ptype[0] == TypeFloat32) {
				call1<CBR,float>(ff, ret, param);
				return true;
			} else if (ptype[0]->uses_call_by_reference()) {
				call1<CBR,CBR>(ff, ret, param);
				return true;
			}
		}
	} else if (ptype.num == 2) {
		if (f->return_type == TypeInt) {
			if ((ptype[0] == TypeInt) and(ptype[1] == TypeInt)) {
				call2<int,int,int>(ff, ret, param);
				return true;
			}
		} else if (f->return_type == TypeFloat32) {
			if ((ptype[0] == TypeFloat32) and(ptype[1] == TypeFloat32)) {
				call2<float,float,float>(ff, ret, param);
				return true;
			}
		} else if (f->return_type->uses_return_by_memory()) {
			if ((ptype[0] == TypeInt) and(ptype[1] == TypeInt)) {
				call2<CBR,int,int>(ff, ret, param);
				return true;
			} else if ((ptype[0] == TypeFloat32) and(ptype[1] == TypeFloat32)) {
				call2<CBR,float,float>(ff, ret, param);
				return true;
			} else if ((ptype[0]->uses_call_by_reference()) and (ptype[1]->uses_call_by_reference())) {
				call2<CBR,CBR,CBR>(ff, ret, param);
				return true;
			}
		}
	} else if (ptype.num == 3) {
		if (f->return_type->uses_return_by_memory()) {
			if ((ptype[0] == TypeFloat32) and(ptype[1] == TypeFloat32) and(ptype[2] == TypeFloat32)) {
				((void(*)(void*, float, float, float))ff)(ret, *(float*)param[0], *(float*)param[1], *(float*)param[2]);
				return true;
			}
		}
	} else if (ptype.num == 4) {
		if (f->return_type->uses_return_by_memory()) {
			if ((ptype[0] == TypeFloat32) and(ptype[1] == TypeFloat32) and(ptype[2] == TypeFloat32) and(ptype[3] == TypeFloat32)) {
				((void(*)(void*, float, float, float, float))ff)(ret, *(float*)param[0], *(float*)param[1], *(float*)param[2], *(float*)param[3]);
				return true;
			}
		}
	}
	db_out(".... NOPE");
	return false;
}


Node *eval_function_call(SyntaxTree *tree, Node *c, Function *f) {
	db_out("??? " + f->signature());

	if (!f->is_pure)
		return c;
	db_out("-pure");

	if (!Value::can_init(f->return_type))
		return c;
	db_out("-constr");

	void *ff = f->address_preprocess;
	if (!ff)
		return c;
	db_out("-addr");

	// parameters
	Array<void*> p;
	if (c->instance) {
		if (c->instance->kind != NodeKind::CONSTANT)
			return c;
		db_out("i: " + c->instance->str());
		p.add(c->instance->as_const()->p());
	}
	for (int i=0;i<c->params.num;i++) {
		if (c->params[i]->kind != NodeKind::CONSTANT)
			return c;
		db_out("p: " + c->params[i]->str());
		p.add(c->params[i]->as_const()->p());
	}
	db_out("-param const");

	Value temp;
	temp.init(f->return_type);
	if (!call_function(f, ff, temp.p(), p))
		return c;
	Node *r = tree->add_node_const(tree->add_constant(f->return_type));
	r->as_const()->set(temp);
	db_out(">>>  " + r->str());
	return r;
}


// BEFORE transforming to call-by-reference!
Node *SyntaxTree::conv_eval_const_func(Node *c) {
	if (c->kind == NodeKind::OPERATOR) {
		return eval_function_call(this, c, c->as_op()->f);
	} else if (c->kind == NodeKind::FUNCTION_CALL) {
		return eval_function_call(this, c, c->as_func());
	} else if (c->kind == NodeKind::ARRAY_BUILDER) {
		bool all_consts = true;
		for (int i=0; i<c->params.num; i++)
			if (c->params[i]->kind != NodeKind::CONSTANT)
				all_consts = false;
		if (all_consts) {
			Node *c_array = add_node_const(add_constant(c->type));
			int el_size = c->type->parent->size;
			DynamicArray *da = &c_array->as_const()->as_array();
			da->init(el_size);
			da->simple_resize(c->params.num);
			for (int i=0; i<c->params.num; i++)
				memcpy((char*)da->data + el_size * i, c->params[i]->as_const()->p(), el_size);
			// TODO  use kaba_var_assign() instead... prevent double free?!?
			return c_array;
		}
	}

	//else if (c->kind == KindReference) {
	// no... we don't know the addresses of globals/constants yet...
	return c;
}


// may not use AddConstant()!!!
Node *SyntaxTree::pre_process_node_addresses(Node *c) {
	if (c->kind == NodeKind::OPERATOR) {
		Operator *o = c->as_op();
		if (o->f->address) {
			bool all_const = true;
			bool is_address = false;
			bool is_local = false;
			for (int i=0;i<c->params.num;i++)
				if (c->params[i]->kind == NodeKind::ADDRESS)
					is_address = true;
				else if (c->params[i]->kind == NodeKind::LOCAL_ADDRESS)
					is_address = is_local = true;
				else if (c->params[i]->kind != NodeKind::CONSTANT)
					all_const = false;
			if (all_const) {
				op_func *f = (op_func*)o->f->address;
				if (is_address) {
					// pre process address
					Value d1, d2;
					d1.init(c->params[0]->type);
					d2.init(c->params[1]->type);
					*(void**)d1.p() = (void*)c->params[0]->link_no;
					*(void**)d2.p() = (void*)c->params[1]->link_no;
					if (c->params[0]->kind == NodeKind::CONSTANT)
					    d1.set(*c->params[0]->as_const());
					if (c->params[1]->kind == NodeKind::CONSTANT)
					    d2.set(*c->params[1]->as_const());
					Value r;
					r.init(c->type);
					f(r, d1, d2);
					return new Node(is_local ? NodeKind::LOCAL_ADDRESS : NodeKind::ADDRESS, *(int_p*)r.p(), c->type);
				}
			}
		}
	} else if (c->kind == NodeKind::REFERENCE) {
		Node *p0 = c->params[0];
		if (p0->kind == NodeKind::VAR_GLOBAL) {
			return new Node(NodeKind::ADDRESS, (int_p)p0->as_global_p(), c->type);
		} else if (p0->kind == NodeKind::VAR_LOCAL) {
			return new Node(NodeKind::LOCAL_ADDRESS, (int_p)p0->as_local()->_offset, c->type);
		} else if (p0->kind == NodeKind::CONSTANT) {
			return new Node(NodeKind::ADDRESS, (int_p)p0->as_const_p(), c->type);
		}
	} else if (c->kind == NodeKind::DEREFERENCE) {
		Node *p0 = c->params[0];
		if (p0->kind == NodeKind::ADDRESS) {
			return new Node(NodeKind::MEMORY, p0->link_no, c->type);
		} else if (p0->kind == NodeKind::LOCAL_ADDRESS) {
			return new Node(NodeKind::LOCAL_MEMORY, p0->link_no, c->type);
		}
	}
	return c;
}

void SyntaxTree::eval_const_expressions() {
	transform([&](Node *n){ return conv_eval_const_func(n); });
}

void SyntaxTree::pre_processor_addresses() {
	transform([&](Node *n){ return pre_process_node_addresses(n); });
}

};
