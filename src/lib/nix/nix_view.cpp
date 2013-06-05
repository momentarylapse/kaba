/*----------------------------------------------------------------------------*\
| Nix view                                                                     |
| -> camera etc...                                                             |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"
#ifdef _X_USE_IMAGE_
#include "../image/image.h"
#endif

void TestGLError(const string &);

void NixUpdateLights();


matrix NixViewMatrix, NixProjectionMatrix;
matrix NixProjectionMatrix2d;
matrix NixWorldMatrix, NixWorldViewProjectionMatrix;
vector _NixCamPos_;
bool NixMode3d = false;

float NixViewJitterX = 0, NixViewJitterY = 0;

static int OGLViewPort[4];

int RenderingToTexture = -1;

#ifdef OS_WINDOWS
	extern HDC hDC;
	extern HGLRC hRC;
#endif

void _NixSetMode2d()
{
	if (!NixMode3d)
		return;
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float*)&NixProjectionMatrix2d);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	NixMode3d = false;
	TestGLError("Set2d");
}

void _NixSetMode3d()
{
	if (NixMode3d)
		return;
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf((float*)&NixProjectionMatrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf((float*)&NixViewMatrix);
	glMultMatrixf((float*)&NixWorldMatrix);
	NixMode3d = true;
	TestGLError("Set3d");
}

void NixResize()
{
	if (!NixUsable)
		return;

	msg_db_r("NixResize",5);

	if (NixTargetWidth<=0)
		NixTargetWidth=1;
	if (NixTargetHeight<=0)
		NixTargetHeight=1;
	NixTargetRect = rect(0, NixTargetWidth, 0, NixTargetHeight);

	// screen
	glViewport(0,0,NixTargetWidth,NixTargetHeight);
	//glViewport(0,0,NixTargetWidth,NixTargetHeight);
	OGLViewPort[0]=0;
	OGLViewPort[1]=0;
	OGLViewPort[2]=NixTargetWidth;
	OGLViewPort[3]=NixTargetHeight;

	// camera
	NixSetProjectionMatrix(NixProjectionMatrix);
	NixSetView(NixViewMatrix);

	TestGLError("Resize");
	msg_db_l(5);
}

void NixSetWorldMatrix(const matrix &mat)
{
	NixWorldMatrix = mat;
	NixWorldViewProjectionMatrix = NixProjectionMatrix * NixViewMatrix * NixWorldMatrix;
	if (NixMode3d){
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((float*)&NixViewMatrix);
		glMultMatrixf((float*)&NixWorldMatrix);
	}
	TestGLError("SetWorldMatrix");
}

static vector ViewPos,ViewDir;
static vector Frustrum[8];
static plane FrustrumPl[6];

void NixSetView(const vector &view_pos,const vector &view_ang)
{
	ViewPos = view_pos;
	ViewDir = view_ang.ang2dir();

	matrix t, r;
	MatrixTranslation(t, -view_pos);
	MatrixRotationView(r, view_ang);
	NixViewMatrix = r * t;
	NixSetView(NixViewMatrix);

	// die Eckpunkte des Sichtfeldes
	/*NixGetVecUnproject(Frustrum[0],vector(                   0,                    0,0.0f));
	NixGetVecUnproject(Frustrum[1],vector(float(NixScreenWidth-1),                    0,0.0f));
	NixGetVecUnproject(Frustrum[2],vector(                   0,float(NixScreenHeight-1),0.0f));
	NixGetVecUnproject(Frustrum[3],vector(float(NixScreenWidth-1),float(NixScreenHeight-1),0.0f));
	NixGetVecUnproject(Frustrum[4],vector(                   0,                    0,0.9f));
	NixGetVecUnproject(Frustrum[5],vector(float(NixScreenWidth-1),                    0,0.9f));
	NixGetVecUnproject(Frustrum[6],vector(                   0,float(NixScreenHeight-1),0.9f));
	NixGetVecUnproject(Frustrum[7],vector(float(NixScreenWidth-1),float(NixScreenHeight-1),0.9f));

	// Ebenen des Sichtfeldes (gegen UZS nach innen!?)
	PlaneFromPoints(FrustrumPl[0],Frustrum[0],Frustrum[1],Frustrum[2]); // nahe Ebene
	//PlaneFromPoints(FrustrumPl[1],Frustrum[4],Frustrum[6],Frustrum[7]); // ferne Ebene
	//PlaneFromPoints(FrustrumPl[2],Frustrum[0],Frustrum[2],Frustrum[3]); // linke Ebene
	//PlaneFromPoints(FrustrumPl[3],Frustrum[1],Frustrum[5],Frustrum[7]); // rechte Ebene
	//PlaneFromPoints(FrustrumPl[4],Frustrum[0],Frustrum[4],Frustrum[5]); // untere Ebene
	//PlaneFromPoints(FrustrumPl[5],Frustrum[2],Frustrum[3],Frustrum[7]); // obere Ebene*/
	TestGLError("SetView");
}

