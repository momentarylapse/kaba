#include "../../file/file.h"
#include "../kaba.h"
#include "../../config.h"
#include "lib.h"
#include "../dynamic/exception.h"
#include "../../base/callable.h"

#if __has_include("../../terminal/CommandLineParser.h")
#define HAS_TERMINAL 1
#include "../../terminal/CommandLineParser.h"
#endif

class vector;


namespace kaba {


extern const Class *TypeDate;
extern const Class *TypeStringList;
const Class *TypePath;
const Class *TypePathList;
const Class *TypeStreamP;
//const Class *TypeStreamSP;

const Class* TypeCallback;
const Class* TypeCallbackString;


static FileStream *_kaba_stdin = nullptr;


#pragma GCC push_options
#pragma GCC optimize("no-omit-frame-pointer")
#pragma GCC optimize("no-inline")
#pragma GCC optimize("0")



class KabaFileError : public KabaException {
public:
	KabaFileError() : KabaException(){}
	KabaFileError(const string &t) : KabaException(t){}
	void __init__(const string &t) {
		new(this) KabaFileError(t);
	}
};

class KabaFileStream : public FileStream {
public:
	void _cdecl __delete__() {
		this->~KabaFileStream();
	}
	int _cdecl _write(const string &s) {
		return write(s);
	}
	bytes _cdecl _read_size(int size) {
		bytes data;
		data.resize(size);
		int r = read(data);
		if (r < 0)
			return bytes();
		data.resize(r);
		return data;
	}
	int _cdecl _read_bytes(bytes &data) {
		return read(data);
	}
};

template<class F>
class KabaFormatter : public F {
public:
	/*void __init__(shared<Stream> stream) {
		msg_write("KabaFormatter init " + p2s(this));
		msg_write("  stream " + p2s(stream.get()));
		msg_write("  stream.count " + i2s(stream->_pointer_ref_counter));
		msg_write(stream->get_pos());
		new(this) F(stream);
	}*/
	void __init__(Stream* stream) {
		new(this) F(stream);
	}
	void __delete__() {
		this->F::~F();
	}
	void _cdecl _read_int(int &i) {
		KABA_EXCEPTION_WRAPPER(i = F::read_int());
	}
	void _cdecl _read_float(float &f) {
		 KABA_EXCEPTION_WRAPPER(f = F::read_float());
	}
	void _cdecl _read_bool(bool &b) {
		KABA_EXCEPTION_WRAPPER(b = F::read_bool());
	}
	void _cdecl _read_vector(vector &v) {
		KABA_EXCEPTION_WRAPPER(F::read_vector(&v));
	}
	void _cdecl _read_str(string &s) {
		KABA_EXCEPTION_WRAPPER(s = F::read_str());
	}
	void _cdecl _write_vector(const vector &v) {
		KABA_EXCEPTION_WRAPPER(F::write_vector(&v));
	}

	void _write_int(int i) { F::write_int(i); }
	void _write_float(float f) { F::write_float(f); }
	void _write_bool(bool b) { F::write_bool(b); }
	void _write_str(const string &s) { F::write_str(s); }

