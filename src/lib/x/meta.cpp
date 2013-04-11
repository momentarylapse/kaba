/*----------------------------------------------------------------------------*\
| Meta                                                                         |
| -> administration of animations, models, items, materials etc                |
| -> centralization of game data                                               |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
|  - paths get completed with the belonging root-directory of the file type    |
|    (model,item)                                                              |
|  - independent models                                                        |
|     -> equal loading commands create new instances                           |
|     -> equal loading commands copy existing models                           |
|         -> databases of original and copied models                           |
|         -> some data is referenced (skin...)                                 |
|         -> additionally created: effects, absolute physical data,...         |
|     -> each object has its own model                                         |
|  - independent items (managed by CMeta)                                      |
|     -> new items additionally saved as an "original item"                    |
|     -> an array of pointers points to each item                              |
|     -> each item has its unique ID (index in the array) for networking       |
|  - materials stay alive forever, just one instance                           |
|                                                                              |
| last updated: 2009.12.09 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#include "x.h"
#ifdef _X_ALLOW_SCRIPT_
	#include "../script/script.h"
#endif

string MetaVersion = "0.3.1.4";

float TimeScale=1.0f,TimeScaleLast,TimeScalePreBrake=1.0f,Elapsed,ElapsedRT;

int NumRealColTests;

bool FileErrorsAreCritical=false;

// game configuration
bool Debug,ShowTimings,ConsoleEnabled,WireMode;
bool Record;
int DetailLevel;
float DetailFactorInv;
int ShadowLevel,ShadowLight;
color ShadowColor;
bool ShadowLowerDetail;
float FpsMax,FpsMin;
int Multisampling = 0;
bool NetworkEnabled,CullingEnabled,SortingEnabled,ZBufferEnabled;
int XFontIndex, DefaultFont;
float XFontZ;
bool ResettingGame = false;



Array<void*> MetaDeleteStuffList;



// game data
string MapDir, ObjectDir, ScriptDir, MaterialDir, FontDir;
void *MetaExitProgram,*MetaFindHosts,*MetaLoadWorld,*MetaScreenShot,*MetaLoadGameFromHost,*MetaSaveGameState,*MetaLoadGameState;
str_float_func *MetaDrawSplashScreen;
void *MetaObjectScriptInit;
Model *ModelToIgnore;

// models
struct sModelRefCopy
{
	Model *model;
	int OriginalIndex;
};
static Array<Model*> ModelOriginal, ModelOriginal2;
static Array<sModelRefCopy> ModelCopy, ModelCopy2;

// materials
static Array<Material*> MetaMaterial;

// fonts
Array<XFont*> _XFont_;




Model *NoModel;

#define _xfont_char_ae_		248
#define _xfont_char_oe_		249
#define _xfont_char_ue_		250
#define _xfont_char_Ae_		251
#define _xfont_char_Oe_		252
#define _xfont_char_Ue_		253
#define _xfont_char_ss_		254



void MetaInit()
{
	msg_db_r("Meta",1);
	XFontIndex=0;
	DefaultFont=0;
	// unnessessary...
	ModelOriginal.clear();
	ModelCopy.clear();
	_XFont_.clear();

	ZBufferEnabled=true;
	CullingEnabled=false;
	SortingEnabled=false;
	ConsoleEnabled=false;

	FpsMax=60;
	FpsMin=10;

	DetailLevel=100;
	DetailFactorInv=1.0f;

#ifdef _X_ALLOW_MODEL_
	// create the default material's default values
	Material *m = new Material;
	m->name = "-default-";
	m->num_textures = 0;
	m->ambient = White;
	m->diffuse = White;
	m->specular = Black;
	m->shininess = 0;
	m->emission = Black;
	m->transparency_mode = TransparencyModeNone;
	m->alpha_source = 0;
	m->alpha_destination = 0;
	m->alpha_factor = 1;
	m->alpha_z_buffer = true;
	m->shader = -1;
	m->reflection_mode = ReflectionNone;
	m->reflection_density = 0;
	m->cube_map = -1;
	m->cube_map_size = 0;
	m->rc_jump = 0.5f;
	m->rc_static = 0.8f;
	m->rc_sliding = 0.4f;
	m->rc_rolling = 0.90f;
	MetaMaterial.add(m);
	
	ModelToIgnore = NULL;
#endif

	msg_db_l(1);
}

void MetaEnd()
{
	MetaReset();
	delete(MetaMaterial[0]);
}

void MetaReset()
{
	msg_db_r("Meta reset",1);
	msg_db_m("-items",1);
	
#ifdef _X_DEBUG_MODEL_MANAGER_
	for (int i=0;i<ModelCopy.num;i++)
		msg_write(p2s(ModelCopy[i].model));
#endif

#ifdef _X_ALLOW_MODEL_
	// delete copies of models
	for (int i=0;i<ModelCopy.num;i++)
		MetaDeleteModel(ModelCopy[i].model);
	ModelCopy.clear();

	// delete original
	msg_db_m("-models",1);
	for (int i=0;i<ModelOriginal.num;i++)
		delete(ModelOriginal[i]);
	ModelOriginal.clear();
#endif
	ModelOriginal2.clear();
	ModelCopy2.clear();

	// delete materials
#ifdef _X_ALLOW_MODEL_
	for (int i=1;i<MetaMaterial.num;i++)
		delete(MetaMaterial[i]);
#endif
	MetaMaterial.resize(1);

	MetaDeleteStuffList.clear();

	ModelToIgnore=NULL;
	DefaultFont=0;
	XFontZ=0;
	XFontIndex=0;
	ShadowLight=0;
	ShadowLowerDetail=false;
	ShadowColor=color(0.5f,0,0,0);
	msg_db_l(1);
}

void MetaSetDirs(const string &texture_dir, const string &map_dir, const string &object_dir, const string &sound_dir, const string &script_dir, const string &material_dir, const string &font_dir)
{
	NixTextureDir = texture_dir;
	NixShaderDir = material_dir;
	MapDir = map_dir;
	ObjectDir = object_dir;
#ifdef _X_USE_SOUND_
	SoundDir = sound_dir;
#endif
	ScriptDir = script_dir;
	MaterialDir = material_dir;
	FontDir = font_dir;
#ifdef _X_ALLOW_SCRIPT_
	Script::config.Directory = script_dir;
#endif
}

void MetaCalcMove()
{
/*#ifdef _X_ALLOW_MODEL_
	for (int i=0;i<NumTextures;i++)
		NixTextureVideoMove(Texture[i],Elapsed);
#endif*/
	 msg_todo("MetaCalcMove...");
	
	DetailFactorInv=100.0f/(float)DetailLevel;
}

