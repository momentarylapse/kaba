/*----------------------------------------------------------------------------*\
| Kaba Lib                                                                     |
| -> "standart library" for the scripting system                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2010.07.07 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include <algorithm>
#include <string.h>

#ifdef WIN32
	#include "windows.h"
#endif
#include "../kaba.h"
#include "common.h"
#include "exception.h"
#include "dict.h"
#include "../../config.h"
#include "../../math/complex.h"
#include "../../any/any.h"



#ifdef _X_USE_HUI_
#include "../../hui/hui.h"
#endif



namespace Kaba{

string LibVersion = "0.17.8.2";


const string IDENTIFIER_CLASS = "class";
const string IDENTIFIER_FUNC_INIT = "__init__";
const string IDENTIFIER_FUNC_DELETE = "__delete__";
const string IDENTIFIER_FUNC_ASSIGN = "__assign__";
const string IDENTIFIER_FUNC_GET = "__get__";
const string IDENTIFIER_FUNC_SUBARRAY = "__subarray__";
const string IDENTIFIER_SUPER = "super";
const string IDENTIFIER_SELF = "self";
const string IDENTIFIER_EXTENDS = "extends";
const string IDENTIFIER_STATIC = "static";
const string IDENTIFIER_NEW = "new";
const string IDENTIFIER_DELETE = "del";
const string IDENTIFIER_SIZEOF = "sizeof";
const string IDENTIFIER_TYPE = "type";
const string IDENTIFIER_STR = "str";
const string IDENTIFIER_REPR = "repr";
const string IDENTIFIER_LEN = "len";
const string IDENTIFIER_LET = "let";
const string IDENTIFIER_NAMESPACE = "namespace";
const string IDENTIFIER_RETURN_VAR = "-return-";
const string IDENTIFIER_VTABLE_VAR = "-vtable-";
const string IDENTIFIER_ENUM = "enum";
const string IDENTIFIER_CONST = "const";
const string IDENTIFIER_OVERRIDE = "override";
const string IDENTIFIER_VIRTUAL = "virtual";
const string IDENTIFIER_EXTERN = "extern";
const string IDENTIFIER_USE = "use";
const string IDENTIFIER_RETURN = "return";
const string IDENTIFIER_RAISE = "raise";
const string IDENTIFIER_TRY = "try";
const string IDENTIFIER_EXCEPT = "except";
const string IDENTIFIER_IF = "if";
const string IDENTIFIER_ELSE = "else";
const string IDENTIFIER_WHILE = "while";
const string IDENTIFIER_FOR = "for";
const string IDENTIFIER_IN = "in";
const string IDENTIFIER_BREAK = "break";
const string IDENTIFIER_CONTINUE = "continue";
const string IDENTIFIER_PASS = "pass";
const string IDENTIFIER_AND = "and";
const string IDENTIFIER_OR = "or";
const string IDENTIFIER_XOR = "xor";
const string IDENTIFIER_NOT = "not";
const string IDENTIFIER_IS = "is";
const string IDENTIFIER_ASM = "asm";
const string IDENTIFIER_MAP = "map";
const string IDENTIFIER_LAMBDA = "lambda";
const string IDENTIFIER_SORTED = "sorted";
const string IDENTIFIER_DYN = "dyn";

CompilerConfiguration config;

struct ExternalLinkData {
	string name;
	void *pointer;
};
Array<ExternalLinkData> ExternalLinks;

struct ClassOffsetData {
	string class_name, element;
	int offset;
	bool is_virtual;
};
Array<ClassOffsetData> ClassOffsets;

struct ClassSizeData {
	string class_name;
	int size;
};
Array<ClassSizeData> ClassSizes;


//------------------------------------------------------------------------------------------------//
//                                             types                                              //
//------------------------------------------------------------------------------------------------//

const Class *TypeUnknown;
const Class *TypeReg128;
const Class *TypeReg64;
const Class *TypeReg32;
const Class *TypeReg16;
const Class *TypeReg8;
const Class *TypeVoid;
const Class *TypePointer;
const Class *TypeChunk;
const Class *TypeBool;
const Class *TypeInt;
const Class *TypeInt64;
const Class *TypeFloat;
const Class *TypeFloat32;
const Class *TypeFloat64;
const Class *TypeChar;
const Class *TypeString;
const Class *TypeCString;

const Class *TypeVector;
const Class *TypeRect;
const Class *TypeColor;
const Class *TypeQuaternion;
const Class *TypeAny;
const Class *TypeAnyList;
const Class *TypeAnyDict;
 // internal:
const Class *TypeDynamicArray;
const Class *TypeDictBase;
const Class *TypePointerList;
const Class *TypeCharPs;
const Class *TypeBoolPs;
const Class *TypeBoolList;
const Class *TypeIntPs;
const Class *TypeIntList;
const Class *TypeIntArray;
const Class *TypeIntDict;
const Class *TypeFloatP;
const Class *TypeFloatPs;
const Class *TypeFloatList;
const Class *TypeFloatArray;
const Class *TypeFloatArrayP;
const Class *TypeFloatDict;
const Class *TypeComplex;
const Class *TypeComplexList;
const Class *TypeStringList;
const Class *TypeStringDict;
const Class *TypeVectorArray;
const Class *TypeVectorArrayP;
const Class *TypeVectorList;
const Class *TypeMatrix;
const Class *TypePlane;
const Class *TypePlaneList;
const Class *TypeMatrix3;
const Class *TypeDate;
const Class *TypeImage;

const Class *TypeException;
const Class *TypeExceptionP;

const Class *TypeClass;
const Class *TypeClassP;
const Class *TypeFunction;
const Class *TypeFunctionP;
const Class *TypeFunctionCode;
const Class *TypeFunctionCodeP;


Array<Script*> Packages;
Script *cur_package = nullptr;


static Function *cur_func = nullptr;
static Class *cur_class;


void add_package(const string &name, bool used_by_default) {
	Script* s = new Script;
	s->used_by_default = used_by_default;
	s->filename = name;
	Packages.add(s);
	cur_package = s;
}

void __add_class__(Class *t) {
	cur_package->syntax->base_class->classes.add(t);
	t->name_space = cur_package->syntax->base_class;
}

const Class *add_type(const string &name, int size, ScriptFlag flag) {
	Class *t = new Class(name, size, cur_package->syntax);
	if ((flag & FLAG_CALL_BY_VALUE) > 0)
		t->force_call_by_value = true;
	__add_class__(t);
	return t;
}

const Class *add_type_p(const string &name, const Class *sub_type, ScriptFlag flag) {
	Class *t = new Class(name, config.pointer_size, cur_package->syntax);
	t->type = Class::Type::POINTER;
	if ((flag & FLAG_SILENT) > 0)
		t->type = Class::Type::POINTER_SILENT;
	t->parent = sub_type;
	__add_class__(t);
	return t;
}
const Class *add_type_a(const string &name, const Class *sub_type, int array_length) {
	Class *t = new Class(name, 0, cur_package->syntax, sub_type);
	if (array_length < 0) {
		// super array
		t->size = config.super_array_size;
		t->type = Class::Type::SUPER_ARRAY;
		script_make_super_array(t);
	} else {
		// standard array
		t->size = sub_type->size * array_length;
		t->type = Class::Type::ARRAY;
		t->array_length = array_length;
	}
	__add_class__(t);
	return t;
}

const Class *add_type_d(const string &name, const Class *sub_type) {
	Class *t = new Class(name, config.super_array_size, cur_package->syntax, sub_type);
	t->type = Class::Type::DICT;
	script_make_dict(t);
	__add_class__(t);
	return t;
}

//------------------------------------------------------------------------------------------------//
//                                           operators                                            //
//------------------------------------------------------------------------------------------------//

//   without type information ("primitive")

PrimitiveOperator PrimitiveOperators[(int)OperatorID::_COUNT_] = {
	{"=",  OperatorID::ASSIGN,        true,  1, IDENTIFIER_FUNC_ASSIGN, 3, false},
	{"+",  OperatorID::ADD,           false, 11, "__add__", 3, false},
	{"-",  OperatorID::SUBTRACT,      false, 11, "__sub__", 3, false},
	{"*",  OperatorID::MULTIPLY,      false, 12, "__mul__", 3, false},
	{"/",  OperatorID::DIVIDE,        false, 12, "__div__", 3, false},
	{"-",  OperatorID::NEGATIVE,      false, 13, "__neg__", 2, false}, // -1 etc
	{"+=", OperatorID::ADDS,          true,  1,  "__iadd__", 3, false},
	{"-=", OperatorID::SUBTRACTS,     true,  1,  "__isub__", 3, false},
	{"*=", OperatorID::MULTIPLYS,     true,  1,  "__imul__", 3, false},
	{"/=", OperatorID::DIVIDES,       true,  1,  "__idiv__", 3, false},
	{"==", OperatorID::EQUAL,         false, 8,  "__eq__", 3, false},
	{"!=", OperatorID::NOTEQUAL,      false, 8,  "__ne__", 3, false},
	{"!",  OperatorID::NEGATE,        false, 2,  "__not__", 2, false},
	{"<",  OperatorID::SMALLER,       false, 9,  "__lt__", 3, false},
	{">",  OperatorID::GREATER,       false, 9,  "__gt__", 3, false},
	{"<=", OperatorID::SMALLER_EQUAL, false, 9,  "__le__", 3, false},
	{">=", OperatorID::GREATER_EQUAL, false, 9,  "__ge__", 3, false},
	{IDENTIFIER_AND, OperatorID::AND, false, 4,  "__and__", 3, false},
	{IDENTIFIER_OR,  OperatorID::OR,  false, 3,  "__or__", 3, false},
	{"%",  OperatorID::MODULO,        false, 12, "__mod__", 3, false},
	{"&",  OperatorID::BIT_AND,       false, 7, "__bitand__", 3, false},
	{"|",  OperatorID::BIT_OR,        false, 5, "__bitor__", 3, false},
	{"<<", OperatorID::SHIFT_LEFT,    false, 10, "__lshift__", 3, false},
	{">>", OperatorID::SHIFT_RIGHT,   false, 10, "__rshift__", 3, false},
	{"++", OperatorID::INCREASE,      true,  2, "__inc__", 1, false},
	{"--", OperatorID::DECREASE,      true,  2, "__dec__", 1, false},
	{IDENTIFIER_IS, OperatorID::IS,   false, 2,  "-none-", 3, false},
	{IDENTIFIER_IN, OperatorID::IN,   false, 12, "__contains__", 3, true}, // INVERTED
	{IDENTIFIER_EXTENDS, OperatorID::EXTENDS, false, 2,  "-none-", 3, false},
	{"^",  OperatorID::EXPONENT,      false, 14,  "__exp__", 3, false}
// Level = 15 - (official C-operator priority)
// priority from "C als erste Programmiersprache", page 552
};

//   with type information

void add_operator(OperatorID primitive_op, const Class *return_type, const Class *param_type1, const Class *param_type2, InlineID inline_index, void *func = nullptr) {
	Operator *o = new Operator;
	o->owner = cur_package->syntax;
	o->primitive = &PrimitiveOperators[(int)primitive_op];
	o->return_type = return_type;
	o->param_type_1 = param_type1;
	o->param_type_2 = param_type2;
	
	bool allow_member = true;
	if (!param_type1)// or ! param_type2)
		allow_member = false;
	if (param_type1)
		//if (!param_type1->uses_call_by_reference())
		if (o->primitive->left_modifiable and !param_type1->uses_call_by_reference())
			allow_member = false;
	allow_member = false;
	if (allow_member) {
		add_class(param_type1);
		o->f = class_add_func(o->primitive->function_name, return_type, func);
		if (param_type2)
			func_add_param("b", param_type2);
	} else {
		o->f = add_func(o->primitive->function_name, return_type, func);
		o->f->is_static = true;
		if (param_type1)
			func_add_param("a", param_type1);
		if (param_type2)
			func_add_param("b", param_type2);
	}
	o->f->is_pure = !o->primitive->left_modifiable;
	func_set_inline(inline_index);
	if (param_type1 == TypeInt) {
	//msg_write(o->sig());
	//msg_write("                          " + o->f->signature());
	}
	//if (param_type1)
	//if (!o->primitive->left_modifiable and !param_type1->uses_call_by_reference())
	//	msg_write(o->sig());
	cur_package->syntax->operators.add(o);
}


//------------------------------------------------------------------------------------------------//
//                                     classes & elements                                         //
//------------------------------------------------------------------------------------------------//


Class *add_class(const Class *root_type) {
	cur_class = const_cast<Class*>(root_type);
	return cur_class;
}

void class_add_element(const string &name, const Class *type, int offset, ScriptFlag flag) {
	auto e = ClassElement(name, type, offset);
	e.hidden = ((flag & FLAG_HIDDEN) > 0);
	cur_class->elements.add(e);
}

void class_derive_from(const Class *parent, bool increase_size, bool copy_vtable) {
	cur_class->derive_from(parent, increase_size);
	if (copy_vtable)
		cur_class->vtable = parent->vtable;
}

int _class_override_num_params = -1;

void _class_add_member_func(const Class *ccc, Function *f, ScriptFlag flag) {
	Class *c = const_cast<Class*>(ccc);
	if ((flag & FLAG_OVERRIDE) > 0) {
		foreachi(Function *ff, c->member_functions, i)
			if (ff->name == f->name) {
				if (_class_override_num_params < 0 or _class_override_num_params == ff->num_params) {
					//msg_write("OVERRIDE");
					c->member_functions[i] = f;
					return;
				}
			}
		msg_error("could not override " + c->name + "." + f->name);
	} else {
		// name alone is not enough for matching...
		/*foreachi(ClassFunction &ff, c->functions, i)
			if (ff.name == f.name) {
				if (_class_override_num_params < 0 or _class_override_num_params == ff.param_types.num) {
					msg_error("missing override " + c->name + "." + f.name);
					break;
				}
			}*/
		c->member_functions.add(f);
	}
}


