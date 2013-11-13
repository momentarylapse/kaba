/*----------------------------------------------------------------------------*\
| Nix textures                                                                 |
| -> texture loading and handling                                              |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2008.11.02 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_TEXTURES_EXISTS_
#define _NIX_TEXTURES_EXISTS_

// textures
void NixTexturesInit();
void NixReleaseTextures();
void NixReincarnateTextures();
void NixProgressTextureLifes();


class NixTexture
{
public:
	string filename;
	int width, height;
	bool is_dynamic, valid, is_cube_map;
	int life_time;
	
	unsigned int glTexture;
	unsigned int glFrameBuffer;
	unsigned int glDepthRenderBuffer;

	Image icon;

	NixTexture();
	~NixTexture();
	void _cdecl __init__();
	void _cdecl __delete__();

	void _cdecl overwrite(const Image &image);
	void _cdecl reload();
	void _cdecl unload();
	void _cdecl set_video_frame(int frame);
	void _cdecl video_move(float elapsed);
	bool _cdecl start_render();
	void _cdecl render_to_cube_map(vector &pos, callback_function *render_scene, int mask);
	void _cdecl fill_cube_map(int side, NixTexture *source);
};

class NixDynamicTexture : public NixTexture
{
public:
	NixDynamicTexture(int width, int height);
	void _cdecl __init__(int width, int height);
};

class NixCubeMap : public NixTexture
{
public:
	NixCubeMap(int size);
	void _cdecl __init__(int size);
};


NixTexture* _cdecl NixLoadTexture(const string &filename);
void _cdecl NixSetTexture(NixTexture *texture);
void _cdecl NixSetTextures(NixTexture **texture, int num_textures);

extern Array<NixTexture*> NixTextures;

extern int NixTextureIconSize;

#endif