inline bool EqualChars(char a,char b)
{
	if (a==b)
		return true;
	if ((a>='a')&&(a<='z'))
		if (a==b+32)
			return true;
	if ((b>='a')&&(b<='z'))
		if (b==a+32)
			return true;
	return false;
}

// case insensitive...
inline bool EqualStrings(const char *a,const char *b)
{
	unsigned int l=strlen(a);
	if (strlen(b)==l){
		for (unsigned int i=0;i<l;i++)
			if (!EqualChars(a[i],b[i]))
				return false;
		return true;
	}
	return false;
}

void MetaListTest()
{
#ifdef _X_DEBUG_MODEL_MANAGER_
	if (abs((int)ModelCopy.num - (int)ModelCopy2.num) > 1){
		msg_error("size !=");
		msg_write(ModelCopy.num);
		msg_write(ModelCopy2.num);
	}
	for (int i=0;i<min(ModelCopy.num, ModelCopy2.num);i++)
		if (ModelCopy[i].model != ModelCopy2[i].model){
			msg_error("model !=");
			msg_write(i);
			msg_write(ModelCopy.num);
			msg_write(ModelCopy2.num);
			msg_write(p2s(ModelCopy[i].model));
			msg_write(p2s(ModelCopy2[i].model));
		}
	if (abs((int)ModelOriginal.num - (int)ModelOriginal2.num) > 1){
		msg_error("size orig !=");
		msg_write(ModelOriginal.num);
		msg_write(ModelOriginal2.num);
	}
	for (int i=0;i<min(ModelOriginal.num, ModelOriginal2.num);i++)
		if (ModelOriginal[i] != ModelOriginal2[i]){
			msg_error("model orig !=");
			msg_write(i);
			msg_write(ModelOriginal.num);
			msg_write(ModelOriginal2.num);
			msg_write(p2s(ModelOriginal[i]));
			msg_write(p2s(ModelOriginal2[i]));
		}
#endif
}

