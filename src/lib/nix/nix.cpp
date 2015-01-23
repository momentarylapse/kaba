/*----------------------------------------------------------------------------*\
| Nix                                                                          |
| -> abstraction layer for api (graphics, sound, networking)                   |
| -> OpenGL and DirectX9 supported                                             |
|   -> able to switch in runtime                                               |
| -> mathematical types and functions                                          |
|   -> vector, matrix, matrix3, quaternion, plane, color                       |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2009.10.03 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/
#include "nix.h"
#include "nix_common.h"
#include "../hui/Controls/HuiControl.h"
#include "../hui/Controls/HuiControlDrawingArea.h"


string NixVersion = "0.12.0.3";


// libraries (in case Visual C++ is used)
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"glu32.lib")
//#pragma comment(lib,"glut32.lib")
//#pragma comment(lib,"glaux.lib ")
#ifdef NIX_ALLOW_VIDEO_TEXTURE
	#pragma comment(lib,"strmiids.lib")
	#pragma comment(lib,"vfw32.lib")
#endif

void TestGLError(const string &);


/*
libraries to link:

-lwinmm
-lcomctl32
-lkernel32
-luser32
-lgdi32
-lwinspool
-lcomdlg32
-ladvapi32
-lshell32
-lole32
-loleaut32
-luuid
-lodbc32
-lodbccp32
-lwinmm
-lgdi32
-lwsock32
-lopengl32
-lglu32
-lvfw_avi32
-lvfw_ms32
*/


#define NIX_GL_IN_WIDGET

#ifdef OS_WINDOWS
	#ifdef HUI_API_GTK
		#include <gdk/gdkwin32.h>
	#endif
#endif


void TestGLError(const char *pos)
{
#if 1
	int err = glGetError();
	if (err == GL_NO_ERROR)
	{}//msg_write("GL_NO_ERROR");
	else if (err == GL_INVALID_ENUM)
		msg_error("GL_INVALID_ENUM at " + string(pos));
	else if (err == GL_INVALID_VALUE)
		msg_error("GL_INVALID_VALUE at " + string(pos));
	else if (err == GL_INVALID_OPERATION)
		msg_error("GL_INVALID_OPERATION at " + string(pos));
	else if (err == GL_INVALID_FRAMEBUFFER_OPERATION)
		msg_error("GL_INVALID_FRAMEBUFFER_OPERATION at " + string(pos));
	else
		msg_error(i2s(err) + " at " + string(pos));
#endif
}

#ifdef OS_LINUX
int xerrorhandler(Display *dsp, XErrorEvent *error)
{
	char errorstring[128];
	XGetErrorText(dsp, error->error_code, errorstring, 128);

	msg_error(string("X error: ") + errorstring);
	HuiRaiseError(string("X error: ") + errorstring);
	exit(-1);
}
#endif


// environment
HuiWindow *NixWindow;
string NixControlID;
bool NixUsable,NixDoingEvilThingsToTheDevice;

// things'n'stuff
static bool WireFrame;
static int ClipPlaneMask;
int NixFontGlyphWidth[256];


int NixApi;
string NixApiName;
int NixScreenWidth,NixScreenHeight,NixScreenDepth;		// current screen resolution
int NixDesktopWidth,NixDesktopHeight,NixDesktopDepth;	// pre-NIX-resolution
int NixTargetWidth,NixTargetHeight;						// render target size (window/texture)
rect NixTargetRect;
bool NixFullscreen;
callback_function *NixRefillAllVertexBuffers;

float NixMouseMappingWidth,NixMouseMappingHeight;		// fullscreen mouse territory
int NixFatalError;
int NixNumTrias;
int NixTextureMaxFramesToLive,NixMaxVideoTextureSize=256;
float NixMaxDepth,NixMinDepth;

bool NixCullingInverted;


int NixFontHeight=20;
string NixFontName = "Times New Roman";

NixVertexBuffer *VBTemp;

#ifdef OS_WINDOWS
	static HMENU hMenu;
	HDC hDC;
	HGLRC hRC;
	static RECT WindowClient,WindowBounds;
	static DWORD WindowStyle;
#endif



//#define ENABLE_INDEX_BUFFERS

#ifdef OS_WINDOWS
	PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
	PFNGLCLIENTACTIVETEXTUREPROC glClientActiveTexture = NULL;
	PFNGLCREATESHADERPROC glCreateShader = NULL;
	PFNGLSHADERSOURCEPROC glShaderSource = NULL;
	PFNGLCOMPILESHADERPROC glCompileShader = NULL;
	PFNGLATTACHSHADERPROC glAttachShader = NULL;
	PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
	PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
	PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
	PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
	PFNGLUSEPROGRAMPROC glUseProgram = NULL;
	PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
	PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
	PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
	PFNGLDELETESHADERPROC glDeleteShader = NULL;
	PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
	PFNGLUNIFORM1FPROC glUniform1f = NULL;
	PFNGLUNIFORM3FPROC glUniform3f = NULL;
	PFNGLUNIFORM1IPROC glUniform1i = NULL;
	
	PFNGLGENBUFFERSPROC glGenBuffers = NULL;
	PFNGLBINDBUFFERPROC glBindBuffer = NULL;
	PFNGLBUFFERDATAPROC glBufferData = NULL;
#endif
bool OGLMultiTexturingSupport = false;
bool OGLShaderSupport = false;
bool OGLDynamicTextureSupport = false;
bool OGLVertexBufferSupport = false;
#ifdef OS_WINDOWS
	PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT = NULL;
	PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT = NULL;
	PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT = NULL;
	PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT = NULL;
	PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT = NULL;
	PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT = NULL;
	PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT = NULL;
	PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT = NULL;
	PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT = NULL;
	PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT = NULL;
	PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT = NULL;
	PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT = NULL;
	PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = NULL;
	PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT = NULL;
	PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT = NULL;
	PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT = NULL;
	PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT = NULL;
#else
#endif

static int OGLPixelFormat;
#ifdef OS_LINUX
	static GLXContext context;
	bool NixGLDoubleBuffered;
