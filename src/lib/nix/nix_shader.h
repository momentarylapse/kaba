/*----------------------------------------------------------------------------*\
| Nix shader                                                                   |
| -> shader files                                                              |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_SHADER_EXISTS_
#define _NIX_SHADER_EXISTS_


// shader files
int _cdecl NixLoadShader(const string &filename);
void _cdecl NixDeleteShader(int index);
void _cdecl NixSetShader(int index);
void _cdecl NixSetShaderData(int index, const string &var_name, const void *data, int size);
void _cdecl NixGetShaderData(int index, const string &var_name, void *data, int size);


#endif
