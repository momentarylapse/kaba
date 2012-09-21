/*----------------------------------------------------------------------------*\
| Nix light                                                                    |
| -> handling light sources                                                    |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_LIGHT_EXISTS_
#define _NIX_LIGHT_EXISTS_



void _cdecl NixEnableLighting(bool enabled);
int _cdecl NixCreateLight();
void _cdecl NixDeleteLight(int index);
void _cdecl NixSetLightRadial(int num,const vector &pos,float radius,const color &ambient,const color &diffuse,const color &specular);
void _cdecl NixSetLightDirectional(int num,const vector &dir,const color &ambient,const color &diffuse,const color &specular);
void _cdecl NixEnableLight(int num,bool enabled);
void _cdecl NixSetAmbientLight(const color &c);
void _cdecl NixSetMaterial(const color &ambient,const color &diffuse,const color &specular,float shininess,const color &emission);
void _cdecl NixSpecularEnable(bool enabled);

#endif