#endif

// shader files
int glShaderCurrent = 0;

// font
int NixOGLFontDPList;

#ifdef OS_LINUX
#ifdef NIX_ALLOW_FULLSCREEN
	XF86VidModeModeInfo *original_mode;
#endif
	int screen;
	XFontStruct *x_font = NULL;
#endif


void CreateFontGlyphWidth()
{
#ifdef OS_WINDOWS
	hDC=GetDC(NixWindow->hWnd);
	SetMapMode(hDC,MM_TEXT);
	HFONT hFont=CreateFont(	NixFontHeight,0,0,0,FW_EXTRALIGHT,FALSE,
							FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,
							VARIABLE_PITCH,hui_tchar_str(NixFontName));
	SelectObject(hDC,hFont);
	unsigned char str[5];
	SIZE size;
	for(int c=0;c<255;c++){
		str[0]=c;
		str[1]=0;
		GetTextExtentPoint32(hDC,hui_tchar_str((char*)str),1,&size);
		NixFontGlyphWidth[c]=size.cx;
	}
	DeleteObject(hFont);
#endif
#ifdef OS_LINUX
	//XQueryTextExtents(hui_x_display, );
	memset(NixFontGlyphWidth, 0, sizeof(NixFontGlyphWidth));
	for(int c=0;c<255;c++)
		NixFontGlyphWidth[c] = XTextWidth(x_font, (char*)&c, 1);
#endif
}

void MatrixOut(matrix &m)
{
	msg_write("MatrixOut");
	msg_write(format("	%f:2	%f:2	%f:2	%f:2",m._00,m._01,m._02,m._03));
	msg_write(format("	%f:2	%f:2	%f:2	%f:2",m._10,m._11,m._12,m._13));
	msg_write(format("	%f:2	%f:2	%f:2	%f:2",m._20,m._21,m._22,m._23));
	msg_write(format("	%f:2	%f:2	%f:2	%f:2",m._30,m._31,m._32,m._33));
}

void mout(matrix &m)
{
	msg_write(format("		%f	%f	%f	%f",m._00,m._01,m._02,m._03));
	msg_write(format("		%f	%f	%f	%f",m._10,m._11,m._12,m._13));
	msg_write(format("		%f	%f	%f	%f",m._20,m._21,m._22,m._23));
	msg_write(format("		%f	%f	%f	%f",m._30,m._31,m._32,m._33));
}

#ifdef OS_LINUX
/* attributes for a single buffered visual in RGBA format with at least
 * 4 bits per color and a 16 bit depth buffer */
int attrListSgl[]={
	GLX_RGBA,
	GLX_RED_SIZE,	4,
	GLX_GREEN_SIZE,	4,
	GLX_BLUE_SIZE,	4,
	GLX_DEPTH_SIZE,	16,
	None
};

/* attributes for a double buffered visual in RGBA format with at least
 * 4 bits per color and a 16 bit depth buffer */
int attrListDbl[]={
	GLX_RGBA,
	GLX_DOUBLEBUFFER,
	GLX_RED_SIZE,	4,
	GLX_GREEN_SIZE,	4,
	GLX_BLUE_SIZE,	4,
	GLX_DEPTH_SIZE,	16,
	GLX_STENCIL_SIZE,8,
//	GLX_STENCIL_SIZE,4,
	GLX_ALPHA_SIZE,	4,
	None
};

int attrListDblAccum[]={
	GLX_RGBA,
	GLX_DOUBLEBUFFER,
	GLX_RED_SIZE,	4,
	GLX_GREEN_SIZE,	4,
	GLX_BLUE_SIZE,	4,
	GLX_DEPTH_SIZE,	16,
	GLX_STENCIL_SIZE,8,
//	GLX_STENCIL_SIZE,4,
	GLX_ALPHA_SIZE,	4,
	GLX_ACCUM_RED_SIZE, 4,
	GLX_ACCUM_GREEN_SIZE, 4,
	GLX_ACCUM_BLUE_SIZE, 4,
	GLX_ACCUM_ALPHA_SIZE, 4,
	None
};

