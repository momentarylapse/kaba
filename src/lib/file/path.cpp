/*
 * path.cpp
 *
 *  Created on: Jul 30, 2020
 *      Author: michi
 */

#include "path.h"
#include "file_op.h"

//const string SEPARATOR = "/";
//const string SEPARATOR_OTHER = "\\";
// const version creates badly compiled code...
#define SEPARATOR "/"
#define SEPARATOR_OTHER "\\"

// internal format:
// * always /
// * no double //
// * ending / allowed

const Path Path::EMPTY;

Path::Path() {
}

Path::Path(const string &_s) {
	s = _s.replace(SEPARATOR_OTHER, SEPARATOR).replace("//", "/");
}

void Path::__init__() {
	new(this) Path();
}

void Path::__init_ext__(const string &s) {
	new(this) Path(s);
}

void Path::__delete__() {
	this->~Path();
}

void Path::operator =(const Path &p) {
	s = p.s;
}

void Path::operator <<=(const Path &p) {
	if (is_empty())
		s = p.s;
	else if (has_dir_ending())
		s += p.s;
	else
		s += SEPARATOR + p.s;
}

void Path::operator <<=(const string &p) {
	*this <<= Path(p);
}

Path Path::operator <<(const Path &p) const {
	Path temp = *this;
	temp <<= p;
	return temp;
}

Path Path::operator <<(const string &p) const {
	return *this << Path(p);
}

// * ignore / at the end
// * ignore recursion
bool Path::operator ==(const Path &p) const {
	return canonical().as_dir().s == p.canonical().as_dir().s;
}

bool Path::operator !=(const Path &p) const {
	return !(*this == p);
}


// transposes path-strings to the current operating system
// accepts windows and linux paths ("/" and "\\")
string Path::str() const {
#if defined(OS_WINDOWS) || defined(OS_MINGW)
	return s.replace("/", "\\");
#else
	return s.replace("\\", "/");
#endif
}

bool Path::is_relative() const {
	if (is_empty())
		return true;
	return (s.head(1) != SEPARATOR);
}

bool Path::is_in(const Path &p) const {
	string dir = p.canonical().as_dir().s;
	return s.head(dir.num) == dir;
}

bool Path::is_empty() const {
	return s.num == 0;
}

bool Path::has_dir_ending() const {
	return s.tail(1) == SEPARATOR;
}

string Path::basename() const {
	int i = s.rfind(SEPARATOR);
	if (i >= 0)
		return s.tail(s.num - i - 1);
	return s;
}

string Path::extension() const {
	int pos = basename().rfind(".");
	if (pos >= 0)
		return s.tail(s.num - pos - 1).lower();
	return "";
}

// ends with '/' or '\'
string Path::dirname() const {
	int i = s.rfind(SEPARATOR);
	if (i >= 0)
		return s.head(i + 1);
	return "";
}

Path Path::absolute() const {
	if (is_relative())
		return get_current_dir() << *this;
	return *this;
}

// cancel "/x/../"
// TODO deal with "./"
Path Path::canonical() const {
//	return _canonical_remove(0, true);
	auto p = s.explode(SEPARATOR);

	for (int i=1; i<p.num; i++) {
		if ((p[i] == "..") and (p[i-1] != "..")) {
			p.erase(i);
			p.erase(i - 1);
			i -= 2;
		}
	}

	return Path(implode(p, SEPARATOR));
}

// make sure the name ends with a slash
Path Path::as_dir() const {
	if (has_dir_ending())
		return *this;
	return Path(s + SEPARATOR);
}


Path Path::_canonical_remove(int n_remove, bool keep_going) const {
	auto xx = s.explode(SEPARATOR);
	while (xx.num > 0 and ((n_remove > 0) or keep_going)) {
		if (xx.back() == "") {
			n_remove ++;
		} else if (xx.back() == ".") {
			n_remove ++;
		} else if (xx.back() == "..") {
			n_remove += 2;
		}
		if (n_remove > 0) {
			xx.pop();
			n_remove --;
		}
	}
	if (xx.num == 0)
		return EMPTY;
	return Path(implode(xx, SEPARATOR)).as_dir();
}

Path Path::parent() const {
	return _canonical_remove(1, false);
}

// starts from root
// not including self
Array<Path> Path::all_parents() const {
	Array<Path> parents;
	auto p = parent();
	while (!p.is_empty()) {
		parents.add(p);
		p = p.parent();
	}
	return parents;
	string temp;
	for (int i=0; i<s.num-1; i++) {
		temp.add(s[i]);
		if (s[i] == '/')
			parents.add(Path(temp));
	}
	return parents;
}



template<> string _xf_str_(const string &f, const Path &value) {
	return value.str();
	/*auto ff = xf_parse(f);
	if (ff.type == 's') {
	} else {
		throw Exception("format evil (Path): " + f);
	}
	return ff.apply_justify(value.str());*/
}

template<> string _xf_str_(const string &f, Path value) { return _xf_str_<const Path&>(f, value); }
