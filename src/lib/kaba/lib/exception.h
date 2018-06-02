/*
 * exception.h
 *
 *  Created on: Jan 27, 2018
 *      Author: michi
 */

#ifndef SRC_LIB_KABA_LIB_EXCEPTION_H_
#define SRC_LIB_KABA_LIB_EXCEPTION_H_

#include "../../base/base.h"

namespace Kaba
{

class KabaException
{
public:
	string text;
	KabaException(){}
	KabaException(const string &message);
	virtual ~KabaException(){}
	void _cdecl __init__(const string &message);
	virtual _cdecl void __delete__();
	virtual _cdecl string message();
};

void _cdecl kaba_raise_exception(KabaException *kaba_exception);



#define KABA_EXCEPTION_WRAPPER(CODE) \
try{ \
	CODE; \
}catch(::Exception &e){ \
	kaba_raise_exception(new KabaException(e.message())); \
}


}


#endif /* SRC_LIB_KABA_LIB_EXCEPTION_H_ */
