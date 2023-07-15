#include "../kaba.h"
#include "lib.h"
#include "list.h"
#include "shared.h"
#include "../dynamic/exception.h"
#include "../dynamic/dynamic.h"


namespace kaba {

extern const Class* TypePath;
extern const Class* TypeSpecialFunction;
extern const Class* TypeSpecialFunctionRef;

#pragma GCC push_options
#pragma GCC optimize("no-omit-frame-pointer")
#pragma GCC optimize("no-inline")
#pragma GCC optimize("0")

class KabaContext : public Context {
public:
	shared<Module> __load_module__(const string &filename, bool just_analyse) {
		KABA_EXCEPTION_WRAPPER( return load_module(filename, just_analyse); );
		return nullptr;
	}

	shared<Module> __create_from_source__(const string &source, bool just_analyse) {
		KABA_EXCEPTION_WRAPPER( return create_module_for_source(source, just_analyse); );
		return nullptr;
	}

	void __execute_single_command__(const string &cmd) {
		KABA_EXCEPTION_WRAPPER( execute_single_command(cmd); );
	}
};

#pragma GCC pop_options


void show_func(Function *f) {
	bool v = config.verbose;
	config.verbose = true;
	f->show("");
	config.verbose = v;
}

class ClassX : public Class {
public:
	string repr() const {
		return class_repr(this);
	}
};

class FunctionX : public Function {
public:
	string repr() const {
		return func_repr(this);
	}
};

template<class T>
class XSharedArray : public shared_array<T> {
public:
	void __init__() {
		new(this) shared_array<T>;
	}
};

void SIAddPackageKaba(Context *c) {
	add_package(c, "kaba");


	TypeClass = add_type  ("Class", sizeof(Class));
	TypeClassP = add_type_p_raw(TypeClass);
	auto TypeClassRef = add_type_ref(TypeClass);
	auto TypeClassPList = add_type_list(TypeClassP);
	auto TypeClassRefList = add_type_list(TypeClassRef);

	TypeFunction = add_type  ("Function", sizeof(Function));
	TypeFunctionP = add_type_p_raw(TypeFunction);
	auto TypeFunctionRef = add_type_ref(TypeFunction);
	auto TypeFunctionRefList = add_type_list(TypeFunctionRef);
	TypeFunctionCode = add_type  ("code", 32); // whatever
	TypeFunctionCodeRef = add_type_ref(TypeFunctionCode);
	TypeSpecialFunction = add_type  ("SpecialFunction", sizeof(SpecialFunction));
	//TypeSpecialFunctionP = add_type_p(TypeSpecialFunction);
	TypeSpecialFunctionRef = add_type_ref(TypeSpecialFunction);
	auto TypeSpecialFunctionRefList = add_type_list(TypeSpecialFunctionRef);
	auto TypeStatement = add_type  ("Statement", sizeof(Statement));
	auto TypeStatementRef = add_type_ref(TypeStatement);
	auto TypeStatementRefList = add_type_list(TypeStatementRef);
		

	auto TypeModule = add_type  ("Module", sizeof(Module));
	auto TypeModuleXfer = add_type_p_xfer(TypeModule);
	auto TypeModuleShared = add_type_p_shared(TypeModule);
	auto TypeModuleSharedList = add_type_list(TypeModuleShared);
	auto TypeModuleRef = add_type_ref(TypeModule);
	auto TypeModuleRefList = add_type_list(TypeModuleRef);

	auto TypeContext = add_type  ("Context", sizeof(Context));
	auto TypeContextRef = add_type_ref(TypeContext);
	auto TypeContextXfer = add_type_p_xfer(TypeContext);

	
	auto TypeClassElement = add_type("ClassElement", sizeof(ClassElement));
	auto TypeClassElementList = add_type_list(TypeClassElement);
	auto TypeVariable = add_type("Variable", sizeof(Variable));
	auto TypeVariableRef = add_type_ref(TypeVariable);
	auto TypeVariableRefList = add_type_list(TypeVariableRef);
	auto TypeConstant = add_type("Constant", sizeof(Constant));
	auto TypeConstantRef = add_type_ref(TypeConstant);
	auto TypeConstantRefList = add_type_list(TypeConstantRef);
	
	lib_create_pointer_xfer(TypeContextXfer);
	lib_create_pointer_shared<Module>(TypeModuleShared, TypeModuleXfer);
	
	add_class(TypeClassElement);
		class_add_element("name", TypeString, &ClassElement::name);
		class_add_element("type", TypeClassP, &ClassElement::type);
		class_add_element("offset", TypeInt, &ClassElement::offset);


	add_class(TypeClass);
		class_add_element("name", TypeString, &Class::name);
		class_add_element("size", TypeInt, &Class::size);
		class_add_element("type", TypeInt, &Class::type);
		class_add_element("parent", TypeClassP, &Class::parent);
		class_add_element("param", TypeClassRefList, &Class::param);
		class_add_element("namespace", TypeClassP, &Class::name_space);
		class_add_element("elements", TypeClassElementList, &Class::elements);
		class_add_element("functions", TypeFunctionRefList, &Class::functions);
		class_add_element("classes", TypeClassRefList, &Class::classes);
		class_add_element("constants", TypeConstantRefList, &Class::constants);
		class_add_element("static_variables", TypeVariableRefList, &Class::static_variables);
		class_add_element(Identifier::SHARED_COUNT, TypeInt, &Class::_pointer_ref_counter);
		class_add_func("is_derived_from", TypeBool, &Class::is_derived_from, Flags::PURE);
			func_add_param("c", TypeClassP);
		class_add_func("is_pointer_raw", TypeBool, &Class::is_pointer_raw, Flags::PURE);
		class_add_func("is_some_pointer", TypeBool, &Class::is_some_pointer, Flags::PURE);
		class_add_func("is_some_pointer_not_null", TypeBool, &Class::is_some_pointer_not_null, Flags::PURE);
		class_add_func("is_list", TypeBool, &Class::is_list, Flags::PURE);
		class_add_func("is_array", TypeBool, &Class::is_array, Flags::PURE);
		class_add_func("is_dict", TypeBool, &Class::is_dict, Flags::PURE);
		class_add_func("get_func", TypeFunctionP, &Class::get_func, Flags::PURE); // selfref
			func_add_param("name", TypeString);
			func_add_param("return_type", TypeClassP);
			func_add_param("params", TypeClassPList);
		class_add_func("long_name", TypeString, &Class::long_name, Flags::PURE);
		class_add_func("cname", TypeString, &Class::cname, Flags::PURE);
			func_add_param("ns", TypeClassP);
		class_add_func(Identifier::Func::STR, TypeString, &ClassX::repr, Flags::PURE);

	add_class(TypeClassP);
		class_add_func(Identifier::Func::STR, TypeString, &class_repr, Flags::PURE);

	add_class(TypeFunction);
		class_add_element("name", TypeString, &Function::name);
		class_add_func("long_name", TypeString, &Function::long_name, Flags::PURE);
		class_add_func("cname", TypeString, &Function::cname, Flags::PURE);
			func_add_param("ns", TypeClassP);
		class_add_func("signature", TypeString, &Function::signature, Flags::PURE);
			func_add_param("ns", TypeClassP);
		class_add_element("namespace", TypeClassP, &Function::name_space);
		class_add_element("num_params", TypeInt, &Function::num_params);
		class_add_element("var", TypeVariableRefList, &Function::var);
		class_add_element("param_type", TypeClassRefList, &Function::literal_param_type);
		class_add_element("return_type", TypeClassRef, &Function::literal_return_type);
		class_add_func("is_static", TypeBool, &Function::is_static, Flags::PURE);
		class_add_func("is_pure", TypeBool, &Function::is_pure, Flags::PURE);
		class_add_func("is_mutable", TypeBool, &Function::is_mutable, Flags::PURE);
		class_add_func("is_extern", TypeBool, &Function::is_extern, Flags::PURE);
		class_add_func("is_selfref", TypeBool, &Function::is_selfref, Flags::PURE);
		class_add_func("throws_exceptions", TypeBool, &Function::throws_exceptions, Flags::PURE);
		class_add_func("needs_overriding", TypeBool, &Function::needs_overriding, Flags::PURE);
		class_add_element("virtual_index", TypeInt, &Function::virtual_index);
		class_add_element("inline_index", TypeInt, &Function::inline_no);
		class_add_element("code", TypeFunctionCodeRef, &Function::address);
		class_add_func(Identifier::Func::STR, TypeString, &FunctionX::repr, Flags::PURE);

	//add_class(TypeFunctionP);

	add_class(TypeVariable);
		class_add_element("name", TypeString, &Variable::name);
		class_add_element("type", TypeClassP, &Variable::type);
		class_add_func("is_mutable", TypeBool, &Variable::is_mutable, Flags::PURE);
		class_add_func("is_extern", TypeBool, &Variable::is_extern, Flags::PURE);
		
	add_class(TypeConstant);
		class_add_element("name", TypeString, &Constant::name);
		class_add_element("type", TypeClassP, &Constant::type);

	add_class(TypeModule);
		class_add_element("name", TypeString, &Module::filename);
		class_add_element("used_by_default", TypeBool, &Module::used_by_default);
		class_add_element(Identifier::SHARED_COUNT, TypeInt, &Module::_pointer_ref_counter);
		class_add_func("classes", TypeClassRefList, &Module::classes, Flags::PURE);
		class_add_func("functions", TypeFunctionRefList, &Module::functions, Flags::PURE);
		class_add_func("variables", TypeVariableRefList, &Module::variables, Flags::PURE);
		class_add_func("constants", TypeConstantRefList, &Module::constants, Flags::PURE);
		class_add_func("base_class", TypeClassRef, &Module::base_class, Flags::PURE);
		//class_add_func("delete", TypeVoid, &remove_module, Flags::STATIC);
		//	func_add_param("script", TypeModule);
	
	add_class(TypeStatement);
		class_add_element("name", TypeString, &Statement::name);
		class_add_element("id", TypeInt, &Statement::id);
		class_add_element("num_params", TypeInt, &Statement::num_params);
	
	add_class(TypeSpecialFunction);
		class_add_element("name", TypeString, &SpecialFunction::name);
		class_add_element("id", TypeInt, &SpecialFunction::id);
		class_add_element("min_params", TypeInt, &SpecialFunction::min_params);
		class_add_element("max_params", TypeInt, &SpecialFunction::max_params);
		
	add_class(TypeModuleSharedList);
		class_add_func(Identifier::Func::INIT, TypeVoid, &XSharedArray<Module>::__init__);

	add_class(TypeClassElementList);
		class_add_func(Identifier::Func::INIT, TypeVoid, &XList<ClassElement>::__init__);
		class_add_func(Identifier::Func::DELETE, TypeVoid, &Array<ClassElement>::clear);

	add_class(TypeContext);
		class_add_element("packages", TypeModuleRefList, &Context::packages);
		class_add_func(Identifier::Func::DELETE, TypeVoid, &Context::__delete__);
		class_add_func("load_module", TypeModuleShared, &KabaContext::__load_module__, Flags::RAISES_EXCEPTIONS);
			func_add_param("filename", TypePath);
			func_add_param("just_analize", TypeBool);
		class_add_func("create_module_for_source", TypeModuleShared, &KabaContext::__create_from_source__, Flags::RAISES_EXCEPTIONS);
			func_add_param("source", TypeString);
			func_add_param("just_analize", TypeBool);
		class_add_func("execute_single_command", TypeVoid, &KabaContext::__execute_single_command__, Flags::RAISES_EXCEPTIONS);
			func_add_param("cmd", TypeString);
		class_add_func("get_dynamic_type", TypeClassP, &Context::get_dynamic_type, Flags::PURE);
			func_add_param("p", TypePointer);
		class_add_func("create", TypeContextXfer, &Context::create, Flags::STATIC);

	add_func("disassemble", TypeString, &Asm::disassemble, Flags::STATIC | Flags::PURE);
		func_add_param("p", TypePointer);
		func_add_param("length", TypeInt);
		func_add_param("comments", TypeBool);
	add_func("show_func", TypeVoid, &show_func, Flags::STATIC);
		func_add_param("f", TypeFunction);

	add_ext_var("default_context", TypeContextRef, (void*)&default_context);
	add_ext_var("statements", TypeStatementRefList, (void*)&Statements);
	add_ext_var("special_functions", TypeSpecialFunctionRefList, (void*)&special_functions);
	add_ext_var("kaba_version", TypeString, (void*)&Version);
}

}
