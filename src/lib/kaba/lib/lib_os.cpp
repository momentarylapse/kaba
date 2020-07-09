#include "../../file/file.h"
#include "../kaba.h"
#include "../../config.h"
#include "common.h"
#include "exception.h"

class vector;


namespace Kaba{


extern const Class *TypeIntPs;
extern const Class *TypeFloatPs;
extern const Class *TypeBoolPs;
extern const Class *TypeDate;
extern const Class *TypeStringList;


static File *_kaba_stdin = nullptr;


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

class KabaFile : public File {
public:
	void _cdecl __delete__()
	{ this->~KabaFile(); }
	int _cdecl _write_buffer(const string &s)
	{ return write_buffer(s); }
	string _cdecl _read_buffer(int size)
	{
		string s;
		s.resize(size);
		int r = read_buffer(s);
		s.resize(r);
		return s;
	}
	void _cdecl _read_int(int &i)
	{ KABA_EXCEPTION_WRAPPER(i = read_int()); }
	void _cdecl _read_float(float &f)
	{ KABA_EXCEPTION_WRAPPER(f = read_float()); }
	void _cdecl _read_bool(bool &b)
	{ KABA_EXCEPTION_WRAPPER(b = read_bool()); }
	void _cdecl _read_vector(vector &v)
	{ KABA_EXCEPTION_WRAPPER(read_vector(&v)); }
	void _cdecl _read_str(string &s)
	{ KABA_EXCEPTION_WRAPPER(s = read_str()); }
	void _cdecl _write_vector(const vector &v)
	{ KABA_EXCEPTION_WRAPPER(write_vector(&v)); }

