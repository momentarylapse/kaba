#if !defined(TREE_H__INCLUDED_)
#define TREE_H__INCLUDED_

#define TREE_MAX_DEPTH			12

struct sOctreeBranch
{
	vector Min, Max; // bounding box
	vector Pos;
	float Size;
	sOctreeBranch *child; // links to all 8 children
	sOctreeBranch *parent; // link to upper level
	Array<void*> data;
};

// store this with the data...
struct sOctreeLocationData
{
	sOctreeBranch *branch;
	int path[TREE_MAX_DEPTH];
	int data_id;
};

struct sOctreePair
{
	void *a, *b;
};

class COctree : sOctreeBranch
{
	public:
	COctree(const vector &pos, float radius);

	// data management
	void Insert(const vector &pos, float radius, void *data, sOctreeLocationData *loc);
	void Insert(const vector &min, const vector &max, void *data, sOctreeLocationData *loc);

	// tests
	void GetNeighbouringPairs(Array<sOctreePair> &pair);
	void GetPointNeighbourhood(const vector &pos, float radius, Array<void*> &data);
};

#endif
