/*
 * symbols.h
 *
 *  Created on: 21 Jul 2022
 *      Author: michi
 */

#ifndef SRC_HELPER_SYMBOLS_H_
#define SRC_HELPER_SYMBOLS_H_

#include "../lib/kaba/kaba.h"

void export_symbols(shared<kaba::Module> s, const Path &symbols_out_file);
void import_symbols(const Path &symbols_in_file);



#endif /* SRC_HELPER_SYMBOLS_H_ */