Function* class_add_func(const string &name, const Class *return_type, void *func, ScriptFlag flag) {
	Function *f = new Function(name, return_type, cur_class);
	f->is_pure = ((flag & FLAG_PURE) > 0);
	f->throws_exceptions = ((flag & FLAG_RAISES_EXCEPTIONS) > 0);
	f->is_static = ((flag & FLAG_STATIC) > 0);
	cur_package->syntax->functions.add(f);
	f->address_preprocess = func;
	if (config.allow_std_lib)
		f->address = func;
	cur_func = f;


	if (f->is_static)
		cur_class->static_functions.add(f);
	else
		_class_add_member_func(cur_class, f, flag);
	return f;
}

int get_virtual_index(void *func, const string &tname, const string &name) {
	if (config.abi == Abi::WINDOWS_32) {
		if (!func)
			return 0;
		unsigned char *pp = (unsigned char*)func;
		try {
			//if ((cur_class->vtable) and (pp[0] == 0x8b) and (pp[1] == 0x01) and (pp[2] == 0xff) and (pp[3] == 0x60)){
			if ((pp[0] == 0x8b) and (pp[1] == 0x44) and (pp[2] == 0x24) and (pp[4] == 0x8b) and (pp[5] == 0x00) and (pp[6] == 0xff) and (pp[7] == 0x60)) {
				// 8b.44.24.**    8b.00     ff.60.10
				// virtual function
				return (int)pp[8] / 4;
			} else if (pp[0] == 0xe9) {
				// jmp
				//msg_write(Asm::Disassemble(func, 16));
				pp = &pp[5] + *(int*)&pp[1];
				//msg_write(Asm::Disassemble(pp, 16));
				if ((pp[0] == 0x8b) and (pp[1] == 0x44) and (pp[2] == 0x24) and (pp[4] == 0x8b) and (pp[5] == 0x00) and (pp[6] == 0xff) and (pp[7] == 0x60)) {
					// 8b.44.24.**    8b.00     ff.60.10
					// virtual function
					return (int)pp[8] / 4;
				} else {
					throw(1);
				}
			} else {
				throw(1);
			}
		} catch (...) {
			msg_error("Script class_add_func_virtual(" + tname + "." + name + "):  can't read virtual index");
			msg_write(string((char*)pp, 4).hex());
			msg_write(Asm::disassemble(func, 16));
		}
	} else {

		int_p p = (int_p)func;
		if ((p & 1) > 0) {
			// virtual function
			return p / sizeof(void*);
		} else if (!func) {
			return 0;
		} else {
			msg_error("Script class_add_func_virtual(" + tname + "." + name + "):  can't read virtual index");
		}
	}
	return -1;
}

