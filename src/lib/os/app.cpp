//
// Created by michi on 7/23/25.
//

#include "app.h"
#include "filesystem.h"


namespace os::app {

	Path filename;
	Path directory_dynamic;
	Path directory_static;
	Path initial_working_directory;
	bool installed = false;

	//   filename -> executable file
	//   directory_dynamic ->
	//      NONINSTALLED:  binary dir
	//      INSTALLED:     ~/.MY_APP/      <<< now always this
	//   directory_static ->
	//      NONINSTALLED:  binary dir/static/
	//      INSTALLED:     /usr/local/share/MY_APP/
	//   initial_working_directory -> working dir before running this program
	void detect(const Array<string> &arg, const string &app_name) {

		initial_working_directory = os::fs::current_directory();
		installed = false;


		// executable file
#if defined(OS_LINUX) || defined(OS_MAC) || defined(OS_MINGW) //defined(__GNUC__) || defined(OS_LINUX)
		if (arg.num > 0)
			filename = arg[0];
#else // OS_WINDOWS
		char *ttt = nullptr;
		int r = _get_pgmptr(&ttt);
		filename = ttt;
		hui_win_instance = (void*)GetModuleHandle(nullptr);
#endif


		// first, assume a local/non-installed version
		directory_dynamic = initial_working_directory; //strip_dev_dirs(filename.parent());
		directory_static = directory_dynamic | "static";

#ifdef INSTALL_PREFIX
		// our build system should define this:
		Path prefix = INSTALL_PREFIX;
#else
		// oh no... fall-back
		Path prefix = "/usr/local";
#endif

#if defined(OS_LINUX) || defined(OS_MAC) || defined(OS_MINGW) //defined(__GNUC__) || defined(OS_LINUX)
		// installed version?
		if (filename.is_in(prefix) or (filename.str().find("/") < 0)) {
			installed = true;
			directory_static = prefix | "share" | app_name;
		}

		// inside an AppImage?
		if (getenv("APPIMAGE")) {
			installed = true;
			directory_static = Path(getenv("APPDIR")) | "usr" | "share" | app_name;
		}

		// inside MacOS bundle?
		if (str(filename).find(".app/Contents/MacOS/") >= 0) {
			installed = true;
			directory_static = filename.parent().parent() | "Resources";
		}

		directory_dynamic = format("%s/.%s/", getenv("HOME"), app_name);
		os::fs::create_directory(directory_dynamic);
#endif
	}


}

