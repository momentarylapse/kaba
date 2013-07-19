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
	void _cdecl from_str(const string &s);
	vli n, k;
	string _cdecl Encrypt(const string &s);
	string _cdecl Decrypt(const string &s, bool cut = true);
	string _cdecl str();

	void _cdecl __init__();
	void _cdecl __delete__();
};

void _cdecl CryptoCreateKeys(Crypto &key1, Crypto &key2, const string &type, int bits);

#endif /* _CRYPTO_INCLUDED_ */
