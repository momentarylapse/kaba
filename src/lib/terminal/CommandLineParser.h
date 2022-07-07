/*
 * CommandLineParser.h
 *
 *  Created on: Feb 12, 2021
 *      Author: michi
 */

#pragma once

#include "../base/base.h"
#include <functional>


class CommandLineParser {
public:
	using Callback = std::function<void()>;
	using CallbackString = std::function<void(const string&)>;
	struct Option {
		string name;
		string parameter;
		Callback callback;
		CallbackString callback_param;
	};
	Array<Option> options;
	void option(const string &name, Callback cb);
	void option(const string &name, const string &p, CallbackString cb);
	string _info;
	void info(const string &i);
	void show();
	Array<string> arg;
	void parse(const Array<string> &_arg);
};


