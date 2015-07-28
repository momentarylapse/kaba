/*
 * hui_main.h
 *
 *  Created on: 26.06.2013
 *      Author: michi
 */

#ifndef HUI_MAIN_H_
#define HUI_MAIN_H_


// execution
void HuiInit(const string &program, bool load_res, const string &def_lang);
void _HuiMakeUsable_();
int HuiRun();
void HuiPushMainLevel();
void HuiPopMainLevel();

void HuiSetIdleFunction(hui_callback *idle_function);
void _HuiSetIdleFunctionM(HuiEventHandler *object, void (HuiEventHandler::*function)());
template<typename T>
void HuiSetIdleFunctionM(HuiEventHandler *object, T fun)
{	_HuiSetIdleFunctionM(object, (void(HuiEventHandler::*)())fun);	}

int HuiRunLater(float time, hui_callback *function);
int _HuiRunLaterM(float time, HuiEventHandler *object, void (HuiEventHandler::*function)());
template<typename T>
int HuiRunLaterM(float time, HuiEventHandler *object, T fun)
{	return _HuiRunLaterM(time, object, (void(HuiEventHandler::*)())fun);	}

int HuiRunRepeated(float time, hui_callback *function);
int _HuiRunRepeatedM(float time, HuiEventHandler *object, void (HuiEventHandler::*function)());
template<typename T>
int HuiRunRepeatedM(float time, HuiEventHandler *object, T fun)
{	return _HuiRunRepeatedM(time, object, (void(HuiEventHandler::*)())fun);	}

void HuiCancelRunner(int i);

void HuiDoSingleMainLoop();
void HuiEnd();


#endif /* HUI_MAIN_H_ */
