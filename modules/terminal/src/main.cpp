#include <cstdio>

#include "lib/base/base.h"
#include "KabaExporter.h"
#include <termios.h>


extern "C" {

__attribute__ ((visibility ("default")))
void export_symbols(kaba::Exporter* e) {
	printf("<terminal export>\n");

	e->declare_class_size("Termios", sizeof(termios));
	e->declare_class_element("Termios.iflag", &termios::c_iflag);
	e->declare_class_element("Termios.oflag", &termios::c_oflag);
	e->declare_class_element("Termios.cflag", &termios::c_cflag);
	e->declare_class_element("Termios.lflag", &termios::c_lflag);

	// iflags
	e->declare_enum("Flags.IGNBRK", IGNBRK);
	e->declare_enum("Flags.BRKINT", BRKINT);
	e->declare_enum("Flags.PARMRK", PARMRK);
	e->declare_enum("Flags.ISTRIP", ISTRIP);
	e->declare_enum("Flags.INLCR", INLCR);
	e->declare_enum("Flags.IGNCR", IGNCR);
	e->declare_enum("Flags.ICRNL", ICRNL);
	e->declare_enum("Flags.IXON", IXON);
	e->declare_enum("Flags.IUTF8", IUTF8);

	// oflags
	e->declare_enum("Flags.OPOST", OPOST);
#ifdef OS_LINUX
	e->declare_enum("Flags.OXTABS", XTABS);
#else
	e->declare_enum("Flags.OXTABS", OXTABS);
#endif

	// lflags
	e->declare_enum("Flags.ISIG", ISIG);
	e->declare_enum("Flags.ICANON", ICANON);
	e->declare_enum("Flags.ECHO", ECHO);
	e->declare_enum("Flags.ECHONL", ECHONL);
	e->declare_enum("Flags.IEXTEN", IEXTEN);

	// cflags
	e->declare_enum("Flags.CSIZE", CSIZE);
	e->declare_enum("Flags.PARENB", PARENB);
	e->declare_enum("Flags.CS8", CS8);

	e->link("_tcgetattr", (void*)&tcgetattr);
	e->link("_tcsetattr", (void*)&tcsetattr);
}
}


