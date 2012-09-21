/*----------------------------------------------------------------------------*\
| Nix light                                                                    |
| -> handling light sources                                                    |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"

Array<sLight> NixLight;

bool NixLightingEnabled;


// general ability of using lights
void NixEnableLighting(bool Enabled)
{
	NixLightingEnabled = Enabled;
	if (Enabled)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
}

int NixCreateLight()
{
	/*if (NumLights>=32)
		return -1;*/
/*#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		switch (NumLights){
			case 0:	OGLLightNo[NumLights]=GL_LIGHT0;	break;
			case 1:	OGLLightNo[NumLights]=GL_LIGHT1;	break;
			case 2:	OGLLightNo[NumLights]=GL_LIGHT2;	break;
			case 3:	OGLLightNo[NumLights]=GL_LIGHT3;	break;
			case 4:	OGLLightNo[NumLights]=GL_LIGHT4;	break;
			case 5:	OGLLightNo[NumLights]=GL_LIGHT5;	break;
			case 6:	OGLLightNo[NumLights]=GL_LIGHT6;	break;
			case 7:	OGLLightNo[NumLights]=GL_LIGHT7;	break;
			default:OGLLightNo[NumLights]=-1;
		}
	}
#endif
	NumLights++;
	return NumLights-1;*/
	for (int i=0;i<NixLight.num;i++)
		if (!NixLight[i].Used){
			NixLight[i].Used = true;
			NixLight[i].Enabled = false;
			return i;
		}
	sLight l;
	l.Used = true;
	l.Enabled = false;
	NixLight.add(l);
	return NixLight.num - 1;
}

void NixDeleteLight(int index)
{
	if ((index < 0) || (index > NixLight.num))	return;
	NixEnableLight(index, false);
	NixLight[index].Used = false;
}

// Punkt-Quelle
void NixSetLightRadial(int index,const vector &pos,float radius,const color &ambient,const color &diffuse,const color &specular)
{
	if ((index < 0) || (index > NixLight.num))	return;
	NixLight[index].Pos = pos;
	NixLight[index].Type = LightTypeRadial;
	//if (OGLLightNo[index]<0)	return;
	glPushMatrix();
	//glLoadIdentity();
	glLoadMatrixf((float*)&NixViewMatrix);
	float f[4];
	f[0]=pos.x;	f[1]=pos.y;	f[2]=pos.z;	f[3]=1;
	glLightfv(GL_LIGHT0+index,GL_POSITION,f); // GL_POSITION,(x,y,z,0)=directional,(x,y,z,1)=positional !!!
	glLightfv(GL_LIGHT0+index,GL_AMBIENT,(float*)&ambient);
	glLightfv(GL_LIGHT0+index,GL_DIFFUSE,(float*)&diffuse);
	glLightfv(GL_LIGHT0+index,GL_SPECULAR,(float*)&specular);
	glLightf(GL_LIGHT0+index,GL_CONSTANT_ATTENUATION,0.9f);
	glLightf(GL_LIGHT0+index,GL_LINEAR_ATTENUATION,2.0f/radius);
	glLightf(GL_LIGHT0+index,GL_QUADRATIC_ATTENUATION,1/(radius*radius));
	glPopMatrix();
}

// parallele Quelle
// dir =Richtung, in die das Licht scheinen soll
void NixSetLightDirectional(int index,const vector &dir,const color &ambient,const color &diffuse,const color &specular)
{
	if ((index < 0) || (index > NixLight.num))	return;
	NixLight[index].Dir = dir;
	NixLight[index].Type = LightTypeDirectional;
	//if (OGLLightNo[index]<0)	return;
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	//glLoadIdentity();
	glLoadMatrixf((float*)&NixViewMatrix);
	float f[4];
	f[0]=dir.x;	f[1]=dir.y;	f[2]=dir.z;	f[3]=0;
	glLightfv(GL_LIGHT0+index,GL_POSITION,f); // GL_POSITION,(x,y,z,0)=directional,(x,y,z,1)=positional !!!
	glLightfv(GL_LIGHT0+index,GL_AMBIENT,(float*)&ambient);
	glLightfv(GL_LIGHT0+index,GL_DIFFUSE,(float*)&diffuse);
	glLightfv(GL_LIGHT0+index,GL_SPECULAR,(float*)&specular);
	glPopMatrix();
}

void NixEnableLight(int index,bool enabled)
{
	if ((index < 0) || (index > NixLight.num))	return;
	NixLight[index].Enabled = enabled;
//	if (OGLLightNo[index]<0)	return;
	if (enabled)
		glEnable(GL_LIGHT0 + index);
	else
		glDisable(GL_LIGHT0 + index);
}

void NixSetAmbientLight(const color &c)
{
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,(float*)&c);
}


void NixSetShading(int mode)
{
	if (mode==ShadingPlane)
		glShadeModel(GL_FLAT);
	if (mode==ShadingRound)
		glShadeModel(GL_SMOOTH);
}

void NixSetMaterial(const color &ambient,const color &diffuse,const color &specular,float shininess,const color &emission)
{
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,(float*)&ambient);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,(float*)&diffuse);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,(float*)&specular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,(float*)&shininess);
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,(float*)&emission);
}

void NixSpecularEnable(bool enabled)
{
}
