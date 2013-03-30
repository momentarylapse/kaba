/*----------------------------------------------------------------------------*\
| Fx                                                                           |
| -> manages particle effects                                                  |
| -> auxiliary functions for drawing shapes and effects (mirrors, shadows,...) |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2009.11.22 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "x.h"

string FxVersion = "0.3.3.0";



#define FX_MAX_FORCEFIELDS		128
#define FX_MAX_ENABLED_LIGHTS	16
#define FX_MAX_LIGHT_FIELDS		128
#define FX_MAX_SHADOWS			256
#define FX_MAX_MIRRORS			128
#define FX_MAX_TAILS			128

#ifdef _X_ALLOW_X_
	#include "../script/script.h"
#endif
#ifdef _X_USE_SOUND_
	#include "../sound/sound.h"
#endif

static int FxVB;



struct sShadow{
	matrix *mat;
	//int NumEdges;
	//int *e;
	const vector *v;
	Skin *skin;
	float Length,Density;
	vector LightSource; // position or direction (depending on Parallel)
	int num_mat;
	bool Parallel;
	bool slow;
};

struct sCubeMap{
	int CubeMap;
	Model *model;
	bool Dynamical;
	int Frame;
	int Size;
};

struct sMirror{
	vector p[3];
};

// particles
static Array<Particle*> Particles;
static Array<Particle*> Beams;

// effects
static Array<Effect*> Effects;

// force fields
//int NumForceFields;
//sForceField *ForceField[FX_MAX_FORCEFIELDS];

static Array<sShadow> Shadow;

static int NumMirrors;
static sMirror *Mirror[FX_MAX_MIRRORS];
render_func *FxRenderFunc;
static int MirrorLevel;

static Array<sCubeMap> CubeMap;

static int NumTails;
static Tail *Tails[FX_MAX_TAILS];

int MirrorLevelMax=1;

vector CamDir;

int ShadowVB[2];
int ShadowCounter = 0;
bool ShadowRecalc = true;

#define MODEL_MAX_TRIANGLES		65536
#define MODEL_MAX_VERTICES		65536

void FxInit()
{
	msg_db_r("FxInit",0);
	FxVB = NixCreateVB(MODEL_MAX_TRIANGLES * 4, 1);
	ShadowVB[0] = NixCreateVB(32768, 1);
	ShadowVB[1] = NixCreateVB(32768, 1);
	NumForceFields=0;
	NumTails=0;
	FxRenderFunc=NULL;
	msg_db_l(0);
}

void FxReset()
{
	msg_db_r("FxReset",1);
	
	// particles
	foreach(Particle *p, Particles)
		delete(p);
	Particles.clear();
	
	// beams
	foreach(Particle *b, Beams)
		delete(b);
	Beams.clear();
	
	// effects
	foreach(Effect *e, Effects)
		delete(e); // TODO call script-destructor
	Effects.clear();

	// shadows
	Shadow.clear();

	// stuff
	/*msg_db_m("force fields",2);
	for (int i=0;i<NumForceFields;i++)
		FxForceFieldDelete(i);*/
	msg_db_l(1);
}

// for DEBUG output
int fx_get_num_effects()
{
	int n=0;
	foreach(Effect *fx, Effects)
		if (fx->used)
			n ++;
	return n;
}

// for DEBUG output
int fx_get_num_particles()
{
	int n=0;
	foreach(Particle *p, Particles)
		if (p->used)
			n ++;
	foreach(Particle *p, Beams)
		if (p->used)
			n ++;
	return n;
}



//#########################################################################
// particles
//#########################################################################

inline void particle_init(Particle *p, const vector &pos, const vector &param, int texture, particle_callback *func, float time_to_live, float radius)
{
	p->enabled = true;
	p->suicidal = (time_to_live >= 0);
	p->texture = texture;
	p->source = r_id;
	p->pos = pos;
	p->parameter = param;
	p->vel = v_0;
	p->_color = White;
	p->time_to_live = time_to_live;
	p->func_delta_t = 0.05f;
	p->elapsed = p->func_delta_t;
	p->func = func;
	p->radius = radius;
}

/*Particle *FxParticleCreate(int type, const vector &pos, const vector &param, int texture, particle_callback *func, float time_to_live, float radius)
{
	msg_db_r("new particle",2);
	xcont_find_new(Particle, p, Particles);
	msg_db_l(2);
	return p;
}*/

Particle *FxParticleCreateDef(const vector &pos, int texture, particle_callback *func, float time_to_live, float radius)
{
	msg_db_r("new particle",2);
	xcont_find_new(XContainerParticle, Particle, p, Particles);
	particle_init(p, pos, v_0, texture, func, time_to_live, radius);
	msg_db_l(2);
	return p;
}

Particle *FxParticleCreateRot(const vector &pos, const vector &ang, int texture, particle_callback *func, float time_to_live, float radius)
{
	msg_db_r("new particle rot",2);
	xcont_find_new(XContainerParticleRot, Particle, p, Particles);
	particle_init(p, pos, ang, texture, func, time_to_live, radius);
	msg_db_l(2);
	return p;
}

Particle *FxParticleCreateBeam(const vector &pos, const vector &length, int texture, particle_callback *func, float time_to_live, float radius)
{
	msg_db_r("new beam",2);
	xcont_find_new(XContainerParticleBeam, Particle, p, Beams);
	particle_init(p, pos, length, texture, func, time_to_live, radius);
	msg_db_l(2);
	return p;
}

void FxParticleDelete(Particle *particle)
{
	particle->used = false;
}

//#########################################################################
// effects
//#########################################################################

