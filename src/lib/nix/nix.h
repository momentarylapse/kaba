/*----------------------------------------------------------------------------*\
| Nix                                                                          |
| -> abstraction layer for api (graphics)                                      |
| -> OpenGL and DirectX9 supported                                             |
|   -> able to switch in runtime                                               |
| -> mathematical types and functions                                          |
|   -> vector, matrix, matrix3, quaternion, plane, color                       |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2008.10.27 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#ifndef _NIX_EXISTS_
#define _NIX_EXISTS_



#include "../config.h"


#include "nix_config.h"
#include "nix_draw.h"
#include "nix_light.h"
#include "nix_vertexbuffer.h"
#include "nix_shader.h"
#include "nix_view.h"
#include "nix_input.h"
#include "nix_textures.h"


extern string NixVersion;


//--------------------------------------------------------------------//
//                     all about graphics                             //
//--------------------------------------------------------------------//
void avi_close(int texture);

void _cdecl NixInit(const string &api,int xres,int yres,int depth,bool fullscreen,CHuiWindow *win);
void _cdecl NixSetVideoMode(const string &api,int xres,int yres,int depth,bool fullscreen);
void NixTellUsWhatsWrong();
void NixKillDeviceObjects();
void NixReincarnateDeviceObjects();
void NixKill();
void NixKillWindows();

// engine properties
void _cdecl NixSetWire(bool enabled);
void _cdecl NixSetCull(int mode);
void _cdecl NixSetZ(bool write,bool test);
void _cdecl NixSetAlpha(int mode);
void _cdecl NixSetAlpha(int src,int dst);
void _cdecl NixSetAlpha(float factor);
void _cdecl NixSetAlphaM(int mode);
void _cdecl NixSetAlphaSD(int src,int dst);
void _cdecl NixSetFog(int mode,float start,float end,float density,const color &c);
void _cdecl NixEnableFog(bool enabled);
void _cdecl NixSetStencil(int mode,unsigned long param=0);
void _cdecl NixSetShading(int mode);


extern rect NixTargetRect;


#endif