	static void declare(const Class *c) {
		using KF = KabaFormatter<F>;
		add_class(c);
			//class_add_element("stream", TypeStreamSP, &KF::stream);
		class_add_element("stream", TypeStreamP, &KF::stream);
			class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, &KF::__init__);
				//func_add_param("stream", TypeStreamSP);
				func_add_param("stream", TypeStreamP);
			class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, &KF::__delete__);
			class_add_func("__lshift__", TypeVoid, &KF::_write_bool, Flags::RAISES_EXCEPTIONS);
				func_add_param("b", TypeBool);
			class_add_func("__lshift__", TypeVoid, &KF::_write_int, Flags::RAISES_EXCEPTIONS);
				func_add_param("i", TypeInt);
			class_add_func("__lshift__", TypeVoid, &KF::_write_float, Flags::RAISES_EXCEPTIONS);
				func_add_param("x", TypeFloat32);
			class_add_func("__lshift__", TypeVoid, &KF::_write_vector, Flags::RAISES_EXCEPTIONS);
				func_add_param("v", TypeVector);
			class_add_func("__lshift__", TypeVoid, &KF::_write_str, Flags::RAISES_EXCEPTIONS);
				func_add_param("s", TypeString, Flags::OUT);
			class_add_func("__rshift__", TypeVoid, &KF::_read_bool, Flags::RAISES_EXCEPTIONS);
				func_add_param("b", TypeBool, Flags::OUT);
			class_add_func("__rshift__", TypeVoid, &KF::_read_int, Flags::RAISES_EXCEPTIONS);
				func_add_param("i", TypeInt, Flags::OUT);
			class_add_func("__rshift__", TypeVoid, &KF::_read_float, Flags::RAISES_EXCEPTIONS);
				func_add_param("x", TypeFloat32, Flags::OUT);
			class_add_func("__rshift__", TypeVoid, &KF::_read_vector, Flags::RAISES_EXCEPTIONS);
				func_add_param("v", TypeVector, Flags::OUT);
			class_add_func("__rshift__", TypeVoid, &KF::_read_str, Flags::RAISES_EXCEPTIONS);
				func_add_param("s", TypeString, Flags::OUT);
	}
};

/*class KabaFileNotFoundError : public KabaFileError
{ public: KabaFileNotFoundError(const string &t) : KabaFileError(t){} };

class KabaFileNotWritableError : public KabaFileError
{ public: KabaFileNotWritableError(const string &t) : KabaFileError(t){} };*/



FileStream* kaba_file_open(const Path &filename, const string &mode) {
	KABA_EXCEPTION_WRAPPER2(return file_open(filename, mode), KabaFileError);
	return nullptr;
}

string kaba_file_read(const Path &filename) {
	KABA_EXCEPTION_WRAPPER2(return file_read_binary(filename), KabaFileError);
	return "";
}

string kaba_file_read_text(const Path &filename) {
	KABA_EXCEPTION_WRAPPER2(return file_read_text(filename), KabaFileError);
	return "";
}

void kaba_file_write(const Path &filename, const string &buffer) {
	KABA_EXCEPTION_WRAPPER2(file_write_binary(filename, buffer), KabaFileError);
}

void kaba_file_write_text(const Path &filename, const string &buffer) {
	KABA_EXCEPTION_WRAPPER2(file_write_text(filename, buffer), KabaFileError);
}

string kaba_file_hash(const Path &filename, const string &type) {
	KABA_EXCEPTION_WRAPPER2(return file_hash(filename, type), KabaFileError);
	return "";
}

void kaba_file_rename(const Path &a, const Path &b) {
	KABA_EXCEPTION_WRAPPER2(file_rename(a, b), KabaFileError);
}

void kaba_file_copy(const Path &a, const Path &b) {
	KABA_EXCEPTION_WRAPPER2(file_copy(a, b), KabaFileError);
}

void kaba_file_delete(const Path &f) {
	KABA_EXCEPTION_WRAPPER2(file_delete(f), KabaFileError);
}

void kaba_dir_create(const Path &f) {
	KABA_EXCEPTION_WRAPPER2(dir_create(f), KabaFileError);
}

void kaba_dir_delete(const Path &f) {
	KABA_EXCEPTION_WRAPPER2(dir_delete(f), KabaFileError);
}


string _cdecl kaba_shell_execute(const string &cmd) {
	try {
		return shell_execute(cmd);
	} catch(::Exception &e) {
		kaba_raise_exception(new KabaException(e.message()));
	}
	return "";
}


#pragma GCC pop_options

