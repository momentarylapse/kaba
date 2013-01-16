/*
 * crypto.h
 *
 *  Created on: 16.01.2013
 *      Author: michi
 */

#ifndef _CRYPTO_INCLUDED_
#define _CRYPTO_INCLUDED_

struct Crypto
{
	Crypto(){}
	void from_str(const string &s);
	vli n, k;
	string Encrypt(const string &s);
	string Decrypt(const string &s, bool cut = true);
	string str();

	void __init__();
};

void _cdecl CryptoCreateKeys(Crypto &key1, Crypto &key2, const string &type, int bits);

#endif /* _CRYPTO_INCLUDED_ */