XVisualInfo *choose_visual()
{
	msg_db_f("choose_visual", 1);
	int screen = DefaultScreen(hui_x_display);

	HuiControl *c = NixWindow->_get_control_(NixControlID);
	HuiControlDrawingArea *da = dynamic_cast<HuiControlDrawingArea*>(c);
	GtkWidget *gl_widget = c->widget;
	if (gtk_widget_get_realized(gl_widget)){
		msg_error("realized -> reset");
		da->hardReset();
		gl_widget = c->widget;
	}

	// we need this one!
	//   at least if possible...
	/*int gdk_visualid = XVisualIDFromVisual(GDK_VISUAL_XVISUAL(gdk_window_get_visual(gtk_widget_get_window(NixWindow->window))));
	msg_write(gdk_visualid);*/

	// let glx select a visual
	XVisualInfo *vi = glXChooseVisual(hui_x_display, screen, attrListDblAccum);
	if (vi){
		msg_db_m("-doublebuffered + accum", 1);
		NixGLDoubleBuffered = true;
	}else{
		vi = glXChooseVisual(hui_x_display, screen, attrListDbl);
		if (vi){
			msg_db_m("-doublebuffered", 1);
			NixGLDoubleBuffered = true;
		}else{
			msg_error("only singlebuffered visual!");
			vi = glXChooseVisual(hui_x_display, screen, attrListSgl);
			NixGLDoubleBuffered = false;
		}
	}

	/*gdk_visualid = XVisualIDFromVisual(GDK_VISUAL_XVISUAL(gdk_window_get_visual(gtk_widget_get_window(gl_widget))));
	msg_write(gdk_visualid);
	if (gtk_widget_get_realized(gl_widget)){
		msg_error("realized 2");
	}*/

	// ok?
	/*if (vi->visualid != gdk_visualid)*/{
		//msg_write(format("%d!=%d", vi->visualid, gdk_visualid));

		// translate visual to gdk
		GdkScreen *gdk_screen = gdk_screen_get_default();
		GdkVisual *visual = gdk_x11_screen_lookup_visual(gdk_screen, vi->visualid);
		//msg_write(p2s(visual));
		// TODO GTK3
		//GdkColormap *colormap = gdk_colormap_new(visual, FALSE);

		// create new gtk_drawing_area with correct visual
		/*gtk_widget_destroy(NixWindow->gl_widget);
		NixWindow->gl_widget = gtk_drawing_area_new();
		gtk_widget_set_colormap(NixWindow->gl_widget, colormap);
		gtk_container_add(GTK_CONTAINER(NixWindow->hbox), NixWindow->gl_widget);
		gtk_widget_show(NixWindow->gl_widget);
		gtk_widget_realize(NixWindow->gl_widget);*/

		// no ...just apply color map
		gtk_widget_set_visual(gl_widget, visual);
	}

	// realize the widget...
	gtk_widget_show(gl_widget);
	gtk_widget_realize(gl_widget);
	gdk_window_invalidate_rect(gtk_widget_get_window(gl_widget), NULL, false);
	gdk_window_process_all_updates();

	int gdk_visualid = XVisualIDFromVisual(GDK_VISUAL_XVISUAL(gdk_window_get_visual(gtk_widget_get_window(gl_widget))));
	//msg_write(gdk_visualid);

	// look for the gdk-ish one...
	/*msg_db_m("-gdk/glx visual mismatch", 1);
	int n_fb_conf;
	GLXFBConfig *fb_conf;
	if (NixGLDoubleBuffered)
		fb_conf = glXChooseFBConfig(hui_x_display, screen, attrListDbl, &n_fb_conf);
	else
		fb_conf = glXChooseFBConfig(hui_x_display, screen, attrListSgl, &n_fb_conf);
	for (int i=0;i<n_fb_conf;i++){
		int value;
		if (glXGetFBConfigAttrib(hui_x_display, fb_conf[i], GLX_VISUAL_ID, &value) == Success)
			if (value == gdk_visualid){
				msg_db_m("  -> match!", 1);
				//msg_write("-----------hurraaaaaaaaaaaaaa");
				vi->visual = GDK_VISUAL_XVISUAL(gdk_window_get_visual(gl_widget->window));
				vi->visualid = gdk_visualid;
			}
	}*/

	return vi;
}
#endif


void NixInit(const string &api, HuiWindow *win, const string &id)
{
	NixUsable = false;
	if (!msg_inited)
		msg_init();

	msg_write("Nix");
	msg_right();
	msg_write("[" + NixVersion + "]");
	


	NixWindow = win;
	NixControlID = id;
	NixFullscreen = false; // before nix is started, we're hopefully not in fullscreen mode

#ifdef HUI_API_WIN
	//CoInitialize(NULL);

	// save window data
	if (NixWindow){
		WindowStyle=GetWindowLong(NixWindow->hWnd,GWL_STYLE);
		hMenu=GetMenu(NixWindow->hWnd);
		GetWindowRect(NixWindow->hWnd,&WindowBounds);
		GetClientRect(NixWindow->hWnd,&WindowClient);
		ShowCursor(FALSE); // will be shown again at next window mode initialization!
		NixWindow->NixGetInputFromWindow = &NixGetInputFromWindow;
	}
#endif

#ifdef OS_WINDOWS
	// save the original video mode
	DEVMODE mode;
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&mode);
	NixDesktopWidth=mode.dmPelsWidth;
	NixDesktopHeight=mode.dmPelsHeight;
	NixDesktopDepth=mode.dmBitsPerPel;
#endif
#ifdef OS_LINUX
	XSetErrorHandler(xerrorhandler);
	#ifdef NIX_ALLOW_FULLSCREEN
		XF86VidModeModeInfo **modes;
		int NumModes;
		screen = DefaultScreen(hui_x_display);
		XF86VidModeGetAllModeLines(hui_x_display, screen, &NumModes, &modes);
		original_mode = modes[0];
		NixDesktopWidth = modes[0]->hdisplay;
		NixDesktopHeight = modes[0]->vdisplay;
		NixDesktopDepth = modes[0]->hdisplay;
	#else
		NixDesktopWidth = XDisplayWidth(hui_x_display, 0);
		NixDesktopHeight = XDisplayHeight(hui_x_display, 0);
		NixDesktopDepth = 32;
	#endif
	/*if (NixWindow){
		Window SomeWindow;
		int x,y;
		unsigned int w,h,borderDummy,x_depth;
		XGetGeometry(hui_x_display,GDK_WINDOW_XWINDOW(NixWindow->window->window),&SomeWindow,&x,&y,&w,&h,&borderDummy,&x_depth);
		NixDesktopDepth=x_depth;
		//msg_write(format("Desktop: %dx%dx%d\n",NixDesktopWidth,NixDesktopHeight,NixDesktopDepth),0);
	}*/
	/*int glxMajorVersion,glxMinorVersion;
	int vidModeMajorVersion,vidModeMinorVersion;
	XF86VidModeQueryVersion(display,&vidModeMajorVersion,&vidModeMinorVersion);
	msg_db_m(format("XF86VidModeExtension-Version %d.%d\n",vidModeMajorVersion,vidModeMinorVersion),1);*/