class KabaPath : public Path {
public:
	Path lshift_p(const Path &p) const {
		return *this << p;
	}
	Path lshift_s(const string &p) const {
		return *this << p;
	}
	bool __contains__(const Path &p) const {
		return p.is_in(*this);
	}
	bool __bool__() const {
		return !this->is_empty();
	}
	static Path from_str(const string &s) {
		return Path(s);
	}
};

class PathList : public Array<Path> {
public:
	void _cdecl assign(const PathList &s) {
		*this = s;
	}
	bool __contains__(const Path &s) const {
		return this->find(s) >= 0;
	}
	Array<Path> __add__(const Array<Path> &o) const {
		return *this + o;
	}
	void __adds__(const Array<Path> &o) {
		append(o);
	}
};

void SIAddPackageOSPath() {
	add_package("os");

	TypePath = add_type("Path", config.super_array_size);

	add_class(TypePath);
		class_add_element_x("_s", TypeString, 0);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, &Path::__init__);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, &Path::__init_ext__);
			func_add_param("p", TypeString);
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, &Path::__delete__);
		class_add_func("absolute", TypePath, &Path::absolute, Flags::CONST);
		class_add_func("dirname", TypeString, &Path::dirname, Flags::PURE);
		class_add_func("basename", TypeString, &Path::basename, Flags::PURE);
		class_add_func("basename_no_ext", TypeString, &Path::basename_no_ext, Flags::PURE);
		class_add_func("extension", TypeString, &Path::extension, Flags::PURE);
		class_add_func("canonical", TypePath, &Path::canonical, Flags::PURE);
		class_add_func(IDENTIFIER_FUNC_STR, TypeString, &Path::str, Flags::PURE);
		class_add_func("is_empty", TypeBool, &Path::is_empty, Flags::PURE);
		class_add_func("is_relative", TypeBool, &Path::is_relative, Flags::PURE);
		class_add_func("is_absolute", TypeBool, &Path::is_absolute, Flags::PURE);
		class_add_func("__bool__", TypeBool, &KabaPath::__bool__, Flags::PURE);
		class_add_func("parent", TypePath, &Path::parent, Flags::PURE);
		class_add_func("compare", TypeInt, &Path::compare, Flags::PURE);
			func_add_param("p", TypePath);
		class_add_func("relative_to", TypePath, &Path::relative_to, Flags::PURE);
			func_add_param("p", TypePath);
		class_add_const("EMPTY", TypePath, &Path::EMPTY);
		class_add_func("@from_str", TypePath, &KabaPath::from_str, Flags::_STATIC__PURE);
			func_add_param("p", TypeString);
		add_operator(OperatorID::ASSIGN, TypeVoid, TypePath, TypePath, InlineID::NONE, &Path::operator =);
		add_operator(OperatorID::EQUAL, TypeBool, TypePath, TypePath, InlineID::NONE, &Path::operator ==);
		add_operator(OperatorID::NOTEQUAL, TypeBool, TypePath, TypePath, InlineID::NONE, &Path::operator !=);
		add_operator(OperatorID::SMALLER, TypeBool, TypePath, TypePath, InlineID::NONE, &Path::operator <);
		add_operator(OperatorID::GREATER, TypeBool, TypePath, TypePath, InlineID::NONE, &Path::operator >);
		add_operator(OperatorID::SHIFT_LEFT, TypePath, TypePath, TypePath, InlineID::NONE, &KabaPath::lshift_p);
		add_operator(OperatorID::SHIFT_LEFT, TypePath, TypePath, TypeString, InlineID::NONE, &KabaPath::lshift_s);
		add_operator(OperatorID::IN, TypeBool, TypePath, TypePath, InlineID::NONE, &KabaPath::__contains__);


	// AFTER TypePath!
	TypePathList = add_type_l(TypePath);

	add_class(TypePath);
		class_add_func("all_parents", TypePathList, &Path::all_parents, Flags::PURE);

	add_class(TypePathList);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, &Array<Path>::__init__);
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, &Array<Path>::clear);
		class_add_func("clear", TypeVoid, &Array<Path>::clear);
		class_add_func("add", TypeVoid, &Array<Path>::add);
			func_add_param("p", TypePath);
		add_operator(OperatorID::ASSIGN, TypeVoid, TypePathList, TypePathList, InlineID::NONE, &PathList::assign);
		add_operator(OperatorID::IN, TypeBool, TypePathList, TypePath, InlineID::NONE, &PathList::__contains__);
		add_operator(OperatorID::ADD, TypePathList, TypePathList, TypePathList, InlineID::NONE, &PathList::__add__);
		add_operator(OperatorID::ADDS, TypeVoid, TypePathList, TypePathList, InlineID::NONE, &PathList::__adds__);
}


