/*----------------------------------------------------------------------------*\
| Fx                                                                           |
| -> manages particle effects                                                  |
| -> auxiliary functions for drawing shapes and effects (mirrors, shadows,...) |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.06.19 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(FX_H__INCLUDED_)
#define FX_H__INCLUDED_


extern string FxVersion;

typedef void particle_callback(void *);
typedef void effect_enable_func(void *, bool);


#define FX_MAX_TAIL_STEPS		8

struct Particle : XContainer
{
	bool suicidal;
	vector pos, vel, parameter;
	float time_to_live, radius;
	color _color;
	int texture;
	rect source;
	particle_callback *func;
	float func_delta_t, elapsed;
};

struct Effect : XContainer
{
	bool suicidal;
	vector pos, vel;
	float time_to_live;
	particle_callback *func, *func_del;
	effect_enable_func *func_enable;
	float func_delta_t, elapsed;
	Array<float> script_var;
	Model *model;
	int vertex, type;
};

enum{
	FXTypeScript,
	FXTypeSound,
	FXTypeLight,
	FXTypeForceField,
};

/*struct ForceField{
	bool Used,Enabled;
	vector Pos;
	float Strength,RadiusMin,RadiusMax,TimeToLife;
	bool InvQuadratic,Dieable;
};*/

struct Tail{
	int NumSteps;
	float MaxLifeTime;
	Model *model;
	matrix mat[FX_MAX_TAIL_STEPS+1];
	float StepLifeTime[FX_MAX_TAIL_STEPS+1];
};

typedef void render_func();
extern render_func *FxRenderFunc;

extern int MirrorLevelMax;
extern bool FxLightFieldsEnabled;

void FxInit();
void FxReset();

// particles
//sParticle *FxParticleCreate(int type,const vector &pos,const vector &param,int texture,particle_callback *func,float time_to_live,float radius);
Particle *_cdecl FxParticleCreateDef(const vector &pos,int texture,particle_callback *func,float time_to_live,float radius);
Particle *_cdecl FxParticleCreateRot(const vector &pos,const vector &ang,int texture,particle_callback *func,float time_to_live,float radius);
Particle *_cdecl FxParticleCreateBeam(const vector &pos,const vector &length,int texture,particle_callback *func,float time_to_live,float radius);
void _cdecl FxParticleDelete(Particle *particle);

// effects
Effect *_cdecl FxCreate(const vector &pos,particle_callback *func,particle_callback *del_func,float time_to_live);
Effect *_cdecl FxCreateScript(Model *m, int vertex, const string &filename);
Effect *_cdecl FxCreateLight(Model *m, int vertex, float radius, const color &am, const color &di, const color &sp);
Effect *_cdecl FxCreateSound(Model *m, int vertex, const string &filename, float radius, float speed);
void _cdecl FxDelete(Effect *effect);
void _cdecl FxEnable(Effect *effect, bool enabled);
//sEffect *FxCreateByModel(Model *m,sModelEffectData *data);
//void FxUpdateByModel(Effect *&effect,const vector &pos,const vector &last_pos);
//void FxResetByModel();

// force fields
/*int FxForceFieldNew();
void FxForceFieldCreate(int index,vector pos,float radius,float strength,bool inv_quad);
void FxForceFieldSetPos(int index,vector pos);
void FxForceFieldDelete(int index);
void FxForceFieldEnable(int index,bool enabled);
void FxForceFieldCalcMove(int index);*/

// cube maps
int FxCubeMapNew(int size);
void FxCubeMapCreate(int cube_map,int tex0,int tex1,int tex2,int tex3,int tex4,int tex5);
void FxCubeMapCreate(int cube_map,Model *m);
void FxCubeMapDraw(int cube_map,int buffer,float density);

// stuff
void FxCreatePolygon(int buffer,int num_points,const vector *p);
void FxDrawPolygon(int num_points,const vector *p);
void FxCreateBall(int buffer,const vector &pos,float radius,int nx,int ny);
void FxDrawBall(const vector &pos,float radius,int nx,int ny);
void FxCreateSpat(int buffer,const vector &p0,const vector &p1,const vector &p2,const vector &p3);
void FxDrawSpat(const vector &p0,const vector &p1,const vector &p2,const vector &p3);
void FxCreateCube(int buffer,const vector &a,const vector &b);
void FxDrawShining(const Skin *s,const int *tex,const matrix *m,const vector &delta); // parallel
void FxTailToDraw(Tail *tail); // rotatable
void FxDrawTails();
void FxAddShadow(Model *m, int detail);
void FxDrawShadows();
void FxDrawMirrors(Skin *s, const matrix &m);

void FxCalcMove();
void FxDraw1();
void FxDraw2();

#endif

