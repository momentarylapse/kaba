/*----------------------------------------------------------------------------*\
| Nix light                                                                    |
| -> handling light sources                                                    |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"


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

bool NixLightingEnabled;

void TestGLError(const string &);


// general ability of using lights
void NixEnableLighting(bool Enabled)
{
	TestGLError("EnableLighting prae");
	NixLightingEnabled = Enabled;
	if (Enabled)
		glEnable(GL_LIGHTING);
	else
		glDisable(GL_LIGHTING);
	TestGLError("EnableLighting");
}

// Punkt-Quelle
void NixSetLightRadial(int index,const vector &pos,float radius,const color &ambient,const color &diffuse,const color &specular)
{
	if ((index < 0))// || (index > NixLight.num))
		return;
	/*NixLight[index].Pos = pos;
	NixLight[index].Type = LightTypeRadial;*/
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
	TestGLError("SetLightRad");
}

// parallele Quelle
// dir =Richtung, in die das Licht scheinen soll
void NixSetLightDirectional(int index,const vector &dir,const color &ambient,const color &diffuse,const color &specular)
{
	if ((index < 0))// || (index > NixLight.num))
		return;
	/*NixLight[index].Dir = dir;
	NixLight[index].Type = LightTypeDirectional;*/
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
	TestGLError("SetLightDir");
}

void NixEnableLight(int index,bool enabled)
{
	if ((index < 0))// || (index > NixLight.num))
		return;
	//NixLight[index].Enabled = enabled;
	if (enabled)
		glEnable(GL_LIGHT0 + index);
	else
		glDisable(GL_LIGHT0 + index);
	TestGLError("EnableLight");
}

void NixSetAmbientLight(const color &c)
{
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT,(float*)&c);
	TestGLError("SetAmbient");
}


void NixSetShading(int mode)
{
	if (mode==ShadingPlane)
		glShadeModel(GL_FLAT);
	if (mode==ShadingRound)
		glShadeModel(GL_SMOOTH);
	TestGLError("SetShading");
}

void NixSetMaterial(const color &ambient,const color &diffuse,const color &specular,float shininess,const color &emission)
{
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,(float*)&ambient);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,(float*)&diffuse);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,(float*)&specular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,(float*)&shininess);
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,(float*)&emission);
	TestGLError("SetMat");
}

void NixSpecularEnable(bool enabled)
{
}



void NixUpdateLights()
{
#if 0
	// OpenGL muss Lichter neu ausrichten, weil sie in Kamera-Koordinaten gespeichert werden!
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	//glLoadIdentity();
	glLoadMatrixf((float*)&NixViewMatrix);

	foreachi(sLight &l, NixLight, i){
		if (!l.Used)
			continue;
		if (!l.Enabled)
			continue;
	//	if (OGLLightNo[i]<0)	continue;
		float f[4];
		/*f[0]=LightVector[i].x;	f[1]=LightVector[i].y;	f[2]=LightVector[i].z;
		if (LightDirectional[i])
			f[3]=0;
		else
			f[3]=1;
		glLightfv(OGLLightNo[i],GL_POSITION,f);*/
		if (l.Type == LightTypeDirectional){
			f[0] = l.Dir.x;
			f[1] = l.Dir.y;
			f[2] = l.Dir.z;
			f[3] = 0;
		}else if (l.Type == LightTypeRadial){
			f[0] = l.Pos.x;
			f[1] = l.Pos.y;
			f[2] = l.Pos.z;
			f[3] = 1;
		}
		glLightfv(GL_LIGHT0+i,GL_POSITION,f);
		//msg_write(i);
	}
	glPopMatrix();
	TestGLError("UpdateLights");
#endif
}
