#if !defined(GUI_H__INCLUDED_)
#define GUI_H__INCLUDED_


namespace Gui
{


struct Text : XContainer
{
	bool centric, vertical;
	int font;
	vector pos;
	float size;
	color _color;
	string text;
	bool IsMouseOver();
};

struct Picture : XContainer
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

struct Picture3d : XContainer
{
	bool lighting, world_3d;
	float z;
	matrix _matrix;
	Model *model;
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

struct SubPicture;
struct SubPicture3d;
struct SubText;

struct Grouping : XContainer
{
	vector pos;
	color _color;

	Array<SubPicture> picture;
	Array<SubPicture3d> picture_3d;
	Array<SubText> text;
};


Text *_cdecl CreateText(const vector &pos, float size, const color &col, const string &str);
void DeleteText(Text *text);
Picture *_cdecl CreatePicture(const vector &pos, float width, float height, int texture);
void DeletePicture(Picture *picture);
Picture3d *_cdecl CreatePicture3d(Model *model, const matrix &mat, float z);
void DeletePicture3d(Picture3d *picture3d);
Grouping *_cdecl CreateGrouping(const vector &pos, bool set_current);
void DeleteGrouping(Grouping *grouping);

void Reset();
void Draw();


extern Grouping *CurrentGrouping;

};

#endif
