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

	int fc_begin(const Array<SerialNodeParam> &_params, const SerialNodeParam &ret, bool is_static);
	void fc_end(int push_size, const Array<SerialNodeParam> &params, const SerialNodeParam &ret);
	void add_function_call(Function *f, const Array<SerialNodeParam> &params, const SerialNodeParam &ret);

	void assemble() override;

	CommandList pre_cmd;
};

} // kaba

#endif //BACKENDARM64_H
