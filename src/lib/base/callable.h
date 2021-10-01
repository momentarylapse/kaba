/*
 * callable.h
 *
 *  Created on: Sep 30, 2021
 *      Author: michi
 */

#pragma once

#include "base.h"

class EmptyCallableError : public Exception {
public:
	EmptyCallableError() : Exception("empty callable") {}
};

template<typename Sig>
class Callable;

template<typename R, typename ...A>
class Callable<R(A...)> {
public:
	enum class Type {
		EMPTY,
		FUNCTION_POINTER
	};
	Type type;
	typedef R t_func(A...);
	t_func *p;

	Callable() {
		type = Type::EMPTY;
		p = nullptr;
	}

	Callable(t_func *_p) {
		type = Type::FUNCTION_POINTER;
		p = _p;
	}

	R operator()(A... args) {
		if (type == Type::FUNCTION_POINTER) {
			return p(args...);
		} else {
			throw EmptyCallableError();
		}
	}
};