Function* class_add_func_virtual(const string &name, const Class *return_type, void *func, ScriptFlag flag) {
	string tname = cur_class->name;
	int index = get_virtual_index(func, tname, name);
	//msg_write("virtual: " + tname + "." + name);
		//msg_write(index);
	Function *f = class_add_func(name, return_type, func, flag);
	cur_func->virtual_index = index;
	if (index >= cur_class->vtable.num)
		cur_class->vtable.resize(index + 1);
	cur_class->_vtable_location_compiler_ = cur_class->vtable.data;
	cur_class->_vtable_location_target_ = cur_class->vtable.data;
	return f;
}

void class_link_vtable(void *p) {
	cur_class->link_external_virtual_table(p);
}


//------------------------------------------------------------------------------------------------//
//                                           constants                                            //
//------------------------------------------------------------------------------------------------//

void class_add_const(const string &name, const Class *type, const void *value) {
	Constant *c = cur_package->syntax->add_constant(type, cur_class);
	c->name = name;

	// config.PointerSize might be smaller than needed for the following assignment
	if ((type == TypeInt) or (type == TypeFloat32) or (type == TypeChar)  or (type == TypeBool) or (type->is_pointer()))
		*(const void**)c->p() = value;
	else
		memcpy(c->p(), value, type->size);
}

void add_const(const string &name, const Class *type, const void *value) {
	cur_class = cur_package->syntax->base_class;
	class_add_const(name, type, value);
}

//------------------------------------------------------------------------------------------------//
//                                    environmental variables                                     //
//------------------------------------------------------------------------------------------------//


void add_ext_var(const string &name, const Class *type, void *var) {
	auto *v = new Variable(name, type);
	cur_package->syntax->base_class->static_variables.add(v);
	if (config.allow_std_lib)
		v->memory = var;
};

//------------------------------------------------------------------------------------------------//
//                                      compiler functions                                        //
//------------------------------------------------------------------------------------------------//



Array<Statement*> Statements;

Function *add_func(const string &name, const Class *return_type, void *func, ScriptFlag flag) {
	add_class(cur_package->base_class());
	return class_add_func(name, return_type, func, flag);
}

Statement *statement_from_id(StatementID id) {
	for (auto *s: Statements)
		if (s->id == id)
			return s;
	return nullptr;
}

int add_statement(const string &name, StatementID id, int num_params = 0) {
	Statement *s = new Statement;
	s->name = name;
	s->id = id;
	s->num_params = num_params;
	Statements.add(s);
	return 0;
}

void func_set_inline(InlineID index) {
	if (cur_func)
		cur_func->inline_no = index;
}

void func_add_param(const string &name, const Class *type) {
	if (cur_func) {
		Variable *v = new Variable(name, type);
		cur_func->var.add(v);
		cur_func->literal_param_type.add(type);
		cur_func->num_params ++;
	}
}