char _el_off_data[1024];
#define evil_member_offset(C, M)	((int_p)((char*)&(reinterpret_cast<C*>(&_el_off_data[0])->M) - &_el_off_data[0]))

template<class C>
class KabaSharedPointer : public shared<C> {
public:
	void __init__() {
		msg_write("new Shared Pointer");
		new(this) shared<C>;
	}
	void __delete__() {
		msg_write("del Shared Pointer");
		this->shared<C>::~shared<C>();
	}
	void assign(shared<C> o) {
		msg_write("Shared Pointer ass1");
		*(shared<C>*)this = o;
	}
	void assign_p(C *o) {
		msg_write("Shared Pointer ass2");
		*(shared<C>*)this = o;
	}
	shared<C> create(C *p) {
		msg_write("Shared Pointer create");
		msg_write("  p: " + p2s(p));
		msg_write("  p.count: " + i2s(p->_pointer_ref_counter));
		shared<C> sp;
		sp = p;
		msg_write(p2s(sp.get()));
		msg_write(p->_pointer_ref_counter);
		return sp;//shared<C>(p);
	}

	static void declare(const Class *c) {
		using SP = KabaSharedPointer<C>;
		add_class(c);
			class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, &SP::__init__);
			class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, &SP::__delete__);
			class_add_func(IDENTIFIER_FUNC_SHARED_CLEAR, TypeVoid, &shared<C>::release);
			class_add_func(IDENTIFIER_FUNC_ASSIGN, TypeVoid, &SP::assign);
				func_add_param("other", c);
			class_add_func(IDENTIFIER_FUNC_ASSIGN, TypeVoid, &SP::assign_p);
				func_add_param("other", c->param[0]->get_pointer());
			class_add_func(IDENTIFIER_FUNC_SHARED_CREATE, c, &SP::create, Flags::STATIC);
				func_add_param("other", c->param[0]->get_pointer());
	}
};

#ifdef HAS_TERMINAL
class KabaCommandLineParser : CommandLineParser {
public:
	void __init__() {
		new(this) CommandLineParser;
	}
	void __delete__() {
		CommandLineParser::~CommandLineParser();
	}
	void option1(const string &name, const string &comment, Callable<void()> &cb) {
		option(name, comment, [&cb] { cb(); });
	}
	void option2(const string &name, const string &p, const string &comment, Callable<void(const string&)> &cb) {
		option(name, p, comment, [&cb] (const string &s) { cb(s); });
	}
	void cmd1(const string &name, const string &p, const string &comment, Callable<void(const Array<string>&)> &cb) {
		cmd(name, p, comment, [&cb] (const Array<string> &s) { cb(s); });
	}
	void parse1(const Array<string> &arg) {
		Array<string> a = {"?"};
		parse(a + arg);
	}
};
#define term_p(p) (p)
#else
struct CommandLineParser{};
#define term_p(p) nullptr
#endif

