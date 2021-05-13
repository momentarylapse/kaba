/*
 * crypto.cpp
 *
 *  Created on: 16.01.2013
 *      Author: michi
 */

#include "../base/base.h"
#include "vli.h"
#include "crypto.h"
#include "../math/math.h"
#include <stdio.h>

int vli_count_bits(vli &v)
{
	int bits = 32 * (v.data.num - 1);
	for (int i=31;i>=0;i--)
		if ((v.data.back() & (1 << i)) != 0){
			bits += i;
			break;
		}
	return bits;
}

void vli_rand(vli &v, int bits)
{
	v.data.resize((bits-1)/32 + 1);
	for (int i=0;i<v.data.num;i++)
		for (int j=0;j<4;j++)
			v.data[i] ^= randi(255) << (j*8);
	if ((bits % 32) != 0)
		v.data.back() &= 0xffffffff >> (32 - (bits % 32));
}

bool miller_rabin_prime(vli &p, int count)
{
	// decompose
	int s = 0;
	vli d = p-1;
	// TODO improve me!!!
	while ((d.data[0] & 1) == 0){
		unsigned int rem;
		d.div(2, rem);
		s ++;
	}
	int bits = vli_count_bits(p);

	// trials...
	vli p_mm = p - 1;
	for (int i=0; i<count; i++){
		vli a;
		do{
			vli_rand(a, bits);
		}while ((a > p) || (a <= 1));

		vli rem;
		vli t = a.pow_mod(d, p);
		if (t == 1)
			continue;
		if (t == p_mm)
			continue;

		bool passed = true;
		for (int r=1;r<s;r++){
			t *= t;
			t.div(p, rem);
			t = rem;
			if (t == p_mm){
				passed = false;
				break;
			}
		}
		if (!passed)
			continue;
		return false;

	}
	return true;
}

void get_prime(vli &p, int bits)
{
	do{
		vli_rand(p, bits-1);
		p = p * 2;
		p += 1;
	}while (!miller_rabin_prime(p, 30));
}

bool find_coprime(vli &e, vli &phi)
{
	int bits = vli_count_bits(phi);
	do{
		vli_rand(e, bits);
	}while (e >= phi);

	while(e > 1){
		if (e.gcd(phi) == 1)
			return true;
		e -= 1;
	}
	return false;
}

bool find_mod_inverse(vli &d, vli &e, vli &phi)
{
	/*d = 1;
	while(d < e){
		vli n = d * e;
		vli rem;
		n._div(phi, rem);
		if (rem == 1)
			return true;
		d += 1;
	}
	return false;*/

	// via extended Euclidean algorithm
	vli a = e;
	vli b = phi;
	vli x = 0;
	vli y = 1;
	vli last_x = 1;
	vli last_y = 0;
	vli vli0 = 0;
	vli rem;
	while (b != vli0){
		vli q;
		a.div(b, rem);
		q = a;

		a = b;
		b = rem;
		vli temp = last_x;
		last_x = x;
		x = temp - q*x;
		temp = last_y;
		last_y = y;
		vli yy = temp - q*y;
		y = yy;
	}
	d = last_x;
	while (d.sign)
		d += phi;
	while (d > phi)
		d -= phi;

	vli ttt = d * e;
	ttt._div(phi, rem);
	if (rem != 1){
		printf("--- 1=%s  (d=%s)\n", rem.to_string().c_str(), d.to_string().c_str());
		return false;
	}
	return true;
}

void CryptoRSACreateKey_trial(Crypto &key1, Crypto &key2, int bits)
{
	vli p, q;
	get_prime(p, bits/2);
	get_prime(q, bits/2);
	vli n = p * q;
	vli phi = (p - 1) * (q - 1);
	vli d, e;
	if (!find_coprime(e, phi))
		throw "Crypto: no coprime found";
	if (!find_mod_inverse(d, e, phi))
		throw "Crypto: no inverse";
	key1.n = n;
	key1.k = e;
	key2.n = n;
	key2.k = d;
}

void CryptoCreateKeys(Crypto &key1, Crypto &key2, const string &type, int bits)
{
	if (type != "rsa")
		return;
	for (int i=0; i<100; i++){
		try{
			CryptoRSACreateKey_trial(key1, key2, bits);
			return;
		}catch(...){
		}
	}
	throw "Crypto: no key found...";
}

vli str2vli(const string &str, int offset, int bytes)
{
	string t = str.sub(offset, offset + bytes);
	t.resize(bytes);
	vli v;
	v.data.resize((bytes - 1) / 4 + 1);
	memcpy(v.data.data, t.data, bytes);
	return v;
}

string vli2str(vli &v, int bytes)
{
	string t;
	t.resize(bytes);
	memcpy(t.data, v.data.data, v.data.num * 4);
	return t;
}

string Crypto::Encrypt(const string &s)
{
	string r;
	int bytes = vli_count_bits(this->n) / 8;
	int offset = 0;
	do{
		vli m = str2vli(s, offset, bytes);
		m = m.pow_mod(this->k, this->n);
		r += vli2str(m, bytes + 1);
		offset += bytes;
	}while (offset < s.num);
	return r;
}

string Crypto::Decrypt(const string &s, bool cut)
{
	string r;
	int bytes = vli_count_bits(this->n) / 8;
	int offset = 0;
	do{
		vli m = str2vli(s, offset, bytes+1);
		m = m.pow_mod(this->k, this->n);
		r += vli2str(m, bytes);
		offset += bytes + 1;
	}while (offset < s.num);

	if (cut)
		for (int i=0;i<r.num;i++)
			if (r[i] == 0){
				r.resize(i);
				break;
			}
	return r;
}

void Crypto::from_str(const string &s)
{
	Array<string> ss = s.explode(":");
	if (ss.num != 2)
		return;
	string h1 = ss[0].unhex();
	n = str2vli(h1, 0, h1.num);
	string h2 = ss[1].unhex();
	k = str2vli(h2, 0, h2.num);
}

string Crypto::str()
{
	int bytes_n = (vli_count_bits(n) + 7) / 8;
	int bytes_k = (vli_count_bits(k) + 7) / 8;
	return vli2str(n, bytes_n).hex() + ":" + vli2str(k, bytes_k).hex();
	//return n.dump() + " " + k.dump();
}

void Crypto::__init__()
{
	new(this) Crypto;
}

void Crypto::__delete__()
{
	this->~Crypto();
}