void script_make_super_array(Class *t, SyntaxTree *ps)
{
	const Class *parent = t->parent;
	t->derive_from(TypeDynamicArray, false);
	t->parent = parent;
	add_class(t);

	Function *sub = t->get_func(IDENTIFIER_FUNC_SUBARRAY, TypeDynamicArray, {nullptr,nullptr});
	sub->literal_return_type = t;
	sub->return_type = t;

	// FIXME  wrong for complicated classes
	if (t->parent->is_simple_class()){
		if (!t->parent->uses_call_by_reference()){
			if (t->parent->is_pointer()){
				class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, &Array<void*>::__init__);
				class_add_funcx("add", TypeVoid, &DynamicArray::append_p_single);
					func_add_param("x", t->parent);
				class_add_funcx("insert", TypeVoid, &DynamicArray::insert_p_single);
					func_add_param("x", t->parent);
					func_add_param("index", TypeInt);
			}else if (t->parent == TypeFloat32){
				class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, &Array<float>::__init__);
				class_add_funcx("add", TypeVoid, &DynamicArray::append_f_single);
					func_add_param("x", t->parent);
				class_add_funcx("insert", TypeVoid, &DynamicArray::insert_f_single);
					func_add_param("x", t->parent);
					func_add_param("index", TypeInt);
			}else if (t->parent == TypeFloat64){
				class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, &Array<double>::__init__);
				class_add_funcx("add", TypeVoid, &DynamicArray::append_d_single);
					func_add_param("x", t->parent);
				class_add_funcx("insert", TypeVoid, &DynamicArray::insert_d_single);
					func_add_param("x", t->parent);
					func_add_param("index", TypeInt);
			}else if (t->parent->size == 4){
				class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, &Array<int>::__init__);
				class_add_funcx("add", TypeVoid, &DynamicArray::append_4_single);
					func_add_param("x", t->parent);
				class_add_funcx("insert", TypeVoid, &DynamicArray::insert_4_single);
					func_add_param("x", t->parent);
					func_add_param("index", TypeInt);
			}else if (t->parent->size == 1){
				class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, &Array<char>::__init__);
				class_add_funcx("add", TypeVoid, &DynamicArray::append_1_single);
					func_add_param("x", t->parent);
				class_add_funcx("insert", TypeVoid, &DynamicArray::insert_1_single);
					func_add_param("x", t->parent);
					func_add_param("index", TypeInt);
			}else{
				msg_error("evil class:  " + t->name);
			}
		}else{
			// __init__ must be defined manually...!
			class_add_funcx("add", TypeVoid, &DynamicArray::append_single);
				func_add_param("x", t->parent);
			class_add_funcx("insert", TypeVoid, &DynamicArray::insert_single);
				func_add_param("x", t->parent);
				func_add_param("index", TypeInt);
		}
		class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, &DynamicArray::simple_clear);
		class_add_funcx("clear", TypeVoid, &DynamicArray::simple_clear);
		class_add_funcx(IDENTIFIER_FUNC_ASSIGN, TypeVoid, &DynamicArray::simple_assign);
			func_add_param("other", t);
		class_add_funcx("remove", TypeVoid, &DynamicArray::delete_single);
			func_add_param("index", TypeInt);
		class_add_funcx("resize", TypeVoid, &DynamicArray::simple_resize);
			func_add_param("num", TypeInt);
	}else if (t->parent == TypeString){
		// handled manually later...
	}else{
		msg_error("evil class:  " + t->name);
	}
}




Array<TypeCast> TypeCasts;
void add_type_cast(int penalty, const Class *source, const Class *dest, const string &cmd)
{
	TypeCast c;
	c.penalty = penalty;
	c.f = nullptr;
	for (auto *f: cur_package->syntax->base_class->static_functions)
		if (f->long_name() == cmd){
			c.f = f;
			break;
		}
	if (!c.f){
#ifdef _X_USE_HUI_
		hui::ErrorBox(nullptr, "", "add_type_cast (ScriptInit): " + string(cmd) + " not found");
		hui::RaiseError("add_type_cast (ScriptInit): " + string(cmd) + " not found");
#else
		msg_error("add_type_cast (ScriptInit): " + string(cmd) + " not found"));
		exit(1);
#endif
	}
	c.source = source;
	c.dest = dest;
	TypeCasts.add(c);
}


void SIAddStatements() {
	// statements
	add_statement(IDENTIFIER_RETURN, StatementID::RETURN); // return: ParamType will be defined by the parser!
	add_statement(IDENTIFIER_IF, StatementID::IF, 2); // [CMP, BLOCK]
	add_statement("-if/else-", StatementID::IF_ELSE, 3); // [CMP, BLOCK, ELSE-BLOCK]
	add_statement(IDENTIFIER_WHILE, StatementID::WHILE, 2); // [CMP, BLOCK]
	add_statement("-for-array-", StatementID::FOR_ARRAY, 4); // [VAR, INDEX, ARRAY, BLOCK]
	add_statement("-for-range-", StatementID::FOR_RANGE, 5); // [VAR, START, STOP, STEP, BLOCK]
	add_statement(IDENTIFIER_FOR, StatementID::FOR_DIGEST, 4); // [INIT, CMP, BLOCK, INC] internally like a while-loop... but a bit different...
	add_statement(IDENTIFIER_BREAK, StatementID::BREAK);
	add_statement(IDENTIFIER_CONTINUE, StatementID::CONTINUE);
	add_statement(IDENTIFIER_NEW, StatementID::NEW);
	add_statement(IDENTIFIER_DELETE, StatementID::DELETE, 1);
	add_statement(IDENTIFIER_SIZEOF, StatementID::SIZEOF, 1);
	add_statement(IDENTIFIER_TYPE, StatementID::TYPE, 1);
	add_statement(IDENTIFIER_STR, StatementID::STR, 1);
	add_statement(IDENTIFIER_REPR, StatementID::REPR, 1);
	add_statement(IDENTIFIER_LEN, StatementID::LEN, 1);
	add_statement(IDENTIFIER_LET, StatementID::LET);
	add_statement(IDENTIFIER_ASM, StatementID::ASM);
	//add_statement(IDENTIFIER_RAISE, StatementID::RAISE); NOPE, now it's a function!
	add_statement(IDENTIFIER_TRY, StatementID::TRY); // return: ParamType will be defined by the parser!
	add_statement(IDENTIFIER_EXCEPT, StatementID::EXCEPT); // return: ParamType will be defined by the parser!
	add_statement(IDENTIFIER_PASS, StatementID::PASS);
	add_statement(IDENTIFIER_MAP, StatementID::MAP);
	add_statement(IDENTIFIER_LAMBDA, StatementID::LAMBDA);
	add_statement(IDENTIFIER_SORTED, StatementID::SORTED);
	add_statement(IDENTIFIER_DYN, StatementID::DYN);
}

#define MAKE_OP_FOR(T) \
	T op_##T##_add(T a, T b) { return a + b; } \
	T op_##T##_sub(T a, T b) { return a - b; } \
	T op_##T##_mul(T a, T b) { return a * b; } \
	T op_##T##_div(T a, T b) { return a / b; } \
	T op_##T##_neg(T a) { return - a; } \
	bool op_##T##_eq(T a, T b) { return a == b; } \
	bool op_##T##_neq(T a, T b) { return a != b; } \
	bool op_##T##_l(T a, T b) { return a < b; } \
	bool op_##T##_le(T a, T b) { return a <= b; } \
	bool op_##T##_g(T a, T b) { return a > b; } \
	bool op_##T##_ge(T a, T b) { return a >= b; }

MAKE_OP_FOR(int)
MAKE_OP_FOR(float)
MAKE_OP_FOR(int64)
MAKE_OP_FOR(double)

int op_int_mod(int a, int b) { return a % b; }
int op_int_shr(int a, int b) { return a >> b; }
int op_int_shl(int a, int b) { return a << b; }
int64 op_int64_mod(int64 a, int64 b) { return a % b; }
int64 op_int64_shr(int64 a, int64 b) { return a >> b; }
int64 op_int64_shl(int64 a, int64 b) { return a << b; }
int64 op_int64_add_int(int64 a, int b) { return a + b; }

complex op_complex_add(complex &a, complex &b) { return a + b; }
complex op_complex_sub(complex &a, complex &b) { return a - b; }
complex op_complex_mul(complex &a, complex &b) { return a * b; }
complex op_complex_div(complex &a, complex &b) { return a / b; }



