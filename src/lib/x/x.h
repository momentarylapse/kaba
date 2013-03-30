#ifndef _GAME_EXISTS_
#define _GAME_EXISTS_

#include "../config.h"
#include "../nix/nix.h"
#include "../file/file.h"

class Model;
class Object;
class Terrain;
struct Effect;
struct Material;
struct Skin;
struct CollisionData;
struct sView;

struct XContainer
{
	int type;
	bool used, enabled;
};

enum
{
	XContainerEffect,
	XContainerParticle,
	XContainerParticleRot,
	XContainerParticleBeam,
	XContainerPicture,
	XContainerText,
	XContainerPicture3d,
	XContainerGrouping,
	XContainerView,
	XContainerModel
};

#define xcont_find_new(xtype, var_type, var, array)	\
	var_type *var = NULL;\
	for (int i=0;i<array.num;i++)\
		if (!array[i]->used){\
			var = array[i];\
			break;\
		}\
	if (!var){\
		var = new var_type;\
		array.add(var);\
	}\
	var->type = xtype;\
	var->used = true;

#ifdef _X_ALLOW_MODEL_
	#include "model.h"
#endif
#ifdef _X_ALLOW_OBJECT_
	#include "object.h"
#endif
#ifdef _X_ALLOW_CAMERA_
	#include "camera.h"
#endif
#ifdef _X_ALLOW_GOD_
	#include "god.h"
#endif
#ifdef _X_ALLOW_TERRAIN_
	#include "terrain.h"
#endif
#ifdef _X_ALLOW_FX_
	#include "fx.h"
#endif
#ifdef _X_ALLOW_META_
	#include "meta.h"
#endif
#ifdef _X_ALLOW_COLLISION_
	#include "collision.h"
#endif
#ifdef _X_ALLOW_PHYSICS_
	#include "physics.h"
#endif
#ifdef _X_ALLOW_MATRIXN_
	#include "matrixn.h"
#endif
#ifdef _X_ALLOW_LINKS_
	#include "links.h"
#endif
#ifdef _X_ALLOW_TREE_
	#include "tree.h"
#endif
#ifdef _X_ALLOW_GUI_
	#include "gui.h"
#endif
#ifdef _X_ALLOW_LIGHT_
	#include "light.h"
#endif

#endif