void create_pixel_projection_matrix(matrix &m)
{
	matrix s, t;
	MatrixTranslation(t, vector(-float(NixTargetWidth)/2.0f,-float(NixTargetHeight)/2.0f,0));
	MatrixScale(s, 2.0f / float(NixTargetWidth), -2.0f / float(NixTargetHeight), 1);
	m = s * t;
}

// 3D-Matrizen erstellen (Einstellungen ueber SetPerspectiveMode vor NixStart() zu treffen)
// enable3d: true  -> 3D-Ansicht auf (View3DWidth,View3DHeight) gemapt
//           false -> Pixel-Angaben~~~
// beide Bilder sind um View3DCenterX,View3DCenterY (3D als Fluchtpunkt) verschoben

void NixSetProjectionPerspective()
{
	NixSetProjectionPerspectiveExt(NixTargetWidth / 2, NixTargetHeight / 2, NixTargetHeight, NixTargetHeight, 1, 10000);
}

// center_x/y: pixel coordinates of perspective center
// height_1/width_1: pixel sizes of 45° frustrum
void NixSetProjectionPerspectiveExt(float center_x, float center_y, float width_1, float height_1, float z_min, float z_max)
{
	matrix trans, persp, scale;
	// perspective projection
	MatrixTranslation(trans,
		vector((center_x + NixViewJitterX) / float(NixTargetWidth) * 2.0f - 1,
			1 - (center_y + NixViewJitterY) / float(NixTargetHeight) * 2.0f,
			0));
	MatrixPerspective(persp, pi / 2, 1, z_min, z_max);
	MatrixScale(scale, 2 * width_1 / NixTargetWidth,
			2 * height_1 / NixTargetHeight,
			- 1); // z reflection: right/left handedness

	NixSetProjectionMatrix(trans * persp * scale);
}

// center_x/y: pixel coordinates of (0,0,0)
// map_width/height: pixel sizes of projected base vectors
void NixSetProjectionOrthoExt(float center_x, float center_y, float map_width, float map_height, float z_min, float z_max)
{
	matrix scale, trans;
	MatrixScale(scale, 2.0f / float(NixTargetWidth) * map_width, -2.0f / float(NixTargetHeight) * map_height, 2 / (z_max - z_min));
	MatrixTranslation(trans, vector(2 * center_x / NixTargetWidth - 1, 1 - 2 * center_y / NixTargetHeight, -(z_max + z_min) / (z_max - z_min)));
	NixSetProjectionMatrix(trans * scale);
}

