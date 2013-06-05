/*----------------------------------------------------------------------------*\
| Nix draw                                                                     |
| -> drawing functions                                                         |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"

float NixLineWidth = 1;
bool NixSmoothLines = false;
static color NixColor = White;



void NixSetColor(const color &c)
{
	NixColor = c;
	glColor4fv((float*)&c);
	TestGLError("SetColor");
}

color NixGetColor()
{
	return NixColor;
}

void NixDrawChar(float x, float y, char c)
{
	char str[2];
	str[0]=c;
	str[1]=0;
	NixDrawStr(x,y,str);
}

string str_utf8_to_ubyte(const string &str)
{
	string r;
	for (int i=0;i<str.num;i++)
		if (((unsigned int)str[i] & 0x80) > 0){
			r.add(((str[i] & 0x1f) << 6) + (str[i + 1] & 0x3f));
			i ++;
		}else
			r.add(str[i]);
	return r;
}

void NixDrawStr(float x, float y, const string &str)
{
	msg_db_r("NixDrawStr",10);
	string str2 = str_utf8_to_ubyte(str);
	_NixSetMode2d();

	glRasterPos3f(x, (y+2+int(float(NixFontHeight)*0.75f)),-1.0f);
	glListBase(NixOGLFontDPList);
	glCallLists(str2.num,GL_UNSIGNED_BYTE,(char*)str2.data);
	glRasterPos3f(0,0,0);
	TestGLError("DrawStr");
	msg_db_l(10);
}

int NixGetStrWidth(const string &str)
{
	string str2 = str_utf8_to_ubyte(str);
	int w = 0;
	for (int i=0;i<str2.num;i++)
		w += NixFontGlyphWidth[(unsigned char)str2[i]];
	return w;
}

void NixDrawLine(float x1, float y1, float x2, float y2, float depth)
{
	float dx=x2-x1;
	if (dx<0)	dx=-dx;
	float dy=y2-y1;
	if (dy<0)	dy=-dy;
	_NixSetMode2d();

#ifdef OS_LINUX
	// internal line drawing function \(^_^)/
	if (NixSmoothLines){
		// antialiasing!
		glLineWidth(NixLineWidth + 0.5);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}else
		glLineWidth(NixLineWidth);
	glBegin(GL_LINES);
		glVertex3f(x1, y1, depth);
		glVertex3f(x2, y2, depth);
	glEnd();
	if (NixSmoothLines){
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
	}
#else

	// own line drawing function (T_T)
	if (dx>dy){
		if (x1>x2){
			float x=x2;	x2=x1;	x1=x;
			float y=y2;	y2=y1;	y1=y;
		}
		glBegin(GL_TRIANGLES);
			glVertex3f(x1,y1+1,depth);
			glVertex3f(x1,y1  ,depth);
			glVertex3f(x2,y2+1,depth);
			glVertex3f(x2,y2  ,depth);
			glVertex3f(x2,y2+1,depth);
			glVertex3f(x1,y1  ,depth);
		glEnd();
	}else{
		if (y1<y2){
			float x=x2;	x2=x1;	x1=x;
			float y=y2;	y2=y1;	y1=y;
		}
		glBegin(GL_TRIANGLES);
			glVertex3f(x1+1,y1,depth);
			glVertex3f(x1  ,y1,depth);
			glVertex3f(x2+1,y2,depth);
			glVertex3f(x2  ,y2,depth);
			glVertex3f(x2+1,y2,depth);
			glVertex3f(x1  ,y1,depth);
		glEnd();
	}
#endif
	TestGLError("DrawLine");
}

void NixDrawLines(float *x, float *y, int num_lines, bool contiguous, float depth)
{
	_NixSetMode2d();
	// internal line drawing function \(^_^)/
	if (NixSmoothLines){
		// antialiasing!
		glLineWidth(NixLineWidth + 0.5);
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}else
		glLineWidth(NixLineWidth);

		if (contiguous){
			glBegin(GL_LINE_STRIP);
				glVertex3f(*x, *y, depth);	x++;	y++;
				for (int i=0;i<num_lines;i++){
					glVertex3f(*x, *y, depth);	x++;	y++;
				}
			glEnd();
		}else{
			glBegin(GL_LINES);
				for (int i=0;i<num_lines;i++){
					glVertex3f(*x, *y, depth);	x++;	y++;
					glVertex3f(*x, *y, depth);	x++;	y++;
				}
			glEnd();
		}
	if (NixSmoothLines){
		glDisable(GL_LINE_SMOOTH);
		glDisable(GL_BLEND);
	}
	TestGLError("DrawLines");
}

void NixDrawLineV(float x, float y1, float y2, float depth)
{
	NixDrawLine(x, y1, x, y2, depth);
	/*if (y1>y2){
		float y=y2;	y2=y1;	y1=y;
	}
	NixDraw2D(r_id, rect(x, x + 1, y1, y2), depth);*/
}

void NixDrawLineH(float x1, float x2, float y, float depth)
{
	NixDrawLine(x1, y, x2, y, depth);
	/*if (x1>x2){
		float x=x2;
		x2=x1;
		x1=x;
	}
	NixDraw2D(r_id, rect(x1, x2, y, y + 1), depth);*/
}

