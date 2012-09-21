/*----------------------------------------------------------------------------*\
| Nix vertex buffer                                                            |
| -> handling vertex buffers                                                   |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_VERTEXBUFFER_EXISTS_
#define _NIX_VERTEXBUFFER_EXISTS_


// vertex buffers
int _cdecl NixCreateVB(int max_trias,int index=-1);
int _cdecl NixCreateVBM(int max_trias,int num_textures,int index=-1);
void _cdecl NixDeleteVB(int buffer);
bool NixVBAddTria(int buffer,	const vector &p1,const vector &n1,float tu1,float tv1,
								const vector &p2,const vector &n2,float tu2,float tv2,
								const vector &p3,const vector &n3,float tu3,float tv3);
bool NixVBAddTriaM(int buffer,	const vector &p1,const vector &n1,const float *t1,
								const vector &p2,const vector &n2,const float *t2,
								const vector &p3,const vector &n3,const float *t3);
void _cdecl NixVBAddTrias(int buffer,int num_trias,const vector *p,const vector *n,const float *t);
void _cdecl NixVBAddTriasM(int buffer,int num_trias,const vector *p,const vector *n,const float *t);
void _cdecl NixVBAddTriasIndexed(int buffer,int num_points,int num_trias,const vector *p,const vector *n,const float *tu,const float *tv,const unsigned short *indices);
void _cdecl NixVBClear(int buffer);
//void _cdecl NixVBClearM(int buffer);
int NixVBGetMaxTrias(int buffer);

#endif
