#if !defined(TREE_H__INCLUDED_)
#define TREE_H__INCLUDED_


struct OctreeBranch;
struct QuadtreeBranch;

struct TreePair
{
	void *a, *b;
};

class Octree
{
public:
	Octree(const vector &pos, float radius);
	~Octree();

	// data management
	void Insert(const vector &pos, float radius, void *data);
	void Remove(void *data);

	// tests
	void GetNeighbouringPairs(Array<TreePair> &pair);
	void GetPointNeighbourhood(const vector &pos, float radius, Array<void*> &data);
	void Get(const vector &pos, Array<void*> &data);
	
private:
	OctreeBranch *root;
};

class Quadtree
{
public:
	Quadtree(const vector &pos, float radius);
	~Quadtree();

	// data management
	void Insert(const vector &pos, float radius, void *data);
	void Remove(void *data);

	// tests
	void GetNeighbouringPairs(Array<TreePair> &pair);
	void GetPointNeighbourhood(const vector &pos, float radius, Array<void*> &data);
	void Get(const vector &pos, Array<void*> &data);
	
private:
	QuadtreeBranch *root;
};

#endif
