/**************************************************************************************************/
// centralization of api data
//
// last update: 2005.03.11 (c) by MichiSoft TM
/**************************************************************************************************/

#define _X_USE_HUI_MINIMAL_
#define _X_USE_NET_
#if HAS_LIB_GL
	#define _X_USE_NIX_
#endif
#define _X_USE_IMAGE_
#define _X_USE_KABA_
#define _X_USE_THREADS_
#define _X_USE_ALGEBRA_
#define _X_USE_ANY_
#if HAS_LIB_VULKAN
	#define _X_USE_VULKAN_
#endif
#define _X_USE_PDF_

//#####################################################################
// Hui-API
//
// graphical user interface in the hui/* files
//#####################################################################

#define HUI_USE_GTK_ON_WINDOWS		// use gtk instead of windows api on windows



#define IMAGE_ALLOW_PNG

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


//#define SOUND_ALLOW_OPENAL				1
//#define SOUND_ALLOW_OGG					1
//#define SOUND_ALLOW_FLAC				1