Effect *_cdecl FxCreate(const vector &pos,particle_callback *func,particle_callback *del_func,float time_to_live)
{
	msg_db_r("new effect",2);
	xcont_find_new(XContainerEffect, Effect, p, Effects);
	p->enabled = true;
	p->suicidal = (time_to_live >= 0);
	p->pos = pos;
	p->vel = v_0;
	p->time_to_live = time_to_live;
	p->func_delta_t = 0.1f;
	p->elapsed = p->func_delta_t;
	p->func = func;
	p->func_del = del_func;
	p->func_enable = NULL;
	p->model = NULL;
	p->type = FXTypeScript;
	msg_db_l(2);
	return p;
}
Effect *FxCreateScript(Model *m, int vertex, const string &filename)
{
	msg_db_r("FxCreateScript", 2);
	Effect *fx = FxCreate(m->GetVertex(vertex, 0), NULL, NULL, -1);
	fx->vertex = vertex;
	fx->model = m;
	fx->type = FXTypeScript;
#ifdef _X_ALLOW_X_
	Script::Script *s = Script::Load(filename);
	if (!s->Error){
		particle_callback *func_create = (particle_callback*)s->MatchFunction("OnEffectCreate", "void", 1, "effect");
		fx->func = (particle_callback*)s->MatchFunction("OnEffectIterate", "void", 1, "effect");
		fx->func_del = (particle_callback*)s->MatchFunction("OnEffectDelete", "void", 1, "effect");
		fx->func_enable = (effect_enable_func*)s->MatchFunction("OnEffectEnable", "void", 2, "effect", "bool");
		if (func_create)
			func_create(fx);
	}
#endif
	msg_db_l(2);
	return fx;
}

Effect *FxCreateLight(Model *m, int vertex, float radius, const color &am, const color &di, const color &sp)
{
	msg_db_r("FxCreateLight", 2);
	Effect *fx = FxCreate(m->GetVertex(vertex, 0), NULL, NULL, -1);
	fx->vertex = vertex;
	fx->model = m;
	fx->type = FXTypeLight;
	fx->script_var.resize(14);
#ifdef _X_ALLOW_LIGHT_
	*(int*)&fx->script_var[0] = Light::Create();
#endif
	fx->script_var[1] = radius;
	*(color*)&fx->script_var[2] = am;
	*(color*)&fx->script_var[6] = di;
	*(color*)&fx->script_var[10] = sp;
	msg_db_l(2);
	return fx;
}

Effect *FxCreateSound(Model *m, int vertex, const string &filename, float radius, float speed)
{
	msg_db_r("FxCreateSound", 2);
	Effect *fx = FxCreate(m->GetVertex(vertex, 0), NULL, NULL, -1);
	fx->vertex = vertex;
	fx->model = m;
	fx->type = FXTypeSound;
	fx->script_var.resize(3);
#ifdef _X_USE_SOUND_
	*(int*)&fx->script_var[0] = SoundLoad(filename);
#endif
	fx->script_var[1] = radius;
	fx->script_var[2] = speed;
	msg_db_l(2);
	return fx;
}

void _cdecl FxDelete(Effect *effect)
{
	msg_db_r("FxDelete", 3);
	if (effect)
		for (int i=0;i<Effects.num;i++)
			if (effect == Effects[i]){
				effect->used = false;
				effect->enabled = false;

				// send a "kill" message
				if (effect->func_del)
					effect->func_del(effect);

				// remove from model
				if (effect->model)
					for (int j=0;j<effect->model->fx.num;j++)
						if (effect->model->fx[j] == effect)
							effect->model->fx[j] = NULL;

				effect->script_var.clear();
			}
	msg_db_l(3);
}

void FxEnable(Effect *fx, bool enabled)
{
	msg_db_r("FxEnable", 3);
	fx->enabled = enabled;
	if (fx->type == FXTypeSound){
#ifdef _X_USE_SOUND_
		if (enabled)
			SoundSetData(*(int*)&fx->script_var[0], fx->pos, fx->vel, fx->script_var[1], fx->script_var[1] * 0.2f, 1, fx->script_var[2]);
		else
			SoundSetData(*(int*)&fx->script_var[0], fx->pos, fx->vel, fx->script_var[1], fx->script_var[1] * 0.2f, 0, 0);
#endif
	}else if (fx->type == FXTypeLight){
#ifdef _X_ALLOW_LIGHT_
		Light::Enable(*(int*)&fx->script_var[0], enabled);
#endif
	}

	if (fx->func_enable)
		fx->func_enable(fx, enabled);
	msg_db_l(3);
}

#if 0
Effect *FxCreateByModel(Model *m, sModelEffectData *data)
{
#ifdef _X_ALLOW_X_
	msg_db_r("new effect model",2);
	vector pos = v0; // ...well, not yet.... would have to be managed and not loaded unless model has been drawn once
	Effect *fx = FxCreate(pos, NULL, NULL, -1);
	fx->type = data->type;
	fx->model = m;
	fx->vertex = data->vertex;
	if (data->type == FXTypeScript){
		char *f = GetScriptFunction(data->filename, (char*)data->paramsf);
		fx->ScriptVar.resize(2);
		((int*)fx->ScriptVar.data)[0] = (int)(long)f;
		fx->type = FXTypeScriptPreLoad;
	}else{
		if (data->type == FXTypeLight){
			fx->ScriptVar.resize(14);
			((int*)fx->ScriptVar.data)[0] = FxLightCreate();
			memcpy(&((int*)fx->ScriptVar.data)[1], &data->paramsf, sizeof(float) * 13);
		}else if (data->type == FXTypeSound){
			fx->ScriptVar.resize(3);
			((int*)fx->ScriptVar.data)[0] = MetaSoundManagedNew(data->filename);
			((float*)fx->ScriptVar.data)[1] = data->paramsf[0];
			((float*)fx->ScriptVar.data)[2] = data->paramsf[1];
		}
		//fx->Enabled=false;
	}
	msg_db_l(2);
	return fx;
#else
	return NULL;
#endif
}

