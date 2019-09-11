/*
 * Constant.cpp
 *
 *  Created on: 18.02.2019
 *      Author: michi
 */
#include "../kaba.h"
#include "../asm/asm.h"
#include "../../file/file.h"
#include <stdio.h>

namespace Kaba{



Value::Value() {
	type = TypeVoid;
}

Value::~Value() {
	clear();
}

bool Value::can_init(const Class *t) {
	if (!t->needs_constructor())
		return true;
	if (t->is_super_array())
		return true;
	return false;
}

void Value::init(const Class *_type) {
	clear();
	type = _type;

	if (type->is_super_array()) {
		value.resize(sizeof(DynamicArray));
		as_array().init(type->parent->size);
	} else {
		value.resize(max(type->size, (long long)16));
	}
}

void Value::clear() {
	if (type->is_super_array())
		as_array().simple_clear();

	value.clear();
	type = TypeVoid;
}

void Value::set(const Value &v) {
	init(v.type);
	if (type->is_super_array()) {
		as_array().simple_resize(v.as_array().num);
		memcpy(as_array().data, v.as_array().data, as_array().num * type->parent->size);

	} else {
		// plain old data
		memcpy(p(), v.p(), type->size);
	}
}

void* Value::p() const {
	return value.data;
}

int& Value::as_int() const {
	return *(int*)value.data;
}

long long& Value::as_int64() const {
	return *(long long*)value.data;
}

float& Value::as_float() const {
	return *(float*)value.data;
}

double& Value::as_float64() const {
	return *(double*)value.data;
}

complex& Value::as_complex() const {
	return *(complex*)value.data;
}

string& Value::as_string() const {
	return *(string*)value.data;
}

DynamicArray& Value::as_array() const {
	return *(DynamicArray*)p();
}

int map_size_complex(void *p, const Class *type) {
	if (type == TypeCString)
		return strlen((char*)p) + 1;
	if (type->is_super_array()) {
		int size = config.super_array_size;
		DynamicArray *ar = (DynamicArray*)p;
		if (type->parent->is_super_array()) {
			for (int i=0; i<ar->num; i++)
				size += map_size_complex((char*)ar->data + i * ar->element_size, type->parent);
			return size;
		}

		return config.super_array_size + (ar->num * type->parent->size);
	}
	return type->size;
}

int Value::mapping_size() const {
	return map_size_complex(p(), type);
}

int map_into_complex(char *memory, char *addr, void *p, const Class *type) {
	if (type->is_super_array()) {
		DynamicArray *ar = (DynamicArray*)p;

		int size = ar->element_size * ar->num;
		int data_offset = config.super_array_size;

		AAAAAAAAAAA needs to "allocate" range.... param!

		*(void**)&memory[0] = addr + data_offset; // .data
		*(int*)&memory[config.pointer_size    ] = ar->num;
		*(int*)&memory[config.pointer_size + 4] = 0; // .reserved
		*(int*)&memory[config.pointer_size + 8] = ar->element_size;

		if (type->parent->is_super_array()) {
			for (int i=0; i<ar->num; i++) {
				int el_offset = i * ar->element_size;
				map_into_complex(memory + data_offset + el_offset, addr + data_offset + el_offset, (char*)ar->data + el_offset, type->parent);
			}

		} else {
			memcpy(&memory[data_offset], ar->data, size);
		}
		return config.super_array_size;
	} else if (type == TypeCString) {
		strcpy(memory, (char*)p);
		return strlen((char*)p) + 1;
	} else {
		memcpy(memory, p, type->size);
	}
	return type->size;
}

void Value::map_into(char *memory, char *addr) const {
	map_into_complex(memory, addr, p(), type);
}

string Value::str() const {
	return type->var2str(value.data);
}

Constant::Constant(const Class *_type, SyntaxTree *_owner) {
	init(_type);
	name = "-none-";
	owner = _owner;
	used = false;
	address = nullptr;
}

string Constant::str() const {
	return Value::str();
}

}

