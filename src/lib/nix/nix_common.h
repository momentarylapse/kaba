/*----------------------------------------------------------------------------*\
| Nix common                                                                   |
| -> common data references                                                    |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/


#include <stdio.h>

#ifdef OS_WINDOWS
	#define _WIN32_WINDOWS 0x500
	#include <stdio.h>
	#include <io.h>
	#include <direct.h>
	#include <mmsystem.h>
	#pragma warning(disable : 4995)
	#ifdef NIX_ALLOW_VIDEO_TEXTURE
		#include "vfw.h"
	#endif
#endif

#ifdef OS_LINUX
#ifdef NIX_ALLOW_FULLSCREEN
	#include <X11/extensions/xf86vmode.h>
#endif
	#include <X11/keysym.h>
	#include <stdlib.h>
	#include <gdk/gdkx.h>
	#include <sys/time.h>
	#include <sys/types.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#define _open	open
	#define _close	close
#endif

#ifdef OS_WINDOWS
	#include <gl\gl.h>
	#include <gl\glu.h>
	#include <gl\glext.h>
	#include <gl\wglext.h>
#endif
#ifdef OS_LINUX
	#define GL_GLEXT_PROTOTYPES
	#include <GL/glx.h>
	#include <GL/gl.h>
	#include <GL/glext.h>
	#include <GL/glu.h>
#endif





#ifdef OS_WINDOWS
	// multitexturing
	extern PFNGLACTIVETEXTUREPROC glActiveTexture;
	extern PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture;
	// shader
	extern PFNGLCREATESHADERPROC glCreateShader;
	extern PFNGLSHADERSOURCEPROC glShaderSource;
	extern PFNGLCOMPILESHADERPROC glCompileShader;
	extern PFNGLATTACHSHADERPROC glAttachShader;
	extern PFNGLGETSHADERIVPROC glGetShaderiv;
	extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
	extern PFNGLCREATEPROGRAMPROC glCreateProgram;
	extern PFNGLLINKPROGRAMPROC glLinkProgram;
	extern PFNGLUSEPROGRAMPROC glUseProgram;
	extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
	extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
	extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
	extern PFNGLDELETESHADERPROC glDeleteShader;
	extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
	extern PFNGLUNIFORM1FPROC glUniform1f;
	extern PFNGLUNIFORM3FPROC glUniform3f;
	extern PFNGLUNIFORM1IPROC glUniform1i;
#endif
extern bool OGLMultiTexturingSupport;
extern bool OGLShaderSupport;

#ifdef OS_WINDOWS
	extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
	extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
	extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
	extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
	extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
	extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
	extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
	extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
	extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
	extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
	extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
	extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
	extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
	extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
	extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
	extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
	extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;
#endif

extern bool OGLDynamicTextureSupport;
extern bool NixGLDoubleBuffered;
extern int NixOGLFontDPList;
extern int NixGLCurrentProgram;


extern matrix NixViewMatrix, NixProjectionMatrix;
extern matrix NixWorldMatrix, NixWorldViewProjectionMatrix;

extern bool NixUsable, NixDoingEvilThingsToTheDevice;
extern bool NixEnabled3D;

extern int NixFontGlyphWidth[256];

void TestGLError(const char *);





