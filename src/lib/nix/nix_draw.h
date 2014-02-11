/*----------------------------------------------------------------------------*\
| Nix draw                                                                     |
| -> drawing functions                                                         |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_DRAW_EXISTS_
#define _NIX_DRAW_EXISTS_

class NixVertexBuffer;
class NixTexture;

void _cdecl NixResetToColor(const color &c);
void _cdecl NixResetZ();
void _cdecl NixSetColor(const color &c);
color _cdecl NixGetColor();
void _cdecl NixDrawChar(float x, float y, char c);
void _cdecl NixDrawStr(float x, float y, const string &str);
int _cdecl NixGetStrWidth(const string &str);
void _cdecl NixDrawLine(float x1, float y1, float x2, float y2, float depth);
void _cdecl NixDrawLineV(float x, float y1, float y2, float depth);
void _cdecl NixDrawLineH(float x1, float x2, float y, float depth);
void _cdecl NixDrawLines(float *x, float *y, int num_lines, bool contiguous, float depth);
void _cdecl NixDrawLine3D(const vector &l1, const vector &l2);
void _cdecl NixDrawRect(float x1, float x2, float y1, float y2, float depth);
void _cdecl NixDraw2D(const rect &src, const rect &dest, float depth);
void _cdecl NixDrawSpriteR(const rect &src, const vector &pos, const rect &dest);
void _cdecl NixDrawSprite(const rect &src, const vector &pos, float radius);
void _cdecl NixDraw3DCubeMapped(NixTexture *cube_map, NixVertexBuffer *vb);

extern float NixLineWidth;
extern bool NixSmoothLines;

#endif
