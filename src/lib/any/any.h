#include "../file/file.h"

#ifndef __ANY_INCLUDED__
#define __ANY_INCLUDED__

enum
{
	TYPE_NONE,
	TYPE_INT,
	TYPE_FLOAT,
	TYPE_BOOL,
	TYPE_STRING,
	TYPE_ARRAY,
	TYPE_HASH
};

class HashPair;

class Any
{
public:
	Any();
	Any(const Any &a);
	Any(int i);
	Any(float f);
	Any(bool b);
	Any(const string &s);
	Any(const char *s);
	Any(const Array<Any> &a);
	Any(const Array<HashPair> &a);
	~Any();
	void clear();
	string dump() const;
	Any &operator = (const Any &a);
	Any operator + (const Any &a) const;
	Any operator - (const Any &a) const;
	void operator += (const Any &a);

	// array
	void add(const Any &a);
	void append(const Any &a);
	Any &operator[] (int index);
	Any &back();

	// hash
	Any operator[] (const string &key) const;
	Any &operator[] (const string &key);

	// data
	int type;
	void *data;

	// kaba
	void __init__();
	void set(const Any &a){	*this = a;	}
	void set_int(int i){	*this = i;	}
	void set_float(float f){	*this = f;	}
	void set_bool(bool b){	*this = b;	}
	void set_str(const string &s){	*this = s;	}
	void set_array(const Array<Any> &a){	*this = a;	}
	void _add(const Any &a){	Any b = *this + a;	*this = b;	}
	void _sub(const Any &a){	Any b = *this - a;	*this = b;	}
};

class HashPair
{
public:
	string key;
	Any value;
};


extern Any EmptyVar;
extern Any EmptyHash;
extern Any EmptyArray;

void print(const Any &a);


#define foreacha(a,p)	foreach(*(Array<Any>*)(a).data, p)



#endif

