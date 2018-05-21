/*----------------------------------------------------------------------------*\
| CHui                                                                         |
| -> Heroic User Interface                                                     |
| -> abstraction layer for GUI                                                 |
|   -> Windows (default api) or Linux (Gtk+)                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2010.06.21 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/


#include "hui.h"
#include "../file/file.h"


#include <stdio.h>
#include <signal.h>

#include "internal.h"
#ifdef OS_WINDOWS
	#include <shlobj.h>
	#include <winuser.h>
	#include <direct.h>
	#include <commctrl.h>
	#include <tchar.h>
	#pragma comment(lib,"winmm.lib")
	#ifdef HUI_API_GTK
		#pragma comment(lib,"gtk-win32-3.0.lib")
		#pragma comment(lib,"glib-2.0.lib")
		#pragma comment(lib,"pango-1.0.lib")
		#pragma comment(lib,"pangowin32-1.0.lib")
		#pragma comment(lib,"cairo.lib")
		#pragma comment(lib,"pangocairo-1.0.lib")
		#pragma comment(lib,"gdk-win32-3.0.lib")
		#pragma comment(lib,"gdk_pixbuf-2.0.lib")
		#pragma comment(lib,"gobject-2.0.lib")
	#else
		#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
	#endif
	#pragma warning(disable : 4995)
#endif
#ifdef OS_LINUX
#if HAS_LIB_XLIB
	#include <gdk/gdkx.h>
#endif
#endif

namespace hui
{


string Version = "0.6.7.0";


#ifdef OS_WINDOWS
	HINSTANCE hui_win_instance;
#endif
#ifdef HUI_API_WIN
	HFONT hui_win_default_font;
	HICON hui_win_main_icon;
#endif
#ifdef OS_LINUX
	Display* x_display;
#endif




bool _screen_opened_ = false;

// HUI configuration
string ComboBoxSeparator;




#ifdef HUI_API_GTK
	void *invisible_cursor = NULL;
#endif

Array<HuiImage> _all_images_;



Array<string> make_args(int num_args, char *args[])
{
	Array<string> a;
	for (int i=0; i<num_args; i++)
		a.add(args[i]);
	return a;
}


//----------------------------------------------------------------------------------
// system independence of main() function



}


int hui_main(const Array<string> &);

// for a system independent usage of this library

#ifdef OS_WINDOWS

#ifdef _CONSOLE

int _tmain(int NumArgs, _TCHAR *Args[])
{
	return hui_main(MakeArgs(NumArgs, Args));
}

#else

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	Array<string> a;
	a.add("-dummy-");
	string s = lpCmdLine;
	if (s.num > 0){
		if ((s[0] == '\"') and (s.back() == '\"'))
			s = s.substr(1, -2);
		a.add(s);
	}
	return hui_main(a);
}

#endif

#endif
#if  defined(OS_LINUX) || defined(OS_MINGW)

int main(int NumArgs, char *Args[])
{
	return hui_main(hui::make_args(NumArgs, Args));
}

#endif




// usage:
//
// int hui_main(const Array<string> &arg)
// {
//     HuiInit();
//     ....
//     return HuiRun();
// }


namespace hui
{



void _MakeUsable_()
{
	if (_screen_opened_)
		return;
#ifdef HUI_API_WIN

	//InitCommonControls(); comctl32.lib
	CoInitialize(NULL);
	//WinStandartFont=CreateFont(8,0,0,0,FW_NORMAL,FALSE,FALSE,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH|FF_SWISS,"MS Sans Serif");
	hui_win_default_font=(HFONT)GetStockObject(DEFAULT_GUI_FONT);

	hui_win_main_icon=ExtractIcon(hui_win_instance,sys_str(AppFilename),0);

#endif
#ifdef HUI_API_GTK
	gtk_init(NULL, NULL);
	#ifdef OS_LINUX
#if HAS_LIB_XLIB
		x_display = XOpenDisplay(0);
#endif
	#endif

#if GTK_CHECK_VERSION(3,16,0)
	invisible_cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_BLANK_CURSOR);
#else
	invisible_cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
#endif

#endif
	_screen_opened_ = true;
}




static int _current_image_no_ = 0;

string SetImage(const Image &image)
{
	HuiImage img;
	img.type = 1;
	img.image = image;
	img.filename = format("image:%d", _current_image_no_ ++);
	_all_images_.add(img);
	return img.filename;
}

void DeleteImage(const string &name)
{
	for (int i=0;i<_all_images_.num;i++)
		if (_all_images_[i].filename == name)
			_all_images_.erase(i);
}


};
