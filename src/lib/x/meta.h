/*----------------------------------------------------------------------------*\
| Meta                                                                         |
| -> administration of animations, models, items, materials etc                |
| -> centralization of game data                                               |
|                                                                              |
| last updated: 2009.11.22 (c) by MichiSoft TM                                 |
\*----------------------------------------------------------------------------*/
#if !defined(META_H__INCLUDED_)
#define META_H__INCLUDED_


extern string MetaVersion;

#ifndef _X_ALLOW_MODEL_
	#define Model		void
	#define Material	void
	#define MetaMove	void
#endif

#ifndef _X_ALLOW_OBJECT_
	#define ObjectTemplate	void
#endif

#define META_MAX_MOVE_OPERATIONS	8

struct XGlyph
{
	float dx, dx2, x_offset, width;
	rect src;
};

struct XFont
{
	string filename;
	int texture;
	int unknown_glyph_no;
	XGlyph glyph[256];
	float y_offset, height, x_factor, y_factor;
};

extern string AppName;
extern int NumRealColTests;
extern float TimeScale,TimeScaleLast,Elapsed,ElapsedRT;

extern bool FileErrorsAreCritical;


// game configuration
extern bool Debug,ShowTimings,ConsoleEnabled,WireMode;
extern bool Record;
extern int DetailLevel;
extern float DetailFactorInv;
extern int ShadowLevel,ShadowLight;
extern color ShadowColor;
extern bool ShadowLowerDetail;
extern float FpsMax,FpsMin;
extern int Multisampling;
extern bool NetworkEnabled,CullingEnabled,SortingEnabled,ZBufferEnabled;
extern int XFontIndex,DefaultFont;
extern bool ResettingGame;

typedef void str_float_func(const string&,float);


	void MetaInit();
	void MetaEnd();
	void MetaReset();
	void MetaCalcMove();

// data to CMeta
	void MetaSetDirs(const string &texture_dir, const string &map_dir, const string &object_dir, const string &sound_dir, const string &script_dir, const string &material_dir, const string &font_dir);

// models
	Model *_cdecl MetaLoadModel(const string &filename);
	Model *MetaCopyModel(Model *m);
	void _cdecl MetaDeleteModel(Model *m);
	void _cdecl MetaModelMakeEditable(Model *m);
	int _cdecl MetaGetModelOID(Model *m);

// materials
	Material *MetaLoadMaterial(const string &filename,bool as_default=false);

// fonts
	int _cdecl MetaLoadFont(const string &filename);

// all
	void _cdecl MetaDelete(void *p);
	void _cdecl MetaDeleteLater(void *);
	void _cdecl MetaDeleteSelection();



// game data
	extern string MapDir, ObjectDir, SoundDir, ScriptDir, MaterialDir, FontDir;
	extern void *MetaExitProgram,*MetaFindHosts,*MetaLoadWorld,*MetaScreenShot,*MetaLoadGameFromHost,*MetaSaveGameState,*MetaLoadGameState;
	extern str_float_func *MetaDrawSplashScreen;
	extern void *MetaObjectScriptInit;
	extern Model *ModelToIgnore;

	extern float MetaListenerRate;

// only used by meta itself and the editor...
	extern Array<XFont*> _XFont_;
	// gui...
	extern int XFontIndex, DefaultFont;
	extern float XFontZ;


float _cdecl XFGetWidth(float h,const string &str);
float _cdecl XFDrawStr(float x,float y,float height,const string &str,bool centric=false);
float _cdecl XFDrawVertStr(float x,float y,float h,const string &str);

enum{
	ErrorLoadingWorld,
	ErrorLoadingMap,
	ErrorLoadingModel,
	ErrorLoadingObject,
	ErrorLoadingItem,
	ErrorLoadingScript
};

enum{
	ScriptLocationCalcMovePrae,
	ScriptLocationCalcMovePost,
	ScriptLocationRenderPrae,
	ScriptLocationRenderPost1,
	ScriptLocationRenderPost2,
	ScriptLocationGetInputPrae,
	ScriptLocationGetInputPost,
	ScriptLocationNetworkSend,
	ScriptLocationNetworkRecieve,
	ScriptLocationNetworkAddClient,
	ScriptLocationNetworkRemoveClient,
	ScriptLocationWorldInit,
	ScriptLocationWorldDelete,
	ScriptLocationOnKeyDown,
	ScriptLocationOnKeyUp,
	ScriptLocationOnKey,
	ScriptLocationOnLeftButtonDown,
	ScriptLocationOnLeftButtonUp,
	ScriptLocationOnLeftButton,
	ScriptLocationOnMiddleButtonDown,
	ScriptLocationOnMiddleButtonUp,
	ScriptLocationOnMiddleButton,
	ScriptLocationOnRightButtonDown,
	ScriptLocationOnRightButtonUp,
	ScriptLocationOnRightButton,
};

#endif

