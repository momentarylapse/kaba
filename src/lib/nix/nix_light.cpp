/*----------------------------------------------------------------------------*\
| Nix light                                                                    |
| -> handling light sources                                                    |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#if HAS_LIB_GL

#include "nix.h"
#include "nix_common.h"

namespace nix{


// light-sources
/*enum{
	LightTypeDirectional,
	LightTypeRadial
};
struct sLight{
	bool Used,Allowed,Enabled;
	int Type;
	int OGLLightNo;
	int Light;
	vector Pos,Dir;
	float Radius;
	color Ambient,Diffuse,Specular;
};

Array<sLight> NixLight;*/

Material material;
Light lights[8];


void TestGLError(const char*);



// Punkt-Quelle
void SetLightRadial(int index, const vector &pos, float radius, const color &col, float harshness) {
	if ((index < 0) or (index >= 8))
		return;
	index = 0;

	lights[index].col = col;
	lights[index].harshness = harshness;
	lights[index].pos = pos;
	lights[index].radius = radius;
}

// parallele Quelle
// dir =Richtung, in die das Licht scheinen soll
void SetLightDirectional(int index, const vector &dir, const color &col, float harshness) {
	if ((index < 0) or (index >= 8))
		return;
	index = 0;

	lights[index].col = col;
	lights[index].harshness = harshness;
	lights[index].pos = dir;
	lights[index].radius = -1;
}

void EnableLight(int index,bool enabled) {
	if ((index < 0) or (index >= 8))
		return;
	lights[index].enabled = true;
}

void SetMaterial(const color &ambient,const color &diffuse,const color &specular,float shininess,const color &emission) {
	material.ambient = ambient;
	material.diffusive = diffuse;
	material.specular = specular;
	material.shininess = shininess;
	material.emission = emission;
}



};

#endif
