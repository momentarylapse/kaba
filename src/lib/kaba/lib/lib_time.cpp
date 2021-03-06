#include "../../file/file.h"
#include "../kaba.h"
#include "../../config.h"
#include "lib.h"

#ifdef _X_USE_HUI_
	#include "../../hui/hui.h"
#elif defined(_X_USE_HUI_MINIMAL_)
	#include "../../hui_minimal/hui.h"
#else
	we are re screwed.... TODO: test for _X_USE_HUI_
#endif


namespace kaba {


extern const Class *TypeDate;
const Class *TypeTimer;

void SIAddPackageTime() {
	add_package("time");

	TypeDate = add_type("Date", sizeof(Date));
	TypeTimer = add_type("Timer", sizeof(hui::Timer));


	add_class(TypeDate);
		class_add_element("time", TypeInt64, &Date::time);
		class_add_func("format", TypeString, &Date::format, Flags::PURE);
			func_add_param("f", TypeString);
		class_add_func(IDENTIFIER_FUNC_STR, TypeString, &Date::str, Flags::PURE);
		class_add_func(IDENTIFIER_FUNC_ASSIGN, TypeVoid, &Date::__assign__);
			func_add_param("o", TypeDate);
		class_add_func("now", TypeDate, &Date::now, Flags::STATIC);


	add_class(TypeTimer);
		class_add_func(IDENTIFIER_FUNC_INIT, TypeVoid, &hui::Timer::reset);
		class_add_func("get", TypeFloat32, &hui::Timer::get);
		class_add_func("reset", TypeVoid, &hui::Timer::reset);
		class_add_func("peek", TypeFloat32, &hui::Timer::peek);



	add_func("sleep", TypeVoid, &hui::Sleep, Flags::STATIC);
		func_add_param("duration", TypeFloat32);
}

};