#endif
	NixScreenWidth = NixDesktopWidth;
	NixScreenHeight = NixDesktopHeight;

	TestGLError("Init a");

	// reset data
	NixApi = -1;
	NixApiName = "";
	//NixFullscreen = fullscreen;
	NixFatalError = FatalErrorNone;
	NixRefillAllVertexBuffers = NULL;


	// default values of the engine
	MatrixIdentity(NixViewMatrix);
	MatrixIdentity(NixProjectionMatrix);
	NixMouseMappingWidth = 1024;
	NixMouseMappingHeight = 768;
	NixMaxDepth = 100000.0f;
	NixMinDepth = 1.0f;
	NixTextureMaxFramesToLive = 4096 * 8;
	ClipPlaneMask = 0;
	NixCullingInverted = false;

	// set the new video mode
	NixSetVideoMode(api, 640, 480, false);
	if (NixFatalError != FatalErrorNone){
		msg_left();
		return;
	}
	TestGLError("Init c");

	// more default values of the engine
	if (NixWindow){
		NixWindow->_get_control_(NixControlID)->getSize(NixTargetWidth, NixTargetHeight);
	}else{
		NixTargetWidth = 800;
		NixTargetHeight = 600;
	}
	NixTargetRect = rect(0, (float)NixTargetWidth, 0, (float)NixTargetHeight);

	NixSetCull(CullDefault);
	NixSetWire(false);
	NixSetAlpha(AlphaNone);
	NixEnableLighting(false);
	NixSetMaterial(White, White, White, 0, color(0.1f, 0.1f, 0.1f, 0.1f));
	NixSetAmbientLight(Black);
	NixSpecularEnable(false);
	NixCullingInverted = false;
	NixSetProjectionPerspective();
	NixResize();

	NixInputInit();

	NixTexturesInit();

	VBTemp = new NixVertexBuffer(1);
	NixUsable = true;

	TestGLError("Init post");


	msg_ok();
	msg_left();
}

void NixKill()
{
	NixKillDeviceObjects();
	if (NixFullscreen){
		#ifdef OS_WINDOWS
			/*DEVMODE dmScreenSettings;								// Device Mode
			memset(&dmScreenSettings,0,sizeof(dmScreenSettings));	// Makes Sure Memory's Cleared
			dmScreenSettings.dmSize=sizeof(dmScreenSettings);		// Size Of The Devmode Structure
			dmScreenSettings.dmPelsWidth	= 0;			// Selected Screen Width
			dmScreenSettings.dmPelsHeight	= 0;			// Selected Screen Height
			dmScreenSettings.dmBitsPerPel	= 0;			// Selected Bits Per Pixel
			dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
			ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN);*/
		#endif

		#ifdef OS_LINUX
			msg_write("Restore Video Mode");
			int r = system(format("xrandr --size %dx%d", NixDesktopWidth, NixDesktopHeight).c_str());
			/*bool b=XF86VidModeSwitchToMode(hui_x_display,screen,original_mode);
			XFlush(hui_x_display);
			XF86VidModeSetViewPort(hui_x_display,screen,0,0);*/
		#endif
	}
	if (NixWindow){
		NixWindow->setFullscreen(false);
		NixWindow->showCursor(true);
	}
}

// erlaubt dem Device einen Neustart
void NixKillDeviceObjects()
{
	msg_db_r("NixKillDeviceObjects",1);
	// textures
	msg_db_m("-textures",2);
	NixReleaseTextures();
	msg_db_l(1);
}

void NixReincarnateDeviceObjects()
{
	msg_db_r("ReincarnateDeviceObjects",1);
	// textures
	msg_db_m("-textures",2);
	NixReincarnateTextures();
	if (NixRefillAllVertexBuffers)
		NixRefillAllVertexBuffers();
	msg_db_l(1);
}


bool nixDevNeedsUpdate = true;


