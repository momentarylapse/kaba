#include "../../base/base.h"
#include "../kaba.h"
#include "../../file/file.h"
#include "Class.h"

namespace Kaba{

ClassElement::ClassElement() {
	offset = 0;
	type = nullptr;
	hidden = false;
}

ClassElement::ClassElement(const string &_name, const Class *_type, int _offset) {
	name = _name;
	offset = _offset;
	type = _type;
	hidden = false;
}

string ClassElement::signature(bool include_class) const {
	if (include_class)
		return type->name + " " + name;
	return type->name + " " + name;
}


ClassFunction::ClassFunction() {
	func = nullptr;
	return_type = nullptr;
}

ClassFunction::ClassFunction(const Class *_return_type, Function *_f) {
	return_type = _return_type;
	func = _f;
}

string ClassFunction::signature() const {
	return func->signature();
}

bool type_match(const Class *given, const Class *wanted) {
	// exact match?
	if (given == wanted)
		return true;

	// allow any pointer?
	if ((given->is_pointer()) and (wanted == TypePointer))
		return true;

	// FIXME... quick'n'dirty hack to allow nil as parameter
	if ((given == TypePointer) and wanted->is_pointer() and !wanted->is_pointer_silent())
		return true;

	// compatible pointers (of same or derived class)
	if (given->is_pointer() and wanted->is_pointer())
		return given->parent->is_derived_from(wanted->parent);

	return given->is_derived_from(wanted);
}


// allow same classes... TODO deprecate...
bool _type_match(const Class *given, bool same_chunk, const Class *wanted) {
	if ((same_chunk) and (wanted == TypeChunk))
		return true;

	return type_match(given, wanted);
}

Class::Class(const string &_name, int _size, SyntaxTree *_owner, const Class *_parent) {
	name = _name;
	owner = _owner;
	size = _size;
	type = Type::OTHER;
	array_length = 0;
	parent = _parent;
	name_space = nullptr;
	force_call_by_value = false;
	fully_parsed = true;
	_vtable_location_target_ = nullptr;
	_vtable_location_compiler_ = nullptr;
	_vtable_location_external_ = nullptr;
};

Class::~Class() {
	for (auto *c: constants)
		delete c;
	for (auto *c: classes)
		delete c;
}



string namespacify(const string &name, const Class *name_space) {
	if (name_space)
		if (name_space->name[0] != '-')
			return namespacify(name_space->name + "." + name, name_space->name_space);
	return name;
}

string Class::long_name() const {
	return namespacify(name, name_space);
}

bool Class::is_array() const
{ return type == Type::ARRAY; }

bool Class::is_super_array() const
{ return type == Type::SUPER_ARRAY; }

bool Class::is_pointer() const
{ return type == Type::POINTER or type == Type::POINTER_SILENT; }

bool Class::is_pointer_silent() const
{ return type == Type::POINTER_SILENT; }

bool Class::is_dict() const
{ return type == Type::DICT; }

bool Class::uses_call_by_reference() const {
	return (!force_call_by_value and !is_pointer()) or is_array();
}

bool Class::uses_return_by_memory() const {
	return (!force_call_by_value and !is_pointer()) or is_array();
}



bool Class::is_simple_class() const {
	if (!uses_call_by_reference())
		return true;
	/*if (is_array)
		return false;*/
	if (is_super_array())
		return false;
	if (is_dict())
		return false;
	if (vtable.num > 0)
		return false;
	if (parent)
		if (!parent->is_simple_class())
			return false;
	if (get_constructors().num > 0)
		return false;
	if (get_destructor())
		return false;
	if (get_assign())
		return false;
	for (ClassElement &e: elements)
		if (!e.type->is_simple_class())
			return false;
	return true;
}

bool Class::usable_as_super_array() const {
	if (is_super_array())
		return true;
	if (is_array() or is_dict() or is_pointer())
		return false;
	if (parent)
		return parent->usable_as_super_array();
	return false;
}

const Class *Class::get_array_element() const {
	if (is_array() or is_super_array() or is_dict())
		return parent;
	if (is_pointer())
		return nullptr;
	if (parent)
		return parent->get_array_element();
	return nullptr;
}

bool Class::needs_constructor() const {
	if (!uses_call_by_reference())
		return false;
	if (is_super_array() or is_dict())
		return true;
	if (is_array())
		return parent->needs_constructor();
	if (vtable.num > 0)
		return true;
	if (parent)
		if (parent->needs_constructor())
			return true;
	for (ClassElement &e: elements)
		if (e.type->needs_constructor())
			return true;
	return false;
}

bool Class::is_size_known() const {
	if (!fully_parsed)
		return false;
	if (is_super_array() or is_dict() or is_pointer())
		return true;
	for (ClassElement &e: elements)
		if (!e.type->is_size_known())
			return false;
	return true;
}

bool Class::needs_destructor() const {
	if (!uses_call_by_reference())
		return false;
	if (is_super_array() or is_dict())
		return true;
	if (is_array())
		return parent->get_destructor();
	if (parent) {
		if (parent->get_destructor())
			return true;
		if (parent->needs_destructor())
			return true;
	}
	for (ClassElement &e: elements) {
		if (e.type->get_destructor())
			return true;
		if (e.type->needs_destructor())
			return true;
	}
	return false;
}

bool Class::is_derived_from(const Class *root) const {
	if (this == root)
		return true;
	if (is_super_array() or is_array() or is_dict() or is_pointer())
		return false;
	if (!parent)
		return false;
	return parent->is_derived_from(root);
}

bool Class::is_derived_from_s(const string &root) const {
	if (name == root)
		return true;
	if (is_super_array() or is_array() or is_dict() or is_pointer())
		return false;
	if (!parent)
		return false;
	return parent->is_derived_from_s(root);
}

ClassFunction *Class::get_func(const string &_name, const Class *return_type, const Array<const Class*> &params) const {
	for (auto &f: member_functions)
		if ((f.func->name == _name) and (f.return_type == return_type) and (f.func->num_params == params.num)) {
			bool match = true;
			for (int i=0; i<params.num; i++) {
				if (params[i] and (f.func->literal_param_type[i] != params[i])) {
					match = false;
					break;
				}
			}
			if (match)
				return &f;
		}
	return nullptr;
}

ClassFunction *Class::get_same_func(const string &_name, Function *ff) const {
	return get_func(_name, ff->literal_return_type, ff->literal_param_type);
}

ClassFunction *Class::get_default_constructor() const {
	return get_func(IDENTIFIER_FUNC_INIT, TypeVoid, {});
}

Array<ClassFunction*> Class::get_constructors() const {
	Array<ClassFunction*> c;
	for (auto &f: member_functions)
		if ((f.func->name == IDENTIFIER_FUNC_INIT) and (f.return_type == TypeVoid))
			c.add(&f);
	return c;
}

ClassFunction *Class::get_destructor() const {
	return get_func(IDENTIFIER_FUNC_DELETE, TypeVoid, {});
}

ClassFunction *Class::get_assign() const {
	return get_func(IDENTIFIER_FUNC_ASSIGN, TypeVoid, {this});
}

ClassFunction *Class::get_get(const Class *index) const {
	for (ClassFunction &cf: member_functions) {
		if (cf.func->name != "__get__")
			continue;
		if (cf.func->num_params != 1)
			continue;
		if (cf.func->literal_param_type[0] != index)
			continue;
		return &cf;
	}
	return nullptr;
}

ClassFunction *Class::get_virtual_function(int virtual_index) const {
	for (ClassFunction &f: member_functions)
		if (f.func->virtual_index == virtual_index)
			return &f;
	return nullptr;
}

void Class::link_virtual_table() {
	if (vtable.num == 0)
		return;

	//msg_write("link vtable " + name);
	// derive from parent
	if (parent)
		for (int i=0; i<parent->vtable.num; i++)
			vtable[i] = parent->vtable[i];
	if (config.abi == ABI_WINDOWS_32)
		vtable[0] = mf(&VirtualBase::__delete_external__);
	else
		vtable[1] = mf(&VirtualBase::__delete_external__);

	// link virtual functions into vtable
	for (ClassFunction &cf: member_functions) {
		if (cf.func->virtual_index >= 0) {
			//msg_write(i2s(cf.virtual_index) + ": " + cf.GetFunc()->name);
			if (cf.func->virtual_index >= vtable.num)
				owner->do_error("LinkVirtualTable");
				//vtable.resize(cf.virtual_index + 1);
			if (config.verbose)
				msg_write("VIRTUAL   " + i2s(cf.func->virtual_index) + "   " + cf.func->signature());
			vtable[cf.func->virtual_index] = cf.func->address;
		}
		if (cf.func->needs_overriding) {
			msg_error("needs overriding: " + cf.signature());
		}
	}
}

void Class::link_external_virtual_table(void *p) {
	// link script functions according to external vtable
	VirtualTable *t = (VirtualTable*)p;
	vtable.clear();
	int max_vindex = 1;
	for (ClassFunction &cf: member_functions)
		if (cf.func->virtual_index >= 0) {
			cf.func->address = t[cf.func->virtual_index];
			if (cf.func->virtual_index >= vtable.num)
				max_vindex = max(max_vindex, cf.func->virtual_index);
		}
	vtable.resize(max_vindex + 1);
	_vtable_location_compiler_ = vtable.data;
	_vtable_location_target_ = vtable.data;
	_vtable_location_external_ = (void*)t;

	for (int i=0; i<vtable.num; i++)
		vtable[i] = t[i];
	// this should also link the "real" c++ destructor
	if ((config.abi == ABI_WINDOWS_32) or (config.abi == ABI_WINDOWS_64))
		vtable[0] = mf(&VirtualBase::__delete_external__);
	else
		vtable[1] = mf(&VirtualBase::__delete_external__);
}


bool class_func_match(ClassFunction &a, ClassFunction &b) {
	if (a.func->name != b.func->name)
		return false;
	if (a.return_type != b.return_type)
		return false;
	if (a.func->num_params != b.func->num_params)
		return false;
	for (int i=0; i<a.func->num_params; i++)
		if (!type_match(b.func->literal_param_type[i], a.func->literal_param_type[i]))
			return false;
	return true;
}


const Class *Class::get_pointer() const {
	return owner->make_class(name + "*", Class::Type::POINTER, config.pointer_size, 0, this);
}

const Class *Class::get_root() const {
	const Class *r = this;
	while (r->parent)
		r = r->parent;
	return r;
}

void Class::add_function(SyntaxTree *s, Function *f, bool as_virtual, bool override) {
	if (config.verbose)
		msg_write("CLASS ADD   " + long_name() + "    " + f->signature());
	if (f->is_static) {
		if (config.verbose)
			msg_write("   STATIC");
		static_functions.add(f);
	} else {
		if (config.verbose)
			msg_write("   MEMBER");
		ClassFunction cf;
		cf.func = f;
		cf.return_type = f->return_type;
		if (as_virtual and (f->virtual_index < 0)) {
			if (config.verbose)
				msg_write("VVVVV +");
			f->virtual_index = process_class_offset(name, cf.func->name, max(vtable.num, 2));
		}

		// override?
		ClassFunction *orig = nullptr;
		for (ClassFunction &ocf: member_functions)
			if (class_func_match(cf, ocf))
				orig = &ocf;
		if (override and !orig)
			s->do_error(format("can not override function %s, no previous definition", f->signature().c_str()), f->_exp_no, f->_logical_line_no);
		if (!override and orig) {
			msg_write(f->signature());
			msg_write(orig->signature());
			s->do_error(format("function %s is already defined, use '%s'", f->signature().c_str(), IDENTIFIER_OVERRIDE.c_str()), f->_exp_no, f->_logical_line_no);
		}
		if (override) {
			if (config.verbose)
				msg_write("OVERRIDE    " + orig->func->signature());
			f->virtual_index = orig->func->virtual_index;
			if (orig->func->name_space == this)
				delete orig->func;
			orig->func = cf.func;
		} else {
			member_functions.add(cf);
		}

		if (f->virtual_index >= 0) {
			if (config.verbose) {
				msg_write("VVVVV");
				msg_write(f->virtual_index);
			}
			if (vtable.num <= f->virtual_index)
				vtable.resize(f->virtual_index + 1);
			_vtable_location_compiler_ = vtable.data;
			_vtable_location_target_ = vtable.data;
		}
	}
}

bool Class::derive_from(const Class* root, bool increase_size) {
	parent = const_cast<Class*>(root);
	bool found = false;
	if (parent->elements.num > 0) {
		// inheritance of elements
		elements = parent->elements;
		found = true;
	}
	if (parent->member_functions.num > 0) {
		// inheritance of functions
		for (ClassFunction &f: parent->member_functions) {
			if (f.func->name == IDENTIFIER_FUNC_ASSIGN)
				continue;
			ClassFunction ff = f;
			if ((f.func->name == IDENTIFIER_FUNC_INIT) or (f.func->name == IDENTIFIER_FUNC_DELETE)) {
				ff.func = f.func->create_dummy_clone(this);
			}
			if (config.verbose)
				msg_write("INHERIT   " + ff.signature());
			member_functions.add(ff);
		}
		found = true;
	}
	static_functions.append(parent->static_functions);

	if (increase_size)
		size += parent->size;
	vtable = parent->vtable;
	_vtable_location_compiler_ = vtable.data;
	_vtable_location_target_ = vtable.data;
	return found;
}

void *Class::create_instance() const {
	void *p = malloc(size);
	ClassFunction *c = get_default_constructor();
	if (c) {
		typedef void con_func(void *);
		con_func * f = (con_func*)c->func->address;
		if (f)
			f(p);
	}
	return p;
}

string Class::var2str(const void *p) const {
	if (this == TypeInt) {
		return i2s(*(int*)p);
	} else if (this == TypeFloat32) {
		return f2s(*(float*)p, 3);
	} else if (this == TypeFloat64) {
		return f2s((float)*(double*)p, 3);
	} else if (this == TypeBool) {
		return b2s(*(bool*)p);
	} else if (this == TypeClass) {
		return ((Class*)p)->name;
	} else if (this == TypeClassP) {
		if (p)
			return "&" + (*(Class**)p)->name;
	} else if (is_pointer()) {
		return p2s(*(void**)p);
	} else if (this == TypeString) {
		return "\"" + *(string*)p + "\"";
	} else if (this == TypeCString) {
		return "\"" + string((char*)p) + "\"";
	} else if (is_super_array()) {
		string s;
		DynamicArray *da = (DynamicArray*)p;
		for (int i=0; i<da->num; i++) {
			if (i > 0)
				s += ", ";
			s += parent->var2str(((char*)da->data) + i * da->element_size);
		}
		return "[" + s + "]";
	} else if (is_dict()) {
		return "{...}";
	} else if (elements.num > 0) {
		string s;
		for (auto &e: elements) {
			if (e.hidden)
				continue;
			if (s.num > 0)
				s += ", ";
			s += e.type->var2str(((char*)p) + e.offset);
		}
		return "(" + s + ")";

	} else if (is_array()) {
			string s;
			for (int i=0; i<array_length; i++) {
				if (i > 0)
					s += ", ";
				s += parent->var2str(((char*)p) + i * parent->size);
			}
			return "[" + s + "]";
		}
	return d2h(p, size, false);
}

}

