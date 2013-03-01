#include "../file/file.h"
#include "script.h"
#include "../00_config.h"
#include "script_data_common.h"


#ifdef _X_USE_NET_
	#include "../net/net.h"
#endif

#ifdef _X_ALLOW_X_
	#include "../networking.h"
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

void SIAddPackageNet()
{
	msg_db_r("SIAddPackageNet", 3);

	set_cur_package("net");

	Type *TypeSocket = add_type  ("Socket",		sizeof(Socket));

	add_class(TypeSocket);
		class_add_element("s", TypeInt,0);
		class_add_func("__init__",		TypeVoid,	net_p(mf((tmf)&Socket::__init__)));
		class_add_func("__delete__",		TypeVoid,	net_p(mf((tmf)&Socket::__delete__)));
		class_add_func("__assign__",		TypeVoid,	net_p(mf((tmf)&Socket::__assign__)));
			func_add_param("other",		TypeSocket);
		class_add_func("Create",		TypeBool,	net_p(mf((tmf)&Socket::Create)));
			func_add_param("port",		TypeInt);
		class_add_func("Connect",		TypeBool,	net_p(mf((tmf)&Socket::Connect)));
			func_add_param("addr",		TypeString);
			func_add_param("port",		TypeInt);
		class_add_func("Accept",		TypeBool,	net_p(mf((tmf)&Socket::Accept)));
			func_add_param("con",		TypeSocket);
		class_add_func("Close",		TypeVoid,	net_p(mf((tmf)&Socket::Close)));
		class_add_func("SetBlocking",		TypeVoid,	net_p(mf((tmf)&Socket::SetBlocking)));
			func_add_param("block",		TypeBool);
		class_add_func("Read",		TypeString,	net_p(mf((tmf)&Socket::Read)));
		class_add_func("Write",		TypeBool,	net_p(mf((tmf)&Socket::Write)));
			func_add_param("buf",		TypeString);
		class_add_func("CanRead",		TypeBool,	net_p(mf((tmf)&Socket::CanRead)));
		class_add_func("CanWrite",		TypeBool,	net_p(mf((tmf)&Socket::CanWrite)));
		class_add_func("ConnectionLost",	TypeBool,	net_p(mf((tmf)&Socket::ConnectionLost)));
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(int&))&Socket::operator>>)));
			func_add_param("i",		TypeIntPs);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(float&))&Socket::operator>>)));
			func_add_param("f",		TypeFloatPs);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(bool&))&Socket::operator>>)));
			func_add_param("b",		TypeBoolPs);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(char&))&Socket::operator>>)));
			func_add_param("c",		TypeCharPs);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(string&))&Socket::operator>>)));
			func_add_param("s",		TypeString);
		class_add_func("__rshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(vector&))&Socket::operator>>)));
			func_add_param("v",		TypeVector);
		class_add_func("WriteBuffer",		TypeBool,	net_p(mf((tmf)&Socket::WriteBuffer)));
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(int))&Socket::operator<<)));
			func_add_param("i",		TypeInt);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(float))&Socket::operator<<)));
			func_add_param("f",		TypeFloat);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(bool))&Socket::operator<<)));
			func_add_param("b",		TypeBool);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(char))&Socket::operator<<)));
			func_add_param("c",		TypeChar);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(const string &))&Socket::operator<<)));
			func_add_param("s",		TypeString);
		class_add_func("__lshift__",		TypeVoid,	net_p(mf((tmf)(void(Socket::*)(const vector &))&Socket::operator<<)));
			func_add_param("v",		TypeVector);
	
	// network
/*	add_func("NetConnect",			TypeInt,			net_p(&NetConnect));
		func_add_param("addr",		TypeString);
		func_add_param("port",		TypeInt);
	add_func("NetAccept",			TypeInt,			net_p(&NetAccept));
		func_add_param("s",		TypeInt);
	add_func("NetCreate",			TypeInt,			net_p(&NetCreate));
		func_add_param("port",		TypeInt);
		func_add_param("blocking",		TypeBool);
	add_func("NetClose",			TypeVoid,			net_p(&NetClose));
		func_add_param("s",		TypeIntPs);
	add_func("NetResetBuffer",		TypeVoid,			net_p(&NetResetBuffer));
	add_func("NetReadBuffer",		TypeVoid,			net_p(&NetReadBuffer));
		func_add_param("s",		TypeInt);
	add_func("NetWriteBuffer",		TypeVoid,			net_p(&NetWriteBuffer));
		func_add_param("s",		TypeInt);
	add_func("NetDirectRead",		TypeString,			net_p(&NetRead));
		func_add_param("s",			TypeInt);
	add_func("NetDirectWrite",		TypeInt,			net_p(&NetWrite));
		func_add_param("s",			TypeInt);
		func_add_param("buffer",	TypeString);
	add_func("NetReadyToWrite",		TypeBool,			net_p(&NetReadyToWrite));
		func_add_param("s",		TypeInt);
	add_func("NetReadyToRead",		TypeBool,			net_p(&NetReadyToRead));
		func_add_param("s",		TypeInt);
	add_func("NetReadInt",			TypeInt,			net_p(&NetReadInt));
	add_func("NetReadBool",			TypeBool,			net_p(&NetReadBool));
	add_func("NetReadFloat",		TypeFloat,			net_p(&NetReadFloat));
	add_func("NetReadVector",		TypeVector,			net_p(&NetReadVector));
	add_func("NetReadStr",			TypeString,			net_p(&NetReadStr));
	add_func("NetWriteInt",			TypeInt,			net_p(&NetWriteInt));
		func_add_param("i",		TypeInt);
	add_func("NetWriteBool",		TypeVoid,			net_p(&NetWriteBool));
		func_add_param("b",		TypeBool);
	add_func("NetWriteFloat",		TypeVoid,			net_p(&NetWriteFloat));
		func_add_param("f",		TypeFloat);
	add_func("NetWriteVector",		TypeVoid,			net_p(&NetWriteVector));
		func_add_param("v",		TypeVector);
	add_func("NetWriteStr",			TypeVoid,			net_p(&NetWriteStr));
		func_add_param("s",		TypeString);
	
	add_func("XNetAddMsgHandler",		TypeVoid,			x_p(&XNetAddMsgHandler));
		func_add_param("name",		TypeString);
		func_add_param("func",		TypePointer);
	add_func("XNetSendMsgStart",		TypeVoid,			x_p(&XNetSendMsgStart));
		func_add_param("name",		TypeString);
		func_add_param("target",		TypeInt);
	add_func("XNetSendMsgEnd",		TypeVoid,			x_p(&XNetSendMsgEnd));
	add_func("XNetWriteInt",		TypeVoid,			x_p(&XNetWriteInt));
		func_add_param("i",		TypeInt);
	add_func("XNetWriteFloat",		TypeVoid,			x_p(&XNetWriteFloat));
		func_add_param("f",		TypeFloat);
	add_func("XNetWriteBool",		TypeVoid,			x_p(&XNetWriteBool));
		func_add_param("b",		TypeBool);
	add_func("XNetWriteVector",		TypeVoid,			x_p(&XNetWriteVector));
		func_add_param("v",		TypeVector);
	add_func("XNetWriteString",		TypeVoid,			x_p(&XNetWriteString));
		func_add_param("str",		TypeString);*/

	
	msg_db_l(3);
}

};
