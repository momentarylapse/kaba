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
	//Any(const HashMap &a);
	~Any();
	void _cdecl clear();
	string _cdecl str() const;
	int _cdecl _int() const;
	float _cdecl _float() const;
	bool _cdecl _bool() const;
	Any &_cdecl operator = (const Any &a);
	Any _cdecl operator + (const Any &a) const;
	Any _cdecl operator - (const Any &a) const;
	void _cdecl operator += (const Any &a);

	// array
	void _cdecl add(const Any &a);
	void _cdecl append(const Any &a);
	const Any &operator[] (int index) const;
	Any &operator[] (int index);
	Any &_cdecl back();

	// hash
	const Any &operator[] (const string &key) const;
	Any &operator[] (const string &key);

	// data
	int type;
	void *data;

	// kaba
	void _cdecl __init__();
	void _cdecl set(const Any &a){	*this = a;	}
	void _cdecl set_int(int i){	*this = i;	}
	void _cdecl set_float(float f){	*this = f;	}
	void _cdecl set_bool(bool b){	*this = b;	}
	void _cdecl set_str(const string &s){	*this = s;	}
	void _cdecl set_array(const Array<Any> &a){	*this = a;	}
	void _cdecl _add(const Any &a){	Any b = *this + a;	*this = b;	}
	void _cdecl _sub(const Any &a){	Any b = *this - a;	*this = b;	}
	Any _cdecl at(int i) const;
	void _cdecl aset(int i, const Any &value);
	Any _cdecl get(const string &key) const;
	void _cdecl hset(const string &key, const Any &value);
};


extern Any EmptyVar;
extern Any EmptyHash;
extern Any EmptyArray;

void print(const Any &a);



#endif

