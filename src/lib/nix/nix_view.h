/*----------------------------------------------------------------------------*\
| Nix view                                                                     |
| -> camera etc...                                                             |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_VIEW_EXISTS_
#define _NIX_VIEW_EXISTS_

class NixTexture;

// configuring the view
void _NixSetMode2d();
void _NixSetMode3d();
void _cdecl NixSetProjectionPerspective();
void _cdecl NixSetProjectionPerspectiveExt(float center_x, float center_y, float width_1, float height_1, float z_min, float z_max);
void _cdecl NixSetProjectionOrtho(bool relative);
void _cdecl NixSetProjectionOrthoExt(float center_x, float center_y, float map_width, float map_height, float z_min, float z_max);
void _cdecl NixSetProjectionMatrix(const matrix &mat);
void _cdecl NixSetWorldMatrix(const matrix &mat);
void _cdecl NixSetView(const vector &view_pos, const vector &view_ang);
void _cdecl NixSetView(const matrix &view_mat);
void _cdecl NixSetViewV(const vector &view_pos, const vector &view_ang);
void _cdecl NixSetViewM(const matrix &view_mat);
void _cdecl NixGetVecProject(vector &vout, const vector &vin);
void _cdecl NixGetVecUnproject(vector &vout, const vector &vin);
void _cdecl NixGetVecProjectRel(vector &vout, const vector &vin);
void _cdecl NixGetVecUnprojectRel(vector &vout, const vector &vin);
bool _cdecl NixIsInFrustrum(const vector &pos, float radius);
void _cdecl NixSetClipPlane(int index, const plane &pl);
void _cdecl NixEnableClipPlane(int index, bool enabled);
void NixResize();

bool _cdecl NixStart();
bool _cdecl NixStartIntoTexture(NixTexture *t);
void _cdecl NixScissor(const rect &r);
void _cdecl NixEnd();

void _cdecl NixScreenShot(const string &filename, int width = -1, int height = -1);
void _cdecl NixScreenShotToImage(Image &image);

extern float NixViewJitterX, NixViewJitterY;


#endif
