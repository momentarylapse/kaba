#include "x.h"

#define TREE_MAX_DEPTH			12


struct OctreeBranch
{
	OctreeBranch(const vector &center, float radius, OctreeBranch *parent);
	~OctreeBranch();
	vector center;
	float radius;
	OctreeBranch *child[8]; // links to all 8 children
	OctreeBranch *parent; // link to upper level
	Array<void*> data;
};

// store this with the data...
/*struct OctreeLocationData
{
	OctreeBranch *branch;
	int path[TREE_MAX_DEPTH];
	int data_id;
};*/

inline bool bounding_box_inside(const vector &a1, const vector &a2, const vector &b1, const vector &b2)
{
	if (b1.x < a1.x)	return false;
	if (b1.y < a1.y)	return false;
	if (b1.z < a1.z)	return false;
	if (b2.x > a2.x)	return false;
	if (b2.y > a2.y)	return false;
	if (b2.z > a2.z)	return false;
	return true;
}

inline bool point_in_bounding_box(const vector &a1, const vector &a2, const vector &p)
{
	if (p.x < a1.x)	return false;
	if (p.y < a1.y)	return false;
	if (p.z < a1.z)	return false;
	if (p.x > a2.x)	return false;
	if (p.y > a2.y)	return false;
	if (p.z > a2.z)	return false;
	return true;
}

vector OctreePos[8]={
	vector(0,0,0),	vector(0,0,1),	vector(0,1,0),	vector(0,1,1),
	vector(1,0,0),	vector(1,0,1),	vector(1,1,0),	vector(1,1,1)
};

OctreeBranch::OctreeBranch(const vector &_center, float _radius, OctreeBranch *_parent)
{
	center = _center;
	radius = _radius;
	parent = _parent;
	for (int i=0;i<8;i++)
		child[i] = NULL;
}

OctreeBranch::~OctreeBranch()
{
	for (int i=0;i<8;i++)
		if (child[i])
			delete(child[i]);
}

Octree::Octree(const vector &_pos, float _radius)
{
	root = new OctreeBranch(_pos, _radius, NULL);
}

Octree::~Octree()
{
	delete(root);
}

void Octree::Insert(const vector &pos, float radius, void *data)
{
	OctreeBranch *b = root;
	for (int l=0;l<TREE_MAX_DEPTH;l++){
		vector dpos = pos - b->center;
		int sub = 7;
		if (dpos.x < 0){
			dpos.x = - dpos.x;
			sub -= 4;
		}
		if (dpos.y < 0){
			dpos.y = - dpos.y;
			sub -= 2;
		}
		if (dpos.z < 0){
			dpos.z = - dpos.z;
			sub -= 1;
		}
		if ((dpos.x < radius) || (dpos.y < radius) || (dpos.z < radius))
			break;
		if (!b->child[sub]){
			float r = b->radius * 0.5f;
			b->child[sub] = new OctreeBranch(b->center + OctreePos[sub] * r, r, b);
		}
		b = b->child[sub];
	}
	b->data.add(data);
}

void Octree::GetNeighbouringPairs(Array<TreePair> &pair)
{
}

void Octree::GetPointNeighbourhood(const vector &pos, float radius, Array<void*> &data)
{
	//msg_write("point");
	
	/*OctreeBranch *b = (OctreeBranch*) this;
	for (int l=0;l<TREE_MAX_DEPTH;l++){
		// items from this level
		for (int i=0;i<b->data.num;i++){
			//if (
			data.add(b->data[i]);
		}
		
		// inside a child?
		if (!b->child)
			return;
		bool inside = false;
		for (int i=0;i<8;i++){
			if (point_in_bounding_box(b->child[i].Min, b->child[i].Max, pos)){
				inside = true;
				//msg_write(string2("%d -> %d", l, i));
				b = &b->child[i];
				break;
			}
		}
		// not in any child -> done
		if (!inside)
			return;
	}*/
}

void Octree::Get(const vector &pos, Array<void*> &data)
{
	OctreeBranch *b = root;
	while (b){
		data.append(b->data);
		vector dpos = pos - b->center;
		int sub = 0;
		if (dpos.x > 0)
			sub += 4;
		if (dpos.y > 0)
			sub += 2;
		if (dpos.z > 0)
			sub += 1;
		b = b->child[sub];
	}
}




struct QuadtreeBranch
{
	QuadtreeBranch(const vector &center, float radius, QuadtreeBranch *parent);
	~QuadtreeBranch();
	vector center;
	float radius;
	QuadtreeBranch *child[4]; // links to all 4 children
	QuadtreeBranch *parent; // link to upper level
	Array<void*> data;
};

vector QuadtreePos[4]={
	vector(0,0,0),	vector(0,0,1),	vector(1,0,0),	vector(1,0,1)
};

QuadtreeBranch::QuadtreeBranch(const vector &_center, float _radius, QuadtreeBranch *_parent)
{
	center = _center;
	radius = _radius;
	parent = _parent;
	for (int i=0;i<4;i++)
		child[i] = NULL;
}

QuadtreeBranch::~QuadtreeBranch()
{
	for (int i=0;i<4;i++)
		if (child[i])
			delete(child[i]);
}

Quadtree::Quadtree(const vector &_pos, float _radius)
{
	root = new QuadtreeBranch(_pos, _radius, NULL);
}

Quadtree::~Quadtree()
{
	delete(root);
}

void Quadtree::Insert(const vector &pos, float radius, void *data)
{
	QuadtreeBranch *b = root;
	for (int l=0;l<TREE_MAX_DEPTH;l++){
		vector dpos = pos - b->center;
		int sub = 3;
		if (dpos.x < 0){
			dpos.x = - dpos.x;
			sub -= 2;
		}
		if (dpos.z < 0){
			dpos.z = - dpos.z;
			sub -= 1;
		}
		if ((dpos.x < radius) || (dpos.z < radius))
			break;
		if (!b->child[sub]){
			float r = b->radius * 0.5f;
			b->child[sub] = new QuadtreeBranch(b->center + QuadtreePos[sub] * r, r, b);
		}
		b = b->child[sub];
	}
	b->data.add(data);
}

void Quadtree::Get(const vector &pos, Array<void*> &data)
{
	QuadtreeBranch *b = root;
	while (b){
		data.append(b->data);
		vector dpos = pos - b->center;
		int sub = 0;
		if (dpos.x > 0)
			sub += 2;
		if (dpos.z > 0)
			sub += 1;
		b = b->child[sub];
	}
}