void NixSetProjectionOrtho(bool relative)
{
	matrix m;
	if (relative){
		// orthogonal projection (relative [0,1]x[0x1] coordinates)
		matrix s, t;
		MatrixTranslation(t, vector(-0.5f, -0.5f, 0));
		MatrixScale(s, 2.0f, -2.0f, 1);
		m = s * t;
	}else{
		// orthogonal projection (pixel coordinates)
		//NixSetProjectionOrthoExt(0, 0, 1, 1, )
		create_pixel_projection_matrix(m);
	}

	NixSetProjectionMatrix(m);
}

void NixSetProjectionMatrix(const matrix &mat)
{
	// projection 2d
	create_pixel_projection_matrix(NixProjectionMatrix2d);

	// projection 3d
	NixProjectionMatrix = mat;


	glMatrixMode(GL_PROJECTION);
	if (NixMode3d)
		glLoadMatrixf((float*)&NixProjectionMatrix);
	else
		glLoadMatrixf((float*)&NixProjectionMatrix2d);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	NixViewMatrix = m_id;
	NixWorldMatrix = m_id;

	TestGLError("SetProMat");
}

void NixSetView(const matrix &view_mat)
{
	//SetCull(CullCCW); // ???
	NixViewMatrix = view_mat;

	NixSetWorldMatrix(m_id);

	NixUpdateLights();
}

void NixSetViewV(const vector &view_pos, const vector &view_ang)
{	NixSetView(view_pos, view_ang);	}

void NixSetViewM(const matrix &view_mat)
{	NixSetView(view_mat);	}



#define FrustrumAngleCos	0.83f

bool NixIsInFrustrum(const vector &pos,float radius)
{
	// die absoluten Eckpunkte der BoundingBox
	vector p[8];
	p[0]=pos+vector(-radius,-radius,-radius);
	p[1]=pos+vector( radius,-radius,-radius);
	p[2]=pos+vector(-radius, radius,-radius);
	p[3]=pos+vector( radius, radius,-radius);
	p[4]=pos+vector(-radius,-radius, radius);
	p[5]=pos+vector( radius,-radius, radius);
	p[6]=pos+vector(-radius, radius, radius);
	p[7]=pos+vector( radius, radius, radius);

	bool in=false;
	for (int i=0;i<8;i++)
		//for (int j=0;j<6;j++)
			if (FrustrumPl[0].distance(p[i])<0)
				in=true;
	/*vector d;
	VecNormalize(d,pos-ViewPos); // zu einer Berechnung zusammenfassen!!!!!!
	float fdp=VecLengthFuzzy(pos-ViewPos);
	if (fdp<radius)
		return true;
	if (VecDotProduct(d,ViewDir)>FrustrumAngleCos-radius/fdp*0.04f)
		return true;
	return false;*/
	return in;
}

bool Rendering=false;

bool NixStart(int texture)
{
	if (NixDoingEvilThingsToTheDevice)
		return false;

	msg_db_r("NixStart", 2);
	TestGLError("Start prae");

	NixNumTrias=0;
	RenderingToTexture=texture;
	//msg_write(string("Start ",i2s(texture)));
	if (texture<0){
		#ifdef OS_WINDOWS
			if (OGLDynamicTextureSupport)
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			if (!wglMakeCurrent(hDC,hRC)){
				msg_error("wglMakeCurrent");
				msg_write(GetLastError());
				msg_db_l(2);
				return false;
			}
		#endif

		#ifdef OS_LINUX
			//glXSwapBuffers(hui_x_display, GDK_WINDOW_XWINDOW(NixWindow->gl_widget->window));
		#endif			
	}else{
		if (OGLDynamicTextureSupport){

			glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, NixTextures[texture].glFrameBuffer );
			//glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, NixTextures[texture].glDepthRenderBuffer );
			glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, NixTextures[texture].glTexture, 0 );
			glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, NixTextures[texture].glDepthRenderBuffer );
			GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
			if (status == GL_FRAMEBUFFER_COMPLETE_EXT){
				//msg_write("hurra");
			}else{
				msg_write("we're screwed! (NixStart with dynamic texture target)");
				msg_db_l(2);
				return false;
			}
		}
	}
	TestGLError("Start 1");
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glDisable(GL_SCISSOR_TEST);
	//glClearStencil(0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	//glClear(GL_COLOR_BUFFER_BIT);
	TestGLError("Start 2");

	// adjust target size
	if (texture < 0){
		if (NixFullscreen){
			// fullscreen mode
			NixTargetWidth = NixScreenWidth;
			NixTargetHeight = NixScreenHeight;
		}else{
			// window mode
			irect r = NixWindow->GetInterior();
			NixTargetWidth = r.x2 - r.x1;
			NixTargetHeight = r.y2 - r.y1;
		}
	}else{
		// texture
		NixTargetWidth = NixTextures[texture].width;
		NixTargetHeight = NixTextures[texture].height;
	}
	NixResize();
	Rendering = true;

	/*if (texture < 0)
		NixUpdateInput();*/

	//msg_write("-ok?");
	TestGLError("Start post");
	msg_db_l(2);
	return true;
}

