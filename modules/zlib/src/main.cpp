#include "lib/base/base.h"
#include "KabaExporter.h"
#include <stdio.h>
#include <zlib.h>


extern "C" {


__attribute__ ((visibility ("default")))
void export_symbols(kaba::Exporter* e) {
	//printf("<zlib export>\n");
	e->link("_compressBound", (void*)&compressBound);
	e->link("_deflate", (void*)&deflate);
	e->link("_compress", (void*)&compress);
	e->link("_uncompress", (void*)&uncompress);
}
}