void MetaListUpdate()
{
	MetaListTest();
	ModelCopy2.assign(&ModelCopy);
	ModelOriginal2.assign(&ModelOriginal);
}

static void db_o(const string &msg)
{
	#ifdef _X_DEBUG_MODEL_MANAGER_
		msg_write(msg);
	#endif
}

void AddModelCopy(Model *m, int orig_id)
{
	sModelRefCopy c;
	c.model = m;
	db_o(p2s(m));
	c.OriginalIndex = orig_id;
	ModelCopy.add(c);
	MetaListUpdate();
}

void AddModelOrig(Model *m)
{
	ModelOriginal.add(m);
	db_o(p2s(m));
	MetaListUpdate();
}

void ReplaceModelOrig(int id, Model *m)
{
	Model *m0 = ModelOriginal[id];
	ModelOriginal[id] = m;
	ModelOriginal2[id] = m;
}

Model *MetaLoadModel(const string &filename)
{
#ifdef _X_ALLOW_MODEL_
	if (filename.num == 0)
		return NULL;
	//msg_error("MetaLoadModel");
	//msg_write(filename);


	MetaListTest();

	msg_db_r("MetaLoadModel", 2);
	// already existing? -> copy
	for (int i=0;i<ModelOriginal.num;i++){
		if (filename == ModelOriginal[i]->_template->filename){
			//msg_write("copy...");
			Model *m = ModelOriginal[i]->GetCopy(ModelCopyRecursive);
			AddModelCopy(m, i);
			db_o("#######");
			msg_db_l(2);
			return m;
		}
	}

	//msg_write("new....");
	Model *m = new Model(filename);
	if (m->error){
		msg_db_l(2);
		return NULL;
	}
	AddModelOrig(m);
	db_o("####### orig");
	msg_db_l(2);
	return m;
#endif
	return NULL;
}

// make sure we can edit this object without destroying an original one
void MetaModelMakeEditable(Model *m)
{
#ifdef _X_ALLOW_GOD_
	msg_db_r("MetaModelMakeEditable", 2);
	
	// original -> create copy
	if (!m->is_copy){
		bool registered = m->registered;
		if (m->registered)
			GodUnregisterModel(m);
		int id = -1;
		for (int i=0;i<ModelOriginal.num;i++)
			if (ModelOriginal[i] == m){
				id = i;
				break;
			}
		if (id < 0){
			msg_error("MetaModelMakeEditable: no original model found!");
			msg_db_l(2);
			return;
		}

		// create a "temporary" copy and save it as the original
		Model *m2 = m->GetCopy(ModelCopyKeepSubModels | ModelCopyInverse);
		ReplaceModelOrig(id, m2);

		// swap data
		char *data = new char[sizeof(Model)];
		memcpy(data, m, sizeof(Model));
		memcpy(m, m2, sizeof(Model));
		memcpy(m2, data, sizeof(Model));
		delete[](data);

		// create copy
		sModelRefCopy c;
		db_o(p2s(m));
		db_o(p2s(m2));
		c.model = m; // we are the copy now
		c.OriginalIndex = id;
		ModelCopy.add(c);
		MetaListUpdate();
		if (registered)
			GodRegisterModel(m);
	}

	// must have its own materials
	if (m->material_is_reference){
		m->material.make_own();
		m->material_is_reference = false;
	}
	msg_db_l(2);
#endif
}

Model *MetaCopyModel(Model *m)
{
#ifdef _X_ALLOW_MODEL_
	if (!m)
		return NULL;
	msg_db_r("MetaCopyModel", 2);
	// which original
	int id = -1;
	if (m->is_copy){
		for (int i=0;i<ModelCopy.num;i++)
			if (ModelCopy[i].model == m){
				id = ModelCopy[i].OriginalIndex;
				break;
			}
	}else{
		for (int i=0;i<ModelOriginal.num;i++)
			if (ModelOriginal[i] == m){
				id = i;
				break;
			}
	}
	if (id < 0){
		msg_error("MetaCopyModel: no original model found!");
		msg_db_l(2);
		return NULL;
	}

	Model *copy = ModelOriginal[id]->GetCopy(ModelCopyRecursive);
	AddModelCopy(copy, id);
	
	msg_db_l(2);
	return copy;
#else
	return NULL;
#endif
}

