/*
 * HuiApplication.h
 *
 *  Created on: 13.07.2014
 *      Author: michi
 */

#ifndef HUIAPPLICATION_H_
#define HUIAPPLICATION_H_

#include "../base/base.h"
#include "../base/map.h"
#include "../os/path.h"
#include "common.h"

namespace hui {

class Window;

enum {
	FLAG_DONT_LOAD_RESOURCE = 1,
	FLAG_SILENT = 2,
	FLAG_NO_ERROR_HANDLER = 4,
	FLAG_UNIQUE = 16,
};

class Application : public VirtualBase {
public:
	Application(const string &app_name, const string &def_lang, int flags);
	~Application() override;

	virtual bool on_startup(const Array<string> &arg) = 0;
	virtual void on_end() {}

	static void end();

	int run();
	static void do_single_main_loop();

	static void guess_directories(const Array<string> &arg, const string &app_name);



	static void _cdecl about_box(Window *win);
	static void _cdecl set_property(const string &name, const string &value);
	static string get_property(const string &name);

	static base::map<string, string> _properties_;

	static Path filename;
	static Path directory;			// dir of changeable files (ie. ~/.app/)
	static Path directory_static;	// dir of static files (ie. /usr/shar/app)
	static Path initial_working_directory;
	static bool installed; // installed into system folders?

	static Array<string> _args;

	static GtkApplication *application;
	int try_execute(const Array<string> &args);
};

}

#define HUI_EXECUTE(APP_CLASS) \
int hui_main(const Array<string> &args) { \
	APP_CLASS::_args = args; \
	APP_CLASS *app = new APP_CLASS; \
	int r = app->try_execute(args); \
	delete app; \
	return r; \
}

#endif /* HUIAPPLICATION_H_ */
