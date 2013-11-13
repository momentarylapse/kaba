/*----------------------------------------------------------------------------*\
| Nix shader                                                                   |
| -> shader files                                                              |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_SHADER_EXISTS_
#define _NIX_SHADER_EXISTS_



class NixShader
{
public:
	string filename;
	int glProgram;
	int reference_count;
	NixShader();
	~NixShader();
	void _cdecl unref();
	void _cdecl set_data(const string &var_name, const void *data, int size);
	void _cdecl get_data(const string &var_name, void *data, int size);
};


// shader files
void _cdecl NixDeleteAllShaders();
NixShader* _cdecl NixLoadShader(const string &filename);
NixShader* _cdecl NixCreateShader(const string &source);
void _cdecl NixSetShader(NixShader *s);

extern string NixShaderDir;

#endif
