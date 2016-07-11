#include "any.h"
#include "../base/map.h"

typedef HashMap<string, Any> AnyHashMap;

AnyHashMap _empty_dummy_hash_;
DynamicArray _empty_dummy_array_ = {NULL, 0, 0, sizeof(Any)};

Any EmptyVar;
//Any EmptyHash = _empty_dummy_hash_;
Any EmptyArray = *(Array<Any>*)&_empty_dummy_array_;


static string type_name(int t)
{
	if (t == TYPE_NONE)	return "-none-";
	if (t == TYPE_INT)	return "int";
	if (t == TYPE_FLOAT)	return "float";
	if (t == TYPE_BOOL)	return "bool";
	if (t == TYPE_STRING)	return "string";
	if (t == TYPE_ARRAY)	return "array";
	if (t == TYPE_HASH)	return "hash array";
	return "-unknown type-";
}

Any::Any()
{
	//msg_write(format("any  %p", this));
	type = TYPE_NONE;
	data = NULL;
}

void Any::__init__()
{
	new(this) Any;
}

Any::Any(const Any &a)
{
	//msg_write(format("any any  %p", this));
	type = TYPE_NONE;
	data = NULL;
	*this = a;
}

Any::Any(int i)
{
	//msg_write(format("any int  %p", this));
	type = TYPE_INT;
	data = new int;
	*(int*)data = i;
}

Any::Any(float f)
{
	type = TYPE_FLOAT;
	data = new float;
	*(float*)data = f;
}

Any::Any(bool b)
{
	type = TYPE_BOOL;
	data = new bool;
	*(bool*)data = b;
}

Any::Any(const string &s)
{
	type = TYPE_STRING;
	data = new string;
	*(string*)data = s;
}

Any::Any(const char *s)
{
	type = TYPE_STRING;
	data = new string;
	*(string*)data = string(s);
}

Any::Any(const Array<Any> &a)
{
	type = TYPE_ARRAY;
	data = new Array<Any>;
	*((Array<Any>*)data) = a;
}

/*Any::Any(const AnyHashMap &a)
{
	type = TYPE_HASH;
	data = new AnyHashMap;
	*((AnyHashMap*)data) = a;
}*/

Any::~Any()
{	clear();	}

void Any::clear()
{
	//msg_write(format("clear %s  %p", type->Name, this));
	if (type == TYPE_INT)
		delete((int*)data);
	else if (type == TYPE_FLOAT)
		delete((float*)data);
	else if (type == TYPE_BOOL)
		delete((bool*)data);
	else if (type == TYPE_STRING)
		delete((string*)data);
	else if (type == TYPE_ARRAY)
		delete((Array<Any>*)data);
	else if (type == TYPE_HASH)
		delete((AnyHashMap*)data);
	else if (type != TYPE_NONE)
		msg_error("Any.clear(): " + type_name(type));
	type = TYPE_NONE;
	data = NULL;
}

string Any::str() const
{
	if (type == TYPE_INT)
		return i2s(*(int*)data);
	else if (type == TYPE_FLOAT)
		return f2s(*(float*)data, 6);
	else if (type == TYPE_BOOL)
		return b2s(*(bool*)data);
	else if (type == TYPE_STRING)
		return "\"" + *(string*)data + "\"";
	else if (type == TYPE_ARRAY){
		string s = "[";
		for (Any &p : *(Array<Any>*)data){
			if (s.num > 1)
				s += ", ";
			s += p.str();
		}
		return s + "]";
	}else if (type == TYPE_HASH){
		string s = "[";
		for (AnyHashMap::Entry &p : *(AnyHashMap*)data){
			if (s.num > 1)
				s += ", ";
			s += "\"" + p.key + "\" : " + p.value.str();
		}
		return s + "]";
	}else if (type == TYPE_NONE)
		return "<empty>";
	else
		return "unhandled Any.str(): " + type_name(type);
}

bool Any::_bool() const
{
	if (type == TYPE_BOOL)
		return *(bool*)data;
	msg_error("Any.bool(): " + type_name(type));
	return false;
}

int Any::_int() const
{
	if (type == TYPE_INT)
		return *(int*)data;
	msg_error("Any.int(): " + type_name(type));
	return 0;
}

float Any::_float() const
{
	if (type == TYPE_FLOAT)
		return *(float*)data;
	msg_error("Any.float(): " + type_name(type));
	return 0;
}

void print(const Any &a)
{	msg_write(a.str());	}

Any &Any::operator = (const Any &a)
{
	//msg_write(format("%s = %s  %p = %p", type_name(type).c_str(), type_name(a.type).c_str(), this, &a));
	if (&a != this){
		clear();
		type = a.type;
		if (a.type == TYPE_INT){
			data = new int;
			*(int*)data = *(int*)a.data;
		}else if (a.type == TYPE_FLOAT){
			data = new float;
			*(float*)data = *(float*)a.data;
		}else if (a.type == TYPE_BOOL){
			data = new bool;
			*(bool*)data = *(bool*)a.data;
		}else if (a.type == TYPE_STRING){
			data = new string;
			*(string*)data = *(string*)a.data;
		}else if (a.type == TYPE_ARRAY){
			data = new Array<Any>;
			*(Array<Any>*)data = *(Array<Any>*)a.data;
		}else if (a.type == TYPE_HASH){
			data = new AnyHashMap;
			*(AnyHashMap*)data = *(AnyHashMap*)a.data;
		}else if (a.type != TYPE_NONE){
			type = TYPE_NONE;
			msg_error("Any = Any: " + type_name(a.type));
		}
	}
	return *this;
}