void SIAddOperators() {
	add_operator(OperatorID::ASSIGN, TypeVoid, TypePointer, TypePointer, InlineID::POINTER_ASSIGN);
	add_operator(OperatorID::EQUAL, 	TypeBool, TypePointer, TypePointer, InlineID::POINTER_EQUAL);
	add_operator(OperatorID::NOTEQUAL, TypeBool, TypePointer, TypePointer, InlineID::POINTER_NOT_EQUAL);
	add_operator(OperatorID::ASSIGN, TypeVoid, TypeChar, TypeChar, InlineID::CHAR_ASSIGN);
	add_operator(OperatorID::EQUAL, 	TypeBool, TypeChar, TypeChar, InlineID::CHAR_EQUAL);
	add_operator(OperatorID::NOTEQUAL, TypeBool, TypeChar, TypeChar, InlineID::CHAR_NOT_EQUAL);
	add_operator(OperatorID::GREATER, TypeBool, TypeChar, TypeChar, InlineID::CHAR_GREATER);
	add_operator(OperatorID::GREATER_EQUAL, TypeBool, TypeChar, TypeChar, InlineID::CHAR_GREATER_EQUAL);
	add_operator(OperatorID::SMALLER, TypeBool, TypeChar, TypeChar, InlineID::CHAR_SMALLER);
	add_operator(OperatorID::SMALLER_EQUAL, TypeBool, TypeChar, TypeChar, InlineID::CHAR_SMALLER_EQUAL);
	add_operator(OperatorID::ADD, 	TypeChar, TypeChar, TypeChar, InlineID::CHAR_ADD);
	add_operator(OperatorID::SUBTRACTS, TypeChar, TypeChar, TypeChar, InlineID::CHAR_SUBTRACT_ASSIGN);
	add_operator(OperatorID::ADDS, 	TypeChar, TypeChar, TypeChar, InlineID::CHAR_ADD_ASSIGN);
	add_operator(OperatorID::SUBTRACT, TypeChar, TypeChar, TypeChar, InlineID::CHAR_SUBTRACT);
	add_operator(OperatorID::BIT_AND, TypeChar, TypeChar, TypeChar, InlineID::CHAR_AND);
	add_operator(OperatorID::BIT_OR, 	TypeChar, TypeChar, TypeChar, InlineID::CHAR_OR);
	add_operator(OperatorID::SUBTRACT, TypeChar, nullptr, TypeChar, InlineID::CHAR_NEGATE);
	add_operator(OperatorID::ASSIGN, TypeVoid, TypeBool, TypeBool, InlineID::BOOL_ASSIGN);
	add_operator(OperatorID::EQUAL, 	TypeBool, TypeBool, TypeBool, InlineID::BOOL_EQUAL);
	add_operator(OperatorID::NOTEQUAL, TypeBool, TypeBool, TypeBool, InlineID::BOOL_NOT_EQUAL);
	add_operator(OperatorID::AND, 	TypeBool, TypeBool, TypeBool, InlineID::BOOL_AND);
	add_operator(OperatorID::OR, 	TypeBool, TypeBool, TypeBool, InlineID::BOOL_OR);
	add_operator(OperatorID::NEGATE, TypeBool, nullptr, TypeBool, InlineID::BOOL_NEGATE);
	add_operator(OperatorID::ASSIGN, TypeVoid, TypeInt, TypeInt, InlineID::INT_ASSIGN);
	add_operator(OperatorID::ADD, 	TypeInt, TypeInt, TypeInt, InlineID::INT_ADD, (void*)op_int_add);
	add_operator(OperatorID::SUBTRACT, TypeInt, TypeInt, TypeInt, InlineID::INT_SUBTRACT, (void*)op_int_sub);
	add_operator(OperatorID::MULTIPLY, TypeInt, TypeInt, TypeInt, InlineID::INT_MULTIPLY, (void*)op_int_mul);
	add_operator(OperatorID::DIVIDE, TypeInt, TypeInt, TypeInt, InlineID::INT_DIVIDE, (void*)op_int_div);
	add_operator(OperatorID::ADDS, 	TypeVoid, TypeInt, TypeInt, InlineID::INT_ADD_ASSIGN);
	add_operator(OperatorID::SUBTRACTS, TypeVoid, TypeInt, TypeInt, InlineID::INT_SUBTRACT_ASSIGN);
	add_operator(OperatorID::MULTIPLYS, TypeVoid, TypeInt, TypeInt, InlineID::INT_MULTIPLY_ASSIGN);
	add_operator(OperatorID::DIVIDES, TypeVoid, TypeInt, TypeInt, InlineID::INT_DIVIDE_ASSIGN);
	add_operator(OperatorID::MODULO, TypeInt, TypeInt, TypeInt, InlineID::INT_MODULO, (void*)op_int_mod);
	add_operator(OperatorID::EQUAL, 	TypeBool, TypeInt, TypeInt, InlineID::INT_EQUAL, (void*)op_int_eq);
	add_operator(OperatorID::NOTEQUAL, TypeBool, TypeInt, TypeInt, InlineID::INT_NOT_EQUAL, (void*)op_int_neq);
	add_operator(OperatorID::GREATER, TypeBool, TypeInt, TypeInt, InlineID::INT_GREATER, (void*)op_int_g);
	add_operator(OperatorID::GREATER_EQUAL, TypeBool, TypeInt, TypeInt, InlineID::INT_GREATER_EQUAL, (void*)op_int_ge);
	add_operator(OperatorID::SMALLER, TypeBool, TypeInt, TypeInt, InlineID::INT_SMALLER, (void*)op_int_l);
	add_operator(OperatorID::SMALLER_EQUAL, TypeBool, TypeInt, TypeInt, InlineID::INT_SMALLER_EQUAL, (void*)op_int_le);
	add_operator(OperatorID::BIT_AND, TypeInt, TypeInt, TypeInt, InlineID::INT_AND);
	add_operator(OperatorID::BIT_OR, 	TypeInt, TypeInt, TypeInt, InlineID::INT_OR);
	add_operator(OperatorID::SHIFT_RIGHT, TypeInt, TypeInt, TypeInt, InlineID::INT_SHIFT_RIGHT, (void*)op_int_shr);
	add_operator(OperatorID::SHIFT_LEFT, TypeInt, TypeInt, TypeInt, InlineID::INT_SHIFT_LEFT, (void*)op_int_shl);
	add_operator(OperatorID::NEGATIVE, TypeInt, nullptr, TypeInt, InlineID::INT_NEGATE, (void*)op_int_neg);
	add_operator(OperatorID::INCREASE, TypeVoid, TypeInt, nullptr, InlineID::INT_INCREASE);
	add_operator(OperatorID::DECREASE, TypeVoid, TypeInt, nullptr, InlineID::INT_DECREASE);
	add_operator(OperatorID::ASSIGN, TypeVoid, TypeInt64, TypeInt64, InlineID::INT64_ASSIGN);
	add_operator(OperatorID::ADD, 	TypeInt64, TypeInt64, TypeInt64, InlineID::INT64_ADD, (void*)op_int64_add);
	add_operator(OperatorID::ADD, 	TypeInt64, TypeInt64, TypeInt, InlineID::INT64_ADD_INT, (void*)op_int64_add_int);
	add_operator(OperatorID::SUBTRACT, TypeInt64, TypeInt64, TypeInt64, InlineID::INT64_SUBTRACT, (void*)op_int64_sub);
	add_operator(OperatorID::MULTIPLY, TypeInt64, TypeInt64, TypeInt64, InlineID::INT64_MULTIPLY, (void*)op_int64_mul);
	add_operator(OperatorID::DIVIDE, TypeInt64, TypeInt64, TypeInt64, InlineID::INT64_DIVIDE, (void*)op_int64_div);
	add_operator(OperatorID::ADDS, 	TypeVoid, TypeInt64, TypeInt64, InlineID::INT64_ADD_ASSIGN);
	add_operator(OperatorID::SUBTRACTS, TypeVoid, TypeInt64, TypeInt64, InlineID::INT64_SUBTRACT_ASSIGN);
	add_operator(OperatorID::MULTIPLYS, TypeVoid, TypeInt64, TypeInt64, InlineID::INT64_MULTIPLY_ASSIGN);
	add_operator(OperatorID::DIVIDES, TypeVoid, TypeInt64, TypeInt64, InlineID::INT64_DIVIDE_ASSIGN);
	add_operator(OperatorID::MODULO, TypeInt64, TypeInt64, TypeInt64, InlineID::INT64_MODULO, (void*)op_int64_mod);
	add_operator(OperatorID::EQUAL, 	TypeBool, TypeInt64, TypeInt64, InlineID::INT64_EQUAL, (void*)op_int64_eq);
	add_operator(OperatorID::NOTEQUAL, TypeBool, TypeInt64, TypeInt64, InlineID::INT64_NOT_EQUAL, (void*)op_int64_neq);
	add_operator(OperatorID::GREATER, TypeBool, TypeInt64, TypeInt64, InlineID::INT64_GREATER, (void*)op_int64_g);
	add_operator(OperatorID::GREATER_EQUAL, TypeBool, TypeInt64, TypeInt64, InlineID::INT64_GREATER_EQUAL, (void*)op_int64_ge);
	add_operator(OperatorID::SMALLER, TypeBool, TypeInt64, TypeInt64, InlineID::INT64_SMALLER, (void*)op_int64_l);
	add_operator(OperatorID::SMALLER_EQUAL, TypeBool, TypeInt64, TypeInt64, InlineID::INT64_SMALLER_EQUAL, (void*)op_int64_le);
	add_operator(OperatorID::BIT_AND, TypeInt64, TypeInt64, TypeInt64, InlineID::INT64_AND);
	add_operator(OperatorID::BIT_OR, 	TypeInt64, TypeInt64, TypeInt64, InlineID::INT64_OR);
	add_operator(OperatorID::SHIFT_RIGHT, TypeInt64, TypeInt64, TypeInt64, InlineID::INT64_SHIFT_RIGHT, (void*)op_int64_shr);
	add_operator(OperatorID::SHIFT_LEFT, TypeInt64, TypeInt64, TypeInt64, InlineID::INT64_SHIFT_LEFT, (void*)op_int64_shl);
	add_operator(OperatorID::NEGATIVE, TypeInt64, nullptr, TypeInt64, InlineID::INT64_NEGATE, (void*)op_int64_neg);
	add_operator(OperatorID::INCREASE, TypeVoid, TypeInt64, nullptr, InlineID::INT64_INCREASE);
	add_operator(OperatorID::DECREASE, TypeVoid, TypeInt64, nullptr, InlineID::INT64_DECREASE);
	add_operator(OperatorID::ASSIGN, TypeVoid, TypeFloat32, TypeFloat32, InlineID::FLOAT_ASSIGN);
	add_operator(OperatorID::ADD, 	TypeFloat32, TypeFloat32, TypeFloat32, InlineID::FLOAT_ADD, (void*)op_float_add);
	add_operator(OperatorID::SUBTRACT, TypeFloat32, TypeFloat32, TypeFloat32, InlineID::FLOAT_SUBTARCT, (void*)op_float_sub);
	add_operator(OperatorID::MULTIPLY, TypeFloat32, TypeFloat32, TypeFloat32, InlineID::FLOAT_MULTIPLY, (void*)op_float_mul);
	add_operator(OperatorID::DIVIDE, TypeFloat32, TypeFloat32, TypeFloat32, InlineID::FLOAT_DIVIDE, (void*)op_float_div);
	add_operator(OperatorID::ADDS, 	TypeVoid, TypeFloat32, TypeFloat32, InlineID::FLOAT_ADD_ASSIGN);
	add_operator(OperatorID::SUBTRACTS, TypeVoid, TypeFloat32, TypeFloat32, InlineID::FLOAT_SUBTRACT_ASSIGN);
	add_operator(OperatorID::MULTIPLYS, TypeVoid, TypeFloat32, TypeFloat32, InlineID::FLOAT_MULTIPLY_ASSIGN);
	add_operator(OperatorID::DIVIDES, TypeVoid, TypeFloat32, TypeFloat32, InlineID::FLOAT_DIVIDE_ASSIGN);
	add_operator(OperatorID::EQUAL, 	TypeBool, TypeFloat32, TypeFloat32, InlineID::FLOAT_EQUAL, (void*)op_float_eq);
	add_operator(OperatorID::NOTEQUAL, TypeBool, TypeFloat32, TypeFloat32, InlineID::FLOAT_NOT_EQUAL, (void*)op_float_neq);
	add_operator(OperatorID::GREATER, TypeBool, TypeFloat32, TypeFloat32, InlineID::FLOAT_GREATER, (void*)op_float_g);
	add_operator(OperatorID::GREATER_EQUAL, TypeBool, TypeFloat32, TypeFloat32, InlineID::FLOAT_GREATER_EQUAL, (void*)op_float_ge);
	add_operator(OperatorID::SMALLER, TypeBool, TypeFloat32, TypeFloat32, InlineID::FLOAT_SMALLER, (void*)op_float_l);
	add_operator(OperatorID::SMALLER_EQUAL, TypeBool, TypeFloat32, TypeFloat32, InlineID::FLOAT_SMALLER_EQUAL, (void*)op_float_le);
	add_operator(OperatorID::NEGATIVE, TypeFloat32, nullptr, TypeFloat32, InlineID::FLOAT_NEGATE, (void*)op_float_neg);
	add_operator(OperatorID::ASSIGN, TypeVoid, TypeFloat64, TypeFloat64, InlineID::FLOAT64_ASSIGN);
	add_operator(OperatorID::ADD, 	TypeFloat64, TypeFloat64, TypeFloat64, InlineID::FLOAT64_ADD, (void*)op_double_add);
	add_operator(OperatorID::SUBTRACT, TypeFloat64, TypeFloat64, TypeFloat64, InlineID::FLOAT64_SUBTRACT, (void*)op_double_sub);
	add_operator(OperatorID::MULTIPLY, TypeFloat64, TypeFloat64, TypeFloat64, InlineID::FLOAT64_MULTIPLY, (void*)op_double_mul);
	add_operator(OperatorID::DIVIDE, TypeFloat64, TypeFloat64, TypeFloat64, InlineID::FLOAT64_DIVIDE, (void*)op_double_div);
	add_operator(OperatorID::ADDS, 	TypeVoid, TypeFloat64, TypeFloat64, InlineID::FLOAT64_ADD_ASSIGN);
	add_operator(OperatorID::SUBTRACTS, TypeVoid, TypeFloat64, TypeFloat64, InlineID::FLOAT64_SUBTRACT_ASSIGN);
	add_operator(OperatorID::MULTIPLYS, TypeVoid, TypeFloat64, TypeFloat64, InlineID::FLOAT64_MULTIPLY_ASSIGN);
	add_operator(OperatorID::DIVIDES, TypeVoid, TypeFloat64, TypeFloat64, InlineID::FLOAT64_DIVIDE_ASSIGN);
	add_operator(OperatorID::EQUAL, 	TypeBool, TypeFloat64, TypeFloat64, InlineID::FLOAT64_EQUAL, (void*)op_double_eq);
	add_operator(OperatorID::NOTEQUAL, TypeBool, TypeFloat64, TypeFloat64, InlineID::FLOAT64_NOT_EQUAL, (void*)op_double_neq);
	add_operator(OperatorID::GREATER, TypeBool, TypeFloat64, TypeFloat64, InlineID::FLOAT64_GREATER, (void*)op_double_g);
	add_operator(OperatorID::GREATER_EQUAL, TypeBool, TypeFloat64, TypeFloat64, InlineID::FLOAT64_GREATER_EQUAL, (void*)op_double_ge);
	add_operator(OperatorID::SMALLER, TypeBool, TypeFloat64, TypeFloat64, InlineID::FLOAT64_SMALLER, (void*)op_double_l);
	add_operator(OperatorID::SMALLER_EQUAL, TypeBool, TypeFloat64, TypeFloat64, InlineID::FLOAT64_SMALLER_EQUAL, (void*)op_double_le);
	add_operator(OperatorID::NEGATIVE, TypeFloat32, nullptr, TypeFloat64, InlineID::FLOAT64_NEGATE, (void*)op_double_neg);
//	add_operator(OperatorID::ASSIGN, TypeVoid, TypeComplex, TypeComplex, InlineID::COMPLEX_ASSIGN);
	add_operator(OperatorID::ADD, 	TypeComplex, TypeComplex, TypeComplex, InlineID::COMPLEX_ADD, (void*)op_complex_add);
	add_operator(OperatorID::SUBTRACT, TypeComplex, TypeComplex, TypeComplex, InlineID::COMPLEX_SUBTRACT, (void*)op_complex_sub);
	add_operator(OperatorID::MULTIPLY, TypeComplex, TypeComplex, TypeComplex, InlineID::COMPLEX_MULTIPLY, (void*)op_complex_mul);
	add_operator(OperatorID::MULTIPLY, TypeComplex, TypeFloat32, TypeComplex, InlineID::COMPLEX_MULTIPLY_FC);
	add_operator(OperatorID::MULTIPLY, TypeComplex, TypeComplex, TypeFloat32, InlineID::COMPLEX_MULTIPLY_CF);
	add_operator(OperatorID::DIVIDE, TypeComplex, TypeComplex, TypeComplex, InlineID::NONE /*InlineID::COMPLEX_DIVIDE*/, (void*)op_complex_div);
	add_operator(OperatorID::ADDS, 	TypeVoid, TypeComplex, TypeComplex, InlineID::COMPLEX_ADD_ASSIGN);
	add_operator(OperatorID::SUBTRACTS, TypeVoid, TypeComplex, TypeComplex, InlineID::COMPLEX_SUBTARCT_ASSIGN);
	add_operator(OperatorID::MULTIPLYS, TypeVoid, TypeComplex, TypeComplex, InlineID::COMPLEX_MULTIPLY_ASSIGN);
	add_operator(OperatorID::DIVIDES, TypeVoid, TypeComplex, TypeComplex, InlineID::COMPLEX_DIVIDE_ASSIGN);
	add_operator(OperatorID::EQUAL, 	TypeBool, TypeComplex, TypeComplex, InlineID::COMPLEX_EQUAL);
	add_operator(OperatorID::NEGATIVE, TypeComplex, nullptr, TypeComplex, InlineID::COMPLEX_NEGATE);
	add_operator(OperatorID::ASSIGN, TypeVoid, TypeChunk, TypeChunk, InlineID::CHUNK_ASSIGN);
	add_operator(OperatorID::EQUAL, 	TypeBool, TypeChunk, TypeChunk, InlineID::CHUNK_EQUAL);
	add_operator(OperatorID::NOTEQUAL, TypeBool, TypeChunk, TypeChunk, InlineID::CHUNK_NOT_EQUAL);
	add_operator(OperatorID::ADD, 	TypeVector, TypeVector, TypeVector, InlineID::VECTOR_ADD);
	add_operator(OperatorID::SUBTRACT, TypeVector, TypeVector, TypeVector, InlineID::VECTOR_SUBTRACT);
	add_operator(OperatorID::MULTIPLY, TypeFloat32, TypeVector, TypeVector, InlineID::VECTOR_MULTIPLY_VV);
	add_operator(OperatorID::MULTIPLY, TypeVector, TypeVector, TypeFloat32, InlineID::VECTOR_MULTIPLY_VF);
	add_operator(OperatorID::MULTIPLY, TypeVector, TypeFloat32, TypeVector, InlineID::VECTOR_MULTIPLY_FV);
	add_operator(OperatorID::DIVIDE, TypeVector, TypeVector, TypeFloat32, InlineID::VECTOR_DIVIDE_VF);
	add_operator(OperatorID::ADDS, 	TypeVoid, TypeVector, TypeVector, InlineID::VECTOR_ADD_ASSIGN);
	add_operator(OperatorID::SUBTRACTS, TypeVoid, TypeVector, TypeVector, InlineID::VECTOR_SUBTARCT_ASSIGN);
	add_operator(OperatorID::MULTIPLYS, TypeVoid, TypeVector, TypeFloat32, InlineID::VECTOR_MULTIPLY_ASSIGN);
	add_operator(OperatorID::DIVIDES, TypeVoid, TypeVector, TypeFloat32, InlineID::VECTOR_DIVIDE_ASSIGN);
	add_operator(OperatorID::NEGATIVE, TypeVector, nullptr, TypeVector, InlineID::VECTOR_NEGATE);
}

