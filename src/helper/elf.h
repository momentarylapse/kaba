/*
 * elf.h
 *
 *  Created on: 21 Jul 2022
 *      Author: michi
 */

#ifndef SRC_HELPER_ELF_H_
#define SRC_HELPER_ELF_H_

#include "../lib/kaba/kaba.h"

void output_to_file_elf(shared<kaba::Module> s, const Path &out_file);



#endif /* SRC_HELPER_ELF_H_ */