void set_video_mode_gl(int xres, int yres)
{
	msg_db_f("set_video_mode_gl", 1);
	HuiControl *c = NixWindow->_get_control_(NixControlID);
	gtk_widget_set_double_buffered(c->widget, false);


	#ifdef OS_WINDOWS

#ifdef NIX_GL_IN_WIDGET
	// realize the widget...
	gtk_widget_show(c->widget);
	gtk_widget_realize(c->widget);
	gdk_window_invalidate_rect(gtk_widget_get_window(c->widget), NULL, false);
	gdk_window_process_all_updates();
	
	/*HuiSleep(1);
	for (int i=0;i<20;i++)
		HuiDoSingleMainLoop();
	HuiSleep(1);*/

#endif
	
	bool was_fullscreen = NixFullscreen;
	msg_db_m("-pixelformat",1);
	if ((!NixFullscreen) && (was_fullscreen)){
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth=NixDesktopWidth;
		dmScreenSettings.dmPelsHeight=NixDesktopHeight;
		dmScreenSettings.dmBitsPerPel=NixDesktopDepth;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
		ChangeDisplaySettings(&dmScreenSettings,0);
	}else if (NixFullscreen){
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize=sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth=xres;
		dmScreenSettings.dmPelsHeight=yres;
		dmScreenSettings.dmBitsPerPel=32;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
		ChangeDisplaySettings(&dmScreenSettings,CDS_FULLSCREEN);
	}

	PIXELFORMATDESCRIPTOR pfd={	sizeof(PIXELFORMATDESCRIPTOR),
								1,						// versions nummer
								PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
								PFD_TYPE_RGBA,
								32,
								//8, 0, 8, 8, 8, 16, 8, 24,
								0, 0, 0, 0, 0, 0, 0, 0, 0,
								0, 0, 0, 0,
								24,						// 24bit Z-Buffer
								8,						// 8bit stencil buffer
								0,						// no "Auxiliary"-buffer
								PFD_MAIN_PLANE,
								0, 0, 0, 0 };
#ifdef HUI_API_WIN
		hDC = GetDC(NixWindow->hWnd);
#else

	


	NixWindow->hWnd = (HWND)GDK_WINDOW_HWND(gtk_widget_get_window(NixWindow->window));
//	msg_write(p2s((HWND)GDK_WINDOW_HWND(gtk_widget_get_window(NixWindow->window))));
//	msg_write(p2s((HWND)GDK_WINDOW_HWND(gtk_widget_get_window(NixWindow->gl_widget))));
	#ifdef NIX_GL_IN_WIDGET
		//hDC = GetDC((HWND)gdk_win32_drawable_get_handle(NixWindow->gl_widget->window));
		hDC = GetDC((HWND)GDK_WINDOW_HWND(gtk_widget_get_window(c->widget)));
	#else
		hDC = GetDC(NixWindow->hWnd);
	#endif
#endif
		if (!hDC){
			HuiErrorBox(NixWindow, "Fehler", "GetDC..." + i2s(GetLastError()));
			exit(0);
		}
		OGLPixelFormat = ChoosePixelFormat(hDC, &pfd);
		SetPixelFormat(hDC, OGLPixelFormat, &pfd);
		hRC=wglCreateContext(hDC);
		if (!hRC){
			HuiErrorBox(NixWindow, "Fehler", "wglCreateContext...");
			exit(0);
		}
		int rr=wglMakeCurrent(hDC, hRC);
		if (rr != 1){
			HuiErrorBox(NixWindow, "Fehler", "wglMakeCurrent...");
			exit(0);
		}

	#endif // OS_WINDOWS

	
	#ifdef OS_LINUX

		
		XVisualInfo *vi = choose_visual();
#if 0
	//Colormap cmap;
		_hui_x_display_ = GDK_WINDOW_XDISPLAY(NixWindow->gl_widget->window);
		printf("display %p\n", hui_x_display);
			#ifdef NIX_ALLOW_FULLSCREEN
				XF86VidModeModeInfo **modes;
				int num_modes;
				int best_mode = 0;
	//XSetWindowAttributes attr;
		msg_write("a2");
				XF86VidModeGetAllModeLines(hui_x_display, screen, &num_modes, &modes);
				msg_write("b");
				for (int i=0;i<num_modes;i++)
					if ((modes[i]->hdisplay == xres) && (modes[i]->vdisplay == yres))
						best_mode = i;
			#endif
			vi = glXChooseVisual(hui_x_display, screen, attrListDbl);
		msg_write("c");
			if (vi){
				msg_db_m("-doublebuffered", 1);
				NixGLDoubleBuffered = true;
			}else{
				msg_error("only singlebuffered visual!");
				vi = glXChooseVisual(hui_x_display, screen, attrListSgl);
				NixGLDoubleBuffered = false;
			}
			context = glXCreateContext(hui_x_display, vi, 0, GL_TRUE);
		msg_write(p2s(context));
		msg_write("d");
	//cmap=XCreateColormap(display,RootWindow(display,vi->screen),vi->visual,AllocNone);
	/*attr.colormap=cmap;
	attr.border_pixel=0;*/
	//Window win=GDK_WINDOW_XWINDOW(NixWindow->window->window);
#endif
		Window win = GDK_WINDOW_XID(gtk_widget_get_window(c->widget));
		context = glXCreateContext(hui_x_display, vi, 0, GL_TRUE);
		glXMakeCurrent(hui_x_display, win, context);
		TestGLError("glXMakeCurrent");


#if 0


    /* get a connection */
    _hui_x_display_ = XOpenDisplay(0);
    screen = DefaultScreen(hui_x_display);
//    XF86VidModeQueryVersion(hui_x_display, &vidModeMajorVersion, &vidModeMinorVersion);
 //   printf("XF86VidModeExtension-Version %d.%d\n", vidModeMajorVersion,       vidModeMinorVersion);
//    XF86VidModeGetAllModeLines(GLWin.dpy, GLWin.screen, &modeNum, &modes);
    /* save desktop-resolution before switching modes */
#if 0
    GLWin.deskMode = *modes[0];
    /* look for mode with requested resolution */
    for (i = 0; i < modeNum; i++)
    {
        if ((modes[i]->hdisplay == width) && (modes[i]->vdisplay == height))
        {
            bestMode = i;
        }
    }
#endif
    /* get an appropriate visual */
    vi = glXChooseVisual(hui_x_display, screen, attrListDbl);
    if (vi == NULL)
    {
        vi = glXChooseVisual(hui_x_display, screen, attrListSgl);
        printf("Only Singlebuffered Visual!\n");
    }
    else
    {
        printf("Got Doublebuffered Visual!\n");
    }
   // glXQueryVersion(GLWin.dpy, &glxMajorVersion, &glxMinorVersion);
   // printf("glX-Version %d.%d\n", glxMajorVersion, glxMinorVersion);
    /* create a GLX context */
    context = glXCreateContext(hui_x_display, vi, 0, GL_TRUE);
    /* create a color map */
		


		printf("screen %d\n", DefaultScreen(hui_x_display));
    XSetWindowAttributes attr;
		Window root = RootWindow(hui_x_display, 0);
    attr.colormap = XCreateColormap(hui_x_display, root, vi->visual, AllocNone);
    attr.border_pixel = 0;

int event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
            StructureNotifyMask;
        win = XCreateWindow(hui_x_display, RootWindow(hui_x_display, vi->screen),
            0, 0, 800, 600, 0, vi->depth, InputOutput, vi->visual,
            CWBorderPixel | CWColormap | CWEventMask, &attr);
        /* only set window title and handle wm_delete_events if in windowed mode */
        Atom wmDelete = XInternAtom(hui_x_display, "WM_DELETE_WINDOW", True);
        XSetWMProtocols(hui_x_display, win, &wmDelete, 1);
        XSetStandardProperties(hui_x_display, win, "test",
            "test", None, NULL, 0, NULL);
        XMapRaised(hui_x_display, win);


		
			if ((long)win < 1000)
				msg_error("no GLX window found");
			//XSelectInput(hui_x_display,win, ExposureMask | KeyPress | KeyReleaseMask | StructureNotifyMask);

			// set video mode
			if (NixFullscreen){
				/*XF86VidModeSwitchToMode(hui_x_display, screen, modes[best_mode]);
				XFlush(hui_x_display);
				int x, y;
				XF86VidModeGetViewPort(hui_x_display, screen, &x, &y);
				printf("view port:    %d %d\n", x, y);
				XF86VidModeSetViewPort(hui_x_display, screen, 0, 0);
				XFlush(hui_x_display);
				XF86VidModeGetViewPort(hui_x_display, screen, &x, &y);
				printf("view port:    %d %d\n", x, y);
				//XF86VidModeSetViewPort(hui_x_display, screen, 0, NixDesktopHeight - yres);
				printf("Resolution %d x %d\n", modes[best_mode]->hdisplay, modes[best_mode]->vdisplay);
				XFree(modes);*/
				int r = system(format("xrandr --size %dx%d", xres, yres));
				XWarpPointer(hui_x_display, None, win, 0, 0, 0, 0, xres / 2, yres / 2);
			}
		msg_write("e");
			glXMakeCurrent(hui_x_display, win, context);
#endif
		if (glXIsDirect(hui_x_display, context)){
			msg_db_m("-direct rendering",1);
		}else
			msg_error("-no direct rendering!");
	#endif // OS_LINUX

	msg_db_m("-setting properties", 1);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST); // "Really Nice Perspective Calculations" (...)
	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
	#ifdef OS_LINUX
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	#endif
		TestGLError("prop");

	
	// font
	msg_db_m("-font", 1);