bool MetaIsModel(Model *m)
{
	for (int i=0;i<ModelCopy.num;i++)
		if (ModelCopy[i].model == m)
			return true;
	for (int i=0;i<ModelOriginal.num;i++)
		if (ModelOriginal[i] == m)
			return true;
	return false;
}

void MetaDeleteModel(Model *m)
{
#ifdef _X_ALLOW_MODEL_
	if (!m)	return;
	msg_db_r("MetaDeleteModel",1);
	db_o("meta del");

	if (MetaIsModel(m)){
		db_o("m");
		db_o(p2s(m));
		db_o(p2s(m->_template));
		db_o(m->_template->filename);
		GodUnregisterModel(m);
		if (m->object_id >= 0)
			GodDeleteObject(m->object_id);
	}
		db_o(".");

	// deletes only copies created by meta
	for (int i=0;i<ModelCopy.num;i++)
		if (ModelCopy[i].model == m){
			msg_db_m("-really deleting...",1);
			//msg_error("MetaDeleteModel");
			//msg_write(ModelOriginal[ModelCopy[i].OriginalIndex].Filename);
			delete(m);
			ModelCopy.erase(i);
			break;
		}
	msg_db_l(1);
#endif
}

Material *MetaLoadMaterial(const string &filename, bool as_default)
{
#ifdef _X_ALLOW_MODEL_
	// an empty name loads the default material
	if (filename.num == 0)
		return MetaMaterial[0];

	if (!as_default){
		for (int i=0;i<MetaMaterial.num;i++)
			if (MetaMaterial[i]->name == filename)
				return MetaMaterial[i];
	}
	CFile *f = OpenFile(MaterialDir + filename + ".material");
	if (!f)
		return FileErrorsAreCritical ? NULL : MetaMaterial[0];
	Material *m = new Material;

	int ffv=f->ReadFileFormatVersion();
	int a,r,g,b;
	if (ffv==4){
		m->name = filename;
		// Textures
		m->num_textures = f->ReadIntC();
		for (int i=0;i<m->num_textures;i++)
			m->texture[i] = NixLoadTexture(f->ReadStr());
		// Colors
		f->ReadComment();
		a=f->ReadInt();	r=f->ReadInt();	g=f->ReadInt();	b=f->ReadInt();
		m->ambient=color((float)a/255.0f,(float)r/255.0f,(float)g/255.0f,(float)b/255.0f);
		a=f->ReadInt();	r=f->ReadInt();	g=f->ReadInt();	b=f->ReadInt();
		m->diffuse=color((float)a/255.0f,(float)r/255.0f,(float)g/255.0f,(float)b/255.0f);
		a=f->ReadInt();	r=f->ReadInt();	g=f->ReadInt();	b=f->ReadInt();
		m->specular=color((float)a/255.0f,(float)r/255.0f,(float)g/255.0f,(float)b/255.0f);
		m->shininess=(float)f->ReadInt();
		a=f->ReadInt();	r=f->ReadInt();	g=f->ReadInt();	b=f->ReadInt();
		m->emission=color((float)a/255.0f,(float)r/255.0f,(float)g/255.0f,(float)b/255.0f);
		// Transparency
		m->transparency_mode=f->ReadIntC();
		m->alpha_factor=float(f->ReadInt())*0.01f;
		m->alpha_source=f->ReadInt();
		m->alpha_destination=f->ReadInt();
		m->alpha_z_buffer=f->ReadBool();
		// Appearance
		f->ReadIntC(); //ShiningDensity
		f->ReadInt(); // ShiningLength
		f->ReadBool(); // IsWater
		// Reflection
		m->cube_map=-1;
		m->cube_map_size=0;
		m->reflection_density=0;
		m->reflection_mode=f->ReadIntC();
		m->reflection_density=float(f->ReadInt())*0.01f;
		m->cube_map_size=f->ReadInt();
		int cmt[6];
		for (int i=0;i<6;i++)
			cmt[i] = NixLoadTexture(f->ReadStr());
#ifdef _X_ALLOW_FX_
		if (m->reflection_mode == ReflectionCubeMapDynamical){
			//m->cube_map = FxCubeMapNew(m->cube_map_size);
			//FxCubeMapCreate(m->cube_map,cmt[0],cmt[1],cmt[2],cmt[3],cmt[4],cmt[5]);
		}else if (m->reflection_mode == ReflectionCubeMapStatic){
			m->cube_map = NixCreateCubeMap(m->cube_map_size);
			for (int i=0;i<6;i++)
				NixFillCubeMap(m->cube_map, i, cmt[i]);
		}
#endif
		// ShaderFile
		string ShaderFile = f->ReadStrC();
		m->shader = NixLoadShader(ShaderFile);
		// Physics
		m->rc_jump=(float)f->ReadIntC()*0.001f;
		m->rc_static=(float)f->ReadInt()*0.001f;
		m->rc_sliding=(float)f->ReadInt()*0.001f;
		m->rc_rolling=(float)f->ReadInt()*0.001f;

		MetaMaterial.add(m);
	}else{
		msg_error(format("wrong file format: %d (expected: 4)", ffv));
		m = MetaMaterial[0];
	}
	FileClose(f);
	return m;
#endif
	return NULL;
}


