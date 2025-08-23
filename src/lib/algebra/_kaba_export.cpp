#include "../base/base.h"
#include "vli.h"
#include "crypto.h"
#include "../kabaexport/KabaExporter.h"

class KabaVli : public vli {
public:
	void assign_string(const string& s) {
		*(vli*)this = s;
	}
	void assign_int(int i) {
		*(vli*)this = i;
	}
};

void export_package_vli(kaba::Exporter* e) {
	e->declare_class_size("Vli", sizeof(vli));
	e->link_class_func("Vli.__init__:Vli", &kaba::generic_init<vli>);
	e->link_class_func("Vli.__init__:Vli:i32", &kaba::generic_init_ext<vli, int>);
	e->link_class_func("Vli.__init__:Vli:string", &kaba::generic_init_ext<vli, const string&>);
	e->link_class_func("Vli.__delete__", &kaba::generic_delete<vli>);
	e->link_class_func("Vli.__assign__:Vli:Vli", &kaba::generic_assign<vli>);
	e->link_class_func("Vli.__assign__:Vli:string", &KabaVli::assign_string);
	e->link_class_func("Vli.__assign__:Vli:i32", &KabaVli::assign_int);
	e->link_class_func("Vli.__str__", &vli::to_string);
	e->link_class_func("Vli.compare", &vli::compare);
	e->link_class_func("Vli.idiv", &vli::idiv);
	e->link_class_func("Vli.div", &vli::_div);
	e->link_class_func("Vli.pow", &vli::pow);
	e->link_class_func("Vli.pow_mod", &vli::pow_mod);
	e->link_class_func("Vli.gcd", &vli::gcd);
	e->link_class_func("Vli.__eq__", &vli::operator==);
	e->link_class_func("Vli.__neq__", &vli::operator!=);
	e->link_class_func("Vli.__lt__", &vli::operator<);
	e->link_class_func("Vli.__gt__", &vli::operator>);
	e->link_class_func("Vli.__le__", &vli::operator<=);
	e->link_class_func("Vli.__ge__", &vli::operator>=);
	e->link_class_func("Vli.__add__", &vli::operator+);
	e->link_class_func("Vli.__sub__", &vli::operator-);
	e->link_class_func("Vli.__mul__", &vli::operator*);
	e->link_class_func("Vli.__iadd__", &vli::operator+=);
	e->link_class_func("Vli.__isub__", &vli::operator-=);
	e->link_class_func("Vli.__imul__", &vli::operator*=);
}

void export_package_crypto(kaba::Exporter* e) {
	e->declare_class_size("Crypto", sizeof(Crypto));
	e->declare_class_element("Crypto.n", &Crypto::n);
	e->declare_class_element("Crypto.k", &Crypto::k);
	e->link_class_func("Crypto.__init__", &kaba::generic_init<Crypto>);
	e->link_class_func("Crypto.__delete__", &kaba::generic_delete<Crypto>);
	e->link_class_func("Crypto.__assign__", &kaba::generic_assign<Crypto>);
	e->link_class_func("Crypto.__str__", &Crypto::str);
	e->link_class_func("Crypto.from_str", &Crypto::from_str);
	e->link_class_func("Crypto.encrypt", &Crypto::encrypt);
	e->link_class_func("Crypto.decrypt", &Crypto::decrypt);
	
	e->link_func("create_keys", &CryptoCreateKeys);
}

