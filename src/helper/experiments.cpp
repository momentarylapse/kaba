/*
 * experiments.cpp
 *
 *  Created on: 21 Jul 2022
 *      Author: michi
 */

#include "experiments.h"
#include "../lib/os/msg.h"
#include "../lib/base/optional.h"
#include "../lib/base/variant.h"
#include "../lib/base/iter.h"
#include "../lib/math/vec3.h"
#include "../lib/kaba/kaba.h"
#include "../lib/image/color.h"
#include "../lib/math/complex.h"
#include "../lib/kaba/compiler/Compiler.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

namespace kaba {
	void* get_nice_memory(int64 size, bool executable, Module *module);
}


void show_opt(const base::optional<string> &o) {
	if (o.has_value())
		msg_write(o.value());
	else
		msg_write("NO VALUE");
}

struct S {
	int a;
	string s;
};

void test_optional() {
	base::optional<string> o, p;
	o = "hallo";
	show_opt(o);
	p = o;
	show_opt(p);
	o = base::None;
	show_opt(o);



	auto x = S{13, "hallo"};
	auto [a,b] = x;
	msg_write(a);
	msg_write(b);


	Array<int> aa = {13, 14, 15, 16};
	for (auto i: aa)
		msg_write(i);
	for (auto [k,i]: enumerate(aa))
		msg_write(format("%d => %d", k, i));
	for (auto&& [k,i]: enumerate(aa))
		i += 13;
	for (auto [k,i]: enumerate(aa))
		msg_write(format("%d => %d", k, i));
}

void test_variant() {
	base::variant<int, float> v;
	msg_write(v.index());
	v = 13;
	msg_write(v.index());
	msg_write(v.get<int>());
	v = 13.1f;
	msg_write(v.index());
	msg_write(str(v.get<float>()));
	msg_write(v.get<int>());
}

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

/*string ggg(string& s) {
	return s + ".";
}*/

int skdjfhsjkdfh;

struct XXX {
	int i[128];
};

string kjhsdf, kjhsdf2;

// https://docs.microsoft.com/en-us/cpp/build/prolog-and-epilog?view=msvc-160

[[gnu::noinline]]
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
void f_xxx(int a, int b, int c, int d, int e, int f, int g, int h) {
	//int e = 0, f = 0;
	//msg_write(format("xxx  %d %d %d %d %d %d", a, b, c, d, e, f));
	a = g;
	b = h;
}

void ff1(int a) {
	msg_write(a);
}

void fff2() {
	int a = 8;
	//f_xxx(1, 2, 3, 4, 5, 6, 7, a);
	//f_xxx(1, 2, 3, 4);
	//msg_write("hallo");
	int b = 13;
	int c = a + b;
	int64 d = 4;
	int64 e = 7;
	ff1(c);
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

int f_add(int a, int b) {
	return a + b;
}

[[gnu::noinline]]
void ffff3() {
	string x = ggg(13);
}

void ffff4() {
#ifdef CPU_ARM64
	void *a, *b, *ret, *ff;
	__asm__("mov x0, %1\n"
		"mov x1, %2\n"
		"mov x8, %3\n"
		"mov x7, %0\n"
		"blr x7"
		 : : "r"(ff), "r"(a), "r"(b), "r"(ret) : "r0", "r1", "r8", "r7");
#endif
}

float ffff5(float a, float b) {
	return a + b;
}

bool ffff6(int a, int b) {
	return a > b;
}

void ffff7(int a, int b) {
	bool x = a > b;
	if (x)
		ffff4();
}


vec3 ffv() {
	return vec3(1,2,3);
}

void ffff8() {
	vec3 v = ffv();
}

void ffff9() {
	int dummy[1024];
	int i = 13;
	dummy[2] = 0;
	ffff8();
	i = 14;
}

void do_experiments() {
#if 0
	msg_write(str(13.3f));
	Array<int> i =  {1,2,3};
	msg_write(str(i));
	Array<string> ss = {"hallo", "b", "c"};
	msg_write(str(ss));
	//kaba::link_external("xxx_delete", (void*)&xxx_delete);

	msg_write(disassemble((void*)&xxx_delete0, -1));
	//msg_write(disassemble((void*)&fff, 30));
	//msg_write(disassemble((void*)&fff2, -1));
	//msg_write(disassemble((void*)&ggg, -1));
	//msg_write(disassemble(kaba::mf(&CCC::ff), -1));
#endif
	//msg_write(disassemble((void*)&f_add, 64));
	msg_write(disassemble((void*)&fff2, 64));
	//msg_write(disassemble((void*)&f_xxx, -1));
	//msg_write(disassemble((void*)&ffff3, 64));
	//msg_write(disassemble((void*)&ffff9, 160));

	return;

	//test_optional();
	//test_variant();

#ifdef CPU_ARM64
	auto m = kaba::default_context->create_empty_module("-test-");

	kaba::Compiler c(m.get());
	//c.allocate_memory()
	//c._compile();
	auto mem = kaba::get_nice_memory(4096, true, m.get());
	msg_write(p2s(mem));

	auto mi = (unsigned int*)mem;
	msg_write("A");
#ifdef OS_MAC
	pthread_jit_write_protect_np(0);
#endif
	msg_write("A2");
	mi[0] = 0xd10043ff;
	mi[1] = 0xb9000fe0;
	mi[2] = 0xb9000be1;
	mi[3] = 0xb9400fe8;
	mi[4] = 0xb9400be9;
	mi[5] = 0x0b090100;
	mi[6] = 0x910043ff;
	mi[7] = 0xd65f03c0;
	msg_write("B");
#ifdef OS_MAC
	pthread_jit_write_protect_np(1);
#endif
	msg_write("B2");

	using funcp = int(*)(int, int);
	auto f = (funcp)mem;

	int x = (*f)(1, 2);
	msg_write(x);
#endif
}

#pragma GCC diagnostic pop



