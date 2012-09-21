#if !defined(COLLISION_H)
#define COLLISION_H



#define MAX_COLLISIONS					256

// set of collision points, created by Collide...()
struct CollisionData
{
	int num;
	vector pos[MAX_COLLISIONS];
	vector normal[MAX_COLLISIONS]; // directing from o1 to o2
	float depth[MAX_COLLISIONS];

	CObject *o1, *o2;
};


extern CollisionData ColData, *pColData;

//	int GetCollision(CModel *partner, matrix *mat,matrix *mat_old,CollisionData &col,bool set_crash);


bool CollideObjects(CObject *o1, CObject *o2);
#ifdef _X_ALLOW_TERRAIN_
bool CollideObjectTerrain(CObject *o, CTerrain *t);//terrain);
#endif




bool CollideModels(CModel *m1, CModel *m2); // model - model
bool CollideModelTerrain(CModel *m, TriangleHull *hull); // model - triangles

#endif
