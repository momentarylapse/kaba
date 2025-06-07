#include "lib/base/base.h"
//#include "lib/kaba/kaba.h"
//#include "lib/kaba/lib/lib.h"
#include <stdio.h>
#include <clocale>
#include <ncurses.h>

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


/*func extern _initscr() -> void*
func extern curs_set(mode: int) -> int
func extern has_colors() -> bool
func extern start_color() -> int
func extern use_default_colors() -> int
func extern init_pair(i: int, a: Color, b: Color) -> int
func extern _waddnstr(w: void*, p: u8*, n: int) -> int
func extern _waddnwstr(w: void*, p: int*, n: int) -> int
func extern _wrefresh(w: void*) -> int
func extern _getmaxx(w: void*) -> int
func extern _getmaxy(w: void*) -> int
func extern _wmove(w: void*, y: int, x: int) -> int
func extern _wgetch(w: void*) -> int
func extern _wclear(w: void*) -> int
func extern _newwin(h: int, w: int, y: int, x: int) -> void*
func extern _wborder(w: void*, ls: int, rs: int, ts: int, bs: int, tl: int, tr: int, bl: int, br: int) -> int
func extern _wattr_on(w: void*, a: int, xxx: void*) -> int
func extern _wattr_off(w: void*, a: int, xxx: void*) -> int
func extern _wbkgd(w: void*, c: int) -> int
func extern _waddch(w: void*, c: int) -> int
func extern endwin() -> int
func extern raw() -> int
func extern noraw() -> int
func extern echo() -> int
func extern noecho() -> int
func extern _keypad(w: void*, b: bool) -> int

func extern _setlocale(category: int, locale: u8*) -> u8*  */


void export_symbols(KabaExporter* e) {
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