void _cdecl MetaDelete(void *p)
{
	if (!p)	return;
	msg_db_r("MetaDelete", 2);
	int type = ((XContainer*)p)->type;
	if (false){}
#ifdef _X_ALLOW_GUI_
	else if (type == XContainerPicture)
		Gui::DeletePicture((Gui::Picture*)p);
	else if (type == XContainerPicture3d)
		Gui::DeletePicture3d((Gui::Picture3d*)p);
	else if (type == XContainerText)
		Gui::DeleteText((Gui::Text*)p);
	else if (type == XContainerGrouping)
		Gui::DeleteGrouping((Gui::Grouping*)p);
#endif
#ifdef _X_ALLOW_FX_
	else if (type == XContainerParticle)
		FxParticleDelete((Particle*)p);
	else if (type == XContainerParticleRot)
		FxParticleDelete((Particle*)p);
	else if (type == XContainerParticleBeam)
		FxParticleDelete((Particle*)p);
	else if (type == XContainerEffect)
		FxDelete((Effect*)p);
#endif
#ifdef _X_ALLOW_CAMERA_
	else if (type == XContainerView)
		DeleteCamera((Camera*)p);
#endif
	else // TODO: recognize models!
		MetaDeleteModel((Model*)p);
	msg_db_l(2);
}


void MetaDeleteLater(void *p)
{
	MetaDeleteStuffList.add(p);
}

void MetaDeleteSelection()
{
	msg_db_r("MetaDeleteSelection", 1);
	for (int i=0;i<MetaDeleteStuffList.num;i++)
		MetaDelete(MetaDeleteStuffList[i]);
	MetaDeleteStuffList.clear();
	msg_db_l(1);
}

static string str_utf8_to_ubyte(const string &str)
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

