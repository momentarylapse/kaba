/**************************************************************************************************/
// centralization of api data
//
// last update: 2005.03.11 (c) by MichiSoft TM
/**************************************************************************************************/

#define _X_USE_HUI_
#define _X_USE_NET_
#define _X_USE_NIX_
#define _X_USE_IMAGE_
#define _X_USE_SCRIPT_
#define _X_USE_THREADS_
#define _X_USE_ALGEBRA_
#define _X_USE_SOUND_
#define _X_USE_ANY_

//#####################################################################
// Hui-API
//
// graphical user interface in the hui/* files
//#####################################################################

//#define HUI_USE_GTK_ON_WINDOWS		// use gtk instead of windows api on windows



//#####################################################################
// Nix-API
//
// graphics and sound support in the nix/* files
//#####################################################################

//#define NIX_ALLOW_API_DIRECTX8
//#define NIX_ALLOW_API_DIRECTX9
#define NIX_ALLOW_API_OPENGL
//#define NIX_ALLOW_TYPES_BY_DIRECTX9		// let matrices, quaternios,... be computed by DirectX9?
//#define NIX_ALLOW_VIDEO_TEXTURE			// allow Avi-videos as texture?
#define NIX_ALLOW_DYNAMIC_TEXTURE		// allow textures as render targets?
//#define NIX_ALLOW_CUBEMAPS				// allow cubical environment mapping?
//#define NIX_ALLOW_SOUND					// allow support for sound?
//#define NIX_ALLOW_SOUND_BY_DIRECTX9		// sound controlled by DirectX9 (if allowed in general)?



/*#ifndef OS_WINDOWS
	#undef NIX_ALLOW_DYNAMIC_TEXTURE
	#undef NIX_ALLOW_VIDEO_TEXTURE
	#undef NIX_ALLOW_API_DIRECTX9
	#undef NIX_ALLOW_TYPES_BY_DIRECTX9
	#undef NIX_ALLOW_SOUND_BY_DIRECTX9
#endif*/





//#####################################################################
// Sound-API
//
// sound support...
//#####################################################################


#define SOUND_ALLOW_OPENAL				1
#define SOUND_ALLOW_OGG					1
//#define SOUND_ALLOW_FLAC				1



//#####################################################################
// X9-Engine
//
// components each with its own file
// uncomment the ones with their files in this project
//#####################################################################


#define _X_ALLOW_CAMERA_
#define _X_ALLOW_GOD_
#define _X_ALLOW_COLLISION_
#define _X_ALLOW_PHYSICS_
#define _X_ALLOW_MATRIXN_
#define _X_ALLOW_LINKS_
#define _X_ALLOW_TREE_
#define _X_ALLOW_TERRAIN_
#define _X_ALLOW_FX_
#define _X_ALLOW_META_
#define _X_ALLOW_MODEL_
#define _X_ALLOW_OBJECT_
#define _X_ALLOW_GUI_
//#define _X_ALLOW_MODEL_ANIMATION_
#define _X_ALLOW_SCRIPT_
//#define _X_ALLOW_X_

