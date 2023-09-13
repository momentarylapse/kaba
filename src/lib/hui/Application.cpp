/*
 * HuiApplication.cpp
 *
 *  Created on: 13.07.2014
 *      Author: michi
 */

#include "Application.h"

#include "hui.h"
#include "internal.h"
#include "../os/filesystem.h"

#ifdef OS_WINDOWS
#include <windows.h>
#endif

namespace hui
{

Flags operator|(Flags a, Flags b) {
	return (Flags)((int)a | (int)b);
}
int operator&(Flags a, Flags b) {
	return (int)a & (int)b;
}

//extern Callback _idle_function_;

GtkApplication *Application::application = nullptr;


base::map<string, string> Application::_properties_;

Flags Application::flags = Flags::NONE;
Path Application::filename;
Path Application::directory;
Path Application::directory_static;
Path Application::initial_working_directory;
bool Application::installed;
bool Application::adwaita_started = false;

Array<string> Application::_args;


void _init_global_css_classes_() {
	string css = R"foo(
.hui-no-padding { padding: 1px; }
.hui-more-padding { padding: 8px; }
.hui-big-font { font-size: 125%; }
.hui-huge-font { font-size: 150%; }
.hui-small-font { font-size: 75%; }
.hui-no-border { border-style: none; }
)foo";
#if GTK_CHECK_VERSION(4,0,0)
	css += ".hui-rotate-left { transform: rotate(90deg); } /* check? */\n";
	css += ".hui-rotate-right { transform: rotate(-90deg); }\n";
#endif

	auto *css_provider = gtk_css_provider_new();

#if GTK_CHECK_VERSION(4,0,0)
#if GTK_CHECK_VERSION(4,12,0)
	gtk_css_provider_load_from_string(css_provider, css.c_str());
#else
	gtk_css_provider_load_from_data(css_provider, (char*)css.data, css.num);
#endif
	gtk_style_context_add_provider_for_display(gdk_display_get_default(), GTK_STYLE_PROVIDER(css_provider),  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#else
	GError *error = nullptr;
	gtk_css_provider_load_from_data(css_provider, (char*)css.data, css.num, &error);
	if (error) {
		msg_error(string("css: ") + error->message + " (" + css + ")");
		return;
	}
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css_provider),  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
#endif
}

Application::Application(const string &app_name, const string &def_lang, Flags _flags) {
	flags = _flags;

#ifdef HUI_API_GTK
	g_set_prgname(app_name.c_str());
#endif

	guess_directories(_args, app_name);


	_InitInput_();

	separator = "\\";
	_using_language_ = false;
	if ((flags & Flags::NO_ERROR_HANDLER) == 0)
		SetDefaultErrorHandler(nullptr);

	if (os::fs::exists(directory | "config.txt"))
		config.load(directory | "config.txt");


	if ((flags & Flags::DONT_LOAD_RESOURCE) == 0)
		load_resource(directory_static | "hui_resources.txt");

	if (def_lang.num > 0)
		set_language(config.get_str("Language", def_lang));


	// default "logo" used for "about" dialog (full path)
#ifdef OS_LINUX
	if (os::fs::exists(directory_static | "icons" | "hicolor" | "scalable" | "apps" | (app_name + ".svg")))
		set_property("logo", str(directory_static | "icons" | "hicolor" | "scalable" | "apps" | (app_name + ".svg")));
	else if (os::fs::exists(directory_static | "icon.svg"))
		set_property("logo", str(directory_static | "icon.svg"));
	else
#endif
	if (os::fs::exists(directory_static | "icon.png"))
		set_property("logo", str(directory_static | "icon.png"));
	else if (os::fs::exists(directory_static | "icon.ico"))
		set_property("logo", str(directory_static | "icon.ico"));


	// default "icon" used for windows (just name)
	set_property("icon", app_name);
}

Application::~Application() {
	//foreachb(Window *w, _all_windows_)
	//	delete(w);
	if (config.changed)
		config.save(directory | "config.txt");
	if ((msg_inited) /*&& (HuiMainLevel == 0)*/)
		msg_end();

#if GTK_CHECK_VERSION(4,0,0)
	if (application)
		g_object_unref(application);
#endif
}

Path strip_dev_dirs(const Path &p) {
	if (p.basename() == "build")
		return p.parent();
	if (p.basename() == "Debug")
		return strip_dev_dirs(p.parent());
	if (p.basename() == "Release")
		return strip_dev_dirs(p.parent());
	if (p.basename() == "x64")
		return p.parent();
	if (p.basename() == "Unoptimized")
		return p.parent();
	return p;
}