//	NixOGLFontDPList=glGenLists(256);
	#ifdef OS_WINDOWS
		HFONT hFont=CreateFont(NixFontHeight,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_TT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,FF_DONTCARE|DEFAULT_PITCH,hui_tchar_str(NixFontName));
		SelectObject(hDC,hFont);
		wglUseFontBitmaps(hDC,0,255,NixOGLFontDPList);
	#endif
	#ifdef OS_LINUX
    /*fontInfo = XLoadQueryFont(GLWin.dpy, FontName);
	if (fontInfo){
		printf("------------Font--------------\n");
	}else{
		printf("------------kein Font--------------\n");
	}
    font = fontInfo->fid;*/ 
			//font=XLoadFont(display,"-adobe-new century schoolbook-medium-r-normal--14-140-75-75-p-82-iso8859-1");
		x_font=XLoadQueryFont(hui_x_display,"*century*medium-r-normal*--14*");
		if (!x_font)
			x_font = XLoadQueryFont(hui_x_display,"*medium-r-normal*--14*");
		if (!x_font)
			x_font = XLoadQueryFont(hui_x_display,"*--14*");
		if (x_font){
			//msg_write(NixOGLFontDPList);
			NixOGLFontDPList = 1000;
			glXUseXFont(x_font->fid,0,256,NixOGLFontDPList);
		}else
			msg_error("could not load any font");
		/*	int num;   
		char **fl=XListFonts(GLWin.dpy, "*", 10240, &num);
			msg_write(num);
		for (int i=0;i<num;i++)
			msg_write(fl[i]);
		XFreeFontNames(fl);
			printf("----\n");*/
		TestGLError("font");


#endif
		
		char *ext = (char*)glGetString(GL_EXTENSIONS);
		//msg_write(p2s(ext));
		
#ifdef OS_WINDOWS

		// multitexturing
		glActiveTexture = (PFNGLACTIVETEXTUREPROC)wglGetProcAddress("glActiveTexture");
		glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)wglGetProcAddress("glClientActiveTexture");
		OGLMultiTexturingSupport = (glActiveTexture && glClientActiveTexture);
		if (!OGLMultiTexturingSupport)
			msg_error("no multitexturing support");

		// shader
		glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
		glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
		glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
		glAttachShader = (PFNGLATTACHSHADERPROC)wglGetProcAddress("glAttachShader");
		glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
		glCreateProgram = (PFNGLCREATEPROGRAMPROC)wglGetProcAddress("glCreateProgram");
		glLinkProgram = (PFNGLLINKPROGRAMPROC)wglGetProcAddress("glLinkProgram");
		glUseProgram = (PFNGLUSEPROGRAMPROC)wglGetProcAddress("glUseProgram");
		glGetProgramiv = (PFNGLGETPROGRAMIVPROC)wglGetProcAddress("glGetProgramiv");
		glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)wglGetProcAddress("glGetProgramInfoLog");
		glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
		glDeleteProgram = (PFNGLDELETEPROGRAMPROC)wglGetProcAddress("glDeleteProgram");
		glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)wglGetProcAddress("glGetUniformLocation");
		glUniform1f = (PFNGLUNIFORM1FPROC)wglGetProcAddress("glUniform1f");
		glUniform1i = (PFNGLUNIFORM1IPROC)wglGetProcAddress("glUniform1i");
		glUniform3f = (PFNGLUNIFORM3FPROC)wglGetProcAddress("glUniform3f");
		OGLShaderSupport = (glCreateShader && glShaderSource && glCompileShader && glAttachShader
			&& glGetShaderiv && glGetShaderInfoLog && glCreateProgram && glLinkProgram
			&& glUseProgram && glGetProgramiv && glGetProgramInfoLog && glDeleteShader && glDeleteProgram
			&& glGetUniformLocation && glUniform1f && glUniform1i && glUniform3f);
		if (!OGLShaderSupport)
			msg_error("no shader support");

		
		glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress("glGenBuffers");
		glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress("glBindBuffer");
		glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress("glBufferData");
		OGLVertexBufferSupport = (glGenBuffers && glBindBuffer && glBufferData);
		if (!OGLVertexBufferSupport)
			msg_error("no vertex buffer support");
#else
		OGLMultiTexturingSupport = true;
		OGLShaderSupport = true;
		OGLVertexBufferSupport = true;
#endif

		msg_db_m("-RenderToTexture-Support", 1);

		if (ext){
			if (strstr(ext,"EXT_framebuffer_object")==NULL){
				msg_error("EXT_framebuffer_object extension was not found");
			}else{
				OGLDynamicTextureSupport = true;
#ifdef OS_WINDOWS
				glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)wglGetProcAddress("glIsRenderbufferEXT");
				glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
				glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
				glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
				glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
				glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)wglGetProcAddress("glGetRenderbufferParameterivEXT");
				glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)wglGetProcAddress("glIsFramebufferEXT");
				glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
				glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
				glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
				glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
				glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)wglGetProcAddress("glFramebufferTexture1DEXT");
				glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
				glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)wglGetProcAddress("glFramebufferTexture3DEXT");
				glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
				glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)wglGetProcAddress("glGetFramebufferAttachmentParameterivEXT");
				glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)wglGetProcAddress("glGenerateMipmapEXT");

				if (!glIsRenderbufferEXT || !glBindRenderbufferEXT || !glDeleteRenderbuffersEXT ||
					!glGenRenderbuffersEXT || !glRenderbufferStorageEXT || !glGetRenderbufferParameterivEXT ||
					!glIsFramebufferEXT || !glBindFramebufferEXT || !glDeleteFramebuffersEXT ||
					!glGenFramebuffersEXT || !glCheckFramebufferStatusEXT || !glFramebufferTexture1DEXT ||
					!glFramebufferTexture2DEXT || !glFramebufferTexture3DEXT || !glFramebufferRenderbufferEXT||
					!glGetFramebufferAttachmentParameterivEXT || !glGenerateMipmapEXT ){
						msg_error("One or more EXT_framebuffer_object functions were not found");
				}
