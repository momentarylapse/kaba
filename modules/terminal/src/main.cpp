#include "lib/base/base.h"
//#include "lib/kaba/kaba.h"
//#include "lib/kaba/lib/lib.h"
#include <stdio.h>
#include <termios.h>

class KabaExporter {
public:
	virtual ~KabaExporter() = default;
	virtual void declare_class_size(const string& name, int size) = 0;
	virtual void declare_enum(const string& name, int value) = 0;
	virtual void declare_class_element(const string& name, int offset) = 0;
	virtual void link(const string& name, void* p) = 0;
	virtual void link_virtual(const string& name, void* p, void* instance) = 0;
};


extern "C" {


void export_symbols(KabaExporter* e) {
	//printf("<terminal export>\n");
	e->link("_tcgetattr", (void*)&tcgetattr);
	e->link("_tcsetattr", (void*)&tcsetattr);
}
}


