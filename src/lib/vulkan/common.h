/*
 * common.h
 *
 *  Created on: 17.09.2022
 *      Author: michi
 */
#pragma once

#if HAS_LIB_VULKAN

namespace vulkan {

extern int verbosity;

enum class Requirements {
	NONE = 0,
	ANISOTROPY = 1,
	SWAP_CHAIN = 2,
	PRESENT = 4,
	GRAPHICS = 8,
	COMPUTE = 16,
    VALIDATION = 32,
    RTX = 64,
	MESH_SHADER = 128,
	GEOMETRY_SHADER = 256,
	TESSELATION_SHADER = 512
};
inline bool operator&(Requirements a, Requirements b) {
	return ((int)a & (int)b);
}
inline Requirements operator|(Requirements a, Requirements b) {
	return (Requirements)((int)a | (int)b);
}


}


#endif
