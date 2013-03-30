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

int NixCreateEmptyTexture();
int NixLoadTexture(const string &filename);
int _cdecl NixCreateDynamicTexture(int width, int height);
void NixOverwriteTexture(int texture, const Image &image);
//void NixReloadTexture(int texture);
//void NixUnloadTexture(int texture);
void NixSetTexture(int texture);
void NixSetTextures(int *texture, int num_textures);
void NixSetTextureVideoFrame(int texture, int frame);
void NixTextureVideoMove(int texture, float elapsed);
int NixCreateCubeMap(int size);
void NixRenderToCubeMap(int cube_map,vector &pos,callback_function *render_scene,int mask);
void NixFillCubeMap(int cube_map, int side, int source_tex);



struct NixTexture
{
	string filename;
	int width, height;
	bool is_dynamic, valid, is_cube_map;
	int life_time;
	
	unsigned int glTexture;
	unsigned int glFrameBuffer;
	unsigned int glDepthRenderBuffer;

	Image icon;
};

extern Array<NixTexture> NixTextures;

extern int NixTextureIconSize;

#endif
