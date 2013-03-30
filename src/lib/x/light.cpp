#include "x.h"

#ifdef _X_ALLOW_LIGHT_

#include "../nix/nix.h"
#include "../base/set.h"

namespace Light
{


struct sLight{
	bool Used;
	bool Directional, Enabled;
	vector Pos, Dir;
	float Radius;
	color Ambient, Diffuse, Specular;
};

struct sLightField{
	vector Min, Max;
	bool SunEnabled;
	color Ambient;
};

#define NUM_ACTIVE_POINT_LIGHTS		3
#define NUM_ACTIVE_LIGHTS			(NUM_ACTIVE_POINT_LIGHTS + 1)

#define FX_MAX_LIGHT_FIELDS			64

static Array<sLight> Lights;
static int ActiveLights[NUM_ACTIVE_LIGHTS];
static int SunLight;

// light fields
bool FxLightFieldsEnabled;
static int NumLightFields;
static sLightField *LightField[FX_MAX_LIGHT_FIELDS];


void Reset()
{
	EndApply();
	Lights.clear();
	SunLight = -1;
}


int Create()
{
	msg_write("new light");
	for (int i=0;i<Lights.num;i++)
		if (!Lights[i].Used){
			Lights[i].Enabled = false;
			Lights[i].Used = true;
			return i;
		}
	sLight l;
	l.Used = true;
	l.Enabled = false;
	Lights.add(l);
	return Lights.num - 1;
}

void Delete(int index)
{
	if ((index < 0) || (index >= Lights.num))
		return;
	Lights[index].Used = false;
	Lights[index].Enabled = false;
	if (SunLight == index)
		SunLight = -1;
}

void SetColors(int index, const color &am, const color &di, const color &sp)
{
	if ((index < 0) || (index >= Lights.num))
		return;
	Lights[index].Ambient = am;
	Lights[index].Diffuse = di;
	Lights[index].Specular = sp;
}

void SetDirectional(int index, const vector &dir)
{
	if ((index < 0) || (index >= Lights.num))
		return;
	Lights[index].Directional = true;
	Lights[index].Dir = dir;
	SunLight = index;
}

void SetRadial(int index, const vector &pos, float radius)
{
	if ((index < 0) || (index >= Lights.num))
		return;
	Lights[index].Directional = false;
	Lights[index].Pos = pos;
	Lights[index].Radius = radius;
}

void Enable(int index, bool enabled)
{
	if ((index < 0) || (index >= Lights.num))
		return;
	Lights[index].Enabled = enabled;
}

void BeginApply()
{
	for (int i=0;i<NUM_ACTIVE_LIGHTS;i++){
		NixEnableLight(i, false);
		ActiveLights[i] = -1;
	}
	if (SunLight >= 0)
		NixSetLightDirectional(0, Lights[SunLight].Dir, Lights[SunLight].Ambient, Lights[SunLight].Diffuse, Lights[SunLight].Specular);
}

void EndApply()
{
	for (int i=0;i<NUM_ACTIVE_LIGHTS;i++){
		NixEnableLight(i, false);
		ActiveLights[i] = -1;
	}
}


struct BestLight
{
	int index;
	float w;
	bool operator < (const BestLight &b) const
	{	return w < b.w;	}
	bool operator > (const BestLight &b) const
	{	return w > b.w;	}
	bool operator == (const BestLight &b) const
	{	return w == b.w;	}
};

inline void SetLights(bool enable_sun, Set<BestLight> &best)
{
	if (SunLight >= 0)
		if (enable_sun != (ActiveLights[0] >= 0)){
			NixEnableLight(0, enable_sun);
			ActiveLights[0] = enable_sun ? SunLight : -1;
		}

	foreachi(BestLight &b, best, i){
		sLight &l = Lights[b.index];
		NixSetLightRadial(i + 1, l.Pos, l.Radius, l.Ambient, l.Diffuse, l.Specular);
		NixEnableLight(i + 1, true);
		ActiveLights[i + 1] = b.index;
	}
	for (int i=best.num;i<NUM_ACTIVE_POINT_LIGHTS;i++){
		ActiveLights[i + 1] = -1;
		NixEnableLight(i + 1, false);
	}
}

void Apply(const vector &pos)
{
	Set<BestLight> best;

	foreachi(sLight &l, Lights, i){
		if ((!l.Used) || (!l.Enabled))
			continue;
		if (l.Directional)
			continue;
		BestLight b;
		b.index = i;
		b.w = (l.Pos - pos).length_fuzzy() / l.Radius;
		if (b.w > 2)
			continue;
		best.add(b);
		if (best.num > NUM_ACTIVE_POINT_LIGHTS)
			best.resize(NUM_ACTIVE_POINT_LIGHTS);
	}

	SetLights(true, best);
}

vector GetSunDir()
{
	if (SunLight < 0)
		return e_z;
	return Lights[SunLight].Dir;
}


#if 0
void FxAddLighField(const vector &min,const vector &max,bool sun,const color &ambient)
{
	LightField[NumLightFields]=new sLightField;
	LightField[NumLightFields]->Min=min;
	LightField[NumLightFields]->Max=max;
	LightField[NumLightFields]->SunEnabled=sun;
	LightField[NumLightFields]->Ambient=ambient;
	NumLightFields++;
}

void FxTestForLightField(const vector &pos)
{
#ifdef _X_ALLOW_SKY_
/*	if ((!LightFieldsEnabled)||(!sky->WeatherDeltaTime)||(!sky->AstronomyEnabled))	return;
	for (int i=0;i<NumLightFields;i++)
		if (VecBetween(pos,LightField[i]->Min,LightField[i]->Max)){
			LightEnable(0,LightField[i]->SunEnabled);
			NixSetAmbientLight(LightField[i]->Ambient);
			NixEnableFog(!LightField[i]->SunEnabled);
		}*/
#endif
}

void FxResetLightField()
{
#ifdef _X_ALLOW_SKY_
	if ((!LightFieldsEnabled)||(!sky->WeatherDeltaTime)||(!sky->AstronomyEnabled))	return;
/*	if (NumLights>0){
		LightEnable(0,true);
		NixEnableFog(true);
		NixSetAmbientLight(sky->SunAmbient);
	}*/
#endif
}
#endif

};

#endif
