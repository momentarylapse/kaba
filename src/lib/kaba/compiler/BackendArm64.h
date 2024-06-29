//
// Created by Michael Ankele on 2024-06-23.
//

#ifndef BACKENDARM64_H
#define BACKENDARM64_H

#include "BackendARM.h"

namespace kaba {

class BackendArm64 : public BackendARM {
public:
	explicit BackendArm64(Serializer* serializer);

	void process(Function *f, int index) override;
	void correct() override;
	void correct_implement_commands();
	void add_function_intro_params(Function *f);
	void implement_return(const SerialNodeParam &p);
	void assemble() override;

	CommandList pre_cmd;
};

} // kaba

#endif //BACKENDARM64_H