void NixScissor(const rect &_r)
{
	bool enable_scissors = true;
	rect r = _r;
	if (r.x1 < 0){
		enable_scissors=false;
		r = NixTargetRect;
	}
	if (enable_scissors)
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);
	glScissor(r.x1, NixTargetHeight - r.y2, r.x2 - r.x1, r.y2 - r.y1);
	glClearDepth(1.0f);
	TestGLError("StartPart");
}

void NixEnd()
{
	if (!Rendering)
		return;
	msg_db_r("NixEnd", 2);
	TestGLError("End prae");
	Rendering=false;
	NixSetTexture(-1);
	glDisable(GL_SCISSOR_TEST);
	if (RenderingToTexture<0){
		// auf den Bildschirm
		#ifdef OS_WINDOWS
			if (RenderingToTexture<0)
				SwapBuffers(hDC);
		#endif
		#ifdef OS_LINUX
			#ifdef NIX_ALLOW_FULLSCREEN
				if (NixFullscreen)
					XF86VidModeSetViewPort(hui_x_display,screen,0,NixDesktopHeight-NixScreenHeight);
			#endif
			//glutSwapBuffers();
			if (NixGLDoubleBuffered)
				glXSwapBuffers(hui_x_display,GDK_WINDOW_XID(gtk_widget_get_window(NixWindow->gl_widget)));
		#endif
	}
	if (OGLDynamicTextureSupport)
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

	NixProgressTextureLifes();
	TestGLError("End post");
	msg_db_l(2);
}

void NixSetClipPlane(int index,const plane &pl)
{
	GLdouble d[4];
	d[0]=pl.n.x;
	d[1]=pl.n.y;
	d[2]=pl.n.z;
	d[3]=pl.d;
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf((float*)&NixViewMatrix);
	glClipPlane(GL_CLIP_PLANE0+index,d);
	glPopMatrix();
	//msg_todo("SetClipPlane fuer OpenGL");
	TestGLError("SetClip");
}

void NixEnableClipPlane(int index,bool enabled)
{
	if (enabled)
		glEnable(GL_CLIP_PLANE0+index);
	else
		glDisable(GL_CLIP_PLANE0+index);
	TestGLError("EnableClip");
}