#endif
			}
		}
		TestGLError("end");

}

void NixSetVideoMode(const string &api, int xres, int yres, bool fullscreen)
{
	msg_db_f("setting video mode", 1);

	NixApiName = api;
	if (NixApiName == "")
		NixApiName = "NoApi";
	if (NixApiName == "NoApi")
		fullscreen=false;
	if (fullscreen){
		msg_db_m(format("[ %s - fullscreen - %d x %d ]", NixApiName.c_str(), xres, yres).c_str(), 0);
	}else{
		msg_db_m(format("[ %s - window mode ]", NixApiName.c_str()).c_str(), 0);
		xres=NixDesktopWidth;
		yres=NixDesktopHeight;
	}

	if (NixApiName == "NoApi")
		return;

	

	NixFullscreen = fullscreen;
	if (api == "OpenGL")
		NixApi = NIX_API_OPENGL;
	else{
		msg_error("unknown graphics api: " + NixApiName);
		NixFatalError = FatalErrorUnknownApi;
		return;
	}
	NixUsable = false;
	NixFatalError = FatalErrorNone;
	NixDoingEvilThingsToTheDevice = true;
	NixKillDeviceObjects();


	set_video_mode_gl(xres, yres);

	

	/*			char *ext = (char*)glGetString( GL_EXTENSIONS );
				if (ext){
					msg_write(strlen(ext));
					for (int i=0;i<strlen(ext);i++)
						if (ext[i]==' ')
							ext[i]='\n';
					msg_write(ext);
				}else
					msg_error("keine Extensions?!?");*/


	NixDoingEvilThingsToTheDevice = false;

	CreateFontGlyphWidth();

	// adjust window for new mode
	NixWindow->setFullscreen(NixFullscreen);
	//NixWindow->SetPosition(0, 0);
/*#ifdef OS_WINDOWS
	msg_db_m("-window",1);
	if (NixFullscreen){
		DWORD style=WS_POPUP|WS_SYSMENU|WS_VISIBLE;
		//SetWindowLong(hWnd,GWL_STYLE,WS_POPUP);
		SetWindowLong(NixWindow->hWnd,GWL_STYLE,style);

		WINDOWPLACEMENT wpl;
		GetWindowPlacement(NixWindow->hWnd,&wpl);
		wpl.rcNormalPosition.left=0;
		wpl.rcNormalPosition.top=0;
		wpl.rcNormalPosition.right=xres;
		wpl.rcNormalPosition.bottom=yres;
		AdjustWindowRect(&wpl.rcNormalPosition, style, FALSE);
		SetWindowPlacement(NixWindow->hWnd,&wpl);
	}else{
		//SetWindowLong(hWnd,GWL_STYLE,WindowStyle);
	}
#endif*/

	if (NixFullscreen){
		NixScreenWidth=xres;
		NixScreenHeight=yres;
		NixScreenDepth=32;
	}else{
		NixScreenWidth			=NixDesktopWidth;
		NixScreenHeight			=NixDesktopHeight;
		NixScreenDepth			=NixDesktopDepth;
/*#ifdef OS_WINDOWS
		msg_db_m("SetWindowPos",1);
		SetWindowPos(	NixWindow->hWnd,HWND_NOTOPMOST,
						WindowBounds.left,
						WindowBounds.top,
						(WindowBounds.right-WindowBounds.left),
						(WindowBounds.bottom-WindowBounds.top),
						SWP_SHOWWINDOW );
#endif*/
	}
	TestGLError("xxx");
	NixStart();
	NixDrawStr(100, 100, "test");
	NixEnd();

// recreate vertex buffers and textures
	NixReincarnateDeviceObjects();

	NixUsable = true;
	NixResize();
}

void NixTellUsWhatsWrong()
{
	if (NixFatalError == FatalErrorNoDirectX9)
		HuiErrorBox(NixWindow, "DirectX 9 nicht gefunden!","Es tut mir au&serordentlich leid, aber ich hatte Probleme, auf Ihrem\nSystem DirectX 9 zu starten und mich damit zu verbinden!");
	if (NixFatalError == FatalErrorNoDevice)
		HuiErrorBox(NixWindow,"DirectX 9: weder Hardware- noch Softwaremodus!!","Es tut mir au&serordentlich leid, aber ich hatte Probleme, auf Ihrem\nSystem DirectX 9 weder einen Hardware- noch einen Softwaremodus abzuringen!\n...Unerlaubte Afl&osung?");
}

// shoot down windows
void NixKillWindows()
{
#ifdef OS_WINDOWS
	msg_db_r("Killing Windows...",0);
	HANDLE t;
	OpenProcessToken(	GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&t);
	_TOKEN_PRIVILEGES tp;
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tp.Privileges[0].Luid); 
	tp.PrivilegeCount=1;
	tp.Privileges[0].Attributes=SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(t,FALSE,&tp,0,NULL,0);
	InitiateSystemShutdown(NULL,(win_str)hui_tchar_str("Resistance is futile!"),10,TRUE,FALSE);
	//ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE,0);
	msg_db_l(0);
#endif
}



