/*----------------------------------------------------------------------------*\
| Nix vertex buffer                                                            |
| -> handling vertex buffers                                                   |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"



NixVertexBuffer::NixVertexBuffer(int num_textures)
{
	msg_db_r(format("creating vertex buffer (%d tex coords)",num_textures).c_str(), 1);
	if (num_textures > NIX_MAX_TEXTURELEVELS)
		num_textures = NIX_MAX_TEXTURELEVELS;
	indexed = false;

	#ifdef ENABLE_INDEX_BUFFERS
		/*VBIndex[NumVBs]=new ...; // noch zu bearbeiten...
		if (!OGLVBIndex[index]){
			msg_error("IndexBuffer konnte nicht erstellt werden");
			return -1;
		}*/
	#endif
	numTextures = num_textures;
	msg_db_l(1);
}

NixVertexBuffer::~NixVertexBuffer()
{
	msg_db_r("deleting vertex buffer", 1);
	clear();
	msg_db_l(1);
}

void NixVertexBuffer::__init__(int num_textures)
{
	new(this) NixVertexBuffer(num_textures);
}

void NixVertexBuffer::__delete__()
{
	this->~NixVertexBuffer();
}

void NixVertexBuffer::clear()
{
	vertices.clear();
	normals.clear();
	for (int i=0;i<numTextures;i++)
		texCoords[i].clear();
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
	texCoords[0].add(tu1);
	texCoords[0].add(1 - tv1);
	texCoords[0].add(tu2);
	texCoords[0].add(1 - tv2);
	texCoords[0].add(tu3);
	texCoords[0].add(1 - tv3);
	indexed = false;
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
	for (int i=0;i<numTextures;i++){
		texCoords[i].add(t1[i*2  ]);
		texCoords[i].add(1 - t1[i*2+1]);
		texCoords[i].add(t2[i*2  ]);
		texCoords[i].add(1 - t2[i*2+1]);
		texCoords[i].add(t3[i*2  ]);
		texCoords[i].add(1 - t3[i*2+1]);
	}
	indexed = false;
}

// for each triangle there have to be 3 vertices (p[i],n[i],t[i*2],t[i*2+1])
void NixVertexBuffer::addTrias(int num_trias,const vector *p,const vector *n,const float *t)
{
	int nv0 = vertices.num;
	vertices.resize(vertices.num + num_trias * 3);
	normals.resize(normals.num + num_trias * 3);
	memcpy(&vertices[nv0], p, sizeof(vector) * num_trias * 3);
	memcpy(&normals[nv0], n, sizeof(vector) * num_trias * 3);
	//memcpy(OGLVBTexCoords[buffer][0],t,sizeof(float)*num_trias*6);
	int nt0 = texCoords[0].num;
	texCoords[0].resize(nt0 + 6 * num_trias);
	for (int i=0;i<num_trias*3;i++){
		texCoords[0][nt0 + i*2  ] = t[i*2];
		texCoords[0][nt0 + i*2+1] = t[i*2+1];
	}
}

void NixVBAddTriasIndexed(int buffer,int num_points,int num_trias,const vector *p,const vector *n,const float *tu,const float *tv,const unsigned short *indices)
{
	#ifdef ENABLE_INDEX_BUFFERS
	#endif
}

void NixVertexBuffer::draw()
{
	_NixSetMode3d();


	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, vertices.data);
	glNormalPointer(GL_FLOAT, 0, normals.data);

	// set multitexturing
	if (OGLMultiTexturingSupport){
		for (int i=0;i<numTextures;i++){
			glClientActiveTexture(GL_TEXTURE0 + i);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, 0, texCoords[i].data);
		}
	}else{
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, texCoords[0].data);
	}

	// draw
	glDrawArrays(GL_TRIANGLES, 0, vertices.num);

	// unset multitexturing
	/*if (OGLMultiTexturingSupport){
		for (int i=1;i<numTextures;i++){
			glActiveTexture(GL_TEXTURE0 + i);
			glDisable(GL_TEXTURE_2D);
			glClientActiveTexture(GL_TEXTURE0 + i);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		glActiveTexture(GL_TEXTURE0);
		glClientActiveTexture(GL_TEXTURE0);
	}*/

	NixNumTrias += vertices.num / 3;
	TestGLError("Draw3D");
}
