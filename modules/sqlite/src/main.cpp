#include "lib/base/base.h"
#include "KabaExporter.h"
#include <stdio.h>
#include <sqlite3.h>


extern "C" {

__attribute__ ((visibility ("default")))
void export_symbols(kaba::Exporter* e) {
	//printf("<zlib export>\n");
	e->link("sqlite3_libversion_number", (void*)&sqlite3_libversion_number);
	e->link("_sqlite3_open_v2", (void*)&sqlite3_open_v2);
	e->link("_sqlite3_close", (void*)&sqlite3_close);
	e->link("_sqlite3_finalize", (void*)&sqlite3_finalize);
	e->link("_sqlite3_prepare", (void*)&sqlite3_prepare);
	e->link("_sqlite3_column_count", (void*)&sqlite3_column_count);
	e->link("_sqlite3_column_text", (void*)&sqlite3_column_text);
	e->link("_sqlite3_column_int", (void*)&sqlite3_column_int);
	e->link("_sqlite3_column_double", (void*)&sqlite3_column_double);
	e->link("_sqlite3_step", (void*)&sqlite3_step);
	e->link("_sqlite3_errmsg", (void*)&sqlite3_errmsg);
	e->link("_sqlite3_column_name", (void*)&sqlite3_column_name);
	e->link("_sqlite3_column_type", (void*)&sqlite3_column_type);
}
}


