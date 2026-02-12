/*
 * Block.h
 *
 *  Created on: May 9, 2021
 *      Author: michi
 */

#pragma once

#include "Node.h"
#include "Flags.h"

namespace kaba {

// {...}-block
class Block {
public:
	Block(Function *f, Block *parent);
	Array<Variable*> vars;
	Function *function;
	Block *parent;
	Flags flags;
	void *_start, *_end; // opcode range
	int _label_start, _label_end;
	int level;
	bool is_trust_me() const;
	bool is_in_try() const;

	const Class *name_space() const;

	Variable *get_var(const string &name) const;
	Variable *add_var(const string &name, const Class *type, Flags flags = Flags::Mutable);
	Variable *insert_var(int index, const string &name, const Class *type, Flags flags = Flags::Mutable);
};


}