void FxUpdateByModel(Effect *&effect,const vector &pos,const vector &last_pos)
{
	if (!effect)
		return;
	msg_db_r("FxUpdateByModel", 1);
	effect->Pos=pos;
	if (Elapsed>0)
		effect->Vel=(pos-last_pos)/Elapsed;
	effect->Enabled=true;
	if (effect->type==FXTypeScriptPreLoad){
		msg_db_r("script effect",1);
		typedef Effect* t_fx_creation_func(const vector*);
		t_fx_creation_func *f = *(t_fx_creation_func**)effect->ScriptVar.data;
		Effect *fx=f(&pos);
		fx->model=effect->model;
		fx->vertex=effect->vertex;
		FxDelete(effect);
		effect=fx;
		msg_db_l(1);
	}
	msg_db_l(1);
}

void FxResetByModel()
{
	for (int i=0;i<Effects.num;i++)
		if (Effects[i]->type!=FXTypeScript){
			Effects[i]->Enabled=false;
		}
}
#endif

//#########################################################################
// force fields
//#########################################################################
#if 0
int FxForceFieldNew()
{
    int i,n=-1;
	msg_db_r("new forcefield",2);
	msg_db_m(i2s(NumForceFields),3);
	for (i=0;i<NumForceFields;i++)
		if (!ForceField[i]->Used){
			n=i;
			break;
		}
	if (n<0){
		if (NumForceFields>=FX_MAX_FORCEFIELDS){
			msg_error("FX: too many force fields!");
			msg_db_l(2);
			return -1;
		}
		n=NumForceFields;
		ForceField[n]=new sForceField;
		NumForceFields++;
	}
	ForceField[n]->Used=true;
	FxForceFieldEnable(n,true);
	msg_db_l(2);
	return n;
}

void FxForceFieldDelete(int index)
{
	if ((index<0)||(index>=NumForceFields))	return;
	if (!ForceField[index]->Used)	return;
	ForceField[index]->Used=false;
}

void FxForceFieldCreate(int index,vector pos,float radius,float strength,bool inv_quad)
{
	if (index<0)	return;
	ForceField[index]->RadiusMin=radius/1000;
	ForceField[index]->RadiusMax=radius;
	ForceField[index]->Strength=strength;
	ForceField[index]->InvQuadratic=inv_quad;
	ForceField[index]->Enabled=false;
}

void FxForceFieldSetPos(int index,vector pos)
{
	if (index<0)	return;
	ForceField[index]->Pos=pos;
	ForceField[index]->Enabled=true;
}

void FxForceFieldEnable(int index,bool enabled)
{
	if (index<0)	return;
	ForceField[index]->Enabled=enabled;
}
#endif


//#########################################################################
// cube maps
//#########################################################################
int FxCubeMapNew(int size)
{
	sCubeMap c;
	c.CubeMap = NixCreateCubeMap(size);
	c.Size = size;
	c.Dynamical = false;
	c.Frame = -2;
	CubeMap.add(c);
	return CubeMap.num - 1;
}

void FxCubeMapCreate(int cube_map,Model *m)
{
	if (cube_map < 0)
		return;
	CubeMap[cube_map].Dynamical=true;
	CubeMap[cube_map].model=m;
	CubeMap[cube_map].Frame=-2;
}

void FxCubeMapCreate(int cube_map,int tex0,int tex1,int tex2,int tex3,int tex4,int tex5)
{
	if (cube_map < 0)
		return;
	CubeMap[cube_map].Dynamical=false;
	NixFillCubeMap(CubeMap[cube_map].CubeMap, 0, tex0);
	NixFillCubeMap(CubeMap[cube_map].CubeMap, 1, tex1);
	NixFillCubeMap(CubeMap[cube_map].CubeMap, 2, tex2);
	NixFillCubeMap(CubeMap[cube_map].CubeMap, 3, tex3);
	NixFillCubeMap(CubeMap[cube_map].CubeMap, 4, tex4);
	NixFillCubeMap(CubeMap[cube_map].CubeMap, 5, tex5);
}

void FxCubeMapDraw(int cube_map,int buffer,float density)
{
	if (cube_map < 0)
		return;
	bool el = NixLightingEnabled;
	NixEnableLighting(!CubeMap[cube_map].Dynamical);
	NixSetAlpha(density);
	NixDraw3DCubeMapped(CubeMap[cube_map].CubeMap, buffer);
	NixSetAlpha(AlphaNone);
	NixEnableLighting(el);
}

//#########################################################################
// Zeuchs
//#########################################################################
void FxCreatePolygon(int buffer,int num_points,const vector *p)
{
	plane pl;
	PlaneFromPoints(pl,p[0],p[1],p[2]);
	vector n=pl.n;
	for (int i=0;i<num_points-2;i++)
		NixVBAddTria(buffer,	p[0],n,0,0,
								p[i+1],n,0,0,
								p[i+2],n,0,0);
}

void FxDrawPolygon(int num_points,const vector *p)
{
	NixVBClear(VBTemp);
	FxCreatePolygon(VBTemp,num_points,p);
	NixDraw3D(VBTemp);
}

void FxCreateBall(int buffer,const vector &pos,float radius,int nx,int ny)
{
	for (int x=0;x<nx;x++)
		for (int y=0;y<ny;y++){
			vector v;
			vector n0=vector(pi*(x  -nx/2)/nx,pi*2.0f* y   /ny,0).ang2dir();
			vector n1=vector(pi*(x+1-nx/2)/nx,pi*2.0f* y   /ny,0).ang2dir();
			vector n2=vector(pi*(x  -nx/2)/nx,pi*2.0f*(y+1)/ny,0).ang2dir();
			vector n3=vector(pi*(x+1-nx/2)/nx,pi*2.0f*(y+1)/ny,0).ang2dir();
			vector p0=pos+radius*n0;
			vector p1=pos+radius*n1;
			vector p2=pos+radius*n2;
			vector p3=pos+radius*n3;
			NixVBAddTria(buffer,	p0,n0,float(x  )/(float)nx,float(y  )/(float)ny,
									p1,n1,float(x+1)/(float)nx,float(y  )/(float)ny,
									p2,n2,float(x  )/(float)nx,float(y+1)/(float)ny);
			NixVBAddTria(buffer,	p2,n2,float(x  )/(float)nx,float(y+1)/(float)ny,
									p1,n1,float(x+1)/(float)nx,float(y  )/(float)ny,
									p3,n3,float(x+1)/(float)nx,float(y+1)/(float)ny);
		}
}