void NixScreenShot(const string &filename, int width, int height)
{
	Image image;
	int dx = NixTargetWidth;
	int dy = NixTargetHeight;
	image.data.resize(dx * dy);
	glReadBuffer(GL_FRONT);
	glReadPixels(	0,
					0,
					dx,
					dy,
					GL_RGBA, GL_UNSIGNED_BYTE, &image.data[0]);
	if ((width >= 0) && (height >= 0)){
		Array<unsigned int> data2;
		image.width = width;
		image.height = height;
		data2.resize(width * height);
		// flip image...
		for (int x=0;x<width;x++)
			for (int y=0;y<height;y++){
				int x1 = (x * dx) / width;
				int y1 = dy - (y * dy) / height - 1;
				int n1 = (x1 + dx * y1);
				int n2 = (x + width * y );
				data2[n2] = image.data[n1];
			}
		image.data.exchange((DynamicArray&)data2);
		data2.clear();
	}else{
		image.width = dx;
		image.height = dy;
		// flip image...
		for (int x=0;x<dx;x++)
			for (int y=0;y<(dy+1)/2;y++){
				int y2 = dy - y - 1;
				int n1 = (x + dx * y );
				int n2 = (x + dx * y2);
				int c = image.data[n1];
				image.data[n1] = image.data[n2];
				image.data[n2] = c;
			}
	}
	// set alpha to 1
	for (int i=0;i<image.data.num;i++)
		image.data[i] |= 0xff000000;
	// save
	image.Save(filename);
	msg_write("screenshot saved: " + filename.sys_filename());
}

void NixScreenShotToImage(Image &image)
{
	image.Create(NixTargetWidth, NixTargetHeight, Black);
	glReadBuffer(GL_FRONT);
	glReadPixels(	0,
					0,
					NixTargetWidth,
					NixTargetHeight,
					GL_RGBA, GL_UNSIGNED_BYTE, &image.data[0]);
}



// world -> screen (0...NixTargetWidth,0...NixTargetHeight,0...1)
void NixGetVecProject(vector &vout,const vector &vin)
{
	/*matrix m;
	MatrixIdentity(m);*/
	double vm[16];
	int i;
	for (i=0;i<16;i++)
		vm[i]=NixViewMatrix.e[i];
		//vm[i]=m.e[i];
	double pm[16];
	for (i=0;i<16;i++)
			pm[i]=NixProjectionMatrix.e[i];
		//pm[i]=m.e[i];
	double x,y,z;
	gluProject(vin.x,vin.y,vin.z,vm,pm,OGLViewPort,&x,&y,&z);
	vout.x=(float)x;
	vout.y=float((OGLViewPort[1]*2+OGLViewPort[3])-y); // y-Spiegelung
	vout.z=(float)z;//0.999999970197677613f;//(float)z;
	/*VecTransform(vout,NixViewMatrix,vin);
	VecTransform(vout,NixProjectionMatrix,vout);
	vout.y=((ViewPort[1]*2+ViewPort[3])-vout.y*16)/2;
	vout.x=((ViewPort[0]*2+ViewPort[2])+vout.x*16)/2;
	vout.z=0.99999997f;*/
	TestGLError("VecPro");
}

// world -> screen (0...1,0...1,0...1)
void NixGetVecProjectRel(vector &vout,const vector &vin)
{
	NixGetVecProject(vout,vin);
	vout.x/=(float)NixTargetWidth;
	vout.y/=(float)NixTargetHeight;
}

// screen (0...NixTargetWidth,0...NixTargetHeight,0...1) -> world
void NixGetVecUnproject(vector &vout,const vector &vin)
{
	double vin_y=OGLViewPort[1]*2+OGLViewPort[3]-(double)vin.y; // y-Spiegelung
	double vm[16];
	int i;
	for (i=0;i<16;i++)
		vm[i]=NixViewMatrix.e[i];
	double pm[16];
	for (i=0;i<16;i++)
		pm[i]=NixProjectionMatrix.e[i];
	double x,y,z;
	gluUnProject(vin.x,vin_y,vin.z,vm,pm,OGLViewPort,&x,&y,&z);
	vout.x=(float)x;
	vout.y=(float)y;
	vout.z=(float)z;
	TestGLError("VecUnpro");
}

// screen (0...1,0...1,0...1) -> world
void NixGetVecUnprojectRel(vector &vout,const vector &vin)
{
	vector vi_r=vin;
	vi_r.x*=(float)NixTargetWidth;
	vi_r.y*=(float)NixTargetHeight;
	NixGetVecUnproject(vout,vi_r);
}
