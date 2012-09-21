#include "x.h"

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

// make children usable
inline void octree_create_branch(sOctreeBranch *b)
{
	b->child = new sOctreeBranch[8];
	float child_size = b->Size * 0.5f;
	vector child_d = vector(child_size, child_size, child_size);
	for (int i=0;i<8;i++){
		b->child[i].Size = child_size;
		b->child[i].Min = b->Min + OctreePos[i] * child_size;
		b->child[i].Max = b->child[i].Min + child_d;
		b->child[i].child = NULL;
	}
}

COctree::COctree(const vector &pos, float radius)
{
	parent = NULL;
	Min = pos - vector(radius, radius, radius);
	Max = pos + vector(radius, radius, radius);
	Size = radius * 2;
	child = NULL;
	//octree_create_branch((sOctreeBranch*)this);
}

void COctree::Insert(const vector &pos, float radius, void *data, sOctreeLocationData *loc)
{
	vector min = pos - vector(radius, radius, radius);
	vector max = pos + vector(radius, radius, radius);
	Insert(min, max, data, loc);
}

void COctree::Insert(const vector &min, const vector &max, void *data, sOctreeLocationData *loc)
{
	//msg_write("insert");
	// which level to insert into?
	sOctreeBranch *b = (sOctreeBranch*) this;
	for (int l=0;l<TREE_MAX_DEPTH;l++){
		loc->path[l] = -1;
		if (!b->child)
			octree_create_branch(b);
		// inside a child?       (would be faster to compare to b->Pos to deside which child...)
		bool inside = false;
		for (int i=0;i<8;i++)
			if (bounding_box_inside(b->child[i].Min, b->child[i].Max, min, max)){
				inside = true;
				//msg_write(string2("%d -> %d", l, i));
				b = &b->child[i];
				loc->path[l] = i;
				break;
			}
		// not in any child -> use this level
		if (!inside)
			break;
	}

	/*msg_write("-----");
	msg_write(string2("%f  %f  %f", b->Min.x, b->Min.y, b->Min.z));
	msg_write(string2("%f  %f  %f", b->Max.x, b->Max.y, b->Max.z));*/
	// insert into b
	b->data.add(data);
	loc->branch = b;
	loc->data_id = b->data.num - 1;
}

void COctree::GetNeighbouringPairs(Array<sOctreePair> &pair)
{
}

void COctree::GetPointNeighbourhood(const vector &pos, float radius, Array<void*> &data)
{
	//msg_write("point");
	data.clear();
	
	sOctreeBranch *b = (sOctreeBranch*) this;
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
	}
}