void SIAddPackageOS() {
	add_package("os");

	const Class *TypeStream = add_type("Stream", sizeof(Stream));
	TypeStreamP = add_type_p(TypeStream);
	//TypeStreamSP = add_type_p(TypeStream, Flags::SHARED);
	const Class *TypeFileStream = add_type("FileStream", sizeof(FileStream));
	const Class *TypeFileStreamP = add_type_p(TypeFileStream);
	const Class *TypeBinaryFormatter = add_type("BinaryFormatter", sizeof(BinaryFormatter));
	const Class *TypeTextLinesFormatter = add_type("TextLinesFormatter", sizeof(TextLinesFormatter));
	const Class *TypeFilesystem = add_type("fs", 0);
	const Class *TypeFileError = add_type("FileError", sizeof(KabaFileError));
	//Class *TypeFileNotFoundError= add_type  ("FileError", sizeof(KabaFileNotFoundError));
	//Class *TypeFileNotWritableError= add_type  ("FileError", sizeof(KabaFileNotWritableError));
	auto TypeCommandLineParser = add_type("CommandLineParser", sizeof(CommandLineParser));

	TypeCallback = add_type_f(TypeVoid, {});
	TypeCallbackString = add_type_f(TypeVoid, {TypeString});
	auto TypeCallbackStringList = add_type_f(TypeVoid, {TypeStringList});

	add_class(TypeStream);
		class_add_element(IDENTIFIER_SHARED_COUNT, TypeInt, evil_member_offset(FileStream, _pointer_ref_counter));
		// FIXME &FileStream::_pointer_ref_counter does not work here
		// we get a base-class-pointer... \(O_O)/
		//class_add_func_virtual(IDENTIFIER_FUNC_DELETE, TypeVoid, &Stream::__delete__);
		//class_add_func_virtual("write", TypeVoid, Stream::write);
		//class_add_element(IDENTIFIER_SHARED_COUNT, TypeInt, &Stream::_pointer_ref_counter);
		//const_cast<Class*>(TypeStream)->elements.back().offset = offsetof(Stream, _pointer_ref_counter);


	//KabaSharedPointer<FileStream>::declare(TypeStreamSP);

	add_class(TypeFileStream);
		class_derive_from(TypeStream, false, false);
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, &KabaFileStream::__delete__);
		//class_add_func("getCDate", TypeDate, &File::GetDateCreation);
		class_add_func("mtime", TypeDate, &FileStream::mtime);
		//class_add_func("getADate", TypeDate, &FileStream::GetDateAccess);
		class_add_func("get_size", TypeInt, &FileStream::get_size32);
		class_add_func("get_pos", TypeInt, &FileStream::get_pos);
		class_add_func("set_pos", TypeVoid, &FileStream::set_pos, Flags::RAISES_EXCEPTIONS);
			func_add_param("pos", TypeInt);
		class_add_func("seek", TypeVoid, &FileStream::seek, Flags::RAISES_EXCEPTIONS);
			func_add_param("delta", TypeInt);
		class_add_func("read", TypeString, &KabaFileStream::_read_size, Flags::RAISES_EXCEPTIONS);
			func_add_param("size", TypeInt);
		class_add_func("read", TypeInt, &KabaFileStream::_read_bytes, Flags::RAISES_EXCEPTIONS);
			func_add_param("s", TypeString);
		class_add_func("write", TypeInt, &KabaFileStream::_write, Flags::RAISES_EXCEPTIONS);
		//class_add_func_virtual("write", TypeInt, &FileStream::write);
			func_add_param("s", TypeString);
		class_add_func("is_end", TypeBool, &KabaFileStream::is_end);
		//class_set_vtable(FileStream);


	KabaFormatter<BinaryFormatter>::declare(TypeBinaryFormatter);
	KabaFormatter<TextLinesFormatter>::declare(TypeTextLinesFormatter);


	add_class(TypeFileError);
		class_derive_from(TypeException, false, false);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, &KabaFileError::__init__, Flags::OVERRIDE);
		class_add_func_virtual(IDENTIFIER_FUNC_DELETE, TypeVoid, &KabaFileError::__delete__, Flags::OVERRIDE);
		class_set_vtable(KabaFileError);


	add_class(TypeCommandLineParser);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, term_p(&KabaCommandLineParser::__init__));
		class_add_func(IDENTIFIER_FUNC_DELETE, TypeVoid, term_p(&KabaCommandLineParser::__delete__));
		class_add_func("info", TypeVoid, term_p(&CommandLineParser::info));
			func_add_param("cmd", TypeString);
			func_add_param("i", TypeString);
		class_add_func("show", TypeVoid, term_p(&CommandLineParser::show));
		class_add_func("parse", TypeVoid, term_p(&KabaCommandLineParser::parse1));
			func_add_param("arg", TypeStringList);
		class_add_func("option", TypeVoid, term_p(&KabaCommandLineParser::option1));
			func_add_param("name", TypeString);
			func_add_param("comment", TypeString);
			func_add_param("f", TypeCallback);
		class_add_func("option", TypeVoid, term_p(&KabaCommandLineParser::option2));
			func_add_param("name", TypeString);
			func_add_param("p", TypeString);
			func_add_param("comment", TypeString);
			func_add_param("f", TypeCallbackString);
		class_add_func("cmd", TypeVoid, term_p(&KabaCommandLineParser::cmd1));
			func_add_param("name", TypeString);
			func_add_param("p", TypeString);
			func_add_param("comment", TypeString);
			func_add_param("f", TypeCallbackStringList);


	// file access
	add_class(TypeFilesystem);
		class_add_func("open", TypeFileStreamP, &kaba_file_open, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypePath);
			func_add_param("mode", TypeString);
		class_add_func("read", TypeString, &kaba_file_read, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypePath);
		class_add_func("read_text", TypeString, &kaba_file_read_text, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypePath);
		class_add_func("write", TypeVoid, &kaba_file_write, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypePath);
			func_add_param("buffer", TypeString);
		class_add_func("write_text", TypeVoid, &kaba_file_write_text, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypePath);
			func_add_param("buffer", TypeString);
		class_add_func("exists", TypeBool, &file_exists, Flags::STATIC);
			func_add_param("filename", TypePath);
		class_add_func("size", TypeInt64, &file_size, Flags::STATIC);
			func_add_param("filename", TypePath);
		class_add_func("mtime", TypeDate, &file_mtime, Flags::STATIC);
			func_add_param("filename", TypePath);
		class_add_func("is_directory", TypeBool, &file_is_directory, Flags::STATIC);
			func_add_param("filename", TypePath);
		class_add_func("hash", TypeString, &kaba_file_hash, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypePath);
			func_add_param("type", TypeString);
		class_add_func("rename", TypeVoid, &kaba_file_rename, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("source", TypePath);
			func_add_param("dest", TypePath);
		class_add_func("copy", TypeVoid, &kaba_file_copy, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("source", TypePath);
			func_add_param("dest", TypePath);
		class_add_func("delete", TypeVoid, &kaba_file_delete, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypePath);
		class_add_func("search", TypePathList, &dir_search, Flags::STATIC);
			func_add_param("dir", TypePath);
			func_add_param("filter", TypeString);
			func_add_param("options", TypeString);
		class_add_func("create_directory", TypeVoid, &kaba_dir_create, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("dir", TypePath);
		class_add_func("delete_directory", TypeVoid, &kaba_dir_delete, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("dir", TypePath);
		class_add_func("current_directory", TypePath, &get_current_dir, Flags::STATIC);
		
		_kaba_stdin = new FileStream(0);
		add_ext_var("stdin", TypeFileStreamP, &_kaba_stdin);

	// system
	add_func("shell_execute", TypeString, &kaba_shell_execute, Flags::_STATIC__RAISES_EXCEPTIONS);
		func_add_param("cmd", TypeString);
}

};
