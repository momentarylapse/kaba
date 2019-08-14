
#ifndef CLASS_H_
#define CLASS_H_


namespace Kaba{

class Script;
class SyntaxTree;
class Class;
class Function;
class Constant;
class Variable;


class ClassElement {
public:
	string name;
	const Class *type;
	long long offset;
	bool hidden;
	ClassElement();
	ClassElement(const string &name, const Class *type, int offset);
	string signature(bool include_class) const;
};

typedef void *VirtualTable;

class Class {
public:
	//Class();
	Class(const string &name, int size, SyntaxTree *owner, const Class *parent = nullptr);
	~Class();
	string name;
	string long_name() const;
	long long size; // complete size of type
	int array_length;

	enum class Type {
		OTHER,
		ARRAY,
		SUPER_ARRAY,
		POINTER,
		POINTER_SILENT, // pointer silent (&)
		DICT,
	};
	Type type;

	bool is_array() const;
	bool is_super_array() const;
	bool is_dict() const;
	bool is_pointer() const;
	bool is_pointer_silent() const;
	bool fully_parsed;
	Array<ClassElement> elements;
	Array<Function*> member_functions;
	Array<Function*> static_functions;
	Array<Variable*> static_variables;
	Array<Constant*> constants;
	Array<const Class*> classes;
	const Class *parent;
	const Class *name_space;
	SyntaxTree *owner; // to share and be able to delete...
	int _logical_line_no;
	int _exp_no;
	bool _amd64_allow_pass_in_xmm;
	Array<void*> vtable;
	void *_vtable_location_compiler_; // may point to const/opcode
	void *_vtable_location_target_; // (opcode offset adjusted)
	void *_vtable_location_external_; // from linked classes (just for reference)

	bool force_call_by_value;
	bool uses_call_by_reference() const;
	bool uses_return_by_memory() const;
	bool is_simple_class() const;
	bool is_size_known() const;
	const Class *get_array_element() const;
	bool usable_as_super_array() const;
	bool needs_constructor() const;
	bool needs_destructor() const;
	bool is_derived_from(const Class *root) const;
	bool is_derived_from_s(const string &root) const;
	void derive_from(const Class *root, bool increase_size);
	const Class *get_pointer() const;
	const Class *get_root() const;
	void add_function(SyntaxTree *s, Function *f, bool as_virtual = false, bool override = false);
	Function *get_func(const string &name, const Class *return_type, const Array<const Class*> &params) const;
	Function *get_same_func(const string &name, Function *f) const;
	Function *get_default_constructor() const;
	Array<Function*> get_constructors() const;
	Function *get_destructor() const;
	Function *get_assign() const;
	Function *get_virtual_function(int virtual_index) const;
	Function *get_get(const Class *index) const;
	void link_virtual_table();
	void link_external_virtual_table(void *p);
	void *create_instance() const;
	string var2str(const void *p) const;
};
extern const Class *TypeUnknown;
extern const Class *TypeReg128; // dummy for compilation
extern const Class *TypeReg64; // dummy for compilation
extern const Class *TypeReg32; // dummy for compilation
extern const Class *TypeReg16; // dummy for compilation
extern const Class *TypeReg8; // dummy for compilation
extern const Class *TypeVoid;
extern const Class *TypePointer;
extern const Class *TypeChunk;
extern const Class *TypeBool;
extern const Class *TypeInt;
extern const Class *TypeInt64;
extern const Class *TypeFloat32;
extern const Class *TypeFloat64;
extern const Class *TypeChar;
extern const Class *TypeCString;
extern const Class *TypeString;

extern const Class *TypeComplex;
extern const Class *TypeVector;
extern const Class *TypeRect;
extern const Class *TypeColor;
extern const Class *TypeQuaternion;

extern const Class *TypeException;
extern const Class *TypeExceptionP;

extern const Class *TypeClass;
extern const Class *TypeClassP;
extern const Class *TypeFunction;
extern const Class *TypeFunctionP;
extern const Class *TypeFunctionCode;
extern const Class *TypeFunctionCodeP;

};

#endif /* CLASS_H_ */
