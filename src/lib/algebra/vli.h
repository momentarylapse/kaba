#ifndef _VLI_INCLUDED_
#define _VLI_INCLUDED_

class vli
{
public:
	vli();
	vli(const vli &v);
	vli(int v);
	vli(const string &str);

private:
	void shift_bits(int n);
	void shift_units(int n);
	void add_abs(const Array<unsigned int> &_data);
	void sub_abs(const Array<unsigned int> &_data);
	void mul_ui(unsigned int i);
	void normalize();

public:
	void operator = (const vli &v);
	void operator += (const vli &v);
	void operator -= (const vli &v);
	void operator *= (const vli &v)
	{	*this = *this * v;	}
	vli operator + (const vli &v) const
	{	vli r = *this; r += v;	return r;	}
	vli operator - (const vli &v) const
	{	vli r = *this; r -= v;	return r;	}
	vli operator * (const vli &v) const;
	void div(unsigned int divisor, unsigned int &remainder);
	void div(const vli &divisor, vli &remainder);
	int compare_abs(const vli &v) const;
	int compare(const vli &v) const;
	bool operator == (const vli &v) const;
	bool operator != (const vli &v) const
	{	return !(*this == v);	}
	bool operator < (const vli &v) const;
	bool operator <= (const vli &v) const
	{	return !(v < *this);	}
	bool operator > (const vli &v) const
	{	return (v < *this);	}
	bool operator >= (const vli &v) const
	{	return !(*this < v);	}
	string to_string() const;
	string dump() const;

	// higher functions
	vli pow(const vli &e) const;
	vli pow_mod(const vli &e, const vli &m) const;
	vli gcd(const vli &v) const;
	
	// kaba
	void __init__();
	void __delete__();
	void set_vli(const vli &v)
	{	*this = v;	}
	void set_int(int i)
	{	*this = i;	}
	void set_str(const string &s)
	{	*this = s;	}
	void idiv(const vli &d, vli &rem)
	{	div(d, rem);	}
	vli _div(const vli &d, vli &rem) const
	{	vli r = *this;	r.div(d, rem);	return r;	}

	// data
	bool sign;
	Array<unsigned int> data;
};

#endif
