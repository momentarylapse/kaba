/*
 * elf.cpp
 *
 *  Created on: 21 Jul 2022
 *      Author: michi
 */

#include "elf.h"
#include "../lib/os/file.h"
#include "../lib/os/formatter.h"

void output_to_file_elf(shared<kaba::Module> s, const Path &out_file) {
	auto f = new BinaryFormatter(os::fs::open(out_file, "wb"));

	bool is64bit = (kaba::config.pointer_size == 8);

	// 16b header
	f->write_char(0x7f);
	f->write_char('E');
	f->write_char('L');
	f->write_char('F');
	f->write_char(0x02); // 64 bit
	f->write_char(0x01); // little-endian
	f->write_char(0x01); // version
	for (int i=0; i<9; i++)
		f->write_char(0x00);

	f->write_word(0x0003); // 3=shared... 2=exec
	if (kaba::config.instruction_set == Asm::InstructionSet::AMD64) {
		f->write_word(0x003e); // machine
	} else if (kaba::config.instruction_set == Asm::InstructionSet::X86) {
		f->write_word(0x0003); // machine
	} else if (kaba::config.instruction_set == Asm::InstructionSet::ARM32) {
		f->write_word(0x0028); // machine
	} else if (kaba::config.instruction_set == Asm::InstructionSet::ARM64) {
		f->write_word(0x0028); // machine ?!?!?!?
	}
	f->write_int(1); // version

	if (is64bit){
		f->write_int(0);	f->write_int(0);// entry point
		f->write_int(0x40);	f->write_int(0x00); // program header table offset
		f->write_int(0x00);	f->write_int(0x00); // section header table
	} else {
		f->write_int(0);// entry point
		f->write_int(0x00); // program header table offset
		f->write_int(0x00); // section header table
	}
	f->write_int(0); // flags
	f->write_word(is64bit ? 64 : 52); // header size
	f->write_word(0); // prog header size
	f->write_word(0); // # prog header table entries
	f->write_word(0); // size of section header entry table
	f->write_word(0); // # section headers
	f->write_word(0); // names entry section header index
	//f->WriteBuffer(s->opcode, s->opcode_size);
	delete(f);

	system(format("chmod a+x %s", out_file).c_str());
}



