/*
 * crypto.h
 *
 *  Created on: 16.01.2013
 *      Author: michi
 */

#ifndef _CRYPTO_INCLUDED_
#define _CRYPTO_INCLUDED_

struct Crypto {
	Crypto() {}
	void _cdecl from_str(const string &s);
	vli n, k;
	bytes _cdecl encrypt(const bytes &s) const;
	bytes _cdecl decrypt(const bytes &s, bool cut = true) const;
	string _cdecl str();
};

void _cdecl CryptoCreateKeys(Crypto &key1, Crypto &key2, const string &type, int bits);

#endif /* _CRYPTO_INCLUDED_ */