int _cdecl MetaLoadFont(const string &filename)
{
	// "" -> default font
	if (filename.num == 0)
		return 0;

	foreachi(XFont *ff, _XFont_, i)
		if (ff->filename  == filename.sys_filename())
			return i;
	CFile *f = OpenFile(FontDir + filename + ".xfont");
	if (!f)
		return -1;
	int ffv=f->ReadFileFormatVersion();
	if (ffv==2){
		XFont *font = new XFont;
		font->filename = filename.sys_filename();
		font->texture = NixLoadTexture(f->ReadStrC());
		int tx = 1;
		int ty = 1;
		if (font->texture >= 0){
			tx = NixTextures[font->texture].width;
			ty = NixTextures[font->texture].height;
		}
		int num_glyphs = f->ReadWordC();
		int height=f->ReadByteC();
		int y1=f->ReadByteC();
		int y2=f->ReadByteC();
		float dy=float(y2-y1);
		font->height=(float)height/dy;
		font->y_offset=(float)y1/dy;
		font->x_factor=(float)f->ReadByteC()*0.01f;
		font->y_factor=(float)f->ReadByteC()*0.01f;
		f->ReadComment();
		int x=0,y=0;
		for (int i=0;i<num_glyphs;i++){
			string name = f->ReadStr();
			int c = (unsigned char)str_utf8_to_ubyte(name)[0];
			int w=f->ReadByte();
			int x1=f->ReadByte();
			int x2=f->ReadByte();
			if (x+w>tx){
				x=0;
				y+=height;
			}
			XGlyph g;
			g.x_offset = (float)x1/dy;
			g.width = (float)w/dy;
			g.dx = (float)(x2-x1)/dy;
			g.dx2 = (float)(w-x1)/dy;
			g.src = rect(	(float)(x+0.5f)/(float)tx,
											(float)(x+0.5f+w)/(float)tx,
											(float)y/(float)ty,
											(float)(y+height)/(float)ty);
			font->glyph[c] = g;
			x+=w;
		}
		/*int u=(unsigned char)f->ReadStrC()[0];
		font->unknown_glyph_no = font->table[u];
		if (font->unknown_glyph_no<0)
			font->unknown_glyph_no=0;
		for (int i=0;i<256;i++)
			if (font->table[i] < 0)
				font->table[i] = font->unknown_glyph_no;*/
		_XFont_.add(font);
	}else{
		msg_error(format("wrong file format: %d (expected: 2)",ffv));
	}
	FileClose(f);

	return _XFont_.num-1;
}

// retrieve the width of a given text
float _cdecl XFGetWidth(float height,const string &str)
{
	XFont *f = _XFont_[XFontIndex];
	if (!f)
		return 0;
	float w = 0;
	float xf = height * f->x_factor;
	string s = str_utf8_to_ubyte(str);
	for (int i=0;i<s.num;i++){
		int n = (unsigned char)s[i];
		w += f->glyph[n].dx * xf;
	}
	return w;
}

// display a string with our font (values relative to screen)
float _cdecl XFDrawStr(float x,float y,float height,const string &str,bool centric)
{
	XFont *f = _XFont_[XFontIndex];
	if (!f)
		return 0;
	msg_db_r("XFDrawStr",10);
	if (centric)
		x-=XFGetWidth(height,str)/2;
	NixSetAlpha(AlphaSourceAlpha,AlphaSourceInvAlpha);
		//NixSetAlpha(AlphaMaterial);
	NixSetTexture(f->texture);
	float xf=height*f->x_factor * 1.33f;
	float yf=height*f->y_factor;
	float w=0;
	y-=f->y_offset*yf;
	rect d;
	string s = str_utf8_to_ubyte(str);
	for (int i=0;i<s.num;i++){
		if (s[i]=='\r')
			continue;
		if (s[i]=='\n'){
			w=0;
			y+=f->height*yf;
			continue;
		}
		int n=(unsigned char)s[i];
		d.x1=(x+w-f->glyph[n].x_offset*xf);
		d.x2=(x+w+f->glyph[n].dx2     *xf);
		d.y1=(y             );
		d.y2=(y+f->height*yf);
		NixDraw2D(f->glyph[n].src, d, XFontZ);
		w += f->glyph[n].dx * xf;
	}
	NixSetAlpha(AlphaNone);
	msg_db_l(10);
	return w;
}

// vertically display a text 
float _cdecl XFDrawVertStr(float x,float y,float height,const string &str)
{
	XFont *f = _XFont_[XFontIndex];
	if (!f)
		return 0;
	msg_db_r("XFDrawVertStr",10);
	NixSetAlpha(AlphaSourceAlpha,AlphaSourceInvAlpha);
	NixSetTexture(f->texture);
	float xf=height*f->x_factor;
	float yf=height*f->y_factor;
	y-=f->y_offset*yf;
	rect d;
	string s = str_utf8_to_ubyte(str);
	for (int i=0;i<s.num;i++){
		if (s[i]=='\r')
			continue;
		if (s[i]=='\n')
			continue;
		int n=(unsigned char)s[i];
		d.x1=(x-f->glyph[n].x_offset*xf);
		d.x2=(x+f->glyph[n].dx2     *xf);
		d.y1=(y             );
		d.y2=(y+f->height*yf);
		NixDraw2D(f->glyph[n].src, d, XFontZ);
		y+=yf*0.8f;
	}
	NixSetAlpha(AlphaNone);
	msg_db_l(10);
	return 0;
}

