/*----------------------------------------------------------------------------*\
| Nix vertex buffer                                                            |
| -> handling vertex buffers                                                   |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"



NixVertexBuffer::NixVertexBuffer(int _num_textures)
{
	num_textures = _num_textures;
	msg_db_f(format("creating vertex buffer (%d tex coords)", num_textures).c_str(), 1);
	if (num_textures > NIX_MAX_TEXTURELEVELS)
		num_textures = NIX_MAX_TEXTURELEVELS;
	indexed = false;

	num_triangles = 0;

	buffers_created = false;
	optimized = false;

	#ifdef ENABLE_INDEX_BUFFERS
		/*VBIndex[NumVBs]=new ...; // noch zu bearbeiten...
		if (!OGLVBIndex[index]){
			msg_error("IndexBuffer konnte nicht erstellt werden");
			return -1;
		}*/
	#endif
}

NixVertexBuffer::~NixVertexBuffer()
{
	msg_db_f("deleting vertex buffer", 1);
	clear();
}

void NixVertexBuffer::__init__(int _num_textures)
{
	new(this) NixVertexBuffer(_num_textures);
}

void NixVertexBuffer::__delete__()
{
	this->~NixVertexBuffer();
}

void NixVertexBuffer::clear()
{
	vertices.clear();
	normals.clear();
	for (int i=0; i<num_textures; i++)
		tex_coords[i].clear();
	num_triangles = 0;
	optimized = false;
}

void NixVertexBuffer::optimize()
{
	if (!buffers_created){
		glGenBuffers(1, &buf_v);
		glGenBuffers(1, &buf_n);
		glGenBuffers(num_textures, buf_t);
		buffers_created = true;
	}
	glBindBuffer(GL_ARRAY_BUFFER_ARB, buf_v);
	glBufferData(GL_ARRAY_BUFFER, vertices.num * sizeof(vertices[0]), vertices.data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, buf_n);
	glBufferData(GL_ARRAY_BUFFER, normals.num * sizeof(normals[0]), normals.data, GL_STATIC_DRAW);
	for (int i=0; i<num_textures; i++){
		glBindBuffer(GL_ARRAY_BUFFER_ARB, buf_t[i]);
		glBufferData(GL_ARRAY_BUFFER, tex_coords[i].num * sizeof(tex_coords[i][0]), tex_coords[i].data, GL_STATIC_DRAW);
	}
	optimized = true;
}

void NixVertexBuffer::addTria(const vector &p1,const vector &n1,float tu1,float tv1,
								const vector &p2,const vector &n2,float tu2,float tv2,
								const vector &p3,const vector &n3,float tu3,float tv3)
{
	vertices.add(p1);
	vertices.add(p2);
	vertices.add(p3);
	normals.add(n1);
	normals.add(n2);
	normals.add(n3);
	tex_coords[0].add(tu1);
	tex_coords[0].add(1 - tv1);
	tex_coords[0].add(tu2);
	tex_coords[0].add(1 - tv2);
	tex_coords[0].add(tu3);
	tex_coords[0].add(1 - tv3);
	indexed = false;
	num_triangles ++;
	optimized = false;
}

void NixVertexBuffer::addTriaM(const vector &p1,const vector &n1,const float *t1,
								const vector &p2,const vector &n2,const float *t2,
								const vector &p3,const vector &n3,const float *t3)
{
	vertices.add(p1);
	vertices.add(p2);
	vertices.add(p3);
	normals.add(n1);
	normals.add(n2);
	normals.add(n3);
	for (int i=0;i<num_textures;i++){
		tex_coords[i].add(t1[i*2  ]);
		tex_coords[i].add(1 - t1[i*2+1]);
		tex_coords[i].add(t2[i*2  ]);
		tex_coords[i].add(1 - t2[i*2+1]);
		tex_coords[i].add(t3[i*2  ]);
		tex_coords[i].add(1 - t3[i*2+1]);
	}
	indexed = false;
	num_triangles ++;
	optimized = false;
}

// for each triangle there have to be 3 vertices (p[i],n[i],t[i*2],t[i*2+1])
void NixVertexBuffer::addTrias(int num_trias, const vector *p, const vector *n, const float *t)
{
	int nv0 = vertices.num;
	vertices.resize(vertices.num + num_trias * 3);
	normals.resize(normals.num + num_trias * 3);
	memcpy(&vertices[nv0], p, sizeof(vector) * num_trias * 3);
	memcpy(&normals[nv0], n, sizeof(vector) * num_trias * 3);
	//memcpy(OGLVBTexCoords[buffer][0],t,sizeof(float)*num_trias*6);
	int nt0 = tex_coords[0].num;
	tex_coords[0].resize(nt0 + 6 * num_trias);
	for (int i=0;i<num_trias*3;i++){
		tex_coords[0][nt0 + i*2  ] = t[i*2];
		tex_coords[0][nt0 + i*2+1] = t[i*2+1];
	}
	num_triangles += num_trias;
	optimized = false;
}

void NixVBAddTriasIndexed(int buffer,int num_points,int num_trias,const vector *p,const vector *n,const float *tu,const float *tv,const unsigned short *indices)
{
	#ifdef ENABLE_INDEX_BUFFERS
	#endif
}

void NixVertexBuffer::draw()
{
	_NixSetMode3d();

	if (optimized){
		glBindBuffer(GL_ARRAY_BUFFER, buf_v);
		glVertexPointer(3, GL_FLOAT, 0, (char *)NULL);
		glBindBuffer(GL_ARRAY_BUFFER, buf_n);
		glNormalPointer(GL_FLOAT, 0, (char *)NULL);
	}else{
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, vertices.data);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, 0, normals.data);
	}

	// set multitexturing
	if (OGLMultiTexturingSupport){
		for (int i=0;i<num_textures;i++){
			glClientActiveTexture(GL_TEXTURE0 + i);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			if (optimized){
				glBindBuffer(GL_ARRAY_BUFFER_ARB, buf_t[i]);
				glTexCoordPointer(2, GL_FLOAT, 0, (char *)NULL);
			}else
				glTexCoordPointer(2, GL_FLOAT, 0, tex_coords[i].data);
		}
	}else{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, tex_coords[0].data);
	}

	// draw
	glDrawArrays(GL_TRIANGLES, 0, num_triangles * 3);

	/*glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);*/

	NixNumTrias += num_triangles;
	TestGLError("Draw3D");
}