void NixDrawLine3D(const vector &l1, const vector &l2)
{
	_NixSetMode3d();
	glLineWidth(1);
	glBegin(GL_LINES);
		glVertex3fv((float*)&l1);
		glVertex3fv((float*)&l2);
	glEnd();
	/*vector p1, p2;
	NixGetVecProject(p1,l1);
	NixGetVecProject(p2,l2);
	if ((p1.z>0)&&(p2.z>0)&&(p1.z<1)&&(p2.z<1))
		NixDrawLine(p1.x, p1.y, p2.x, p2.y, (p1.z + p2.z)/2);*/
	//NixDrawLine(l1.x, l1.y, l2.x, l2.y, (l1.z+l2.z)/2);
	TestGLError("DrawLine3d");
}

void NixDrawRect(float x1, float x2, float y1, float y2, float depth)
{
	float t;
	if (x1>x2){
		t=x1;	x1=x2;	x2=t;
	}
	if (y1>y2){
		t=y1;	y1=y2;	y2=t;
	}
	if (!NixFullscreen){
		int pa=40;
		for (int i=0;i<int(x2-x1-1)/pa+1;i++){
			for (int j=0;j<int(y2-y1-1)/pa+1;j++){
				float _x1=x1+i*pa;
				float _y1=y1+j*pa;

				float _x2=x2;
				if (x2-x1-i*pa>pa)	_x2=x1+i*pa+pa;
				float _y2=y2;
				if (y2-y1-j*pa>pa)	_y2=y1+j*pa+pa;

				NixDraw2D(r_id, rect(_x1, _x2, _y1, _y2), depth);
			}
		}
		return;
	}
	NixDraw2D(r_id, rect(x1, x2, y1, y2), depth);
}

void NixDraw2D(const rect &src, const rect &dest, float depth)
{
	_NixSetMode2d();
	//if (depth==0)	depth=0.5f;
	
	//msg_write("2D");
//	SetShaderFileData(texture,-1,-1,-1);
	depth=depth*2-1;
	glBegin(GL_QUADS);
		glTexCoord2f(src.x1,1-src.y2);
		glVertex3f(dest.x1,dest.y2,depth);
		glTexCoord2f(src.x1,1-src.y1);
		glVertex3f(dest.x1,dest.y1,depth);
		glTexCoord2f(src.x2,1-src.y1);
		glVertex3f(dest.x2,dest.y1,depth);
		glTexCoord2f(src.x2,1-src.y2);
		glVertex3f(dest.x2,dest.y2,depth);
	glEnd();
	TestGLError("Draw2D");
}

void NixDraw3DCubeMapped(int cube_map,int buffer)
{
	if (buffer<0)	return;
	if (cube_map<0)	return;
	_NixSetMode3d();

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		NixSetCubeMapDX(cube_map);
		lpDevice->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR);
		matrix tm=NixViewMatrix;
		tm._03=tm._13=tm._23=tm._33=tm._30=tm._31=tm._32=0;
		/*quaternion q;
		QuaternionRotationM(q,tm);
		vector ang=QuaternionToAngle(q);
		MatrixRotationView(tm,ang);*/
		MatrixTranspose(tm,tm);
		lpDevice->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_COUNT3);

		lpDevice->SetTransform(D3DTS_TEXTURE0,(D3DMATRIX*)&tm);


		NixDraw3D(-2,buffer);
		lpDevice->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,D3DTSS_TCI_PASSTHRU);
		lpDevice->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		NixSetTexture(cube_map);
		TestGLError("Draw3dCube 0");
		glTexGenf(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		glTexGenf(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		glTexGenf(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP);
		TestGLError("Draw3dCube a");
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_R);
		TestGLError("Draw3dCube c");
		//glEnable(GL_TEXTURE_CUBE_MAP_ARB);
		NixDraw3D(buffer);
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_R);
		NixSetTexture(-1);

		//Draw3D(-1,buffer,mat);
	}
#endif
	TestGLError("Draw3dCube");
}

void NixDrawSpriteR(const rect &src, const vector &pos, const rect &dest)
{
#if 0
	rect d;
	float depth;
	vector p;
	NixGetVecProject(p,pos);
	if ((p.z<=0.0f)||(p.z>=1.0))
		return;
	depth=p.z;
	vector u = NixViewMatrix * pos;
	float q=NixMaxDepth/(NixMaxDepth-NixMinDepth);
	float f=1.0f/(u.z*q*NixMinDepth*NixView3DRatio);
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		//depth=depth*2.0f-1.0f;
		//f*=2;
	}
#endif
	//if (f>20)	f=20;
	d.x1=p.x+f*(dest.x1)*NixViewScale.x*NixTargetWidth;
	d.x2=p.x+f*(dest.x2)*NixViewScale.x*NixTargetWidth;
	d.y1=p.y+f*(dest.y1)*NixViewScale.y*NixTargetHeight*NixView3DRatio;
	d.y2=p.y+f*(dest.y2)*NixViewScale.y*NixTargetHeight*NixView3DRatio;
	NixDraw2D(src, d, depth);
#endif
}

void NixDrawSprite(const rect &src,const vector &pos,float radius)
{
	rect d;
	d.x1=-radius;
	d.x2=radius;
	d.y1=-radius;
	d.y2=radius;
	NixDrawSpriteR(src, pos, d);
}

void NixResetToColor(const color &c)
{
	glClearColor(c.r, c.g, c.b, c.a);
	glClear(GL_COLOR_BUFFER_BIT);
	TestGLError("ResetToColor");
}
