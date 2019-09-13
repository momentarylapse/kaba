
#ifndef SERIALIZER_ARM_H_
#define SERIALIZER_ARM_H_

#include "serializer.h"

namespace Kaba
{


class SerializerARM : public Serializer
{
public:
	SerializerARM(Script *script, Asm::InstructionWithParamsList *list) : Serializer(script, list){};
	virtual ~SerializerARM(){}
	void add_function_call(Function *f, const SerialNodeParam &instance, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) override;
	void add_virtual_function_call(int virtual_index, const SerialNodeParam &instance, const Array<SerialNodeParam> &params, const SerialNodeParam &ret) override;
	void add_pointer_call(const SerialNodeParam &pointer, const Array<SerialNodeParam> &param, const SerialNodeParam &ret) override;
	int fc_begin(const SerialNodeParam &instance, const Array<SerialNodeParam> &param, const SerialNodeParam &ret) override;
	void fc_end(int push_size, const SerialNodeParam &ret) override;
	void AddFunctionIntro(Function *f) override;
	void AddFunctionOutro(Function *f) override;
	SerialNodeParam serialize_parameter(Node *link, Block *block, int index) override;
	void serialize_statement(Node *com, const SerialNodeParam &ret, Block *block, int index) override;
	void serialize_inline_function(Node *com, const Array<SerialNodeParam> &param, const SerialNodeParam &ret) override;

	void do_mapping() override;
	void ConvertGlobalLookups();
	void CorrectUnallowedParamCombis();
	void ConvertMemMovsToLdrStr(SerialNode &c);
	void ConvertGlobalRefs();
	void ProcessReferences();
	void ProcessDereferences();
	void CorrectReturn() override;

	void transfer_by_reg_in(SerialNode &c, int &i, int pno);
	void transfer_by_reg_out(SerialNode &c, int &i, int pno);
	void gr_transfer_by_reg_in(SerialNode &c, int &i, int pno);
	void gr_transfer_by_reg_out(SerialNode &c, int &i, int pno);
};

};

#endif /* SERIALIZER_ARM_H_ */
