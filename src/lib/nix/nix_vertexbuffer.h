/*----------------------------------------------------------------------------*\
| Nix vertex buffer                                                            |
| -> handling vertex buffers                                                   |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#if HAS_LIB_GL

#ifndef _NIX_VERTEXBUFFER_EXISTS_
#define _NIX_VERTEXBUFFER_EXISTS_

#define MAX_VB_ATTRIBUTES 8
#define MAX_VB_BUFFERS 8


namespace nix
{

class OldVertexBuffer {
public:
	int num_textures;
	int num_triangles;
	bool indexed, buffers_created, dirty;
	Array<vector> vertices;
	Array<vector> normals;
	Array<float> tex_coords[NIX_MAX_TEXTURELEVELS];
	unsigned int buf_v, buf_n, buf_t[NIX_MAX_TEXTURELEVELS];

	OldVertexBuffer(int num_textures);
	void _cdecl __init__(int num_textures);
	~OldVertexBuffer();
	void _cdecl __delete__();

	void _cdecl clear();
	void _cdecl addTria(const vector &p1, const vector &n1, float tu1, float tv1,
	                    const vector &p2, const vector &n2, float tu2, float tv2,
	                    const vector &p3, const vector &n3, float tu3, float tv3);
	void _cdecl addTriaM(const vector &p1, const vector &n1, const float *t1,
	                     const vector &p2, const vector &n2, const float *t2,
	                     const vector &p3, const vector &n3, const float *t3);
	void _cdecl addTrias(int num_trias, const vector *p, const vector *n, const float *t);
	void _cdecl addTriasM(int num_trias, const vector *p, const vector *n, const float *t);
	void _cdecl addTriasIndexed(int num_points, int num_trias, const vector *p, const vector *n, const float *tu, const float *tv, const int *indices);
	void _cdecl update();
};

class VertexBuffer {
public:
	struct Buffer {
		unsigned int buffer;
		int count;
	} buf[MAX_VB_BUFFERS];
	struct Attribute {
		unsigned int buffer;
		int num_components;
		unsigned int type;
		bool normalized;
		int stride;
	} attr[MAX_VB_ATTRIBUTES];
	int num_attributes;
	int num_buffers;

	VertexBuffer(const string &f);
	~VertexBuffer();

	void _cdecl __init__(const string &f);
	void _cdecl __delete__();

	void _cdecl update(int index, const DynamicArray &a);
	int count() const;
};

void init_vertex_buffers();

void SetVertexBuffer(VertexBuffer *vb);

};


#endif

#endif