void FxDrawBall(const vector &pos,float radius,int nx,int ny)
{
	NixVBClear(FxVB);
	FxCreateBall(FxVB,pos,radius,nx,ny);
	NixDraw3D(FxVB);
}

void FxCreateSpat(int buffer,const vector &p0,const vector &p1,const vector &p2,const vector &p3)
{
	vector d=VecCrossProduct(p1-p0,p2-p0);
	plane pl;
	PlaneFromPoints(pl,p0,p1,p2);
	vector p0d=p0+d;
	float d1=pl.distance(p0d);
	float d2=pl.distance(p3);
	vector _p1, _p2;
	if (((d1>0)&&(d2<0))||((d1<0)&&(d2>0))){
		_p1 = p2;
		_p2 = p1;
	}else{
		_p1 = p1;
		_p2 = p2;
	}
	vector dp1=_p1-p0;
	vector dp2=_p2-p0;
	vector dp3= p3-p0;
	vector n;
	// vorne
	n=VecCrossProduct(dp2,dp1);
	n.normalize();
	NixVBAddTria(buffer,	p0,n,0,0,
							p0+dp2,n,0,0,
							p0+dp1+dp2,n,0,0);
	NixVBAddTria(buffer,	p0+dp1+dp2,n,0,0,
							p0+dp1,n,0,0,
							p0,n,0,0);
	// oben
	n=VecCrossProduct(dp3,dp1);
	n.normalize();
	NixVBAddTria(buffer,	p0+dp2,n,0,0,
							p0+dp2+dp3,n,0,0,
							p0+dp1+dp2+dp3,n,0,0);
	NixVBAddTria(buffer,	p0+dp1+dp2+dp3,n,0,0,
							p0+dp1+dp2,n,0,0,
							p0+dp2,n,0,0);
	// unten
	n=VecCrossProduct(dp1,dp3);
	n.normalize();
	NixVBAddTria(buffer,	p0,n,0,0,
							p0+dp1,n,0,0,
							p0+dp1+dp3,n,0,0);
	NixVBAddTria(buffer,	p0+dp1+dp3,n,0,0,
							p0+dp3,n,0,0,
							p0,n,0,0);
	// rechts
	n=VecCrossProduct(dp2,dp3);
	n.normalize();
	NixVBAddTria(buffer,	p0+dp1,n,0,0,
							p0+dp1+dp2,n,0,0,
							p0+dp1+dp2+dp3,n,0,0);
	NixVBAddTria(buffer,	p0+dp1+dp2+dp3,n,0,0,
							p0+dp1+dp3,n,0,0,
							p0+dp1,n,0,0);
	// links
	n=VecCrossProduct(dp3,dp2);
	n.normalize();
	NixVBAddTria(buffer,	p0,n,0,0,
							p0+dp3,n,0,0,
							p0+dp2+dp3,n,0,0);
	NixVBAddTria(buffer,	p0+dp2+dp3,n,0,0,
							p0+dp2,n,0,0,
							p0,n,0,0);
	// hinten
	n=VecCrossProduct(dp1,dp2);
	n.normalize();
	NixVBAddTria(buffer,	p0+dp3,n,0,0,
							p0+dp1+dp3,n,0,0,
							p0+dp1+dp2+dp3,n,0,0);
	NixVBAddTria(buffer,	p0+dp1+dp2+dp3,n,0,0,
							p0+dp2+dp3,n,0,0,
							p0+dp3,n,0,0);
}

void FxDrawSpat(const vector &p0,const vector &p1,const vector& p2,const vector &p3)
{
	NixVBClear(FxVB);
	FxCreateSpat(FxVB,p0,p1,p2,p3);
	NixDraw3D(FxVB);
}

void FxCreateCube(int buffer,const vector &a,const vector &b)
{
	FxCreateSpat(buffer,a,
						vector(b.x,a.y,a.z),
						vector(a.x,b.y,a.z),
						vector(a.x,a.y,b.z));
}

void FxDrawShining(const Skin *s,const int *tex,const matrix &m,const vector &delta)
{
#if 0
	NixSetMaterial(Black,White,Black,0,Black);
	NixVBClear(FxVB);
	for (int e=0;e<s->NumEdges;e++){
		int a=s->EdgeIndex[e*2],b=s->EdgeIndex[e*2+1];
		vector p1=s->Vertex[a],p2=s->Vertex[b];
		VecTransform(p1,m,p1);
		VecTransform(p2,m,p2);
		NixVBAddTria(FxVB,	p1      , s->Normal[a]*2, s->SkinVertex[a*2], s->SkinVertex[a*2+1],
							p2      , s->Normal[b]*2, s->SkinVertex[b*2], s->SkinVertex[b*2+1],
							p1+delta, v0            , s->SkinVertex[a*2], s->SkinVertex[a*2+1]);
		NixVBAddTria(FxVB,	p2      , s->Normal[b]*2, s->SkinVertex[b*2], s->SkinVertex[b*2+1],
							p2+delta, v0            , s->SkinVertex[b*2], s->SkinVertex[b*2+1],
							p1+delta, v0            , s->SkinVertex[a*2], s->SkinVertex[a*2+1]);
	}
	NixSetCull(CullNone);
	NixSetAlpha(AlphaSourceColor,AlphaOne);
	NixEnableFog(false);
	NixDraw3D(*tex,FxVB,m_id);
	NixEnableFog(true);
	NixSetAlpha(AlphaNone);
	NixSetCull(CullDefault);
#endif
}

void FxTailToDraw(Tail *tail)
{
	if (NumTails>=FX_MAX_TAILS)	return;
	Tails[NumTails]=tail;
	NumTails++;
}

