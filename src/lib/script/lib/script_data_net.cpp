#include "../../file/file.h"
#include "../script.h"
#include "../../config.h"
#include "script_data_common.h"


#ifdef _X_USE_NET_
	#include "../../net/net.h"
#endif

namespace Script{

#ifdef _X_USE_NET_
	#define net_p(p)		(void*)p
#else
	typedef int Socket;
	#define net_p(p)		NULL
#endif
#ifdef _X_ALLOW_X_
	#define x_p(p)		(void*)p
#else
	#define x_p(p)		NULL
#endif

extern Type *TypeIntPs;
extern Type *TypeFloatPs;
extern Type *TypeBoolPs;
extern Type *TypeCharPs;

Type *TypeSocket;
Type *TypeSocketP;
Type *TypeSocketPList;

void SIAddPackageNet()
{
	msg_db_f("SIAddPackageNet", 3);

	add_package("net", false);

	TypeSocket     = add_type  ("Socket",		sizeof(Socket));
	TypeSocketP    = add_type_p("Socket*",	TypeSocket);
	TypeSocketPList = add_type_a("Socket*[]",	TypeSocketP, -1);

	add_class(TypeSocket);
		class_add_element("uid", TypeInt,0);
		class_add_func("__init__",		TypeVoid,	net_p(mf(&Socket::__init__)));
		class_add_func("__delete__",		TypeVoid,	net_p(mf(&Socket::__delete__)));
		class_add_func("accept",		TypeSocketP,	net_p(mf(&Socket::accept)));
		class_add_func("close",		TypeVoid,	net_p(mf(&Socket::close)));
		class_add_func("setBlocking",		TypeVoid,	net_p(mf(&Socket::setBlocking)));
			func_add_param("block",		TypeBool);
		class_add_func("read",		TypeString,	net_p(mf(&Socket::read)));
		class_add_func("write",		TypeBool,	net_p(mf(&Socket::write)));
			func_add_param("buf",		TypeString);
		class_add_func("canRead",		TypeBool,	net_p(mf(&Socket::canRead)));
		class_add_func("canWrite",		TypeBool,	net_p(mf(&Socket::canWrite)));
		class_add_func("isConnected",	TypeBool,	net_p(mf(&Socket::isConnected)));
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((void(Socket::*)(int&))&Socket::operator>>)));
			func_add_param("i",		TypeIntPs);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((void(Socket::*)(float&))&Socket::operator>>)));
			func_add_param("f",		TypeFloatPs);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((void(Socket::*)(bool&))&Socket::operator>>)));
			func_add_param("b",		TypeBoolPs);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((void(Socket::*)(char&))&Socket::operator>>)));
			func_add_param("c",		TypeCharPs);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((void(Socket::*)(string&))&Socket::operator>>)));
			func_add_param("s",		TypeString);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((void(Socket::*)(vector&))&Socket::operator>>)));
			func_add_param("v",		TypeVector);
		class_add_func("writeBuffer",		TypeBool,	net_p(mf(&Socket::writeBuffer)));
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((void(Socket::*)(int))&Socket::operator<<)));
			func_add_param("i",		TypeInt);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((void(Socket::*)(float))&Socket::operator<<)));
			func_add_param("f",		TypeFloat);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((void(Socket::*)(bool))&Socket::operator<<)));
			func_add_param("b",		TypeBool);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((void(Socket::*)(char))&Socket::operator<<)));
			func_add_param("c",		TypeChar);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((void(Socket::*)(const string &))&Socket::operator<<)));
			func_add_param("s",		TypeString);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((void(Socket::*)(const vector &))&Socket::operator<<)));
			func_add_param("v",		TypeVector);

	add_func("SocketListen",		TypeSocketP,	net_p(&NetListen));
		func_add_param("port",		TypeInt);
		func_add_param("block",		TypeBool);
	add_func("SocketConnect",		TypeSocketP,	net_p(&NetConnect));
		func_add_param("addr",		TypeString);
		func_add_param("port",		TypeInt);
}

};