void SIAddXCommands();
void SIAddPackageBase();
void SIAddPackageKaba();
void SIAddPackageFile();
void SIAddPackageMath();
void SIAddPackageThread();
void SIAddPackageHui();
void SIAddPackageNix();
void SIAddPackageNet();
void SIAddPackageImage();
void SIAddPackageSound();
void SIAddPackageX();

void Init(Asm::InstructionSet instruction_set, Abi abi, bool allow_std_lib) {
	Asm::init(instruction_set);
	config.instruction_set = Asm::instruction_set.set;
	if (abi == Abi::NATIVE){
		if (config.instruction_set == Asm::InstructionSet::AMD64){
			abi = Abi::GNU_64;
#ifdef OS_WINDOWS
			abi = Abi::WINDOWS_64;
#endif
		}else if (config.instruction_set == Asm::InstructionSet::X86){
			abi = Abi::GNU_32;
#ifdef OS_WINDOWS
			abi = ABI_WINDOWS_32;
#endif
		}else if (config.instruction_set == Asm::InstructionSet::ARM){
			abi = Abi::GNU_ARM_32;
		}
	}
	config.abi = abi;
	config.allow_std_lib = allow_std_lib;
	config.pointer_size = Asm::instruction_set.pointer_size;
	if ((abi != Abi::NATIVE) or (instruction_set != Asm::InstructionSet::NATIVE))
		config.super_array_size = mem_align(config.pointer_size + 3 * sizeof(int), config.pointer_size);
	else
		config.super_array_size = sizeof(DynamicArray);
	config.stack_size = DEFAULT_STACK_SIZE;

	config.allow_simplification = true;
	config.allow_registers = true;
	config.stack_mem_align = 8;
	config.function_align = 2 * config.pointer_size;
	config.stack_frame_align = 2 * config.pointer_size;

	config.compile_silently = false;
	config.verbose = false;
	config.verbose_func_filter = "*";
	config.verbose_stage_filter = "*";
	config.show_compiler_stats = true;

	config.compile_os = false;
	config.override_variables_offset = false;
	config.variables_offset = 0;
	config.override_code_origin = false;
	config.code_origin = 0;
	config.add_entry_point = false;
	config.no_function_frame = false;


	SIAddStatements();

	SIAddPackageBase();
	SIAddPackageKaba();
	SIAddPackageMath();
	SIAddPackageFile();
	SIAddPackageImage();
	SIAddPackageHui();
	SIAddPackageNix();
	SIAddPackageNet();
	SIAddPackageSound();
	SIAddPackageThread();
	SIAddPackageX();

	cur_package = Packages[0];
	SIAddXCommands();
	SIAddOperators();




	add_type_cast(10, TypeInt, TypeFloat32, "i2f");
	add_type_cast(10, TypeInt, TypeInt64, "i2i64");
	add_type_cast(15, TypeInt64, TypeInt, "i642i");
	add_type_cast(10, TypeFloat32, TypeFloat64,"f2f64");
	add_type_cast(20, TypeFloat32, TypeInt, "f2i");
	add_type_cast(10, TypeInt, TypeChar, "i2c");
	add_type_cast(20, TypeChar, TypeInt, "c2i");
	add_type_cast(50, TypePointer, TypeBool, "p2b");
	add_type_cast(50, TypePointer, TypeString, "p2s");
	cur_package = Packages[2];
	add_type_cast(50, TypeInt, TypeAny, "@int2any");
	add_type_cast(50, TypeFloat32, TypeAny, "@float2any");
	add_type_cast(50, TypeBool, TypeAny, "@bool2any");
	add_type_cast(50, TypeString, TypeAny, "@str2any");
	add_type_cast(50, TypePointer, TypeAny, "@pointer2any");


	// consistency checks
#ifndef NDEBUG
	for (auto *p: Packages)
		for (auto *c: p->classes()) {
			if (c->is_super_array()) {
				if (!c->get_default_constructor() or !c->get_assign() or !c->get_destructor())
					msg_error("SUPER ARRAY INCONSISTENT: " + c->name);
			}
			// x package failing
			/*for (auto *f: c->member_functions)
				if (f->needs_overriding and (f->name != IDENTIFIER_FUNC_SUBARRAY))
					msg_error(f->signature());*/
		}
#endif
}