void FxDrawTails()
{
#if 0
	NixSetMaterial(Black,White,Black,0,Black);
	NixSetCull(CullNone);
	NixSetAlpha(AlphaSourceColor,AlphaOne);
	NixEnableFog(false);
	for (int t=0;t<NumTails;t++){
		if (Tail[t]->NumSteps<1)
			continue;
		NixVBClear(FxVB);
		sSkin *skin=Tail[t]->model->Skin[Tail[t]->model->_Detail_];
		if (Tail[t]->model->_Detail_==SkinHigh)
			skin=Tail[t]->model->Skin[SkinMedium];
		vector p1,p2,p3,p4,na,nb;
		for (int s=0;s<Tail[t]->NumSteps;s++){
			for (int e=0;e<skin->NumEdges;e++){
				int a=skin->EdgeIndex[e*2],b=skin->EdgeIndex[e*2+1];
				VecTransform(p1,Tail[t]->mat[s  ],skin->Vertex[a]);
				VecTransform(p2,Tail[t]->mat[s  ],skin->Vertex[b]);
				VecTransform(p3,Tail[t]->mat[s+1],skin->Vertex[a]);
				VecTransform(p4,Tail[t]->mat[s+1],skin->Vertex[b]);
				VecNormalTransform(na,Tail[t]->mat[s],skin->Vertex[a]);
				VecNormalTransform(nb,Tail[t]->mat[s],skin->Vertex[b]);
				float o0=(1-Tail[t]->StepLifeTime[s  ]/Tail[t]->MaxLifeTime)*0.6f;
				float o1=(1-Tail[t]->StepLifeTime[s+1]/Tail[t]->MaxLifeTime)*0.6f;
				na=nb=vector(0,1,0);
				vector va,vb,vc;
				NixVBAddTria(FxVB,	p1,va=na*o0 ,skin->SkinVertex[a*2],skin->SkinVertex[a*2+1],
										p2,vb=nb*o0 ,skin->SkinVertex[b*2],skin->SkinVertex[b*2+1],
										p3,vc=na*o1 ,skin->SkinVertex[a*2],skin->SkinVertex[a*2+1]);
				NixVBAddTria(FxVB,	p2,va=nb*o0 ,skin->SkinVertex[b*2],skin->SkinVertex[b*2+1],
										p4,vb=nb*o1 ,skin->SkinVertex[b*2],skin->SkinVertex[b*2+1],
										p3,vc=na*o1 ,skin->SkinVertex[a*2],skin->SkinVertex[a*2+1]);
			}
		}
		NixDraw3D(Tail[t]->model->Material[0].Texture[0],FxVB,m_id);
	}
	//NixEnableFog(true); // ->Grafik-Fehler!!!
	NixSetAlpha(AlphaNone);
	NixSetCull(CullDefault);
	NumTails=0;
	NixEnableFog(true);
#endif
}

vector SunDir = e_y;

void FxAddShadow(Model *m, int detail)
{
	if (ShadowLevel < 1)
		return;
	if (Shadow.num >= FX_MAX_SHADOWS)
		return;

	sShadow s;

	s.mat = &m->_matrix;
//	s.NumEdges = s->NumEdges;
//	s.e = s->EdgeIndex;
	s.v = &m->skin[detail]->vertex[0];
	if (m->vertex_dyn[detail])
		s.v = m->vertex_dyn[detail];
	s.skin = m->skin[detail];
	s.num_mat = m->material.num;
	s.Length = 10000;
	s.Density = 0.5f;
	// vorerst nur die Sonne als Lichtquelle ermoeglichen
	s.LightSource = SunDir;
	s.Parallel = true;
	s.slow = false;

	Shadow.add(s);
}

#define MAX_EDGES_PER_SHADOW	1024

static int Edge[MAX_EDGES_PER_SHADOW*2];

inline void AddEdge(int *edge, int &NumEdges, int p1, int p2)
{
	if (NumEdges >= MAX_EDGES_PER_SHADOW)
		return;
	for (int i=0;i<NumEdges;i++){
		if (/*((edge[2*i+0] == p1) && (edge[2*i+1] == p2)) || */((edge[2*i+0] == p2) && (edge[2*i+1] == p1))){
			if (NumEdges > 1){
				edge[2*i+0] = edge[2*(NumEdges-1)+0];
				edge[2*i+1] = edge[2*(NumEdges-1)+1];
			}
			NumEdges --;
			return;
		}
	}
	edge[2*NumEdges+0] = p1;
	edge[2*NumEdges+1] = p2;
    NumEdges ++;
}

