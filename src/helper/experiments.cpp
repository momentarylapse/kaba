/*
 * experiments.cpp
 *
 *  Created on: 21 Jul 2022
 *      Author: michi
 */

#include "experiments.h"
#include "../lib/os/msg.h"
#include "../lib/kaba/kaba.h"
#include "../lib/image/color.h"
#include "../lib/math/complex.h"


float fff(int i, int j, int k, float f1, float f2) {
	return f1 + f2;
}

string disassemble(void* d, int size) {
	unsigned char* c = (unsigned char*)d;
	if (c[0] == 0xe9) {
		msg_write("(indirect)");
		int offset = *(int*)&c[1];
		return Asm::disassemble(c + offset + 5, size);
	}
	return Asm::disassemble(d, size);
}

void xxx_delete0(VirtualBase* v) {
	v->__delete_external__();
	v->__delete__();
	v->~VirtualBase();
	//delete v;
}

void xxx_delete(VirtualBase* v) {
	msg_write("...xxx_delete");// +p2s(v));
	delete v;
	//v->__delete_external__();
	//v->__delete__();
}

/*string ggg(string& s) {
	return s + ".";
}*/

int skdjfhsjkdfh;

struct XXX {
	int i[128];
};

string kjhsdf, kjhsdf2;

// https://docs.microsoft.com/en-us/cpp/build/prolog-and-epilog?view=msvc-160

string ggg(int i) {
	//skdjfhsjkdfh = i;
	kjhsdf = kjhsdf2;
	return kjhsdf;
}

color COLORX1, COLORX2, COLORX3;
complex COMPLEX1;
complex fff3() {
	return complex(COLORX3.r*rand(), COLORX2.r);
}

#ifdef OS_WINDOWS
__declspec(noinline)
#else
[[gnu::noinline]]
#endif
void f_xxx(int a, int b, int c, int d, int e, int f) {
	//int e = 0, f = 0;
	msg_write(format("xxx  %d %d %d %d %d %d", a, b, c, d, e, f));
}

void fff2() {
	f_xxx(1, 2, 3, 4, 5, 6);
	//f_xxx(1, 2, 3, 4);
	msg_write("hallo");
}

class CCC {
public:
	int a, b, c, d, e, f, g;
	XXX ff(int i) {
		a = 13;
		b = i;
		XXX x;
		return x;
	}
};

void do_experiments() {
	kaba::link_external("xxx_delete", (void*)&xxx_delete);

	msg_write(disassemble((void*)&xxx_delete0, -1));
	//msg_write(disassemble((void*)&fff, 30));
	//msg_write(disassemble((void*)&fff2, -1));
	//msg_write(disassemble((void*)&ggg, -1));
	//msg_write(disassemble(kaba::mf(&CCC::ff), -1));
}



