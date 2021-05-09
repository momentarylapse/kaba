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
class Block : public Node {
public:
	Block(Function *f, Block *parent);
	Array<Variable*> vars;
	Function *function;
	Block *parent;
	void *_start, *_end; // opcode range
	int _label_start, _label_end;
	int level;
	void add(shared<Node> c);
	void set(int index, shared<Node> c);

	const Class *name_space() const;

	Variable *get_var(const string &name) const;
	Variable *add_var(const string &name, const Class *type, Flags flags = Flags::NONE);
};


}