void FxDrawShadows()
{
	msg_db_r("FxDrawShadows",2);

	int NumEdges;
	vector LightDir;

	int nnn = 0;

	if (ShadowRecalc){
		ShadowCounter ++;
		bool update_slow = ((ShadowCounter & 8) == 0);
		NixVBClear(ShadowVB[0]);
		if (update_slow)
			NixVBClear(ShadowVB[1]);
		foreach(sShadow &s, Shadow){
			if ((s.slow) && (!update_slow))
				continue;
			matrix m = *s.mat;
			matrix inv;
			MatrixTranspose(inv, m);
			LightDir = s.LightSource;
			vector rel_light = LightDir.transform_normal(inv);

			const vector *v = s.v;
			/*int *e = s.e;
			if (!e)
				continue;*/

			NumEdges=0;
			for (int tt=0;tt<s.num_mat;tt++){
				SubSkin *sub = &s.skin->sub[tt];
				for (int t=0;t<sub->num_triangles;t++){
					if (NumEdges >= MAX_EDGES_PER_SHADOW)
						break;
					vector a = v[sub->triangle_index[t*3  ]];
					vector b = v[sub->triangle_index[t*3+1]];
					vector c = v[sub->triangle_index[t*3+2]];
					vector n = VecCrossProduct(b - a, c - a);
					if (VecDotProduct(n, rel_light) >= 0){
						AddEdge(Edge, NumEdges, sub->triangle_index[t*3  ], sub->triangle_index[t*3+1]);
						AddEdge(Edge, NumEdges, sub->triangle_index[t*3+1], sub->triangle_index[t*3+2]);
						AddEdge(Edge, NumEdges, sub->triangle_index[t*3+2], sub->triangle_index[t*3  ]);
					}
				}
			}

			int vb = s.slow ? ShadowVB[1] : ShadowVB[0];
			for (int i=0;i<NumEdges;i++){
				//msg_write(i);
				vector p1 = m * v[Edge[i*2  ]];
				vector p2 = m * v[Edge[i*2+1]];
				vector p3 = p1 - LightDir*100000;//s.Length;
				vector p4 = p2 - LightDir*100000;//s.Length;

				vector n=v_0;
				NixVBAddTria(vb,	p1,n,0,0,
									p2,n,0,0,
									p3,n,0,0);
				NixVBAddTria(vb,	p2,n,0,0,
									p4,n,0,0,
									p3,n,0,0);
				nnn++;
			}
		}
	}
	//ShadowRecalc = false;
	//printf("shad %d %d\n", NumShadows, nnn);
	if (Shadow.num > 0){
		NixSetZ(false,true);
		NixSetAlpha(AlphaZero, AlphaOne);
		NixEnableLighting(false);
		NixSetTexture(-1);
		NixSetWorldMatrix(m_id);


		NixSetStencil(StencilReset, 0);
		
		// render back side
		NixSetCull(CullCW);
		NixSetStencil(StencilIncrease, 1);
		NixDraw3D(ShadowVB[0]);
		NixDraw3D(ShadowVB[1]);

		// render front
		NixSetCull(CullCCW);
		NixSetStencil(StencilDecreaseNotNegative,1);
		NixDraw3D(ShadowVB[0]);
		NixDraw3D(ShadowVB[1]);


		// draw the shadow itself
		NixSetShading(ShadingRound);
		NixSetCull(CullCCW);
		//NixSetStencil(StencilMaskLessEqual, 1);
		NixSetStencil(StencilMaskNotEqual, 0);
		NixSetAlpha(AlphaMaterial);

		NixSetColor(ShadowColor);
		NixDraw2D(r_id, NixTargetRect, 0);

		NixSetStencil(StencilNone);
		NixSetZ(true, true);
		NixSetAlpha(AlphaNone);
	}

	// reset all shadows
	Shadow.clear();

	msg_db_l(2);
}

// pro Polygon in der Oberflaeche ein Spiegel!
void FxDrawMirrors(Skin *s,const matrix &mat)
{
#if 0
#ifdef _X_ALLOW_CAMERA_
	if (!FxRenderFunc)
		return;
	if (MirrorLevel>=MirrorLevelMax)
		return;

	//msg_write("DrawMirrors");
	//int n=(s->NumTriangles<4)?s->NumTriangles:4;
	int n=s->NumTriangles;
	for (int i=0;i<n;i++){

		if (!Mirror[NumMirrors])
			Mirror[NumMirrors]=new sMirror;
		NumMirrors++;
		sMirror *m=Mirror[NumMirrors-1];

		VecTransform(m->p[0],mat,s->Vertex[s->TriangleIndex[i*3  ]]);
		VecTransform(m->p[1],mat,s->Vertex[s->TriangleIndex[i*3+1]]);
		VecTransform(m->p[2],mat,s->Vertex[s->TriangleIndex[i*3+2]]);

		plane pl;

		// Stencil-Maske erstellen
		NixSetStencil(StencilReset,0);
		vector n=v0;
		NixVBClear(VBTemp);
		NixVBAddTria(VBTemp,m->p[0],n,0,0,m->p[1],n,0,0,m->p[2],n,0,0);
		NixSetStencil(StencilIncrease,1);
		NixSetAlpha(AlphaZero,AlphaOne);
		NixSetZ(false,true);
		NixDraw3D(-1,VBTemp,m_id);

		NixSetStencil(StencilMaskEqual,1);

		// Z-Buffer im Spiegel leeren
		NixSetZ(true,false);
		NixSetAlpha(AlphaZero,AlphaOne);
		NixSetStencil(StencilNone);
		NixDraw2D(-1,White,r_id,NixTargetRect,0.9999999f);
		NixSetStencil(StencilMaskEqual,1);
		NixSetAlpha(AlphaNone);
		NixSetZ(true,true);

		PlaneFromPoints(pl,m->p[0],m->p[1],m->p[2]);
		NixSetClipPlane(0,pl);
		NixEnableClipPlane(0,true);

		matrix rot,trans,reflect,view;
		MatrixTranslation(trans,-view_cur->Pos);
		MatrixRotationView(rot,view_cur->Ang);
		MatrixMultiply(view,trans,rot);
		MatrixReflect(reflect,pl);
		MatrixMultiply(view,reflect,view);
		NixSetView(true,view);

		//msg_write("DrawScene");

		NixCullingInverted=true;
		vector ViewPosOld=view_cur->ViewPos;
		VecTransform(view_cur->ViewPos,reflect,view_cur->ViewPos);
		MirrorLevel++;
		FxRenderFunc();
		MirrorLevel--;
		NixCullingInverted=false;
		NixEnableClipPlane(0,false);

		view_cur->ViewPos=ViewPosOld;
		NixSetView(true,view_cur->ViewPos,view_cur->Ang, vector(1,1,1));
		NixSetStencil(StencilNone);
		//msg_left();
	}
	//msg_write("//DrawMirrors");
#endif
#endif
}

void TCTransform(float &u,float &v,vector n,vector p)
{
#ifdef _X_ALLOW_CAMERA_
	p-=cur_cam->view_pos;
	p.normalize();
	//vector sd=meta->GetSunDirection();
	float f=VecDotProduct(CamDir,n)+VecDotProduct(CamDir,p);
	/*if (f<0)
		f=-f;*/
	u=f/2+0.5f;
	v=0;//u;//VecDotProduct(CamAng,n)/2+0.5f;
#endif
}