	void _write_int(int i){ write_int(i); }
	void _write_float(float f){ write_float(f); }
	void _write_bool(bool b){ write_bool(b); }
	void _write_str(const string &s){ write_str(s); }
};

/*class KabaFileNotFoundError : public KabaFileError
{ public: KabaFileNotFoundError(const string &t) : KabaFileError(t){} };

class KabaFileNotWritableError : public KabaFileError
{ public: KabaFileNotWritableError(const string &t) : KabaFileError(t){} };*/



File* kaba_file_open(const string &filename) {
	KABA_EXCEPTION_WRAPPER2(return FileOpen(filename), KabaFileError);
	return nullptr;
}

File* kaba_file_open_text(const string &filename) {
	KABA_EXCEPTION_WRAPPER2(return FileOpenText(filename), KabaFileError);
	return nullptr;
}

File* kaba_file_create(const string &filename) {
	KABA_EXCEPTION_WRAPPER2(return FileCreate(filename), KabaFileError);
	return nullptr;
}

File* kaba_file_create_text(const string &filename) {
	KABA_EXCEPTION_WRAPPER2(return FileCreateText(filename), KabaFileError);
	return nullptr;
}

string kaba_file_read(const string &filename) {
	KABA_EXCEPTION_WRAPPER2(return FileRead(filename), KabaFileError);
	return "";
}

string kaba_file_read_text(const string &filename) {
	KABA_EXCEPTION_WRAPPER2(return FileReadText(filename), KabaFileError);
	return "";
}

void kaba_file_write(const string &filename, const string &buffer) {
	KABA_EXCEPTION_WRAPPER2(FileWrite(filename, buffer), KabaFileError);
}

void kaba_file_write_text(const string &filename, const string &buffer) {
	KABA_EXCEPTION_WRAPPER2(FileWriteText(filename, buffer), KabaFileError);
}

string kaba_file_hash(const string &filename, const string &type) {
	KABA_EXCEPTION_WRAPPER2(return file_hash(filename, type), KabaFileError);
	return "";
}

void kaba_file_rename(const string &a, const string &b) {
	KABA_EXCEPTION_WRAPPER2(file_rename(a, b), KabaFileError);
}

void kaba_file_copy(const string &a, const string &b) {
	KABA_EXCEPTION_WRAPPER2(file_copy(a, b), KabaFileError);
}

void kaba_file_delete(const string &f) {
	KABA_EXCEPTION_WRAPPER2(file_delete(f), KabaFileError);
}

void kaba_dir_create(const string &f) {
	KABA_EXCEPTION_WRAPPER2(dir_create(f), KabaFileError);
}

void kaba_dir_delete(const string &f) {
	KABA_EXCEPTION_WRAPPER2(dir_delete(f), KabaFileError);
}


#pragma GCC pop_options

void SIAddPackageOS() {
	add_package("os", false);

	const Class *TypeFile = add_type("File", 0);
	const Class *TypeFileP = add_type_p(TypeFile);
	const Class *TypeFilesystem = add_type("Filesystem", 0);
	const Class *TypeFileError = add_type("FileError", sizeof(KabaFileError));
	//Class *TypeFileNotFoundError= add_type  ("FileError", sizeof(KabaFileNotFoundError));
	//Class *TypeFileNotWritableError= add_type  ("FileError", sizeof(KabaFileNotWritableError));

	add_class(TypeFile);
		class_add_funcx(IDENTIFIER_FUNC_DELETE, TypeVoid, &KabaFile::__delete__);
		//class_add_funcx("getCDate", TypeDate, &File::GetDateCreation);
		class_add_funcx("mtime", TypeDate, &File::mtime);
		//class_add_funcx("getADate", TypeDate, &File::GetDateAccess);
		class_add_funcx("get_size", TypeInt, &File::get_size32);
		class_add_funcx("get_pos", TypeInt, &File::get_pos);
		class_add_funcx("set_pos", TypeVoid, &File::set_pos, Flags::RAISES_EXCEPTIONS);
			func_add_param("pos", TypeInt);
		class_add_funcx("seek", TypeVoid, &File::seek, Flags::RAISES_EXCEPTIONS);
			func_add_param("delta", TypeInt);
		class_add_funcx("read", TypeString, &KabaFile::_read_buffer, Flags::RAISES_EXCEPTIONS);
			func_add_param("size", TypeInt);
		class_add_funcx("write", TypeInt, &KabaFile::_write_buffer, Flags::RAISES_EXCEPTIONS);
			func_add_param("s", TypeString);
		class_add_funcx("__lshift__", TypeVoid, &KabaFile::_write_bool, Flags::RAISES_EXCEPTIONS);
			func_add_param("b", TypeBool);
		class_add_funcx("__lshift__", TypeVoid, &KabaFile::_write_int, Flags::RAISES_EXCEPTIONS);
			func_add_param("i", TypeInt);
		class_add_funcx("__lshift__", TypeVoid, &KabaFile::_write_float, Flags::RAISES_EXCEPTIONS);
			func_add_param("x", TypeFloat32);
		class_add_funcx("__lshift__", TypeVoid, &KabaFile::_write_vector, Flags::RAISES_EXCEPTIONS);
			func_add_param("v", TypeVector);
		class_add_funcx("__lshift__", TypeVoid, &KabaFile::_write_str, Flags::RAISES_EXCEPTIONS);
			func_add_param("s", TypeString);
		class_add_funcx("__rshift__", TypeVoid, &KabaFile::_read_bool, Flags::RAISES_EXCEPTIONS);
			func_add_param("b", TypeBoolPs);
		class_add_funcx("__rshift__", TypeVoid, &KabaFile::_read_int, Flags::RAISES_EXCEPTIONS);
			func_add_param("i", TypeIntPs);
		class_add_funcx("__rshift__", TypeVoid, &KabaFile::_read_float, Flags::RAISES_EXCEPTIONS);
			func_add_param("x", TypeFloatPs);
		class_add_funcx("__rshift__", TypeVoid, &KabaFile::_read_vector, Flags::RAISES_EXCEPTIONS);
			func_add_param("v", TypeVector);
		class_add_funcx("__rshift__", TypeVoid, &KabaFile::_read_str, Flags::RAISES_EXCEPTIONS);
			func_add_param("s", TypeString);
		class_add_funcx("eof", TypeBool, &KabaFile::eof);

	add_class(TypeFileError);
		class_derive_from(TypeException, false, false);
		class_add_funcx(IDENTIFIER_FUNC_INIT, TypeVoid, &KabaFileError::__init__, Flags::OVERRIDE);
		class_add_func_virtualx(IDENTIFIER_FUNC_DELETE, TypeVoid, &KabaFileError::__delete__, Flags::OVERRIDE);
		class_set_vtable(KabaFileError);

	// file access
	add_class(TypeFilesystem);
		class_add_funcx("open", TypeFileP, &kaba_file_open, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
		class_add_funcx("open_text", TypeFileP, &kaba_file_open_text, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
		class_add_funcx("create", TypeFileP, &kaba_file_create, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
		class_add_funcx("create_text", TypeFileP, &kaba_file_create_text, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
		class_add_funcx("read", TypeString, &kaba_file_read, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
		class_add_funcx("read_text", TypeString, &kaba_file_read_text, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
		class_add_funcx("write", TypeVoid, &kaba_file_write, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
			func_add_param("buffer", TypeString);
		class_add_funcx("write_text", TypeVoid, &kaba_file_write_text, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
			func_add_param("buffer", TypeString);
		class_add_funcx("exists", TypeBool, &file_exists, Flags::STATIC);
			func_add_param("filename", TypeString);
		class_add_funcx("size", TypeInt64, &file_size, Flags::STATIC);
			func_add_param("filename", TypeString);
		class_add_funcx("mtime", TypeDate, &file_mtime, Flags::STATIC);
			func_add_param("filename", TypeString);
		class_add_funcx("is_directory", TypeBool, &file_is_directory, Flags::STATIC);
			func_add_param("filename", TypeString);
		class_add_funcx("hash", TypeString, &kaba_file_hash, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
			func_add_param("type", TypeString);
		class_add_funcx("rename", TypeVoid, &kaba_file_rename, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("source", TypeString);
			func_add_param("dest", TypeString);
		class_add_funcx("copy", TypeVoid, &kaba_file_copy, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("source", TypeString);
			func_add_param("dest", TypeString);
		class_add_funcx("delete", TypeVoid, &kaba_file_delete, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("filename", TypeString);
		class_add_funcx("search", TypeStringList, &dir_search, Flags::STATIC);
			func_add_param("dir", TypeString);
			func_add_param("filter", TypeString);
			func_add_param("show_dirs", TypeBool);
		class_add_funcx("create_directory", TypeVoid, &kaba_dir_create, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("dir", TypeString);
		class_add_funcx("delete_directory", TypeVoid, &kaba_dir_delete, Flags::_STATIC__RAISES_EXCEPTIONS);
			func_add_param("dir", TypeString);
		class_add_funcx("current_directory", TypeString, &get_current_dir, Flags::STATIC);
		
		_kaba_stdin = new File();
		_kaba_stdin->handle = 0;
		add_ext_var("stdin", TypeFileP, &_kaba_stdin);
}

};
