/*----------------------------------------------------------------------------*\
| Terrain                                                                      |
| -> terrain of a world                                                        |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last updated: 2008.06.23 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(TERRAIN_H__INCLUDED_)
#define TERRAIN_H__INCLUDED_


#define TERRAIN_MAX_TEXTURES		8

enum{
	TerrainTypeContingous,
	TerrainTypePattern
};

#define TerrainUpdateNormals	1
#define TerrainUpdateVertices	2
#define TerrainUpdatePlanes		4
#define TerrainUpdateAll		7

class Terrain
{
public:
	Terrain();
	Terrain(const string &filename, const vector &pos);
	bool Load(const string &filename, const vector &pos, bool deep = true);
	~Terrain();
	void reset();
	void Update(int x1,int x2,int z1,int z2,int mode);
	float gimme_height(const vector &p);
	float gimme_height_n(const vector &p, vector &n);

	void GetTriangleHull(void *hull,vector &pos,float radius);

	bool Trace(vector &p1, vector &p2, vector &dir, float range, vector &tp, bool simple_test);

	void CalcDetail();
	void Draw();

	string filename;
	int type;
	bool error;

	vector pos;
	int num_x, num_z;
	Array<float> height;
	Array<vector> vertex, normal;
	Array<plane> pl; // for collision detection
	int vertex_buffer;
	int partition[128][128], partition_old[128][128];
	vector pattern, min, max;
	Material *material;
	string material_file;

	int num_textures;
	int texture[TERRAIN_MAX_TEXTURES];
	string texture_file[TERRAIN_MAX_TEXTURES];
	vector texture_scale[TERRAIN_MAX_TEXTURES];

	float dhx, dhz;


	bool changed;
	bool redraw, force_redraw;
	vector pos_old;
};


#endif

