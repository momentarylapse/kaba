/*----------------------------------------------------------------------------*\
| CModel                                                                       |
| -> can be a skeleton                                                        |
|    -> sub-models                                                             |
|    -> animation data                                                         |
| -> model                                                                     |
|    -> vertex and triangle data for rendering                                 |
|    -> consists of 4 skins                                                    |
|       -> 0-2 = visible detail levels (LOD) 0=high detail                     |
|       -> 3   = dynamical (for animation)                                     |
|    -> seperate physical skin (vertices, balls and convex polyeders)          |
|       -> absolute vertex positions in a seperate structure                   |
| -> strict seperation:                                                        |
|    -> dynamical data (changed during use)                                    |
|    -> unique data (only one instance for several copied models)              |
| -> can contain effects (fire, light, water,...)                              |
| MOSTLY WRONG!!!!                                                             |
|                                                                              |
| vital properties:                                                            |
|  - vertex buffers get filled temporaryly per frame                           |
|                                                                              |
| last update: 2008.01.22 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#if !defined(MODEL_H__INCLUDED_)
#define MODEL_H__INCLUDED_


#ifdef _X_ALLOW_ODE_
#include <ode/ode.h>
#endif


#define MODEL_MAX_TEXTURES				8
#define MODEL_MAX_POLY_FACES			32
#define MODEL_MAX_POLY_EDGES			(MODEL_MAX_POLY_FACES*4)
#define MODEL_MAX_POLY_VERTICES_PER_FACE	16
#define MODEL_MAX_MOVE_OPS				8



template <class T>
class CopyAsRefArray : public Array<T>
{
	public:
		void operator = (const CopyAsRefArray<T> &a)
		{
			set_ref(a);
		}
		void forget()
		{
			Array<T>::data = NULL;
			Array<T>::allocated = 0;
			Array<T>::num = 0;
		}
		void make_own()
		{
			T *dd = (T*)Array<T>::data;
			int n = Array<T>::num;
			forget();
			Array<T>::resize(n);
			for (int i=0;i<Array<T>::num;i++)
				(*this)[i] = dd[i];
		}
};


struct SubSkin
{
	int num_triangles;
	
	// vertices
	CopyAsRefArray<int> triangle_index;
	
	// texture mapping
	CopyAsRefArray<float> skin_vertex;

	// normals
	CopyAsRefArray<vector> normal;

	int vertex_buffer;

	// refill the vertex buffer etc...
	bool force_update;
};

// visual skin
struct Skin
{
	CopyAsRefArray<int> bone_index; // skeletal reference
	CopyAsRefArray<vector> vertex;

	CopyAsRefArray<SubSkin> sub;
	
	// bounding box
	vector min, max;

	bool copy_as_ref;
};

// the face of a polyhedron (=> a polygon)
struct ConvexPolyhedronFace
{
	int num_vertices;
	int index[MODEL_MAX_POLY_VERTICES_PER_FACE];
	plane pl; // in model space
};

// a convex polyhedron (for the physical skin)
struct ConvexPolyhedron
{
	int num_faces;
	ConvexPolyhedronFace face[MODEL_MAX_POLY_FACES];

	// non redundant vertex list!
	int num_vertices;
	int *vertex;

	// non redundant edge list!
	int num_edges;
	int *edge_index;

	// "topology"
	bool *edge_on_face; // [edge * num_faces + face]
	int *faces_joining_edge; // [face1 * num_faces + face2]
};

// a ball (for the physical skin)
struct Ball
{
	int index;
	float radius;
};

// data for collision detection
struct PhysicalSkin
{
	int num_vertices;
	int *bone_nr;
	vector *vertex; // original vertices
	vector *vertex_dyn; // here the animated vertices are stored before collision detection

	/*int num_triangles;
	unsigned short *triangle_index;*/

	/*int NumEdges;
	unsigned short *EdgeIndex;*/

	int num_balls;
	Ball *ball;

	int num_polys;
	ConvexPolyhedron *poly;
};

// physical skin, but in world coordinates
struct PhysicalSkinAbsolute
{
	bool is_ok;
	vector *p;
	plane *pl;
};

// visual and physical properties
struct Material
{
	// name of the material
	string name;

	// textures
	int num_textures;
	int texture[MODEL_MAX_TEXTURES];

