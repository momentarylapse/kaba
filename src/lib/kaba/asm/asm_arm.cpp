/*
 * asm_arm.cpp
 *
 *  Created on: 09.09.2019
 *      Author: michi
 */

#include "../../base/base.h"
#include "../../os/msg.h"
#include "asm.h"
#include "internal.h"

namespace Asm {



struct CPUInstructionARM {
	InstID inst;
	int code, filter;
	int p1, p2, p3;
};
static Array<CPUInstructionARM> cpu_instructions_arm;



enum {
	AP_NONE,
	AP_REG_0,
	AP_REG_0P5,
	AP_WREG_0P5,
	AP_FREG_0_5,
	AP_REG_5P5,
	AP_WREG_5P5,
	AP_REG_8,
	AP_REG_10P5,
	AP_REG_12,
	AP_FREG_12_22,
	AP_REG_16,
	AP_REG_16_W21,
	AP_WREG_16,
	AP_FREG_16_7,
	AP_REG_SET,
	AP_OFFSET24_0,
	AP_IMM12_0,
	AP_IMM12_10,
	AP_IMM12_10SH,
	AP_IMM9_12,
	AP_IMM16E2_5,
	AP_IMM26X4REL_0, // relative to rip x4
	AP_SHIFTED12_0,
	AP_DEREF_REG_16_OFFSET,
	AP_DEREF_S32_REG_5P5_PLUS_IMM12P10,
	AP_DEREF_S64_REG_5P5_PLUS_IMM12P10,
	AP_DEREF_S128_REG_5P5_PLUS_IMM7P15,
	AP_SHIFTER_0X12_I25,
	AP_XX_R12_W21_UPI23,
	AP_XX_R12_W21_UPI23_BYTE,
};


void add_inst_arm(InstID inst, int code, int filter, int param1, int param2 = AP_NONE, int param3 = AP_NONE) {
	CPUInstructionARM i;
	i.inst = inst;
	i.code = code;
	i.filter = filter;
	i.p1 = param1;
	i.p2 = param2;
	i.p3 = param3;

	//i.name = GetInstructionName(inst);
	cpu_instructions_arm.add(i);
}



RegID s_reg(int n) {
	return (RegID)((int)RegID::S0 + n);
}

RegID r_reg(int n) {
	return (RegID)((int)RegID::R0 + n);
}

RegID w_reg(int n) {
	return (RegID)((int)RegID::W0 + n);
}

void arm32_init() {
	registers.clear();
	for (int i=0; i<16; i++)
		add_reg(format("r%d", i), r_reg(i), RegGroup::GENERAL, SIZE_32, (RegRoot)((int)RegRoot::R0 + i));
	for (int i=0; i<16; i++)
		add_reg(format("s%d", i), s_reg(i), RegGroup::VFP, SIZE_32, (RegRoot)((int)RegRoot::S0 + i));

	// create easy to access array
	register_by_id.clear();
	for (auto &r: registers) {
		if (register_by_id.num <= (int)r.id)
			register_by_id.resize((int)r.id + 1);
		RegisterByID(r.id) = &r;
	}

	cpu_instructions_arm.clear();

	add_inst_arm(InstID::MUL,  0x00000090, 0x0ff0f0f0, AP_REG_16, AP_REG_0, AP_REG_8);
	add_inst_arm(InstID::MULS, 0x00100090, 0x0ff0f0f0, AP_REG_16, AP_REG_0, AP_REG_8);

	// data
	add_inst_arm(InstID::AND,  0x00000000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::ANDS, 0x00100000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::XOR,  0x00200000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::XORS, 0x00300000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::SUB,  0x00400000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::SUBS, 0x00500000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::RSB,  0x00600000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::RSBS, 0x00700000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::ADD,  0x00800000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::ADDS, 0x00900000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::ADC,  0x00a00000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::ADCS, 0x00b00000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::SBC,  0x00c00000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::SBCS, 0x00d00000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::RSC,  0x00e00000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::RSCS, 0x00f00000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::TST,  0x01100000, 0x0df00000, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::TEQ,  0x01300000, 0x0df00000, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::CMP,  0x01500000, 0x0df00000, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::CMN,  0x01700000, 0x0df00000, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::OR,   0x01800000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::ORS,  0x01900000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::MOV,  0x01a00000, 0x0df00000, AP_REG_12, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::MOVS, 0x01b00000, 0x0df00000, AP_REG_12, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::BIC,  0x01c00000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::BICS, 0x01d00000, 0x0df00000, AP_REG_12, AP_REG_16, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::MVN,  0x01e00000, 0x0df00000, AP_REG_12, AP_SHIFTER_0X12_I25);
	add_inst_arm(InstID::MVNS, 0x01f00000, 0x0df00000, AP_REG_12, AP_SHIFTER_0X12_I25);

	// transfer
	add_inst_arm(InstID::LDR,  0x04100000, 0x0c500000, AP_REG_12, AP_XX_R12_W21_UPI23);
	add_inst_arm(InstID::LDRB, 0x04500000, 0x0c500000, AP_REG_12, AP_XX_R12_W21_UPI23_BYTE);
	add_inst_arm(InstID::STR,  0x04000000, 0x0c500000, AP_REG_12, AP_XX_R12_W21_UPI23); // TODO swap parameters...
	add_inst_arm(InstID::STRB, 0x04400000, 0x0c500000, AP_REG_12, AP_XX_R12_W21_UPI23_BYTE);

	// transfer multiple
	add_inst_arm(InstID::LDMIA,  0x08900000, 0x0f900000, AP_REG_16_W21, AP_REG_SET);
	add_inst_arm(InstID::STMIA,  0x08800000, 0x0f900000, AP_REG_16_W21, AP_REG_SET);
	add_inst_arm(InstID::LDMIB,  0x09900000, 0x0f900000, AP_REG_16_W21, AP_REG_SET);
	add_inst_arm(InstID::STMIB,  0x09800000, 0x0f900000, AP_REG_16_W21, AP_REG_SET);
	add_inst_arm(InstID::LDMDA,  0x08100000, 0x0f900000, AP_REG_16_W21, AP_REG_SET);
	add_inst_arm(InstID::STMDA,  0x08000000, 0x0f900000, AP_REG_16_W21, AP_REG_SET);
	add_inst_arm(InstID::LDMDB,  0x09100000, 0x0f900000, AP_REG_16_W21, AP_REG_SET);
	add_inst_arm(InstID::STMDB,  0x09000000, 0x0f900000, AP_REG_16_W21, AP_REG_SET);

	// branch
	add_inst_arm(InstID::BLX, 0x01200030, 0x0ff000f0, AP_REG_0);
	add_inst_arm(InstID::B,   0x0a000000 ,0x0f000000, AP_OFFSET24_0);
	add_inst_arm(InstID::BL,  0x0b000000 ,0x0f000000, AP_OFFSET24_0);


	// -- float --

	add_inst_arm(InstID::FMSR,  0x0e000a10, 0x0ff00f7f, AP_FREG_16_7, AP_REG_12);
	add_inst_arm(InstID::FMRS,  0x0e100a10, 0x0ff00f7f, AP_REG_12, AP_FREG_16_7);

	add_inst_arm(InstID::FLDS,   0x0d100a00, 0x0f300f00, AP_FREG_12_22, AP_DEREF_REG_16_OFFSET);
	add_inst_arm(InstID::FSTS,   0x0d000a00, 0x0f300f00, AP_DEREF_REG_16_OFFSET, AP_FREG_12_22);

	add_inst_arm(InstID::FMACS,  0x0e000a00, 0x0fb00f50, AP_FREG_12_22, AP_FREG_16_7, AP_FREG_0_5);
	add_inst_arm(InstID::FNMACS, 0x0e000a40, 0x0fb00f50, AP_FREG_12_22, AP_FREG_16_7, AP_FREG_0_5);
	add_inst_arm(InstID::FMSCS,  0x0e100a00, 0x0fb00f50, AP_FREG_12_22, AP_FREG_16_7, AP_FREG_0_5);
	add_inst_arm(InstID::FNMSCS, 0x0e100a40, 0x0fb00f50, AP_FREG_12_22, AP_FREG_16_7, AP_FREG_0_5);
	add_inst_arm(InstID::FMULS,  0x0e200a00, 0x0fb00f50, AP_FREG_12_22, AP_FREG_16_7, AP_FREG_0_5);
	add_inst_arm(InstID::FNMULS, 0x0e200a40, 0x0fb00f50, AP_FREG_12_22, AP_FREG_16_7, AP_FREG_0_5);
	add_inst_arm(InstID::FADDS,  0x0e300a00, 0x0fb00f50, AP_FREG_12_22, AP_FREG_16_7, AP_FREG_0_5);
	add_inst_arm(InstID::FSUBS,  0x0e300a40, 0x0fb00f50, AP_FREG_12_22, AP_FREG_16_7, AP_FREG_0_5);
	add_inst_arm(InstID::FDIVS,  0x0e800a00, 0x0fb00f50, AP_FREG_12_22, AP_FREG_16_7, AP_FREG_0_5);

	add_inst_arm(InstID::FCPYS,  0x0eb00a40, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FABSS,  0x0eb00ac0, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FNEGS,  0x0eb10a40, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FSQRTS, 0x0eb10ac0, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FCMPS,   0x0eb40a40, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FCMPES,  0x0eb40ac0, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FCMPZS,  0x0eb50a40, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FCMPEZS, 0x0eb50ac0, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);

	add_inst_arm(InstID::CVTDS, 0x0eb70ac0, 0x0fff0fd0, AP_FREG_12_22, AP_FREG_0_5);
	//add_inst_arm(InstID::CVTSD, 0x0eb70ac0, 0x0fff0fd0, AP_FREG_12_22, AP_FREG_0_5);

	add_inst_arm(InstID::FUITOS, 0x0eb80a40, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FSITOS, 0x0eb80ac0, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);

	add_inst_arm(InstID::FTOUIS,  0x0ebc0a40, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FTOUIZS, 0x0ebc0ac0, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FTOSIS,  0x0ebd0a40, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
	add_inst_arm(InstID::FTOSIZS, 0x0ebd0ac0, 0x0fbf0fd0, AP_FREG_12_22, AP_FREG_0_5);
}



void arm64_init() {
	registers.clear();
	for (int i=0; i<32; i++)
		add_reg(format("r%d", i), r_reg(i), RegGroup::GENERAL, SIZE_64, (RegRoot)((int)RegRoot::R0 + i));
	for (int i=0; i<32; i++)
		add_reg(format("w%d", i), w_reg(i), RegGroup::GENERAL, SIZE_32, (RegRoot)((int)RegRoot::R0 + i));
	for (int i=0; i<16; i++)
		add_reg(format("s%d", i), s_reg(i), RegGroup::VFP, SIZE_32, (RegRoot)((int)RegRoot::S0 + i));

	// create easy to access array
	register_by_id.clear();
	for (auto &r: registers) {
		if (register_by_id.num <= (int)r.id)
			register_by_id.resize((int)r.id + 1);
		RegisterByID(r.id) = &r;
	}

	cpu_instructions_arm.clear();

	add_inst_arm(InstID::SUB,  0xd1000000, 0xff800000, AP_REG_0P5, AP_REG_5P5, AP_IMM12_10SH); // 64bit
	add_inst_arm(InstID::SUB,  0x51000000, 0xffe00000, AP_WREG_0P5, AP_WREG_5P5, AP_IMM12_10); // 32bit
	add_inst_arm(InstID::SUB,  0x4b000000, 0xffe00000, AP_WREG_0P5, AP_WREG_5P5, AP_WREG_16); // 32bit

	add_inst_arm(InstID::ADD,  0x91000000, 0xff800000, AP_REG_0P5, AP_REG_5P5, AP_IMM12_10SH); // 64bit
	add_inst_arm(InstID::ADD,  0x11000000, 0xffe00000, AP_WREG_0P5, AP_WREG_5P5, AP_IMM12_10); // 32bit
	add_inst_arm(InstID::ADD,  0x0b000000, 0xffe00000, AP_WREG_0P5, AP_WREG_5P5, AP_WREG_16); // 32bit

	add_inst_arm(InstID::MUL,  0x9b007c00, 0xff00fc00, AP_WREG_0P5, AP_WREG_5P5, AP_WREG_16); // 32bit
	add_inst_arm(InstID::MUL,  0x1b007c00, 0xff00fc00, AP_REG_0P5, AP_REG_5P5, AP_REG_16); // 64bit

	add_inst_arm(InstID::STR,  0xb9000000, 0xffc00000, AP_WREG_0P5, AP_DEREF_S32_REG_5P5_PLUS_IMM12P10 /*, AP_IMM12_10*/); // 32bit
	add_inst_arm(InstID::STR,  0xf9000000, 0xffc00000, AP_REG_0P5, AP_DEREF_S64_REG_5P5_PLUS_IMM12P10 /*, AP_IMM12_10*/); // 64bit

	add_inst_arm(InstID::STP_POSTINDEX, 0xa8800000, 0xffc00000, AP_REG_0P5, AP_REG_10P5, AP_DEREF_S128_REG_5P5_PLUS_IMM7P15); // 64bit
	add_inst_arm(InstID::STP_PREINDEX, 0xa9800000, 0xffc00000, AP_REG_0P5, AP_REG_10P5, AP_DEREF_S128_REG_5P5_PLUS_IMM7P15); // 64bit
	add_inst_arm(InstID::STP, 0xa9000000, 0xffc00000, AP_REG_0P5, AP_REG_10P5, AP_DEREF_S128_REG_5P5_PLUS_IMM7P15); // 64bit
	// p[2] = [Rn + imm7@15 * 4/8] (32bit / 64bit)

	add_inst_arm(InstID::LDR,  0xb9400000, 0xffc00000, AP_WREG_0P5, AP_DEREF_S32_REG_5P5_PLUS_IMM12P10); // 32bit
	add_inst_arm(InstID::LDR,  0xf9400000, 0xffc00000, AP_REG_0P5, AP_DEREF_S64_REG_5P5_PLUS_IMM12P10); // 64bit

	add_inst_arm(InstID::LDP_POSTINDEX,  0xa8c00000, 0xffc00000, AP_REG_0P5, AP_REG_10P5, AP_DEREF_S128_REG_5P5_PLUS_IMM7P15); // 64bit
	add_inst_arm(InstID::LDP_PREINDEX,  0xa9c00000, 0xffc00000, AP_REG_0P5, AP_REG_10P5, AP_DEREF_S128_REG_5P5_PLUS_IMM7P15); // 64bit
	add_inst_arm(InstID::LDP,  0xa9400000, 0xffc00000, AP_REG_0P5, AP_REG_10P5, AP_DEREF_S128_REG_5P5_PLUS_IMM7P15); // 64bit

	add_inst_arm(InstID::LDRSW,  0xb8800400, 0xffe00c00, AP_REG_0P5, AP_REG_5P5, AP_IMM9_12);
	add_inst_arm(InstID::LDRSW,  0xb9800000, 0xffe00000, AP_REG_0P5, AP_REG_5P5, AP_IMM12_10);

	add_inst_arm(InstID::RET, 0xd65f03c0, 0xffffffff, AP_NONE);
	add_inst_arm(InstID::RET, 0xd65f0000, 0xfffffc1f, AP_REG_5P5);

	add_inst_arm(InstID::ADR, 0x10000000, 0x9f000000, AP_REG_0P5);
	add_inst_arm(InstID::ADRP, 0x90000000, 0x9f000000, AP_REG_0P5);

	add_inst_arm(InstID::MOV, 0xd2800000, 0xff800000, AP_REG_0P5, AP_IMM16E2_5); // 64bit
	add_inst_arm(InstID::MOV, 0x52800000, 0xff800000, AP_REG_0P5, AP_IMM16E2_5); // 32bit

	add_inst_arm(InstID::BL, 0x94000000, 0xfc000000, AP_IMM26X4REL_0);
}

const int NUM_ARM_DATA_INSTRUCTIONS = 32;

InstID ARM_DATA_INSTRUCTIONS[NUM_ARM_DATA_INSTRUCTIONS] = {
	InstID::AND,
	InstID::ANDS,
	InstID::XOR,
	InstID::XORS,
	InstID::SUB,
	InstID::SUBS,
	InstID::RSB,
	InstID::RSBS,
	InstID::ADD,
	InstID::ADDS,
	InstID::ADC,
	InstID::ADCS,
	InstID::SBC,
	InstID::SBCS,
	InstID::RSC,
	InstID::RSCS,
		InstID::INVALID,
	InstID::TST,
		InstID::INVALID,
	InstID::TEQ,
		InstID::INVALID,
	InstID::CMP,
		InstID::INVALID,
	InstID::CMN,
	InstID::OR,
	InstID::ORS,
	InstID::MOV,
	InstID::MOVS,
	InstID::BIC,
	InstID::BICS,
	InstID::MVN,
	InstID::MVNS
};

int arm_decode_imm(int imm) {
	int r = ((imm >> 8) & 0xf);
	int n = (imm & 0xff);
	return n >> (r*2) | n << (32 - r*2);
}

InstructionParam disarm_shift_reg(int code) {
	InstructionParam p = param_reg(r_reg(code & 0xf));
	bool by_reg = (code >> 4) & 0x1;
	int r = ((code >> 7) & 0x1f);
	if (!by_reg and r == 0)
		return p;
	/*if (((code >> 5) & 0x3) == 0)
		s += "<<";
	else
		s += ">>";
	if (by_reg) {
		s += show_reg((code >> 8) & 0xf);
	} else {
		s += format("%d", r);
	}*/
	return p;
}


InstructionParam param_xxx(int code, bool bb) {
	InstructionParam p;

	int Rn = (code >> 16) & 0xf;
	bool imm = ((code >> 25) & 1);
	[[maybe_unused]] bool pre = ((code >> 24) & 1);
	bool up = ((code >> 23) & 1);
	bool ww = ((code >> 21) & 1);
	if (imm) {
		msg_write( " --shifted reg--");
	} else {
		if (code & 0xfff)
			p = param_deref_reg_shift(r_reg(Rn), up ? (code & 0xfff) : (-(code & 0xfff)), bb ? SIZE_8 : SIZE_32);
		else
			p = param_deref_reg(r_reg(Rn), bb ? SIZE_8 : SIZE_32);
	}
	p.write_back = ww;
	return p;
}

InstructionParam disarm_param(int code, int p) {
	if (p == AP_FREG_0_5) {
		int fm = (code & 0x0000000f) << 1;
		if ((code & (1<<5)) != 0)
			fm += 1;
		return param_reg(s_reg(fm));
	} else if (p == AP_FREG_12_22) {
		int fd = (code & 0x0000f000) >> 11;
		if ((code & (1<<22)) != 0)
			fd += 1;
		return param_reg(s_reg(fd));
	} else if (p == AP_FREG_16_7) {
		int fn = (code & 0x000f0000) >> 15;
		if ((code & (1<<7)) != 0)
			fn += 1;
		return param_reg(s_reg(fn));
	} else if (p == AP_REG_0) {
		int fm = (code & 0x0000000f);
		return param_reg(r_reg(fm));
	} else if (p == AP_REG_0P5) {
		int fm = (code & 0x0000001f);
		return param_reg(r_reg(fm));
	} else if (p == AP_WREG_0P5) {
		int fm = (code & 0x0000001f);
		return param_reg(w_reg(fm));
	} else if (p == AP_REG_5P5) {
		int fm = (code & 0x000003e0) >> 5;
		return param_reg(r_reg(fm));
	} else if (p == AP_DEREF_S64_REG_5P5_PLUS_IMM12P10 or p == AP_DEREF_S32_REG_5P5_PLUS_IMM12P10) {
		int fm = (code & 0x000003e0) >> 5;
		int imm = (code & 0x003ffc00) >> 10;
		int size = p == AP_DEREF_S32_REG_5P5_PLUS_IMM12P10 ? SIZE_32 : SIZE_64;
		return param_deref_reg_shift(r_reg(fm), imm * size, size);
	} else if (p == AP_DEREF_S128_REG_5P5_PLUS_IMM7P15) {
		int fm = (code & 0x000003e0) >> 5;
		int imm = (code & 0x003f8000) >> 15;
		if (imm & 0x40)
			imm |= 0xffffff80;
		return param_deref_reg_shift(r_reg(fm), imm * 8, SIZE_128);
	} else if (p == AP_WREG_5P5) {
		int fm = (code & 0x000003e0) >> 5;
		return param_reg(w_reg(fm));
	} else if (p == AP_REG_8) {
		int fm = (code & 0x00000f00) >> 8;
		return param_reg(r_reg(fm));
	} else if (p == AP_REG_10P5) {
		int fm = (code & 0x0000fc00) >> 10;
		return param_reg(r_reg(fm));
	} else if (p == AP_REG_12) {
		int fd = (code & 0x0000f000) >> 12;
		return param_reg(r_reg(fd));
	} else if (p == AP_REG_16) {
		int fn = (code & 0x000f0000) >> 16;
		return param_reg(r_reg(fn));
	} else if (p == AP_WREG_16) {
		int fn = (code & 0x000f0000) >> 16;
		return param_reg(w_reg(fn));
	} else if (p == AP_REG_16_W21) {
		int fm = (code & 0x000f0000) >> 16;
		auto p = param_reg(r_reg(fm));
		p.write_back = ((code >> 21) & 1);
		return p;
	} else if (p == AP_DEREF_REG_16_OFFSET) {
		int Rn = (code >> 16) & 0xf;
		bool up = ((code >> 23) & 1);
		int offset = (code & 0xff) * 4;
		return param_deref_reg_shift(r_reg(Rn), up ? offset : -offset, SIZE_32);
	} else if (p == AP_SHIFTER_0X12_I25) {
		if ((code >> 25) & 1)
			return param_imm(arm_decode_imm(code & 0xfff), SIZE_32);
		return disarm_shift_reg(code & 0xfff);
	} else if (p == AP_OFFSET24_0) {
		return param_imm((code & 0x00ffffff) * 4, SIZE_32);
	} else if (p == AP_XX_R12_W21_UPI23) {
		return param_xxx(code, false);
	} else if (p == AP_XX_R12_W21_UPI23_BYTE) {
		return param_xxx(code, true);
	} else if (p == AP_REG_SET) {
		return param_reg_set(code & 0xffff);
	} else if (p == AP_IMM12_10) {
		return param_imm((code & 0x003ffc00) >> 10, SIZE_64);
	} else if (p == AP_IMM16E2_5) {
		int e = (code & 0x00600000) >> 21;
		return param_imm(((code & 0x001fffe0) >> 5) << e, SIZE_64);
	} else if (p == AP_IMM12_10SH) {
		if (code & 0x00400000)
			return param_imm((code & 0x003ffc00) << 2, SIZE_64);
		else
			return param_imm((code & 0x003ffc00) >> 10, SIZE_64);
	} else if (p == AP_IMM9_12) {
		return param_imm((code & 0x001ff000) >> 12, SIZE_64);
	} else if (p == AP_IMM26X4REL_0) {
		return param_imm((code & 0x03ffffff) >> 2, SIZE_64);
	} else if (p != AP_NONE) {
		msg_error("disasm_param... unhandled " + i2s(p));
	}
	return InstructionParam();
}

InstructionWithParams disarm_general(int code) {
	InstructionWithParams i;
	i.inst = InstID::NOP;
	i.p[0] = param_none;
	i.p[1] = param_none;
	i.p[2] = param_none;
	for (auto &ii: cpu_instructions_arm) {
		if ((code & ii.filter) == ii.code) {
			i.inst = ii.inst;
			i.p[0] = disarm_param(code, ii.p1);
			i.p[1] = disarm_param(code, ii.p2);
			i.p[2] = disarm_param(code, ii.p3);
			return i;
		}
	}
	return i;
}

string arm_disassemble(void *_code_,int length,bool allow_comments) {
	string buf;
	int *code = (int*)_code_;
	for (int ni=0; ni<length/4; ni++) {
		int cur = code[ni];

	//	[[maybe_unused]] int x = (cur >> 25) & 0x7;

		buf += i2h(cur, 4);
		buf += "    ";

		InstructionWithParams iwp;
		iwp = disarm_general(cur);
		if (instruction_set.set == InstructionSet::ARM64) {
			iwp.condition = ArmCond::Always;
		} else {
			iwp.condition = (ArmCond)((cur >> 28) & 0xf);
		}

		buf += iwp.str() + "\n";
	}
	return buf;
}

int arm_reg_no(Register *r) {
	if (r) {
		if (((int)r->id >= (int)RegID::R0) and ((int)r->id <= (int)RegID::R0 + 31))
			return (int)r->id - (int)RegID::R0;
		if (((int)r->id >= (int)RegID::W0) and ((int)r->id <= (int)RegID::W0 + 31))
			return (int)r->id - (int)RegID::W0;
	}
	raise_error("ARM: invalid register: " + r->name);
	return -1;
}

int arm_freg_no(Register *r) {
	if (r)
		if (((int)r->id >= (int)RegID::S0) and ((int)r->id <= (int)RegID::S31))
			return (int)r->id - (int)RegID::S0;
	raise_error("ARM: invalid vfp register: " + r->name);
	return -1;
}

int arm_encode_8l4(unsigned int value) {
	for (int ex=0; ex<=30; ex+=2) {
		unsigned int mask = (0xffffff00 >> ex) | (0xffffff00 << (32-ex));
		if ((value & mask) == 0) {
			unsigned int mant = (value << ex) | (value >> (32 - ex));
			return mant | (ex << (8-1));
		}
	}
	raise_error("ARM: immediate value not representable: " + i2s(value));
	return 0;
}

bool inline arm_is_load_store_reg(InstID inst) {
	return (inst == InstID::LDR) or (inst == InstID::LDRB) or (inst == InstID::STR) or (inst == InstID::STRB);
}

bool inline arm_is_data(InstID inst, int &nn) {
	nn = -1;
	for (int i=0; i<NUM_ARM_DATA_INSTRUCTIONS; i++)
		if (inst == ARM_DATA_INSTRUCTIONS[i]) {
			nn = i;
			return true;
		}
	return false;
}

bool inline arm_is_load_store_multi(InstID inst) {
	if ((inst == InstID::LDMIA) or (inst == InstID::LDMIB) or (inst == InstID::LDMDA) or (inst == InstID::LDMDB))
		return true;
	if ((inst == InstID::STMIA) or (inst == InstID::STMIB) or (inst == InstID::STMDA) or (inst == InstID::STMDB))
		return true;
	return false;
}

void arm_expect(InstructionWithParams &c, ParamType type0 = ParamType::NONE, ParamType type1 = ParamType::NONE, ParamType type2 = ParamType::NONE) {
	ParamType t[3] = {type0, type1, type2};
	for (int i=0; i<3; i++)
		if (c.p[i].type != t[i])
			raise_error(format("param #%d expected to be %s: ", i+1, "???") + c.str());
}


void arm_ass_param(InstructionParam &pp, int p, int &code) {
	if (p == AP_FREG_0_5) {
		int fn = arm_freg_no(pp.reg);
		code |= (fn >> 1);
		if ((fn & 0x01) > 0)
			code |= 1 << 5;
	} else if (p == AP_FREG_12_22) {
		int fn = arm_freg_no(pp.reg);
		code |= (fn >> 1) << 12;
		if ((fn & 0x01) > 0)
			code |= 1 << 22;
	} else if (p == AP_FREG_16_7) {
		int fn = arm_freg_no(pp.reg);
		code |= (fn >> 1) << 16;
		if ((fn & 0x01) > 0)
			code |= 1 << 7;
	} else if (p == AP_REG_0) {
		int fn = arm_reg_no(pp.reg);
		code |= (fn & 0xf);
	} else if (p == AP_REG_8) {
		int fn = arm_reg_no(pp.reg);
		code |= (fn & 0xf) << 8;
	} else if (p == AP_REG_12) {
		int fn = arm_reg_no(pp.reg);
		code |= (fn & 0xf) << 12;
	} else if (p == AP_REG_16) {
		int fn = arm_reg_no(pp.reg);
		code |= (fn & 0xf) << 16;
	}
}

bool arm_ass_gen(InstructionWithParams &iwp, int &code) {
	for (auto &i: cpu_instructions_arm)
		if (i.inst == iwp.inst) {
			code |= i.code;
			arm_ass_param(iwp.p[0], i.p1, code);
			arm_ass_param(iwp.p[1], i.p2, code);
			arm_ass_param(iwp.p[2], i.p3, code);
			return true;
		}
	return false;
}


inline bool label_after_now(InstructionWithParamsList *list, int label_no, int now) {
	if (list->label[label_no].inst_no < 0)
		return true;
	return list->label[label_no].inst_no > now;
}

void InstructionWithParamsList::add_instruction_arm32(char *oc, int &ocs, int n) {
	InstructionWithParams &iwp = (*this)[n];
	current_inst = n;
	state.reset(this);

	if (iwp.inst == InstID::ALIGN_OPCODE)
		return;

	int code = 0;

	code = (int)iwp.condition << 28;
	int data_nn = -1;
	if (arm_is_data(iwp.inst, data_nn)) {
		if ((iwp.inst == InstID::CMP) or (iwp.inst == InstID::CMN) or (iwp.inst == InstID::TST) or (iwp.inst == InstID::TEQ) or (iwp.inst == InstID::MOV)) {
			iwp.p[2] = iwp.p[1];
			if ((iwp.inst == InstID::CMP) or (iwp.inst == InstID::CMN)) {
				iwp.p[1] = iwp.p[0];
				iwp.p[0] = param_reg(RegID::R0);
			} else {
				iwp.p[1] = param_reg(RegID::R0);
			}
		}
//		bool ss = (iwp.inst == InstID::CMP) or (iwp.inst == InstID::CMN) or (iwp.inst == InstID::TST) or (iwp.inst == InstID::TEQ);
		code |= 0x0 << 26;
		code |= (data_nn << 20);
	//	if (ss)
	//		code |= 1 << 20;
		if (iwp.p[2].type == ParamType::REGISTER) {
			arm_expect(iwp, ParamType::REGISTER, ParamType::REGISTER, ParamType::REGISTER);
			code |= arm_reg_no(iwp.p[2].reg) << 0;
			if (iwp.p[2].disp != DispMode::NONE)
				raise_error("p3.disp != DispMode::_NONE");
		} else if (iwp.p[2].type == ParamType::IMMEDIATE) {
			arm_expect(iwp, ParamType::REGISTER, ParamType::REGISTER, ParamType::IMMEDIATE);
			if (iwp.p[2].is_label) {
				add_wanted_label(ocs + 2, iwp.p[2].value, n, false, false, SIZE_8L4);
			} else {
				code |= arm_encode_8l4(iwp.p[2].value) << 0;
				code |= 1 << 25;
			}
		}/*else if (iwp.p[2].type == ParamType::REGISTER_SHIFT) {
			msg_write("TODO reg shift");
			code |= arm_reg_no(iwp.p[2].reg) << 0;
			code |= (iwp.p[2].value & 0xff) << 4;
		}*/ else {
			raise_error("unhandled param #3 in " + iwp.str());
		}
		code |= arm_reg_no(iwp.p[0].reg) << 12;
		code |= arm_reg_no(iwp.p[1].reg) << 16;
	} else if (arm_is_load_store_reg(iwp.inst)) {
		if (iwp.inst == InstID::LDR)
			code |= 0x04100000;
		else if (iwp.inst == InstID::LDRB)
			code |= 0x04500000;
		else if (iwp.inst == InstID::STR)
			code |= 0x04000000;
		else if (iwp.inst == InstID::STRB)
			code |= 0x04400000;

		if ((iwp.p[1].type == ParamType::IMMEDIATE) and (iwp.p[1].deref) and (iwp.p[1].is_label)) {
			add_wanted_label(ocs + 2, iwp.p[1].value, n, true, true, SIZE_12);
			iwp.p[1] = param_deref_reg_shift(RegID::R15, label_after_now(this, iwp.p[1].value, n) ? 1 : -1, SIZE_32);
		}

		if (iwp.p[0].reg == iwp.p[1].reg)
			raise_error("not allowed to use the same register for destination and addressing: " + iwp.str());

		code |= arm_reg_no(iwp.p[0].reg) << 12; // Rd
		code |= arm_reg_no(iwp.p[1].reg) << 16; // Rn

		if ((iwp.p[1].disp == DispMode::_8) or (iwp.p[1].disp == DispMode::_32)) {
			if ((iwp.p[1].value > 0x0fff) or (iwp.p[1].value < - 0x0fff))
				raise_error("offset larger than 12 bit: " + iwp.str());
			if (iwp.p[1].value >= 0)
				code |= 0x01800000 | (iwp.p[1].value & 0x0fff);
			else
				code |= 0x01000000 | ((-iwp.p[1].value) & 0x0fff);
		} else if (iwp.p[1].disp == DispMode::REG2) {
			if (iwp.p[1].value >= 0)
				code |= 0x03800000;
			else
				code |= 0x03000000;
			code |= arm_reg_no(iwp.p[1].reg2);
		}
	} else if (arm_is_load_store_multi(iwp.inst)) {
		arm_expect(iwp, ParamType::REGISTER, ParamType::IMMEDIATE);
		bool ll = ((iwp.inst == InstID::LDMIA) or (iwp.inst == InstID::LDMIB) or (iwp.inst == InstID::LDMDA) or (iwp.inst == InstID::LDMDB));
		bool uu = ((iwp.inst == InstID::LDMIA) or (iwp.inst == InstID::LDMIB) or (iwp.inst == InstID::STMIA) or (iwp.inst == InstID::STMIB));
		bool pp = ((iwp.inst == InstID::LDMIB) or (iwp.inst == InstID::LDMDB) or (iwp.inst == InstID::STMIB) or (iwp.inst == InstID::STMDB));
		bool ww = true;
		if (ll)
			code |= 0x08100000;
		else
			code |= 0x08000000;
		if (uu)
			code |= 0x00800000;
		if (pp)
			code |= 0x01000000;
		if (ww)
			code |= 0x00200000;
		code |= arm_reg_no(iwp.p[0].reg) << 16;
		code |= iwp.p[1].value & 0xffff;
	} else if (iwp.inst == InstID::MUL) {
		code |= 0x00000090;
		arm_expect(iwp, ParamType::REGISTER, ParamType::REGISTER, ParamType::REGISTER);
		code |= arm_reg_no(iwp.p[0].reg) << 16;
		code |= arm_reg_no(iwp.p[1].reg);
		code |= arm_reg_no(iwp.p[2].reg) << 8;
	} else if ((iwp.inst == InstID::BLX) or ((iwp.inst == InstID::CALL) and (iwp.p[0].type == ParamType::REGISTER))) {
		arm_expect(iwp, ParamType::REGISTER);
		code |= 0x012fff30;
		code |= arm_reg_no(iwp.p[0].reg);
	} else if ((iwp.inst == InstID::BL) or (iwp.inst == InstID::B) or (iwp.inst == InstID::JMP) or (iwp.inst == InstID::CALL)) {
		arm_expect(iwp, ParamType::IMMEDIATE);
		if ((iwp.inst == InstID::BL) or (iwp.inst == InstID::CALL))
			code |= 0x0b000000;
		else
			code |= 0x0a000000;
		int value = iwp.p[0].value;
		if (iwp.p[0].is_label) {
			add_wanted_label(ocs + 1, value, n, true, false, SIZE_24);
		} else if (iwp.inst == InstID::CALL)
			value = (iwp.p[0].value - (int_p)&oc[ocs] - 8) >> 2;
		code |= (value & 0x00ffffff);
	} else if (iwp.inst == InstID::DD) {
		arm_expect(iwp, ParamType::IMMEDIATE);
		code = iwp.p[0].value;
	} else if ((iwp.inst == InstID::FLDS) or (iwp.inst == InstID::FSTS)) {
		if (iwp.inst == InstID::FLDS)
			code |= 0x0d100a00;
		else
			code |= 0x0d000a00;
		if (iwp.inst == InstID::FSTS)
			std::swap(iwp.p[0], iwp.p[1]);

		if ((iwp.p[1].type == ParamType::IMMEDIATE) and (iwp.p[1].deref)) {
			if (iwp.p[1].is_label) {
				add_wanted_label(ocs + 3, iwp.p[1].value, n, true, true, SIZE_8S2);
				iwp.p[1] = param_deref_reg_shift(RegID::R15, label_after_now(this, iwp.p[1].value, n) ? 1 : -1, SIZE_32);
			} else {
				iwp.p[1] = param_deref_reg_shift(RegID::R15, iwp.p[1].value - (int_p)&oc[ocs] - 8, SIZE_32);
			}
		}

		int fd = arm_freg_no(iwp.p[0].reg);
		code |= (fd >> 1)<< 12;
		if ((fd & 1) > 0)
			code |= 1 << 22;

		code |= arm_reg_no(iwp.p[1].reg) << 16; // Rn

		if ((iwp.p[1].disp == DispMode::_8) or (iwp.p[1].disp == DispMode::_32)) {
			int v = (iwp.p[1].value >> 2);
			if ((v > 0x00ff) or (v < - 0x00ff))
				raise_error("offset larger than 8 bit: " + iwp.str());
			if (v >= 0)
				code |= 0x00800000 | (v & 0x00ff);
			else
				code |= 0x00000000 | ((-v) & 0x00ff);
		}
	} else if (arm_ass_gen(iwp, code)) {
	} else {
		raise_error("can not assemble instruction: " + iwp.str());
	}

	*(int*)&oc[ocs] = code;
	ocs += 4;
}

bool apply_param(int&code, const InstructionParam& p, int pf) {
	if (pf == AP_NONE and p.type == ParamType::NONE)
		return true;
	if (p.type == ParamType::REGISTER and !p.deref) {
		if ((pf == AP_REG_0P5 or pf == AP_REG_5P5 or pf == AP_REG_10P5) and (p.reg->id >= RegID::R0 and p.reg->id <= RegID::R31)) {
			auto r = arm_reg_no(p.reg);
			if (pf == AP_REG_0P5)
				code |= r << 0;
			else if (pf == AP_REG_5P5)
				code |= r << 5;
			else if (pf == AP_REG_10P5)
				code |= r << 10;
			return true;
		}
		if ((pf == AP_WREG_0P5 or pf == AP_WREG_5P5 or pf == AP_WREG_16) and (p.reg->id >= RegID::W0 and p.reg->id <= RegID::W31)) {
			auto r = arm_reg_no(p.reg);
			if (pf == AP_WREG_0P5)
				code |= r << 0;
			else if (pf == AP_WREG_5P5)
				code |= r << 5;
			else if (pf == AP_WREG_16)
				code |= r << 16;
			return true;
		}
		return false;
	}
	if (p.type == ParamType::REGISTER and p.deref) {
		if ((pf == AP_DEREF_S64_REG_5P5_PLUS_IMM12P10 or pf == AP_DEREF_S32_REG_5P5_PLUS_IMM12P10) and (p.reg->id >= RegID::R0 and p.reg->id <= RegID::R31)) {
			auto r = arm_reg_no(p.reg);
			code |= r << 5;
			int size = (pf == AP_DEREF_S32_REG_5P5_PLUS_IMM12P10) ? SIZE_32 : SIZE_64;
			code |= ((int)(p.value / size) & 0x00000fff) << 10;
			return true;
		}
		return false;
	}
	if (p.type == ParamType::IMMEDIATE) {
		if (pf == AP_IMM12_10SH) {
			if ((p.value & 0xfffffffffffff000) == 0)
				code |= p.value << 10;
			else if ((p.value & 0xffffffffff000fff) == 0)
				code |= p.value >> 2;
			else
				return false; //raise_error("immediate not supported");
			return true;
		} else if (pf == AP_IMM12_10) {
			if ((p.value & 0xfffffffffffff000) == 0)
				code |= (int)p.value << 10;
			else
				return false;
			return true;
		} else if (pf == AP_IMM26X4REL_0) {
			int64 val = p.value - (int_p)&code; // relative to rip
			//if ((val & 0xfffffffff0000003) == 0)
			if ((val < (2<<28)) and (val >= -(2<<28)))
				code |= ((unsigned int)(val) >> 2) & 0x03ffffff;
			else
				return false;
			return true;
		}
		msg_write("WRONG IMM");
		return false;
	}
	return false;
}

void InstructionWithParamsList::add_instruction_arm64(char *oc, int &ocs, int n) {
	InstructionWithParams &iwp = (*this)[n];
	current_inst = n;
	state.reset(this);

	if (iwp.inst == InstID::ALIGN_OPCODE)
		return;

	int& code = *(int*)&oc[ocs];
	code = 0;
	bool found = false;

	//msg_write("assemble: " + iwp.str());

	for (const auto& i: cpu_instructions_arm)
		if (i.inst == iwp.inst) {
			//msg_write("...");
			code = i.code;
			if (!apply_param(code, iwp.p[0], i.p1))
				continue;
			if (!apply_param(code, iwp.p[1], i.p2))
				continue;
			if (!apply_param(code, iwp.p[2], i.p3))
				continue;
			found = true;
			break;
		}

	if (!found)
		raise_error("can not assemble instruction: " + iwp.str());

	ocs += 4;
}
};
