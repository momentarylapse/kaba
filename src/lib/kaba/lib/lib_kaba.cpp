#include "../kaba.h"
#include "common.h"
#include "exception.h"
#include "dynamic.h"


namespace Kaba {


#pragma GCC push_options
#pragma GCC optimize("no-omit-frame-pointer")
#pragma GCC optimize("no-inline")
#pragma GCC optimize("0")

Script *__load_script__(const string &filename, bool just_analyse) {
	KABA_EXCEPTION_WRAPPER( return Load(filename, just_analyse); );
	return nullptr;
}

Script *__create_from_source__(const string &source, bool just_analyse) {
	KABA_EXCEPTION_WRAPPER( return CreateForSource(source, just_analyse); );
	return nullptr;
}

void __execute_single_command__(const string &cmd) {
	KABA_EXCEPTION_WRAPPER( ExecuteSingleScriptCommand(cmd); );
}

#pragma GCC pop_options


void SIAddPackageKaba() {
	add_package("kaba", false);


	TypeClass 			= add_type  ("Class", sizeof(Class));
	TypeClassP			= add_type_p("Class*", TypeClass);
	auto *TypeClassPList = add_type_a("Class*[]", TypeClassP, -1);

	TypeFunction		= add_type  ("Function", sizeof(Function));
	TypeFunctionP		= add_type_p("Function*", TypeFunction);
	auto *TypeFunctionPList = add_type_a("Function*[]", TypeFunctionP, -1);
	TypeFunctionCode	= add_type  ("code", 32); // whatever
	TypeFunctionCodeP	= add_type_p("code*", TypeFunctionCode);
	auto *TypeStatement = add_type  ("Statement", sizeof(Statement));
	auto *TypeStatementP= add_type_p("Statement*", TypeStatement);
	auto *TypeStatementPList = add_type_a("Statement*[]", TypeStatementP, -1);
		

	auto *TypeScript = add_type  ("Script", sizeof(Script));
	auto *TypeScriptP = add_type_p("Script*", TypeScript);
	auto *TypeScriptPList = add_type_a("Script*[]", TypeScriptP, -1);

	
	auto *TypeClassElement = add_type("ClassElement", sizeof(ClassElement));
	auto *TypeClassElementList = add_type_a("ClassElement[]", TypeClassElement, -1);
	auto *TypeVariable = add_type("Variable", sizeof(Variable));
	auto *TypeVariableP = add_type_p("Variable*", TypeVariable);
	auto *TypeVariablePList = add_type_a("Variable*[]", TypeVariableP, -1);
	auto *TypeConstant = add_type("Constant", sizeof(Constant));
	auto *TypeConstantP = add_type_p("Constant*", TypeConstant);
	auto *TypeConstantPList = add_type_a("Constant*[]", TypeConstantP, -1);
	
	
	add_class(TypeClassElement);
		class_add_elementx("name", TypeString, &ClassElement::name);
		class_add_elementx("type", TypeClassP, &ClassElement::type);
		class_add_elementx("offset", TypeInt, &ClassElement::offset);


	add_class(TypeClass);
		class_add_elementx("name", TypeString, &Class::name);
		class_add_elementx("size", TypeInt, &Class::size);
		class_add_elementx("parent", TypeClassP, &Class::parent);
		class_add_elementx("param", TypeClassP, &Class::param);
		class_add_elementx("namespace", TypeClassP, &Class::name_space);
		class_add_elementx("elements", TypeClassElementList, &Class::elements);
		class_add_elementx("functions", TypeFunctionPList, &Class::functions);
		class_add_elementx("classes", TypeClassPList, &Class::classes);
		class_add_elementx("constants", TypeConstantPList, &Class::constants);
		class_add_funcx("is_derived_from", TypeBool, &Class::is_derived_from);
			func_add_param("c", TypeClassP);
		class_add_funcx("long_name", TypeString, &Class::long_name);

	add_class(TypeClassP);
		class_add_funcx("str", TypeString, &class_repr);

	add_class(TypeFunction);
		class_add_elementx("name", TypeString, &Function::name);
		class_add_funcx("long_name", TypeString, &Function::long_name);
		class_add_funcx("signature", TypeString, &Function::signature);
		class_add_elementx("namespace", TypeClassP, &Function::name_space);
		class_add_elementx("num_params", TypeInt, &Function::num_params);
		class_add_elementx("var", TypeVariablePList, &Function::var);
		class_add_elementx("param_type", TypeClassPList, &Function::literal_param_type);
		class_add_elementx("return_type", TypeClassP, &Function::literal_return_type);
		class_add_elementx("is_static", TypeBool, &Function::is_static);
		class_add_elementx("is_pure", TypeBool, &Function::is_pure);
		class_add_elementx("is_extern", TypeBool, &Function::is_extern);
		class_add_elementx("needs_overriding", TypeBool, &Function::needs_overriding);
		class_add_elementx("virtual_index", TypeInt, &Function::virtual_index);
		class_add_elementx("inline_index", TypeInt, &Function::inline_no);
		class_add_elementx("code", TypeFunctionCodeP, &Function::address);

		add_class(TypeFunctionP);
			class_add_funcx("str", TypeString, &func_repr);

	add_class(TypeVariable);
		class_add_elementx("name", TypeString, &Variable::name);
		class_add_elementx("type", TypeClassP, &Variable::type);
		
	add_class(TypeConstant);
		class_add_elementx("name", TypeString, &Constant::name);
		class_add_elementx("type", TypeClassP, &Constant::type);

	add_class(TypeScript);
		class_add_elementx("name", TypeString, &Script::filename);
		class_add_elementx("used_by_default", TypeBool, &Script::used_by_default);
		class_add_funcx("classes", TypeClassPList, &Script::classes);
		class_add_funcx("functions", TypeFunctionPList, &Script::functions);
		class_add_funcx("variables", TypeVariablePList, &Script::variables);
		class_add_funcx("constants", TypeConstantPList, &Script::constants);
		class_add_funcx("base_class", TypeClassP, &Script::base_class);
		class_add_funcx("load", TypeScriptP, &__load_script__, ScriptFlag(FLAG_RAISES_EXCEPTIONS | FLAG_STATIC));
			func_add_param("filename", TypeString);
			func_add_param("just_analize", TypeBool);
		class_add_funcx("create", TypeScriptP, &__create_from_source__, ScriptFlag(FLAG_RAISES_EXCEPTIONS | FLAG_STATIC));
			func_add_param("source", TypeString);
			func_add_param("just_analize", TypeBool);
		class_add_funcx("delete", TypeVoid, &Remove, FLAG_STATIC);
			func_add_param("script", TypeScriptP);
		class_add_funcx("execute_single_command", TypeVoid, &__execute_single_command__, ScriptFlag(FLAG_RAISES_EXCEPTIONS | FLAG_STATIC));
			func_add_param("cmd", TypeString);
	
	add_class(TypeStatement);
		class_add_elementx("name", TypeString, &Statement::name);
		class_add_elementx("id", TypeInt, &Statement::id);
		class_add_elementx("num_params", TypeInt, &Statement::num_params);
		
	add_class(TypeClassElementList);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, &Array<ClassElement>::__init__);

	add_funcx("get_dynamic_type", TypeClassP, &GetDynamicType, FLAG_STATIC);
		func_add_param("p", TypePointer);
	add_funcx("disassemble", TypeString, &Asm::disassemble, FLAG_STATIC);
		func_add_param("p", TypePointer);
		func_add_param("length", TypeInt);
		func_add_param("comments", TypeBool);

	add_ext_var("packages", TypeScriptPList, (void*)&Packages);
	add_ext_var("statements", TypeStatementPList, (void*)&Statements);
	add_ext_var("lib_version", TypeString, (void*)&LibVersion);
	add_ext_var("kaba_version", TypeString, (void*)&Version);
}



}