static vector p[MODEL_MAX_VERTICES];

//#########################################################################
// common stuff
//#########################################################################
void FxCalcMove()
{
	msg_db_r("FXCalcMove",2);
#ifdef _X_ALLOW_CAMERA_
	CamDir = Cam->ang.ang2dir();
#endif

// effecsts
	msg_db_m("-fx",3);
	foreach(Effect *fx, Effects){
		if ((fx->used) && (fx->enabled)){
			
			// automaticly controlled by models
			if (fx->model)
				fx->pos = fx->model->GetVertex(fx->vertex, 0);
			
			if (fx->type == FXTypeLight){
#ifdef _X_ALLOW_LIGHT_
				Light::SetColors(	*(int*)&fx->script_var[0],
									*(color*)&fx->script_var[2],
									*(color*)&fx->script_var[6],
									*(color*)&fx->script_var[10]);
				Light::SetRadial(	*(int*)&fx->script_var[0],
									fx->pos, fx->script_var[1]);
#endif
			}else if (fx->type==FXTypeSound){
#ifdef _X_USE_SOUND_
				SoundSetData(*(int*)&fx->script_var[0], fx->pos, fx->vel, fx->script_var[1], fx->script_var[1] * 0.2f, 1, fx->script_var[2]);
#endif
			}
	
			// script callback
			if (fx->func){
				fx->elapsed+=Elapsed;
				if (fx->func_delta_t <= 0){
					fx->func(fx);
					fx->elapsed=0;
				}else
					while(fx->elapsed>=fx->func_delta_t){
						fx->func(fx);
						fx->elapsed-=fx->func_delta_t;
					}
			}
	
			// general behaviour
			fx->pos += fx->vel * Elapsed;
			if (fx->suicidal){
				fx->time_to_live -= Elapsed;
				if (fx->time_to_live < 0)
					FxDelete(fx);
			}
		}
	}

// particles
	msg_db_m("--Pa",3);
	foreach(Particle *p, Particles){
		if (p->used){
			if (p->func){
				p->elapsed += Elapsed;
				if (p->elapsed >= p->func_delta_t){
					p->func(p);
					p->elapsed = 0;
				}
			}
			p->pos += p->vel * Elapsed;
			if (p->suicidal){
				p->time_to_live -= Elapsed;
				if (p->time_to_live<0)
					FxParticleDelete(p);
			}
		}
	}

// beams
	msg_db_m("--Bm",3);
	foreach(Particle *p, Beams){
		if (p->used){
			if (p->func){
				p->elapsed += Elapsed;
				if (p->elapsed >= p->func_delta_t){
					p->func(p);
					p->elapsed = 0;
				}
			}
			p->pos += p->vel * Elapsed;
			if (p->suicidal){
				p->time_to_live -= Elapsed;
				if (p->time_to_live<0)
					FxParticleDelete(p);
			}
		}
	}

#ifdef _X_ALLOW_CAMERA_
	// dynamical cube maps
	msg_db_m("--CubeMap",3);
	for (int i=0;i<CubeMap.num;i++)
		if (CubeMap[i].Dynamical)
			if (CubeMap[i].model->_detail_>=0){
				vector pos = CubeMap[i].model->pos;
				ModelToIgnore=CubeMap[i].model;
				// render each frame one cube face
				// ...but start with a complete cube rendering
				int n=0;
				vector dir=Cam->ang.ang2dir();
				if (CubeMap[i].Frame==0){
					if (VecDotProduct(vector( 1,0,0),dir)<0)	n|=1;
					if (VecDotProduct(vector(-1,0,0),dir)<0)	n|=2;
				}else if (CubeMap[i].Frame==10){
					if (VecDotProduct(vector(0, 1,0),dir)<0)	n|=4;
					if (VecDotProduct(vector(0,-1,0),dir)<0)	n|=8;
				}else if (CubeMap[i].Frame==20){
					if (VecDotProduct(vector(0,0, 1),dir)<0)	n|=16;
					if (VecDotProduct(vector(0,0,-1),dir)<0)	n|=32;
				}
				if (CubeMap[i].Frame<0)
					n=63;
				NixRenderToCubeMap(CubeMap[i].CubeMap,pos,FxRenderFunc,n);//1<<CubeMap[i].Frame);
				CubeMap[i].Frame++;
				if (CubeMap[i].Frame>30)
					CubeMap[i].Frame=0;
			}
#endif
	ModelToIgnore = NULL;
	NumMirrors = 0;
	ShadowRecalc = true;

	msg_db_l(2);
}

void FxDraw1()
{
	msg_db_r("FXDraw1",2);
	/*NixSetCull(CullNone);
	NixSetZ(false,true);
	NixSetAlpha(AlphaSourceColor,AlphaOne);


	NixSetZ(true,true);
	NixSetAlpha(AlphaNone);	
	NixSetCull(CullDefault);*/

	//msg_write("light");
	//msg_write(NumLights);
#ifdef _X_ALLOW_LIGHT_
	SunDir = Light::GetSunDir();
#endif
	
	//msg_write("ok");
	msg_db_l(2);
}

#include <GL/gl.h>

extern matrix NixViewMatrix, NixProjectionMatrix;
extern float NixView3DRatio;
extern vector NixViewScale;

