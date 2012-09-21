/*----------------------------------------------------------------------------*\
| God                                                                          |
| -> manages objetcs and interactions                                          |
| -> loads and stores the world data (level)                                   |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.12.06 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(GOD_H__INCLUDED_)
#define GOD_H__INCLUDED_

#define GOD_MAX_FORCEFIELDS		64
#define GOD_MAX_MUSIC_FILES		64
#define GOD_MAX_NET_MSGS		64
#define GOD_MAX_MUSICFIELDS		64


struct sGodForceField
{
	vector Pos,Dir;
	int Shape,Kind;
	float Radius,Vel,Acc,TimeToLife;
	bool Visible;
};

struct Fog
{
	bool enabled;
	int mode;
	float start, end, density;
	color _color;
};

struct LevelDataTerrain
{
	string filename;
	vector pos;
};

struct LevelDataObject
{
	string filename, name;
	vector pos, ang, vel, rot;
};

struct LevelDataScriptRule
{
	string function;
	int location;
};

struct sLevelData
{
	string world_filename;
	Array<string> skybox_filename;
	Array<vector> skybox_ang;
	color background_color;
	Array<LevelDataObject> object;
	Array<LevelDataTerrain> terrain;
	int ego_index;
	Array<string> script_filename;
	Array<float> script_var;
	
	bool sun_enabled;
	color sun_color[3];
	vector sun_ang;
	color ambient;

	vector gravity;
	Fog fog;
};

struct sMusicField
{
	vector PosMin,PosMax;
	int NumMusicFiles;
	string MusicFile[16];
};

struct PartialModel{
	CModel *model;
	Material *material;
	int mat_index;
	float d;
	bool shadow, transparent;
};



// game data
extern string InitialWorldFile, SecondWorldFile, CurrentWorldFile;
extern color BackGroundColor;
extern Array<CModel*> SkyBox;
extern color GlobalAmbient;
extern Fog GlobalFog;
extern int SunLight;
extern float SpeedOfSound;
extern bool PhysicsEnabled, CollisionsEnabled;
extern int PhysicsNumSteps, PhysicsNumLinkSteps;

#ifdef _X_ALLOW_ODE_
	#include <ode/ode.h>
	extern dWorldID world_id;
	extern dSpaceID space_id;
	extern dJointGroupID contactgroup;
#endif

#ifdef _X_ALLOW_PHYSICS_DEBUG_
	extern int PhysicsTimer;
	extern float PhysicsTimeCol, PhysicsTimePhysics, PhysicsTimeLinks;
	extern sCollisionData PhysicsDebugColData;
	extern bool PhysicsStopOnCollision;
#endif


void GodInit();
void GodReset();
void GodResetLevelData();
bool GodLoadWorldFromLevelData();
bool GodLoadWorld(const string &filename);

extern bool GodNetMsgEnabled;
CObject* _cdecl GodCreateObject(const string &filename, const string &name, const vector &pos, const vector &ang, int w_index=-1);
void GodDeleteObject(int index);
void GodRegisterModel(CModel *m);
void GodUnregisterModel(CModel *m);
void AddNewForceField(vector pos,vector dir,int kind,int shape,float r,float v,float a,bool visible,float t);
//void DoSounds();
void SetSoundState(bool paused,float scale,bool kill,bool restart);
vector _cdecl GetG(vector &pos);
void GodCalcMove();
void GodCalcMove2(); // debug
void GodDoCollisionDetection();
void GodRegisterModel(CModel *m);
void GodDraw();
CObject *_cdecl GetObjectByName(const string &name);
bool _cdecl NextObject(CObject **o);
void _cdecl GodObjectEnsureExistence(int id);
int _cdecl GodFindObjects(vector &pos, float radius, int mode, Array<CObject*> &a);

void Test4Ground(CObject *o);
void Test4Object(CObject *o1,CObject *o2);
bool _cdecl GodTrace(vector &p1,vector &p2,vector &tp,bool simple_test,int o_ignore=-1);

	// content of the world
extern Array<CObject*> Object;
extern CObject *Ego;
extern CObject *terrain_object;

// esotherical (not in the world)
extern bool AddAllObjectsToLists;

// music fields
extern int NumMusicFields;
extern sMusicField MusicFieldGlobal,MusicField[GOD_MAX_MUSICFIELDS];
extern int MusicCurrent;

// force fields
extern int NumForceFields;
extern sGodForceField *ForceField[GOD_MAX_FORCEFIELDS];
extern sMusicField *MusicFieldCurrent;

extern vector GlobalG;
extern sLevelData LevelData;
extern Array<float> ScriptVar;


extern Array<CTerrain*> Terrain;

// network messages
struct s_net_message_list
{
	int num_msgs,msg[GOD_MAX_NET_MSGS],arg_i[GOD_MAX_NET_MSGS][4];
	string arg_s[GOD_MAX_NET_MSGS];
};
extern s_net_message_list NetMsg;

CObject *_cdecl _CreateObject(const string &filename, const vector &pos);


#define FFKindRadialConst		0
#define FFKindRadialLinear		1
#define FFKindRadialQuad		2
#define FFKindDirectionalConst	10
#define FFKindDirectionalLinear	11
#define FFKindDirectionalQuad	12

#define NetMsgCreateObject		1000
#define NetMsgDeleteObject		1002
#define NetMsgSCText			2000

#define FieldKindLight		0
#define FieldKindMusic		1
#define FieldKindWeather	2
#define FieldKindURW		3

#endif
