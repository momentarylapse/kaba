/*
 * call.h
 *
 *  Created on: May 9, 2021
 *      Author: michi
 */

#pragma once

namespace kaba {

class Function;

bool call_function(Function *f, void *ff, void *ret, const Array<void*> &param);
bool call_function(Function *f, void *ret, const Array<void*> &param);

}
