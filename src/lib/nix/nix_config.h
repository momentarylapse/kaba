/*----------------------------------------------------------------------------*\
| Nix config                                                                   |
| -> configuration for nix                                                     |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_CONFIG_EXISTS_
#define _NIX_CONFIG_EXISTS_

#include "../config.h"



// which developing environment?

#ifdef _MSC_VER
	#define NIX_IDE_VCS
	#if _MSC_VER >= 1400
		#define NIX_IDE_VCS8
	#else
		#define NIX_IDE_VCS6
	#endif
#else
	#define NIX_IDE_DEVCPP
#endif
//#define NIX_IDE_KDEVELOP ...?



// which graphics api possible?

#define NIX_API_NONE					0
#define NIX_API_OPENGL					2




#ifndef OS_WINDOWS
	#undef NIX_ALLOW_VIDEO_TEXTURE
#endif


#include <math.h>
#include "../base/base.h"
#include "../file/file.h"
#include "../hui/hui.h"
#include "../math/math.h"


typedef void callback_function();



#define NIX_MAX_TEXTURELEVELS	8

enum{
	FatalErrorNone,
	FatalErrorNoDirectX8,
	FatalErrorNoDirectX9,
	FatalErrorNoDevice,
	FatalErrorUnknownApi
};

//#define ResX	NixScreenWidth
//#define ResY	NixScreenHeight
#define MaxX	NixTargetWidth
#define MaxY	NixTargetHeight





#define AlphaNone			0
#define AlphaZero			0
#define AlphaOne			1
#define AlphaSourceColor	2
#define AlphaSourceInvColor	3
#define AlphaSourceAlpha	4
#define AlphaSourceInvAlpha	5
#define AlphaDestColor		6
#define AlphaDestInvColor	7
#define AlphaDestAlpha		8
#define AlphaDestInvAlpha	9

#define AlphaColorKey		10
#define AlphaColorKeySmooth	10
#define AlphaColorKeyHard	11
#define AlphaAdd			12
#define AlphaMaterial		13

enum{
	CullNone,
	CullCCW,
	CullCW
};
#define CullDefault		CullCCW

enum{
	StencilNone,
	StencilIncrease,
	StencilDecrease,
	StencilDecreaseNotNegative,
	StencilSet,
	StencilMaskEqual,
	StencilMaskNotEqual,
	StencilMaskLess,
	StencilMaskLessEqual,
	StencilMaskGreater,
	StencilMaskGreaterEqual,
	StencilReset
};

enum{
	FogLinear,
	FogExp,
	FogExp2
};

#define ShadingPlane			0
#define ShadingRound			1



extern int NixFontHeight;
extern string NixFontName;

extern hui::Window *NixWindow;

extern int NixApi;
extern string NixApiName;
extern int NixScreenWidth, NixScreenHeight, NixScreenDepth;		// current screen resolution
extern int NixDesktopWidth, NixDesktopHeight, NixDesktopDepth;	// pre-NIX-resolution
extern int NixTargetWidth, NixTargetHeight;						// render target size (window/texture)
extern bool NixFullscreen;
extern callback_function *NixRefillAllVertexBuffers;			// animate the application to refill lost VertexBuffers
extern bool NixLightingEnabled;
extern bool NixCullingInverted;

extern float NixMouseMappingWidth, NixMouseMappingHeight;		// fullscreen mouse territory
extern int NixFatalError;
extern int NixNumTrias;

extern string NixTextureDir;
extern int NixTextureMaxFramesToLive, NixMaxVideoTextureSize;

class NixVertexBuffer;
extern NixVertexBuffer *VBTemp; // vertex buffer for 1-frame geometries

#endif