void ResetExternalLinkData()
{
	ExternalLinks.clear();
	ClassOffsets.clear();
	ClassSizes.clear();
}

// program variables - specific to the surrounding program, can't always be there...
void LinkExternal(const string &name, void *pointer)
{
	ExternalLinkData l;
	l.name = name;
	l.pointer = pointer;
	ExternalLinks.add(l);

	Array<string> names = name.explode(":");
	string sname = names[0].replace("@list", "[]").replace("@@", ".");
	for (auto *p: Packages)
		foreachi(Function *f, p->syntax->functions, i)
			if (f->long_name() == sname){
				if (names.num > 0)
					if (f->num_params != names[1]._int())
						continue;
				f->address = pointer;
			}
}

void *GetExternalLink(const string &name)
{
	for (ExternalLinkData &l: ExternalLinks)
		if (l.name == name)
			return l.pointer;
	return nullptr;
}

void DeclareClassSize(const string &class_name, int size)
{
	ClassSizeData d;
	d.class_name = class_name;
	d.size = size;
	ClassSizes.add(d);
}

void DeclareClassOffset(const string &class_name, const string &element, int offset)
{
	ClassOffsetData d;
	d.class_name = class_name;
	d.element = element;
	d.offset = offset;
	d.is_virtual = false;
	ClassOffsets.add(d);
}