	// light
	color ambient, diffuse, specular, emission;
	float shininess;

	// transparency
	int transparency_mode;
	int alpha_source, alpha_destination;
	float alpha_factor;
	bool alpha_z_buffer;

	// reflection
	int reflection_mode;
	float reflection_density;
	int cube_map, cube_map_size;

	// shader
	int shader;

	// friction
	float rc_jump, rc_static, rc_sliding, rc_rolling;
};

// single animation
struct Move
{
	int type; // skeletal/vertex
	int num_frames;
	int frame0;

	// properties
	float frames_per_sec_const, frames_per_sec_factor;
	bool inter_quad, inter_loop;
};

// a list of animations
struct MetaMove
{
	// universal animation data
	int num_moves;
	Move *move;

	int num_frames_skeleton, num_frames_vertex;

	// skeletal animation data
	vector *skel_dpos; //   [frame * num_bones + bone]
	quaternion *skel_ang; //   [frame * num_bones + bone]

	// vertex animation data
	struct{
		vector *dpos; // vertex animation data   [frame * num_vertices + vertex]
	}skin[4];
	void reset()
	{	memset(this, 0, sizeof(MetaMove));	}
};

// types of animation
enum
{
	MoveTypeNone,
	MoveTypeVertex,
	MoveTypeSkeletal
};

// commands for animation (move operations)
struct MoveOperation
{
	int move, operation;
	float time, param1, param2;
};

// a list of triangles for collision detection
struct TriangleHull
{
	// large list of vertices
	vector *p;

	int num_vertices;
	int *index;

	int num_triangles;
	int *triangle_index;
	plane *pl;

	int num_edges;
	int *edge_index;
};

// to store data to create effects (when copying models)
struct ModelEffectData
{
	int vertex;
	int type;
	string filename;
	float radius, speed;
	color am, di, sp;
};

struct Bone
{
	int parent;
	vector pos;
	CModel *model;
	// current skeletal data
	matrix dmatrix;
	quaternion cur_ang;
	vector cur_pos;
};

enum{
	SkinHigh,
	SkinMedium,
	SkinLow,
	MODEL_NUM_SKINS
};
#define SkinDynamic					8
#define SkinDynamicViewHigh			(SkinDynamic | SkinHigh)
#define SkinDynamicViewMedium		(SkinDynamic | SkinMedium)
#define SkinDynamicViewLow			(SkinDynamic | SkinLow)
#define SkinPhysical				42
#define SkinDynamicPhysical			43

struct ModelTemplate
{
	string filename, script_filename;
	Array<float> script_var;
	Array<string> item;
	Array<ModelEffectData> fx;
	
	// TODO... script has only single instance... global var "this"... changed by x
	void *script_on_init;
	void *script_on_iterate;
	void *script_on_kill;
	void *script_on_collide_object;
	void *script_on_collide_terrain;
	void *script;

	void reset()
	{
		filename = "";
		script_filename = "";
		script_var.clear();
		item.clear();
		fx.clear();
		script_on_init = NULL;
		script_on_iterate = NULL;
		script_on_kill = NULL;
		script_on_collide_object = NULL;
		script_on_collide_terrain = NULL;
		script = NULL;
	}
};

typedef CModel *pModel;
typedef void *_fx_pointer_;

enum{
	ModelCopyChangeable = 1,
	ModelCopyNoSubModels = 2,
	ModelCopyRecursive = 4,
	ModelCopyKeepSubModels = 8,
	ModelCopyInverse = 16
};

class CModel
{
public:
	// creation
	CModel(const string &filename);
	CModel(); // only used by GetCopy()
	CModel *GetCopy(int mode);
	void ResetData();
	void MakeEditable();
	void SetMaterial(Material *material, int mode);
	//void Update();
	void reset();
	~CModel();

	// animate me
	void _cdecl CalcMove();

	// skeleton
	vector _GetBonePos(int index);
	void SetBoneModel(int index, CModel *sub);

	// animation
	vector _cdecl GetVertex(int index,int skin);

	// helper functions for collision detection
	void _UpdatePhysAbsolute_();
	void _ResetPhysAbsolute_();

	bool Trace(vector &p1, vector &p2, vector &dir, float range, vector &tp, bool simple_test);

