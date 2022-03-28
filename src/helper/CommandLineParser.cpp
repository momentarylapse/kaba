/*
 * CLIParser.cpp
 *
 *  Created on: Feb 12, 2021
 *      Author: michi
 */

#include "CommandLineParser.h"

#include "../lib/file/msg.h"


void CommandLineParser::option(const string &name, Callback cb) {
	options.add({name, "", cb, nullptr});
}

void CommandLineParser::option(const string &name, const string &p, CallbackString cb) {
	options.add({name, p, nullptr, cb});
}

void CommandLineParser::info(const string &i) {
	_info = i;
}

void CommandLineParser::show() {
	msg_write(_info);
	msg_write("");
	msg_write("options:");
	for (auto &o: options)
		if (o.parameter.num > 0)
			msg_write("  " + o.name + " " + o.parameter);
		else
			msg_write("  " + o.name);
}

void CommandLineParser::parse(const Array<string> &_arg) {
	for (int i=1; i<_arg.num; i++) {
		if (_arg[i].head(1) == "-") {
			bool found = false;
			for (auto &o: options) {
				if (sa_contains(o.name.explode("/"), _arg[i])) {
					if (o.parameter.num > 0) {
						if (_arg.num <= i + 1) {
							msg_error("parameter '" + o.parameter + "' expected after " + o.name);
							exit(1);
						}
						o.callback_param(_arg[i+1]);
						i ++;
					} else {
						o.callback();
					}
					found = true;
				}
			}
			if (!found) {
				msg_error("unknown option " + _arg[i]);
				exit(1);
			}
		} else {
			// rest
			arg = _arg.sub_ref(i);
			break;
		}
	}

}



