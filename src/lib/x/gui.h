#if !defined(GUI_H__INCLUDED_)
#define GUI_H__INCLUDED_



struct sText : XContainer
{
	bool centric, vertical;
	int font;
	vector pos;
	float size;
	color _color;
	string text;
	bool IsMouseOver();
};

struct sPicture : XContainer
{
	bool tc_inverted;
	vector pos;
	float width, height;
	color _color;
	int texture;
	rect source;
	int shader;
	
	bool IsMouseOver();
};

struct sPicture3D : XContainer
{
	bool lighting, world_3d;
	float z;
	matrix _matrix;
	CModel *model;
	color _color;
	// temp data
	bool ch_alpha, ch_color;
	color ambient;
	color diffuse;
	color emission;
	int transparency_mode;
	float alpha_factor;
	float reflection_density;
	bool IsMouseOver();
};

struct sSubPicture
{
	sPicture *picture;
	vector pos;
	color _color;
	bool enabled;
};

struct sSubPicture3D
{
	sPicture3D *picture_3d;
	matrix _matrix;
	float z;
	color _color;
	bool enabled;
};

struct sSubText
{
	sText *text;
	vector pos;
	color _color;
	bool enabled;
};

struct sGrouping : XContainer
{
	vector pos;
	color _color;

	Array<sSubPicture> picture;
	Array<sSubPicture3D> picture_3d;
	Array<sSubText> text;
};


sText *_cdecl GuiCreateText(const vector &pos, float size, const color &col, const string &str);
void GuiDeleteText(sText *text);
sPicture *_cdecl GuiCreatePicture(const vector &pos, float width, float height, int texture);
void GuiDeletePicture(sPicture *picture);
sPicture3D *_cdecl GuiCreatePicture3D(CModel *model, const matrix &mat, float z);
void GuiDeletePicture3D(sPicture3D *picture3d);
sGrouping *_cdecl GuiCreateGrouping(const vector &pos, bool set_current);
void GuiDeleteGrouping(sGrouping *grouping);

void GuiReset();
void GuiDraw();


extern sGrouping *CurrentGrouping;

#endif