void NixSetWire(bool enabled)
{
	WireFrame = enabled;
	if (WireFrame){
		glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
		glDisable(GL_CULL_FACE);
	}else{
		glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
		glEnable(GL_CULL_FACE);
	}
	TestGLError("SetWire");
}

void NixSetCull(int mode)
{
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	if (mode==CullNone)		glDisable(GL_CULL_FACE);
	if (mode==CullCCW)		glCullFace(GL_FRONT);
	if (mode==CullCW)		glCullFace(GL_BACK);
	TestGLError("SetCull");
}

void NixSetZ(bool Write, bool Test)
{
	if (Test){
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
		if (Write)
			glDepthMask(1);
		else
			glDepthMask(0);
	}else{
		if (Write){
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_ALWAYS);
			glDepthMask(1);
		}else{
			glDisable(GL_DEPTH_TEST);
		}
	}
	TestGLError("Setz");
}

void NixSetOffset(float offset)
{
	if (offset > 0){
		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(1.0f, 1.0f);
	}else{
		glDisable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(0, 0);
	}
}

void NixSetAlpha(int mode)
{
	glDisable(GL_ALPHA_TEST);
	switch (mode){
		case AlphaNone:
			glDisable(GL_BLEND);
			break;
		case AlphaColorKey:
		case AlphaColorKeyHard:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_ALPHA_TEST);
			if (mode==AlphaColorKeyHard)
				glAlphaFunc(GL_GEQUAL,0.5f);
			else
				glAlphaFunc(GL_GEQUAL,0.04f);
			break;
		case AlphaMaterial:
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
			break;
	}
	TestGLError("SetAlpha");
}

void NixSetAlphaM(int mode)
{	NixSetAlpha(mode);	}

unsigned int OGLGetAlphaMode(int mode)
{
	if (mode==AlphaZero)			return GL_ZERO;
	if (mode==AlphaOne)				return GL_ONE;
	if (mode==AlphaSourceColor)		return GL_SRC_COLOR;
	if (mode==AlphaSourceInvColor)	return GL_ONE_MINUS_SRC_COLOR;
	if (mode==AlphaSourceAlpha)		return GL_SRC_ALPHA;
	if (mode==AlphaSourceInvAlpha)	return GL_ONE_MINUS_SRC_ALPHA;
	if (mode==AlphaDestColor)		return GL_DST_COLOR;
	if (mode==AlphaDestInvColor)	return GL_ONE_MINUS_DST_COLOR;
	if (mode==AlphaDestAlpha)		return GL_DST_ALPHA;
	if (mode==AlphaDestInvAlpha)	return GL_ONE_MINUS_DST_ALPHA;
	// GL_SRC_ALPHA_SATURATE
	return GL_ZERO;
}

void NixSetAlpha(int src,int dst)
{
	glEnable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glBlendFunc(OGLGetAlphaMode(src),OGLGetAlphaMode(dst));
	TestGLError("SetAlphaII");
}

void NixSetAlphaSD(int src,int dst)
{	NixSetAlpha(src,dst);	}

void NixSetAlpha(float factor)
{
	glDisable(GL_ALPHA_TEST);
	float di[4];
	glGetMaterialfv(GL_FRONT,GL_DIFFUSE,di);
	di[3]=factor;
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,di);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	TestGLError("SetAlphaF");
}

void NixSetStencil(int mode,unsigned long param)
{
	glStencilMask(0xffffffff);
	
	if (mode==StencilNone){
		glDisable(GL_STENCIL_TEST);
	}else if (mode==StencilReset){
		glClearStencil(param);
		glClear(GL_STENCIL_BUFFER_BIT);
	}else if ((mode==StencilIncrease)||(mode==StencilDecrease)||(mode==StencilDecreaseNotNegative)||(mode==StencilSet)){
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS,param,0xffffffff);
		if (mode==StencilIncrease)
			glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
		else if ((mode==StencilDecrease)||(mode==StencilDecreaseNotNegative))
			glStencilOp(GL_KEEP,GL_KEEP,GL_DECR);
		else if (mode==StencilSet)
			glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE);
	}else if ((mode==StencilMaskEqual)||(mode==StencilMaskNotEqual)||(mode==StencilMaskLessEqual)||(mode==StencilMaskLess)||(mode==StencilMaskGreaterEqual)||(mode==StencilMaskGreater)){
		glEnable(GL_STENCIL_TEST);
		glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
		if (mode==StencilMaskEqual)
			glStencilFunc(GL_EQUAL,param,0xffffffff);
		else if (mode==StencilMaskNotEqual)
			glStencilFunc(GL_NOTEQUAL,param,0xffffffff);
		else if (mode==StencilMaskLessEqual)
			glStencilFunc(GL_LEQUAL,param,0xffffffff);
		else if (mode==StencilMaskLess)
			glStencilFunc(GL_LESS,param,0xffffffff);
		else if (mode==StencilMaskGreaterEqual)
			glStencilFunc(GL_GEQUAL,param,0xffffffff);
		else if (mode==StencilMaskGreater)
			glStencilFunc(GL_GREATER,param,0xffffffff);
	}
	TestGLError("SetStencil");
}

// mode=FogLinear:			start/end
// mode=FogExp/FogExp2:		density
void NixSetFog(int mode,float start,float end,float density,const color &c)
{
	if (mode==FogLinear)		glFogi(GL_FOG_MODE,GL_LINEAR);
	else if (mode==FogExp)		glFogi(GL_FOG_MODE,GL_EXP);
	else if (mode==FogExp2)		glFogi(GL_FOG_MODE,GL_EXP2);
	glFogfv(GL_FOG_COLOR,(float*)&c);
	glFogf(GL_FOG_DENSITY,density);
	glFogf(GL_FOG_START,start);
	glFogf(GL_FOG_END,end);
	glHint(GL_FOG_HINT,GL_DONT_CARE); // ??
	TestGLError("SetFog");
}

void NixEnableFog(bool Enabled)
{
	if (Enabled)
		glEnable(GL_FOG);
	else
		glDisable(GL_FOG);
	TestGLError("SetEnableFog");
}
