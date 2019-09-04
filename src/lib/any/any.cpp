#include "any.h"
#include "../base/map.h"

class AnyMap : public Map<string, Any> {};

AnyMap _empty_dummy_map_;
DynamicArray _empty_dummy_array_ = {NULL, 0, 0, sizeof(Any)};

Any EmptyVar;
//Any EmptyMap = _empty_dummy_map_;
Any EmptyArray = *(Array<Any>*)&_empty_dummy_array_;


static string type_name(int t)
{
	if (t == TYPE_NONE)	return "-none-";
	if (t == TYPE_INT)	return "int";
	if (t == TYPE_FLOAT)	return "float";
	if (t == TYPE_BOOL)	return "bool";
	if (t == TYPE_STRING)	return "string";
	if (t == TYPE_ARRAY)	return "array";
	if (t == TYPE_HASH)	return "hash map";
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
		delete((AnyMap*)data);
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
		for (Any &p: *as_array()){
			if (s.num > 1)
				s += ", ";
			s += p.str();
		}
		return s + "]";
	}else if (type == TYPE_HASH){
		string s = "{";
		for (AnyMap::Entry &p: *as_map()){
			if (s.num > 1)
				s += ", ";
			s += "\"" + p.key + "\": " + p.value.str();
		}
		return s + "}";
	}else if (type == TYPE_NONE)
		return "<empty>";
	else
		return "unhandled Any.str(): " + type_name(type);
}

bool Any::_bool() const {
	if (type == TYPE_BOOL)
		return *(bool*)data;
	if (type == TYPE_INT)
		return *(int*)data != 0;
	throw Exception("not bool: " + type_name(type));
}

int Any::_int() const {
	if (type == TYPE_INT)
		return *(int*)data;
	if (type == TYPE_BOOL)
		return (int)*(bool*)data;
	if (type == TYPE_FLOAT)
		return (int)*(float*)data;
	if (type == TYPE_STRING)
		return ((string*)data)->_int();
	throw Exception("not int: " + type_name(type));
}

float Any::_float() const {
	if (type == TYPE_INT)
		return (float)*(int*)data;
	if (type == TYPE_FLOAT)
		return *(float*)data;
	if (type == TYPE_STRING)
		return ((string*)data)->_float();
	throw Exception("not float: " + type_name(type));
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
			data = new AnyMap;
			*(AnyMap*)data = *(AnyMap*)a.data;
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

void Any::add(const Any &a) {
	if (type == TYPE_NONE) {
		type = TYPE_ARRAY;
		data = new Array<Any>;
	}
	if (type == TYPE_ARRAY) {
		if (&a == this) {
			Any b = a;
			((Array<Any>*)data)->add(b);
		} else {
			((Array<Any>*)data)->add(a);
		}
	} else {
		throw Exception("not an array: " + type_name(type));
	}
}

void Any::append(const Any &a) {
	if (type == TYPE_NONE) {
		type = TYPE_ARRAY;
		data = new Array<Any>;
	}
	if ((type == TYPE_ARRAY) and (a.type == TYPE_ARRAY)) {
		if (&a == this) {
			Any b = a;
			as_array()->append(*b.as_array());
		} else {
			as_array()->append(*a.as_array());
		}
	} else {
		throw Exception("not an array: " + type_name(type) + " " + type_name(a.type));
	}
}

int Any::length() {
	if (type == TYPE_ARRAY)
		return as_array()->num;
	return 0;
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
		return (*(AnyMap*)data)[key];
	msg_error("Any[]: not a hash map: " + type_name(type));
	return EmptyVar;
}

Any &Any::operator[] (const string &key)
{
	if (type == TYPE_NONE){
		type = TYPE_HASH;
		data = new AnyMap;
	}
	if (type == TYPE_HASH){
		//msg_write(p2s(&(*(HashMap*)data)[key]));
		return (*(AnyMap*)data)[key];
	}
	msg_error("Any[]: not a hash map: " + type_name(type));
	return EmptyVar;
}

int* Any::as_int() const {
	return (int*)data;
}

float* Any::as_float() const {
	return (float*)data;
}

bool* Any::as_bool() const {
	return (bool*)data;
}

string* Any::as_string() const {
	return (string*)data;
}

AnyMap* Any::as_map() const {
	return (AnyMap*)data;
}

Array<Any>* Any::as_array() const {
	return (Array<Any>*)data;
}

Any Any::array_get(int i) const {
	if (type != TYPE_ARRAY)
		throw Exception("not an array: " + type_name(type));
	return (*this)[i];
}

void Any::array_set(int i, const Any &value) {
	if (type == TYPE_NONE) {
		type = TYPE_ARRAY;
		data = new Array<Any>;
	}
	if (type != TYPE_ARRAY)
		throw Exception("not an array: " + type_name(type));
	(*this)[i] = value;
}

Any Any::map_get(const string &key) const {
	if (type != TYPE_HASH)
		throw Exception("not a hash map: " + type_name(type));
	return (*as_map())[key];
}

void Any::map_set(const string &key, const Any &value) {
	if (type == TYPE_NONE) {
		type = TYPE_HASH;
		data = new AnyMap;
	}
	if (type != TYPE_HASH)
		throw Exception("not a hash map: " + type_name(type));
	as_map()->set(key, value);
}