Any Any::operator + (const Any &a) const
{
	if ((type == TYPE_INT) && (a.type == TYPE_INT))
		return *(int*)data + *(int*)a.data;
	if ((type == TYPE_FLOAT) && (a.type == TYPE_FLOAT))
		return *(float*)data + *(float*)a.data;
	if ((type == TYPE_INT) && (a.type == TYPE_FLOAT))
		return (float)*(int*)data + *(float*)a.data;
	if ((type == TYPE_FLOAT) && (a.type == TYPE_INT))
		return *(float*)data + (float)*(int*)a.data;
	if ((type == TYPE_STRING) && (a.type == TYPE_STRING))
		return *(string*)data + *(string*)a.data;
	msg_error("Any + Any: " + type_name(type) + " + " + type_name(a.type));
	return EmptyVar;
}

Any Any::operator - (const Any &a) const
{
	if ((type == TYPE_INT) && (a.type == TYPE_INT))
		return *(int*)data - *(int*)a.data;
	if ((type == TYPE_FLOAT) && (a.type == TYPE_FLOAT))
		return *(float*)data - *(float*)a.data;
	if ((type == TYPE_INT) && (a.type == TYPE_FLOAT))
		return (float)*(int*)data - *(float*)a.data;
	if ((type == TYPE_FLOAT) && (a.type == TYPE_INT))
		return *(float*)data - (float)*(int*)a.data;
	msg_error("Any - Any: " + type_name(type) + " - " + type_name(a.type));
	return EmptyVar;
}

void Any::operator += (const Any &a)
{
	if ((type == TYPE_INT) && (a.type == TYPE_INT))
		*(int*)data += *(int*)a.data;
	else if ((type == TYPE_FLOAT) && (a.type == TYPE_FLOAT))
		*(float*)data += *(float*)a.data;
	else if ((type == TYPE_INT) && (a.type == TYPE_FLOAT))
		*(int*)data += (int)*(float*)a.data;
	else if ((type == TYPE_FLOAT) && (a.type == TYPE_INT))
		*(float*)data += (float)*(int*)a.data;
	else if ((type == TYPE_STRING) && (a.type == TYPE_STRING))
		*(string*)data += *(string*)a.data;
	else if ((type == TYPE_ARRAY) && (a.type == TYPE_ARRAY))
		append(a);
	else if (type == TYPE_ARRAY)
		add(a);
	else
		msg_error("Any += Any: " + type_name(type) + " += " + type_name(a.type));
}

void Any::add(const Any &a)
{
	if (type == TYPE_NONE){
		type = TYPE_ARRAY;
		data = new Array<Any>;
	}
	if (type == TYPE_ARRAY){
		if (&a == this){
			Any b = a;
			((Array<Any>*)data)->add(b);
		}else{
			((Array<Any>*)data)->add(a);
		}
	}else{
		msg_error("Any.add(): not an array: " + type_name(type));
	}
}

void Any::append(const Any &a)
{
	if (type == TYPE_NONE){
		type = TYPE_ARRAY;
		data = new Array<Any>;
	}
	if ((type == TYPE_ARRAY) && (a.type == TYPE_ARRAY)){
		if (&a == this){
			Any b = a;
			((Array<Any>*)data)->append(*(Array<Any>*)b.data);
		}else{
			((Array<Any>*)data)->append(*(Array<Any>*)a.data);
		}
	}else{
		msg_error("Any.append(): not an array: " + type_name(type) + " " + type_name(a.type));
	}
}

Any &Any::operator[] (int index)
{
	if (type == TYPE_ARRAY)
		return (*(Array<Any>*)data)[index];
	msg_error("Any[]: not an array: " + type_name(type));
	return EmptyVar;
}

const Any &Any::operator[] (int index) const
{
	if (type == TYPE_ARRAY)
		return (*(Array<Any>*)data)[index];
	msg_error("Any[]: not an array: " + type_name(type));
	return EmptyVar;
}

Any &Any::back()
{
	if (type == TYPE_ARRAY)
		return ((Array<Any>*)data)->back();
	msg_error("Any.back: not an array: " + type_name(type));
	return EmptyVar;
}

const Any &Any::operator[] (const string &key) const
{
	if (type == TYPE_HASH)
		return (*(AnyHashMap*)data)[key];
	msg_error("Any[]: not a hash array: " + type_name(type));
	return EmptyVar;
}

Any &Any::operator[] (const string &key)
{
	if (type == TYPE_NONE){
		type = TYPE_HASH;
		data = new AnyHashMap;
	}
	if (type == TYPE_HASH){
		//msg_write(p2s(&(*(HashMap*)data)[key]));
		return (*(AnyHashMap*)data)[key];
	}
	msg_error("Any[]: not a hash array: " + type_name(type));
	return EmptyVar;
}

Any Any::at(int i) const
{
	return (*this)[i];
}

void Any::aset(int i, const Any &value)
{
	(*this)[i] = value;
}

Any Any::get(const string &key) const
{
	return (*this)[key];
}

void Any::hset(const string &key, const Any &value)
{
	(*this)[key] = value;
}