void DrawParticles()
{
	NixSetWorldMatrix(m_id);
	NixEnableLighting(true);
	foreach(Particle *p, Particles){
		if ((p->used) && (p->enabled)){
			if (p->type == XContainerParticle){
				NixSetTexture(p->texture);
				NixSetColor(p->_color);
				NixDrawSprite(p->source, p->pos, p->radius);
			}else if (p->type == XContainerParticleRot){
				matrix r,t,m;
				MatrixRotation(r, p->parameter);
				MatrixTranslation(t, p->pos);
				MatrixMultiply(m, t, r);
				vector n;
				vector a = m * vector(-p->radius,-p->radius,0);
				vector b = m * vector( p->radius,-p->radius,0);
				vector c = m * vector(-p->radius, p->radius,0);
				vector d = m * vector( p->radius, p->radius,0);
				NixVBClear(VBTemp);
				NixVBAddTria(VBTemp,	a,n,p->source.x1,p->source.y2,
										c,n,p->source.x1,p->source.y1,
										d,n,p->source.x2,p->source.y1);
				NixVBAddTria(VBTemp,	a,n,p->source.x1,p->source.y2,
										d,n,p->source.x2,p->source.y1,
										b,n,p->source.x2,p->source.y2);
				NixSetMaterial(Black, color(p->_color.a, 0, 0, 0), Black, 0, p->_color);
				NixSetTexture(p->texture);
				NixDraw3D(VBTemp);
			}
		}
	}
}

void DrawParticlesNew()
{
	NixSetProjection(true, true);
	cur_cam->SetView();
	NixSetWorldMatrix(m_id);
	NixEnableLighting(false);
	_NixSetMode3d();

	matrix mi;
	MatrixInverse(mi, NixViewMatrix);
	vector ve_x = e_x.transform_normal(mi);
	vector ve_y = e_y.transform_normal(mi);

	
	foreach(Particle *p, Particles){
		if ((p->used) && (p->enabled))
			if (p->type == XContainerParticle){
				NixSetTexture(p->texture);
				glColor4fv((float*)&p->_color);
				vector a = p->pos - p->radius * ve_x - p->radius * ve_y;
				vector b = p->pos + p->radius * ve_x - p->radius * ve_y;
				vector c = p->pos - p->radius * ve_x + p->radius * ve_y;
				vector d = p->pos + p->radius * ve_x + p->radius * ve_y;
				
				// ab
				// cd
				glBegin(GL_QUADS);
					// c
					glTexCoord2f(p->source.x1, 1 - p->source.y2);
					glVertex3f(c.x, c.y, c.z);
					// a
					glTexCoord2f(p->source.x1, 1 - p->source.y1);
					glVertex3f(a.x, a.y, a.z);
					// b
					glTexCoord2f(p->source.x2, 1 - p->source.y1);
					glVertex3f(b.x, b.y, b.z);
					// d
					glTexCoord2f(p->source.x2, 1 - p->source.y2);
					glVertex3f(d.x, d.y, d.z);
				glEnd();
			}
	}
}

void DrawBeams()
{
	vector dir;
#ifdef _X_ALLOW_CAMERA_
	dir = cur_cam->ang.ang2dir();
#endif
	NixSetWorldMatrix(m_id);
	NixEnableLighting(true);
	foreach(Particle *p, Beams){
		if ((p->used) && (p->enabled)){
			vector r = VecCrossProduct(dir, p->parameter);
			r.normalize();
			vector n;
			vector a = p->pos + r * p->radius;
			vector b = p->pos - r * p->radius;
			vector c = p->pos + r * p->radius + p->parameter;
			vector d = p->pos - r * p->radius + p->parameter;
			NixVBClear(VBTemp);
			NixVBAddTria(VBTemp,	a,n,p->source.x1,p->source.y2,
									c,n,p->source.x1,p->source.y1,
									d,n,p->source.x2,p->source.y1);
			NixVBAddTria(VBTemp,	a,n,p->source.x1,p->source.y2,
									d,n,p->source.x2,p->source.y1,
									b,n,p->source.x2,p->source.y2);
			NixSetMaterial(Black, color(p->_color.a, 0, 0, 0), Black, 0, p->_color);
			NixSetTexture(p->texture);
			NixDraw3D(VBTemp);
		}
	}
}

void DrawBeamsNew()
{
	NixSetProjection(true, true);
	cur_cam->SetView();
	NixSetWorldMatrix(m_id);
	NixEnableLighting(false);
	_NixSetMode3d();
	
	vector dir;
#ifdef _X_ALLOW_CAMERA_
	dir = cur_cam->ang.ang2dir();
#endif
	foreach(Particle *p, Beams){
		if ((p->used) && (p->enabled)){
			vector r = VecCrossProduct(dir, p->parameter);
			r.normalize();
			vector a = p->pos + r * p->radius;
			vector b = p->pos - r * p->radius;
			vector c = p->pos + r * p->radius + p->parameter;
			vector d = p->pos - r * p->radius + p->parameter;
			NixSetTexture(p->texture);
			
			glColor4fv((float*)&p->_color);
			// ab
			// cd
			glBegin(GL_QUADS);
				// c
				glTexCoord2f(p->source.x1,1-p->source.y2);
				glVertex3f(a.x, a.y, a.z);
				// a
				glTexCoord2f(p->source.x1,1-p->source.y1);
				glVertex3f(c.x, c.y, c.z);
				// b
				glTexCoord2f(p->source.x2,1-p->source.y1);
				glVertex3f(d.x, d.y, d.z);
				// d
				glTexCoord2f(p->source.x2,1-p->source.y2);
				glVertex3f(b.x, b.y, b.z);
			glEnd();
		}
	}
}

void FxDraw2()
{
	msg_db_r("FXDraw2",2);

	FxDrawShadows();

	NixSetCull(CullNone);
	NixSetZ(false,true);
	NixSetAlpha(AlphaSourceAlpha,AlphaSourceInvAlpha);


	//NixSetMaterial(White,Black,Black,0,Black);
// particles
	msg_db_m("--Pa",3);
#if 1
	NixEnableFog(true);
	//DrawParticles();
	DrawParticlesNew();
	//DrawBeams();
	DrawBeamsNew();
	
#endif
	//NixSetAlpha(AlphaSourceColor,AlphaOne);
	NixEnableLighting(true);
	NixSetAlpha(AlphaSourceColor,AlphaOne);

// tails (of weapons)
	msg_db_m("--Ta",3);
	FxDrawTails();


	NixSetZ(true,true);
	NixSetAlpha(AlphaNone);	
	NixSetCull(CullDefault);
	msg_db_l(2);
}

