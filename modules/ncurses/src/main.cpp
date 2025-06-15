#include "lib/base/base.h"
#include "KabaExporter.h"
#include <stdio.h>
#include <clocale>
#include <ncurses.h>

extern "C" {


__attribute__ ((visibility ("default")))
void export_symbols(kaba::Exporter* e) {
	//printf("<ncurses export>\n");
	e->link("_initscr", (void*)&initscr);
	e->link("curs_set", (void*)&curs_set);
	e->link("has_colors", (void*)&has_colors);
	e->link("start_color", (void*)&start_color);
	e->link("use_default_colors", (void*)&use_default_colors);
	e->link("init_pair", (void*)&init_pair);
	e->link("_waddnstr", (void*)&waddnstr);
//	e->link("_waddnwstr", (void*)&waddnwstr);
	e->link("_wrefresh", (void*)&wrefresh);
	e->link("_getmaxx", (void*)&getmaxx);
	e->link("_getmaxy", (void*)&getmaxy);
	e->link("_wmove", (void*)&wmove);
	e->link("_wgetch", (void*)&wgetch);
	e->link("_wclear", (void*)&wclear);
	e->link("_newwin", (void*)&newwin);
	e->link("_wborder", (void*)&wborder);
	e->link("_wattr_on", (void*)&wattr_on);
	e->link("_wattr_off", (void*)&wattr_off);
	e->link("_wbkgd", (void*)&wbkgd);
	e->link("_waddch", (void*)&waddch);
	e->link("endwin", (void*)&endwin);
	e->link("raw", (void*)&raw);
	e->link("noraw", (void*)&noraw);
	e->link("echo", (void*)&echo);
	e->link("noecho", (void*)&noecho);
	e->link("_keypad", (void*)&keypad);
	
	e->link("_setlocale", (void*)&setlocale);
}
}