void DeclareClassVirtualIndex(const string &class_name, const string &func, void *p, void *instance)
{
#ifdef OS_WINDOWS
	return;
#endif
	VirtualTable *v = *(VirtualTable**)instance;

	ClassOffsetData d;
	d.class_name = class_name;
	d.element = func;
	d.offset = get_virtual_index(p, class_name, func);
	d.is_virtual = true;
	ClassOffsets.add(d);

	LinkExternal(class_name + "." + func, v[d.offset]);
}

int process_class_offset(const string &class_name, const string &element, int offset)
{
	for (ClassOffsetData &d: ClassOffsets)
		if ((d.class_name == class_name) and (d.element == element))
			return d.offset;
	return offset;
}
int ProcessClassSize(const string &class_name, int size)
{
	for (ClassSizeData &d: ClassSizes)
		if (d.class_name == class_name)
			return d.size;
	return size;
}

int ProcessClassNumVirtuals(const string &class_name, int num_virtual)
{
	for (ClassOffsetData &d: ClassOffsets)
		if ((d.class_name == class_name) and (d.is_virtual))
			num_virtual = max(num_virtual, d.offset + 1);
	return num_virtual;
}

void End()
{
	DeleteAllScripts(true, true);

	Packages.clear();

	ResetExternalLinkData();
}


bool CompilerConfiguration::allow_output_func(const Function *f)
{
	if (!verbose)
		return false;
	if (!f)
		return true;
	Array<string> filters = verbose_func_filter.explode(",");
	for (auto &fil: filters)
		if (f->long_name().match(fil))
			return true;
	return false;
}

bool CompilerConfiguration::allow_output_stage(const string &stage)
{
	if (!verbose)
		return false;
	Array<string> filters = verbose_stage_filter.explode(",");
	for (auto &fil: filters)
		if (stage.match(fil))
			return true;
	return false;
}

bool CompilerConfiguration::allow_output(const Function *f, const string &stage)
{
	if (!verbose)
		return false;
	if (!allow_output_func(f))
		return false;
	if (!allow_output_stage(stage))
		return false;
	return true;
}

};