	// animation
	void ResetAnimation();
	bool IsAnimationDone(int operation_no);
	bool Animate(int mode, float param1, float param2, int move_no, float &time, float elapsed, float vel_param, bool loop);
	int GetFrames(int move_no);
	void BeginEditAnimation();
	void BeginEdit(int detail);
	void EndEdit(int detail);

	// drawing
	void _cdecl Draw(int detail, bool set_fx, bool allow_shadow);
	void JustDraw(int material, int detail);

	// visible skins (shared)
	Skin *skin[MODEL_NUM_SKINS];
	bool skin_is_reference[MODEL_NUM_SKINS];
	// material (shared)
	CopyAsRefArray<Material> material;
	bool material_is_reference;
	// dynamical data (own)
	vector *vertex_dyn[MODEL_NUM_SKINS]; // here the animated vertices are stored before rendering
	vector *normal_dyn[MODEL_NUM_SKINS];
	
	// physical skin (shared)
	PhysicalSkin *phys;
	bool phys_is_reference;
	PhysicalSkinAbsolute phys_absolute;

	// properties
	float detail_dist[MODEL_NUM_SKINS];
	float radius;
	vector min, max; // "bounding box"
	matrix3 theta_0, theta, theta_inv;
	bool test_collisions;
	bool allow_shadow;
	bool flexible;
	bool is_copy, error;

	// physics
	float mass, mass_inv, g_factor;
	bool active_physics, passive_physics;

	// script data (own)
	string name, description;
	Array<CModel*> inventary;
	Array<float> script_var;
	void *script_data;

	int object_id;
	bool on_ground, visible, rotating, moved, frozen;
	float time_till_freeze;
	int ground_id;
	vector ground_normal;


	vector pos, vel, vel_surf, /*pos_old,*/ acc;
	vector ang, /*ang_old,*/ rot;
	matrix _matrix, matrix_old;

	vector force_int, torque_int;
	vector force_ext, torque_ext;

	float rc_jump, rc_static, rc_sliding, rc_rolling;

	// template (shared)
	ModelTemplate *_template;
	string GetFilename();

	// engine data
	bool registered;
	bool _detail_needed_[MODEL_NUM_SKINS]; // per frame
	int _detail_; // per view (more than once a frame...)

	// effects (own)
	Array<sEffect*> fx;

	// skeleton (own)
	Array<Bone> bone;
	vector *bone_pos_0;

	// move operations
	int num_move_operations;
	MoveOperation move_operation[MODEL_MAX_MOVE_OPS];
	MetaMove *meta_move;

#ifdef _X_ALLOW_ODE_
	dBodyID body_id;
	dGeomID geom_id;
#endif
};


// types of shading/normal vectors
enum
{
	NormalModeSmooth,
	NormalModeHard,
	NormalModeSmoothEdges,
	NormalModeAngular,
	NormalModePerVertex,
	NormalModePre=16,
};


// types of transparency
#define TransparencyModeDefault			-1
#define TransparencyModeNone			0
#define TransparencyModeFunctions		1
#define TransparencyModeColorKeyHard	2
#define TransparencyModeColorKeySmooth	3
#define TransparencyModeFactor			4


// types of reflection
enum
{
	ReflectionNone,
	ReflectionMetal,
	ReflectionMirror,
	ReflectionCubeMapStatic,
	ReflectionCubeMapDynamical
};


// data from a Trace() call
extern int TraceHitType,TraceHitIndex,TraceHitSubModel,TraceHitSubModelTemp;

// what is hit (TraceHitType)
#define TraceHitNone				-1
#define TraceHitTerrain				0
#define TraceHitObject				1
// ...
#define TraceMax					100000000000000000000.0f


// move operations
enum
{
	MoveOpSet,			// overwrite
	MoveOpSetNewKeyed,	// overwrite, if current doesn't equal 0
	MoveOpSetOldKeyed,	// overwrite, if last equals 0
	MoveOpAdd1Factor,	// w = w_old         + w_new * f
	MoveOpMix1Factor,	// w = w_old * (1-f) + w_new * f
	MoveOpMix2Factor	// w = w_old * a     + w_new * b
};

// observers for collision detection
void DoCollisionObservers();
extern int NumObservers;

#define SetMaterialAll				0xffff
#define SetMaterialFriction			1
#define SetMaterialColors			2
#define SetMaterialTransparency		4
#define SetMaterialAppearance		6


#endif

