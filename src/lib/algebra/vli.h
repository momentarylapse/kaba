#ifndef _VLI_INCLUDED_
#define _VLI_INCLUDED_

struct vli {
	vli();
	vli(const vli &v);
	vli(int v);
	vli(const string &str);
	~vli();

private:
	void shift_bits(int n);
	void shift_units(int n);
	void add_abs(const Array<unsigned int> &_data);
	void sub_abs(const Array<unsigned int> &_data);
	void mul_ui(unsigned int i);
	void normalize();

public:
	void _cdecl operator = (const vli &v);
	void _cdecl operator += (const vli &v);
	void _cdecl operator -= (const vli &v);
	void _cdecl operator *= (const vli &v)
	{	*this = *this * v;	}
	vli _cdecl operator + (const vli &v) const
	{	vli r = *this; r += v;	return r;	}
	vli _cdecl operator - (const vli &v) const
	{	vli r = *this; r -= v;	return r;	}
	vli _cdecl operator * (const vli &v) const;
	void _cdecl div(unsigned int divisor, unsigned int &remainder);
	void _cdecl div(const vli &divisor, vli &remainder);
	int _cdecl compare_abs(const vli &v) const;
	int _cdecl compare(const vli &v) const;
	bool _cdecl operator == (const vli &v) const;
	bool _cdecl operator != (const vli &v) const
	{	return !(*this == v);	}
	bool _cdecl operator < (const vli &v) const;
	bool _cdecl operator <= (const vli &v) const
	{	return !(v < *this);	}
	bool _cdecl operator > (const vli &v) const
	{	return (v < *this);	}
	bool _cdecl operator >= (const vli &v) const
	{	return !(*this < v);	}
	string _cdecl to_string() const;
	string _cdecl dump() const;

	// higher functions
	static vli pow(const vli &x, const vli &e);
	static vli pow_mod(const vli &x, const vli &e, const vli &m);
	static vli gcd(const vli &a, const vli &b);
	
	// kaba
	void _cdecl idiv(const vli &d, vli &rem)
	{	div(d, rem);	}
	vli _cdecl _div(const vli &d, vli &rem) const
	{	vli r = *this;	r.div(d, rem);	return r;	}

	// data
	bool sign;
	Array<unsigned int> data;
};

#endif
