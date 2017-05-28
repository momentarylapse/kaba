/*
 * hui_error.cpp
 *
 *  Created on: 26.06.2013
 *      Author: michi
 */

#include "hui.h"
#include "internal.h"



#ifdef _X_USE_NET_
	#include "../net/net.h"
#endif

#include <signal.h>

namespace hui
{

extern Callback _idle_function_, _error_function_;

void _HuiSignalHandler(int)
{
	_error_function_();
}

// apply a function to be executed when a critical error occures
void SetErrorFunction(const Callback &function)
{
	_error_function_ = function;
	if (function){
		signal(SIGSEGV, &_HuiSignalHandler);
		/*signal(SIGINT, &_HuiSignalHandler);
		signal(SIGILL, &_HuiSignalHandler);
		signal(SIGTERM, &_HuiSignalHandler);
		signal(SIGABRT, &_HuiSignalHandler);*/
		/*signal(SIGFPE, &_HuiSignalHandler);
		signal(SIGBREAK, &_HuiSignalHandler);
		signal(SIGABRT_COMPAT, &_HuiSignalHandler);*/
	}
}

static Callback _eh_cleanup_function_;


#ifdef _X_USE_NET_

class ReportDialog : public FixedDialog
{
public:
	ReportDialog(Window *parent) :
		FixedDialog(_("Bug Report"), 400, 295, parent, false)
	{
		addLabel("!bold$" + _("Name:"),5,5,360,25,"brd_t_name");
		addEdit("",5,35,385,25,"report_sender");
		addDefButton(_("Ok"),265,255,120,25,"ok");
		setImage("ok", "hui:ok");
		addButton(_("Cancel"),140,255,120,25,"cancel");
		setImage("cancel", "hui:cancel");
		addLabel("!bold$" + _("Comment/what happened:"),5,65,360,25,"brd_t_comment");
		addMultilineEdit("",5,95,385,110,"comment");
		addLabel("!wrap$" + _("Your comments and the contents of the file message.txt will be sent."),5,210,390,35,"brd_t_explanation");

		setString("report_sender",_("(anonymous)"));
		setString("comment",_("Just happened somehow..."));

		event("ok", std::bind(&ReportDialog::onOk, this));
		event("cancel", std::bind(&ReportDialog::destroy, this));
		event("hui:close", std::bind(&ReportDialog::destroy, this));
	}

	void onOk()
	{
		string sender = getString("report_sender");
		string comment = getString("comment");
		string return_msg;
		if (NetSendBugReport(sender, Application::getProperty("name"), Application::getProperty("version"), comment, return_msg))
			InfoBox(NULL, "ok", return_msg);
		else
			ErrorBox(NULL, "error", return_msg);
		destroy();
	}
};

void SendBugReport(Window *parent)
{
	ReportDialog *dlg = new ReportDialog(parent);
	dlg->run();
	delete(dlg);
}

#endif

class ErrorDialog : public FixedDialog
{
public:
	ErrorDialog() :
		FixedDialog(_("Error"), 600, 500, NULL, false)
	{
		addLabel(Application::getProperty("name") + " " + Application::getProperty("version") + _(" has crashed.		The last lines of the file message.txt::"),5,5,590,20,"error_header");
		addListView(_("Messages"),5,30,590,420,"message_list");
		//addEdit("",5,30,590,420,"message_list";
		addButton(_("Ok"),5,460,100,25,"ok");
		setImage("ok", "hui:ok");
		addButton(_("open message.txt"),115,460,200,25,"show_log");
		addButton(_("Send bug report to Michi"),325,460,265,25,"send_report");
	#ifdef _X_USE_NET_
		event("send_report", std::bind(&ErrorDialog::onSendBugReport, this));
	#else
		enable("send_report", false);
		setTooltip("send_report", _("Program was compiled without network support..."));
	#endif
		for (int i=1023;i>=0;i--){
			string temp = msg_get_str(i);
			if (temp.num > 0)
				addString("message_list", temp);
		}
		event("show_log", std::bind(&ErrorDialog::onShowLog, this));
		event("cancel", std::bind(&ErrorDialog::onClose, this));
		event("hui:win_close", std::bind(&ErrorDialog::onClose, this));
		event("ok", std::bind(&ErrorDialog::onClose, this));
	}

	void onShowLog()
	{
		OpenDocument("message.txt");
	}

	void onSendBugReport()
	{
		SendBugReport(this);
	}

	void onClose()
	{
		msg_write("real close");
		exit(0);
	}
};

void hui_default_error_handler()
{
	_idle_function_ = NULL;

	msg_reset_shift();
	msg_write("");
	msg_write("================================================================================");
	msg_write(_("program has crashed, error handler has been called... maybe SegFault... m(-_-)m"));
	//msg_write("---");
	msg_write("      trace:");
	msg_write(msg_get_trace());

	if (_eh_cleanup_function_){
		msg_write(_("i'm now going to clean up..."));
		_eh_cleanup_function_();
		msg_write(_("...done"));
	}

	foreachb(Window *w, _all_windows_)
		delete(w);
	msg_write(_("                  Close dialog box to exit program."));

	//HuiMultiline=true;
	ComboBoxSeparator = "$";

	//HuiErrorBox(NULL,"Fehler","Fehler");

	// dialog
	ErrorDialog *dlg = new ErrorDialog;
	dlg->run();

	//HuiEnd();
	exit(0);
}

void SetDefaultErrorHandler(const Callback &error_cleanup_function)
{
	_eh_cleanup_function_ = error_cleanup_function;
	SetErrorFunction(&hui_default_error_handler);
}

void RaiseError(const string &message)
{
	msg_error(message + " (HuiRaiseError)");
	/*int *p_i=NULL;
	*p_i=4;*/
	hui_default_error_handler();
}


};