//   filename -> executable file
//   directory ->
//      NONINSTALLED:  binary dir
//      INSTALLED:     ~/.MY_APP/      <<< now always this
//   directory_static ->
//      NONINSTALLED:  binary dir/static/
//      INSTALLED:     /usr/local/share/MY_APP/
//   initial_working_directory -> working dir before running this program
void Application::guess_directories(const Array<string> &arg, const string &app_name) {

	initial_working_directory = os::fs::current_directory();
	installed = false;


	// executable file
#if defined(OS_LINUX) || defined(OS_MINGW) //defined(__GNUC__) || defined(OS_LINUX)
	if (arg.num > 0)
		filename = arg[0];
#else // OS_WINDOWS
	char *ttt = nullptr;
	int r = _get_pgmptr(&ttt);
	filename = ttt;
	hui_win_instance = (void*)GetModuleHandle(nullptr);
#endif


	// first, assume a local/non-installed version
	directory = initial_working_directory; //strip_dev_dirs(filename.parent());
	directory_static = directory | "static";

#ifdef INSTALL_PREFIX
	// our build system should define this:
	Path prefix = INSTALL_PREFIX;
#else
	// oh no... fall-back
	Path prefix = "/usr/local";
#endif

	#if defined(OS_LINUX) || defined(OS_MINGW) //defined(__GNUC__) || defined(OS_LINUX)
		// installed version?
		if (filename.is_in(prefix) or (filename.str().find("/") < 0)) {
			installed = true;
			directory_static = prefix | "share" | app_name;
		}

		directory = format("%s/.%s/", getenv("HOME"), app_name);
		os::fs::create_directory(directory);
	#endif
}

#if GTK_CHECK_VERSION(4,0,0)
static bool keep_running = true;
#endif

int Application::run() {
#if GTK_CHECK_VERSION(4,0,0)
	return g_application_run(G_APPLICATION(application), 0, nullptr);
#else
	gtk_main();
	on_end();
	return 0;
#endif
}


static std::function<void()> _run_after_gui_init_func_;

#if GTK_CHECK_VERSION(4,0,0)
static void on_gtk_application_activate(GApplication *_g_app, gpointer user_data) {
	auto app = reinterpret_cast<Application*>(user_data);
	_init_global_css_classes_();
	if (_run_after_gui_init_func_)
		_run_after_gui_init_func_();
	else
		app->on_startup(app->_args);
}
#endif

AppStatus Application::on_startup_before_gui_init(const Array<string> &args) {
	return AppStatus::RUN;
}

void Application::run_after_gui_init(std::function<void()> f) {
	_run_after_gui_init_func_ = f;
}

int Application::try_execute(const Array<string> &args) {
	auto status = on_startup_before_gui_init(args);
	if (status == AppStatus::END)
		return 0;
	if (status == AppStatus::AUTO and !_run_after_gui_init_func_)
		return 0;
	_MakeUsable_();
#if GTK_CHECK_VERSION(4,0,0)
	g_signal_connect(application, "activate", G_CALLBACK(on_gtk_application_activate), this);
	return g_application_run(G_APPLICATION(application), 0, nullptr);
#else
	if (_run_after_gui_init_func_) {
		_run_after_gui_init_func_();
		return run();
	}
	if (on_startup(args) == AppStatus::RUN)
		return run();
	return 0;
#endif
}

void Application::end() {
	set_idle_function(nullptr);
#if GTK_CHECK_VERSION(4,0,0)
	keep_running = false;
#else
	gtk_main_quit();
#endif
}

void Application::do_single_main_loop() {
	// push idle function
	//Callback _if_ = _idle_function_;

	//SetIdleFunction(NULL);
	int counter = 0;
	do {
		g_main_context_iteration(nullptr, false);
#if !GTK_CHECK_VERSION(4,0,0)
		gtk_main_iteration_do(false);
#endif
		counter ++;
		if (counter > 5)
			break;
#if GTK_CHECK_VERSION(4,0,0)
	} while (g_main_context_pending(nullptr));
#else
	} while (gtk_events_pending());
#endif


	// pop idle function
	//SetIdleFunction(_if_);
}


void Application::set_property(const string &name, const string &value) {
	_properties_.set(name, value);
}

string Application::get_property(const string &name) {
	try {
		return _properties_[name];
	} catch(...) {
		return "";
	}
}

void Application::about_box(Window *win) {
	hui::about_box(win);
}

};
